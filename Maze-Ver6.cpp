/************************************************************************/
/*          This program has been written by Jonas De Maeyer            */
/*          The maze includes three types of entities                   */
/*          0: obstacle  1: free space  2: charging station             */
/************************************************************************/

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <random>
#include <set>
#include <stack>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <SFML/Graphics.hpp>

#define ACTION_COUNT 8 // The number of possible actions

/***********************/
/*     -----------     */
/*    | 7 | 0 | 1 |    */
/*     -----------     */
/*    | 6 | X | 2 |    */
/*     -----------     */
/*    | 5 | 4 | 3 |    */
/*     -----------     */
/*                     */
/*  0: Move N          */
/*  1: Move NE         */
/*  2: Move E          */
/*  3: Move SE         */
/*  4: Move S          */
/*  5: Move SW         */
/*  6: Move W          */
/*  7: Move NW         */
/*                     */
/***********************/

#define OBSTACLE 0
#define FREE_SPACE 1
#define CHARGING_STATION 2
#define AGENT 3

#define EPISODE_COUNT 10'000
#define LEARNING_RATE 0.4
#define DISCOUNT_FACTOR 0.9

using namespace std;

/*************************************************************************/
struct HashPair {
    size_t operator()(const pair<int, int> &p) const {
        return hash<int>()(p.first) ^ (hash<int>()(p.second) << 1);
    }
};

/*************************************************************************/
class MazeNode {
public:
    unique_ptr<vector<vector<int> > > maze; // Optional maze, only at root
    unique_ptr<vector<vector<vector<double> > > > qTable; // 3D array Q-table [localRow][localCol][action]
    MazeNode *parent;
    vector<MazeNode *> children;
    int rows, cols;
    int startRow, startCol, endRow, endCol;
    int chargingStationCount;
    double baselineSuccessRate = -1.0; // -1 indicates untrained

    // Constructor
    MazeNode(const vector<vector<int> > &fullMaze, const int rows, const int cols, const int startRow,
             const int startCol, const int endRow, const int endCol, MazeNode *parent = nullptr,
             const bool isRoot = false) : parent(parent), rows(rows), cols(cols), startRow(startRow),
                                          startCol(startCol), endRow(endRow), endCol(endCol) {
        if (isRoot) {
            maze = make_unique<vector<vector<int> > >(fullMaze);
            initQTable();
        }
        chargingStationCount = countChargingStations(fullMaze);
    }

    // Destructor
    ~MazeNode() {
        for (const MazeNode *child: children) {
            delete child;
        }
        children.clear();
    }

    void addChild(MazeNode *child) {
        children.push_back(child);
    }

    // Find leaf sub-environment for a given position
    MazeNode *findSubEnvironment(const int row, const int col) {
        if (row < startRow || row > endRow || col < startCol || col > endCol) {
            return nullptr;
        }
        if (children.empty()) {
            return this;
        }
        for (MazeNode *child: children) {
            MazeNode *result = child->findSubEnvironment(row, col);
            if (result) return result;
        }
        return nullptr;
    }

    // Initialize 3D Q-table array
    void initQTable() {
        if (!qTable) {
            const int localRows = endRow - startRow + 1;
            const int localCols = endCol - startCol + 1;
            qTable = make_unique<vector<vector<vector<double> > > >(
                localRows, vector<vector<double> >(localCols, vector<double>(ACTION_COUNT, 0.0)));
        }
    }

    // Count charging stations in the subenvironment
    [[nodiscard]] int countChargingStations(const vector<vector<int> > &fullMaze) const {
        int count = 0;
        for (int i = startRow; i <= endRow; ++i) {
            for (int j = startCol; j <= endCol; ++j) {
                if (fullMaze[i][j] == CHARGING_STATION) {
                    count++;
                }
            }
        }
        return count;
    }
};


/*************************************************************************/
vector<double> &getLocalValues(vector<vector<vector<double> > > &table, const int globalRow, const int globalCol,
                               const int startRow, const int startCol) {
    const int localRow = globalRow - startRow;
    const int localCol = globalCol - startCol;
    return table[localRow][localCol];
}


/*************************************************************************/
void createMaze(vector<vector<int> > &maze, const int rows, const int cols, const double freeSpaceProb,
                const double obstacleProb, const double chargingStationProb) {
    // Validate that the probabilities sum to 1
    if (abs(freeSpaceProb + obstacleProb + chargingStationProb - 1.0) > 1e-6) {
        cerr << "Error: Probabilities must sum to 1." << endl;
        exit(1);
    }

    // Variable to track if at least one charging station is placed
    bool hasChargingStation = false;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            const double randomValue = static_cast<double>(rand()) / RAND_MAX;
            if (randomValue < freeSpaceProb) {
                maze[i][j] = FREE_SPACE; // Free space
            } else if (randomValue < freeSpaceProb + obstacleProb) {
                maze[i][j] = OBSTACLE; // Obstacle
            } else {
                maze[i][j] = CHARGING_STATION; // Charging station
                hasChargingStation = true;
            }
        }
    }

    // Ensure there is at least one charging station
    if (!hasChargingStation) {
        const int randomRow = rand() % rows;
        const int randomCol = rand() % cols;
        maze[randomRow][randomCol] = CHARGING_STATION; // Place a charging station
    }
}

/*************************************************************************/
void printMatrixInt(const vector<vector<int> > &matrix, const int rows, const int cols, const char text[]) {
    cout << "\n\t\t******* This is The: " << text << "  *******\n\n";

    // Print column headers with [j] format
    cout << "\t\t\t\t";
    cout << "      "; // Space for row index
    for (int j = 0; j < cols; j++) {
        cout << "[" << j << "]  ";
    }
    cout << "\n";

    // Print each row with [i] format
    for (int i = 0; i < rows; i++) {
        cout << "\t\t\t\t";
        cout << "[" << i << "] "; // Row index with padding
        for (int j = 0; j < cols; j++) {
            cout << setw(4) << matrix[i][j] << " ";
        }
        cout << "\n";
    }
    cout << "\n";
}

/*************************************************************************/
void printTree(const MazeNode *node, const string &prefix = "", const bool isLast = true, const bool isRoot = true) {
    if (!node) return;

    // For the root node, don't add any symbols
    if (isRoot) {
        cout << "Node: Start(" << node->startRow << ", " << node->startCol << "), "
                << "End(" << node->endRow << ", " << node->endCol << "), "
                << "Size(" << (node->endRow - node->startRow + 1) << "x" << (node->endCol - node->startCol + 1)
                << "), " << "Charging Stations: " << node->chargingStationCount << "\n";
    } else {
        // For all other nodes, add the appropriate symbols
        const string currentPrefix = prefix + (isLast ? "└─ " : "├─ ");
        cout << currentPrefix
                << "Node: Start(" << node->startRow << ", " << node->startCol << "), "
                << "End(" << node->endRow << ", " << node->endCol << "), "
                << "Size(" << (node->endRow - node->startRow + 1) << "x" << (node->endCol - node->startCol + 1)
                << "), " << "Charging Stations: " << node->chargingStationCount << "\n";
    }

    // Adjust prefix for children
    string childPrefix;
    if (isRoot) {
        childPrefix = " ";
    } else {
        childPrefix = prefix + (isLast ? "    " : "│   ");
    }

    // Traverse children
    for (size_t i = 0; i < node->children.size(); ++i) {
        printTree(node->children[i], childPrefix, i == node->children.size() - 1, false);
    }
}

/*************************************************************************/
int checkExit(const vector<vector<int> > &matrix, const int x, const int y) {
    return (matrix[x][y] == CHARGING_STATION) ? 1 : 0;
}

/*************************************************************************/
pair<int, int> selectFirstPlace(const vector<vector<int> > &maze, const int startRow, const int startCol,
                                const int endRow, const int endCol) {
    int x, y;
    // Keep generating random indices until a free space is found
    do {
        x = rand() % (endRow - startRow) + startRow;
        y = rand() % (endCol - startCol) + startCol;
    } while (maze[x][y] != FREE_SPACE);
    return make_pair(x, y);
}

/*************************************************************************/
struct StartStats {
    int attempts = 0;
    int successes = 0;
    [[nodiscard]] double successRate() const { return attempts > 0 ? static_cast<double>(successes) / attempts : 0.0; }
};

/*************************************************************************/
pair<int, int> selectFirstPlace(const vector<vector<int> > &maze, const int startRow, const int startCol,
                                const int endRow, const int endCol, const int counter,
                                const unordered_map<pair<int, int>, StartStats, HashPair> &startStats, mt19937 &rng) {
    constexpr int initialRandomEpisodes = 10;
    if (counter < initialRandomEpisodes || startStats.empty()) {
        int r, c;
        do {
            r = startRow + rand() % (endRow - startRow + 1);
            c = startCol + rand() % (endCol - startCol + 1);
        } while (maze[r][c] == OBSTACLE);
        return {r, c};
    }

    // Build weights: lower success rate = higher weight
    vector<pair<int, int> > positions;
    vector<double> weights;
    constexpr double epsilon = 0.1; // Ensure non-zero probability
    for (int x = startRow; x <= endRow; ++x) {
        for (int y = startCol; y <= endCol; ++y) {
            if (maze[x][y] == OBSTACLE) continue;
            positions.emplace_back(x, y);
            auto it = startStats.find({x, y});
            const double successRate = it != startStats.end() ? it->second.successRate() : 0.0;
            weights.push_back(1.0 - successRate + epsilon);
        }
    }

    // Weighted random selection
    discrete_distribution<int> dist(weights.begin(), weights.end());
    const auto idx = dist(rng);
    return positions[idx];
}

/*************************************************************************/
int selectAction(const vector<double> &qValues, const int x, const int y, const double epsilon, const int startRow,
                 const int startCol, const int endRow, const int endCol) {
    const double randomValue = static_cast<double>(rand()) / RAND_MAX;

    // Define possible moves
    const vector<pair<int, int> > moves = {
        {-1, 0}, // N
        {-1, 1}, // NE
        {0, 1}, // E
        {1, 1}, // SE
        {1, 0}, // S
        {1, -1}, // SW
        {0, -1}, // W
        {-1, -1} // NW
    };

    // Filter valid actions based on boundaries of the subenvironment
    vector<int> validActions;
    for (int i = 0; i < ACTION_COUNT; ++i) {
        const int newX = x + moves[i].first;
        const int newY = y + moves[i].second;
        if (newX >= startRow && newX <= endRow && newY >= startCol && newY <= endCol) {
            validActions.push_back(i);
        }
    }

    if (validActions.empty()) {
        cerr << "Error: No valid actions available for position (" << x << ", " << y << ")\n";
        return -1; // Indicate an error
    }

    // Epsilon-greedy action selection
    int action;
    if (randomValue < epsilon) {
        // Exploration: Choose a random valid action
        action = validActions[rand() % validActions.size()];
    } else {
        // Exploitation: Choose the action with the highest Q-value among valid actions
        action = validActions[0];
        double maxQValue = qValues[validActions[0]];
        for (const int i: validActions) {
            if (qValues[i] > maxQValue) {
                maxQValue = qValues[i];
                action = i;
            }
        }
    }
    return action;
}

/*************************************************************************/
tuple<int, int, int, double> performAction(const vector<vector<int> > &maze, const int rows, const int cols,
                                           const int x1, const int y1, const int action) {
    double reward = 0.0;
    int changePos = 0;
    int x2 = x1, y2 = y1;

    // Perform the selected action
    switch (action) {
        case 0: // Move N
            if (x1 > 0 && (maze[x1 - 1][y1] == FREE_SPACE || maze[x1 - 1][y1] == CHARGING_STATION)) {
                x2 = x1 - 1;
                changePos = 1;
            }
            break;
        case 1: // Move NE
            if (x1 > 0 && y1 < cols - 1 && (maze[x1 - 1][y1 + 1] == FREE_SPACE || maze[x1 - 1][y1 + 1] ==
                                            CHARGING_STATION)) {
                x2 = x1 - 1;
                y2 = y1 + 1;
                changePos = 1;
            }
            break;
        case 2: // Move E
            if (y1 < cols - 1 && (maze[x1][y1 + 1] == FREE_SPACE || maze[x1][y1 + 1] == CHARGING_STATION)) {
                y2 = y1 + 1;
                changePos = 1;
            }
            break;
        case 3: // Move SE
            if (x1 < rows - 1 && y1 < cols - 1 && (
                    maze[x1 + 1][y1 + 1] == FREE_SPACE || maze[x1 + 1][y1 + 1] == CHARGING_STATION)) {
                x2 = x1 + 1;
                y2 = y1 + 1;
                changePos = 1;
            }
            break;
        case 4: // Move S
            if (x1 < rows - 1 && (maze[x1 + 1][y1] == FREE_SPACE || maze[x1 + 1][y1] == CHARGING_STATION)) {
                x2 = x1 + 1;
                changePos = 1;
            }
            break;
        case 5: // Move SW
            if (x1 < rows - 1 && y1 > 0 && (maze[x1 + 1][y1 - 1] == FREE_SPACE || maze[x1 + 1][y1 - 1] ==
                                            CHARGING_STATION)) {
                x2 = x1 + 1;
                y2 = y1 - 1;
                changePos = 1;
            }
            break;
        case 6: // Move W
            if (y1 > 0 && (maze[x1][y1 - 1] == FREE_SPACE || maze[x1][y1 - 1] == CHARGING_STATION)) {
                y2 = y1 - 1;
                changePos = 1;
            }
            break;
        case 7: // Move NW
            if (x1 > 0 && y1 > 0 && (maze[x1 - 1][y1 - 1] == FREE_SPACE || maze[x1 - 1][y1 - 1] == CHARGING_STATION)) {
                x2 = x1 - 1;
                y2 = y1 - 1;
                changePos = 1;
            }
            break;
        default:
            changePos = 0;
            break;
    }

    // Improved reward system
    if (maze[x2][y2] == CHARGING_STATION) {
        reward = 100.0; // Large reward for reaching the goal
    } else if (changePos == 0) {
        reward = -20.0; // Stronger penalty for hitting obstacles
    } else {
        reward = -2.0; // Increased step penalty to encourage shorter paths
    }

    return make_tuple(x2, y2, action, reward);
}

/*************************************************************************/
void updateQTable(const MazeNode *node, const int x1, const int y1, const int action, const double reward, const int x2,
                  const int y2) {
    // Ensure node and qTable exist
    if (!node || !node->qTable) return;

    // Access Q-values for current state (x1, y1)
    vector<double> &qValues = getLocalValues(*node->qTable, x1, y1, node->startRow, node->startCol);
    if (qValues.empty()) {
        // qValues.resize(ACTION_COUNT, 0.0); // Initialize if not present
        cout << "Warning: Q-values for (" << x1 << ", " << y1 << ") not initialized. Initializing to zero.\n";
    }

    // Access Q-values for next state (x2, y2)
    const vector<double> &nextQValues = getLocalValues(*node->qTable, x2, y2, node->startRow, node->startCol);
    if (nextQValues.empty()) {
        // nextQValues.resize(ACTION_COUNT, 0.0); // Initialize if not present
        cout << "Warning: Q-values for (" << x2 << ", " << y2 << ") not initialized. Initializing to zero.\n";
    }

    // Get the maximum Q-value for the next state
    const double maxQNext = *max_element(nextQValues.begin(), nextQValues.end());

    // Update the Q-value for the current state and action
    qValues[action] += LEARNING_RATE * (reward + DISCOUNT_FACTOR * maxQNext - qValues[action]);
}

/*************************************************************************/
struct Experience {
    int x1, y1, action;
    double reward;
    int x2, y2;
};

/*************************************************************************/
void trainAgentWithStoppingCriterion(MazeNode *node, const vector<vector<int> > &maze, const int rows, const int cols,
                                     const int startRow, const int startCol, const int endRow, const int endCol,
                                     const int maxStepsPerEpisode) {
    int arrival = 0, x2, y2, iteration = 0, counter = 0, stableEpisodes = 0;
    double actionReward = 0;
    bool converged = false;

    // Initialize Q-table if not already done
    node->initQTable();

    // Store previous Q-table state for convergence check
    auto prevQTable = *node->qTable;

    // Convergence parameters
    double epsilon = 1.0;
    constexpr double threshold = 5e-4;
    constexpr int patience = 20;
    constexpr double decayRate = 0.999;
    constexpr int minEpisodes = 500;

    // Experience replay buffer
    vector<Experience> replayBuffer;
    constexpr int bufferSize = 1000;
    replayBuffer.reserve(bufferSize);
    constexpr int batchSize = 64;

    // Track starting position success
    unordered_map<pair<int, int>, StartStats, HashPair> startStats;
    mt19937 rng(random_device{}());

    // Main training loop
    while (!converged && counter < EPISODE_COUNT) {
        auto [x1, y1] = selectFirstPlace(maze, startRow, startCol, endRow, endCol, counter, startStats, rng);
        iteration = 1;

        // Reset episode
        while (arrival == 0 && iteration < maxStepsPerEpisode) {
            // Select action using epsilon-greedy policy and perform it
            vector<double> &qValues = getLocalValues(*node->qTable, x1, y1, startRow, startCol);
            int act = selectAction(qValues, x1, y1, epsilon, startRow, startCol, endRow, endCol);
            tie(x2, y2, act, actionReward) = performAction(maze, rows, cols, x1, y1, act);

            // Store experience in replay buffer and update Q-table
            replayBuffer.push_back({x1, y1, act, actionReward, x2, y2});
            if (replayBuffer.size() > bufferSize) replayBuffer.erase(replayBuffer.begin());
            updateQTable(node, x1, y1, act, actionReward, x2, y2);

            // Perform experience replay
            if (replayBuffer.size() >= batchSize && counter > minEpisodes) {
                for (int i = 0; i < batchSize; i++) {
                    const int idx = rand() % replayBuffer.size();
                    const auto &[x1, y1, action, reward, x2, y2] = replayBuffer[idx];
                    updateQTable(node, x1, y1, action, reward, x2, y2);
                }
            }
            arrival = checkExit(maze, x2, y2);
            x1 = x2;
            y1 = y2;
            iteration++;
        }

        arrival = 0;
        epsilon = max(0.01, epsilon * decayRate);

        // Check for convergence every 50 episodes
        if (counter % 50 == 0 && counter >= minEpisodes) {
            double maxChange = 0.0;
            for (int row = node->startRow; row <= node->endRow; row++) {
                for (int col = node->startCol; col <= node->endCol; col++) {
                    // Get Q-values for the current position
                    vector<double> &qValues = getLocalValues(*node->qTable, row, col, startRow, startCol);
                    vector<double> &prevQValues = getLocalValues(prevQTable, row, col, startRow, startCol);

                    // Calculate maximum change compared to previous Q-table
                    for (int a = 0; a < ACTION_COUNT; a++) {
                        maxChange = max(maxChange, fabs(qValues[a] - prevQValues[a]));
                    }
                }
            }

            // Check for convergence
            if (maxChange < threshold && stableEpisodes >= patience) {
                converged = true;
            } else if (maxChange < threshold) {
                stableEpisodes++;
            } else {
                stableEpisodes = 0;
            }
            prevQTable = *node->qTable;
        }
        counter++;
    }
}

/*************************************************************************/
vector<int> selectTopKActions(const vector<double> &qValues, const int rows, const int cols, const int x, const int y,
                              const int k) {
    const vector<pair<int, int> > moves = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};
    vector<pair<double, int> > validQValues;

    // Collect valid actions with Q-values
    for (int i = 0; i < ACTION_COUNT; ++i) {
        const int newX = x + moves[i].first;
        const int newY = y + moves[i].second;
        if (newX >= 0 && newX < rows && newY >= 0 && newY < cols) {
            validQValues.emplace_back(qValues[i], i);
        }
    }

    if (validQValues.empty()) {
        cerr << "Error: No valid actions at (" << x << ", " << y << ")\n";
        return {};
    }

    // Sort by Q-value descending
    sort(validQValues.begin(), validQValues.end(), greater<pair<double, int> >());

    // Return top k actions (or all if fewer than k)
    vector<int> actions;
    for (int i = 0; i < min(k, static_cast<int>(validQValues.size())); ++i) {
        actions.push_back(validQValues[i].second);
    }
    return actions;
}

/*************************************************************************/
struct PathState {
    int x, y, steps;
    vector<pair<int, int> > path;
};

/*************************************************************************/
tuple<bool, int, vector<pair<int, int> > > findValidPath(const MazeNode *root, const int startX, const int startY,
                                                         const int maxSteps) {
    // Extract maze and dimensions from the root node
    const vector<vector<int> > &maze = *root->maze;
    const int rows = root->rows;
    const int cols = root->cols;

    // Define possible moves
    const vector<pair<int, int> > moves = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};
    queue<PathState> toExplore;
    set<pair<int, int> > visited;
    toExplore.push({startX, startY, 0, {{startX, startY}}});
    visited.insert({startX, startY});

    // BFS to find a valid path
    while (!toExplore.empty()) {
        auto [x, y, steps, path] = toExplore.front();
        toExplore.pop();

        // Check if we reached the maximum steps
        if (steps >= maxSteps) continue;

        // Check if we reached the charging station
        if (maze[x][y] == CHARGING_STATION) {
            return {true, steps, path};
        }

        // Select top k actions based on Q-values
        const vector<double> &qValues = getLocalValues(*root->qTable, x, y, root->startRow, root->startCol);
        vector<int> actions = selectTopKActions(qValues, rows, cols, x, y, 2);
        for (const int act: actions) {
            int newX = x + moves[act].first;
            int newY = y + moves[act].second;
            if (maze[newX][newY] != OBSTACLE && visited.find({newX, newY}) == visited.end()) {
                visited.insert({newX, newY});
                vector<pair<int, int> > newPath = path;
                newPath.emplace_back(newX, newY);
                toExplore.push({newX, newY, steps + 1, newPath});
            }
        }
    }
    return {false, 0, {}}; // No valid path
}

/*************************************************************************/
struct ThreadResult {
    double planningTime = 0.0;
    int successfulPaths = 0;
    int totalSteps = 0;
};

/*************************************************************************/
tuple<double, double, double> testAgent(const MazeNode *root) {
    // Extract maze and dimensions from the root node
    const vector<vector<int> > &maze = *root->maze;
    const int rows = root->rows;
    const int cols = root->cols;
    const int maxStepsPerEpisode = rows + cols;

    // Collect all valid positions to test
    vector<pair<int, int> > positions;
    int totalPositions = 0;
    for (int x1 = 0; x1 < rows; ++x1) {
        for (int y1 = 0; y1 < cols; ++y1) {
            if (maze[x1][y1] != OBSTACLE) {
                positions.emplace_back(x1, y1);
                totalPositions++;
            }
        }
    }

    // Function to process a chunk of positions and return results
    auto processChunk = [&](const size_t startIdx, const size_t endIdx) -> ThreadResult {
        ThreadResult result;
        for (size_t i = startIdx; i < endIdx && i < positions.size(); ++i) {
            const int x1 = positions[i].first;
            const int y1 = positions[i].second;

            auto start = chrono::high_resolution_clock::now();
            auto [success, steps, path] = findValidPath(root, x1, y1, maxStepsPerEpisode);
            auto end = chrono::high_resolution_clock::now();

            result.planningTime += chrono::duration<double>(end - start).count();
            if (success) {
                result.successfulPaths++;
                result.totalSteps += steps;
            }
        }
        return result;
    };

    // Split into chunks and process in parallel
    const size_t totalTasks = positions.size();
    constexpr size_t chunkSize = 20;
    vector<future<ThreadResult> > futures;

    // Determine the number of threads to use
    for (size_t i = 0; i < totalTasks; i += chunkSize) {
        size_t startIdx = i;
        size_t endIdx = min(i + chunkSize, totalTasks);
        futures.push_back(async(launch::async, processChunk, startIdx, endIdx));
    }

    // Aggregate results (single-threaded, after all threads finish)
    double totalPlanningTime = 0.0;
    int successfulPaths = 0;
    int totalSteps = 0;
    for (auto &f: futures) {
        ThreadResult r = f.get();
        totalPlanningTime += r.planningTime;
        successfulPaths += r.successfulPaths;
        totalSteps += r.totalSteps;
    }

    // Compute final metrics
    double successRate = totalPositions > 0 ? static_cast<double>(successfulPaths) / totalPositions : 0.0;
    double avgPathLength = successfulPaths > 0 ? static_cast<double>(totalSteps) / successfulPaths : 0.0;
    double avgPlanningTime = totalPositions > 0 ? totalPlanningTime / totalPositions : 0.0;

    return {avgPlanningTime, successRate, avgPathLength};
}

/*************************************************************************/
void splitMaze(MazeNode *node, const vector<vector<int> > &fullMaze, const int rows, const int cols, const int startRow,
               const int startCol, const int endRow, const int endCol) {
    if ((endRow - startRow + 1) <= 20 && (endCol - startCol + 1) <= 20) {
        return;
    }

    // Split the maze into four quadrants
    const int midRow = (startRow + endRow) / 2;
    const int midCol = (startCol + endCol) / 2;

    // Create child nodes for each quadrant
    auto *child1 = new MazeNode(fullMaze, rows, cols, startRow, startCol, midRow, midCol, node);
    auto *child2 = new MazeNode(fullMaze, rows, cols, startRow, midCol + 1, midRow, endCol, node);
    auto *child3 = new MazeNode(fullMaze, rows, cols, midRow + 1, startCol, endRow, midCol, node);
    auto *child4 = new MazeNode(fullMaze, rows, cols, midRow + 1, midCol + 1, endRow, endCol, node);

    // Add children to the current node
    node->addChild(child1);
    node->addChild(child2);
    node->addChild(child3);
    node->addChild(child4);

    // Recursively split each child node
    splitMaze(child1, fullMaze, rows, cols, startRow, startCol, midRow, midCol);
    splitMaze(child2, fullMaze, rows, cols, startRow, midCol + 1, midRow, endCol);
    splitMaze(child3, fullMaze, rows, cols, midRow + 1, startCol, endRow, midCol);
    splitMaze(child4, fullMaze, rows, cols, midRow + 1, midCol + 1, endRow, endCol);
}

/*************************************************************************/
MazeNode *createSubEnvironments(const vector<vector<int> > &maze, const int rows, const int cols) {
    auto *root = new MazeNode(maze, rows, cols, 0, 0, rows - 1, cols - 1, nullptr, true);
    splitMaze(root, maze, rows, cols, 0, 0, rows - 1, cols - 1);
    return root;
}

/*************************************************************************/
void propagateQTableDownwards(MazeNode *node) {
    if (!node || !node->qTable) return; // Skip if no node or no qTable

    // Propagate to all descendants, updating only those with qTables
    stack<MazeNode *> toVisit;
    toVisit.push(node);

    // DFS to propagate Q-tables
    while (!toVisit.empty()) {
        const MazeNode *current = toVisit.top();
        toVisit.pop();

        for (MazeNode *child: current->children) {
            // If child has no qTable, initialize it
            if (!child->qTable) {
                child->initQTable();
            }
            // Copy Q-values for positions within child's subenvironment
            for (int row = child->startRow; row <= child->endRow; ++row) {
                for (int col = child->startCol; col <= child->endCol; ++col) {
                    vector<double> &childQValues = getLocalValues(*child->qTable, row, col, child->startRow,
                                                                  child->startCol);
                    const vector<double> currentQValues = getLocalValues(*node->qTable, row, col, node->startRow,
                                                                         node->startCol);
                    childQValues = currentQValues; // Copy all action Q-values
                }
            }
            toVisit.push(child); // Continue to child regardless of qTable
        }
    }
}

/*************************************************************************/
void propagateQTableUpwards(const MazeNode *node) {
    if (!node || !node->qTable || !node->parent) return; // Skip if no node, no qTable, or no parent

    const MazeNode *current = node->parent; // Start at parent
    while (current) {
        // Continue until root (no parent)
        if (current->qTable) {
            // Update only if qTable exists
            // Copy Q-values for positions within node's subenvironment
            for (int row = node->startRow; row <= node->endRow; ++row) {
                for (int col = node->startCol; col <= node->endCol; ++col) {
                    vector<double> &parentQValues = getLocalValues(*current->qTable, row, col, current->startRow,
                                                                   current->startCol);
                    const vector<double> nodeQValues = getLocalValues(*node->qTable, row, col, node->startRow,
                                                                      node->startCol);
                    parentQValues = nodeQValues; // Copy all action Q-values
                }
            }
        }
        current = current->parent; // Move up, even if no qTable
    }
}

/*************************************************************************/
vector<pair<int, int> > getObstaclePositions(const vector<vector<int> > &maze, const int rows, const int cols) {
    vector<pair<int, int> > obstaclePositions;

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (maze[i][j] == OBSTACLE) {
                // Check if the cell is an obstacle
                obstaclePositions.emplace_back(i, j);
            }
        }
    }
    return obstaclePositions;
}

/*************************************************************************/
void simulateEnvironmentChanges(const MazeNode *root, const int numSteps, vector<pair<int, int> > &changedPositions) {
    if (!root) {
        cerr << "Error: Root node is null.\n";
        return;
    }

    // Get the obstacle positions
    const int rows = root->rows, cols = root->cols;
    vector<pair<int, int> > obstaclePositions = getObstaclePositions(*root->maze, rows, cols);
    changedPositions.clear(); // Ensure it starts empty

    // Simulate the environment changes
    for (int step = 0; step < numSteps; ++step) {
        // Randomly select an obstacle to move
        if (!obstaclePositions.empty()) {
            const int randomIndex = rand() % obstaclePositions.size();
            int oldRow = obstaclePositions[randomIndex].first;
            int oldCol = obstaclePositions[randomIndex].second;

            vector<pair<int, int> > moves = {
                {oldRow - 1, oldCol}, // Move N
                {oldRow - 1, oldCol + 1}, // Move NE
                {oldRow, oldCol + 1}, // Move E
                {oldRow + 1, oldCol + 1}, // Move SE
                {oldRow + 1, oldCol}, // Move S
                {oldRow + 1, oldCol - 1}, // Move SW
                {oldRow, oldCol - 1}, // Move W
                {oldRow - 1, oldCol - 1} // Move NW
            };

            // Filter valid moves
            vector<pair<int, int> > validMoves;
            for (const auto &move: moves) {
                const int newRow = move.first, newCol = move.second;
                if (newRow >= 0 && newRow < rows &&
                    newCol >= 0 && newCol < cols &&
                    (*root->maze)[newRow][newCol] == FREE_SPACE) {
                    validMoves.push_back(move);
                }
            }

            // If there are valid moves, randomly select one
            if (!validMoves.empty()) {
                const int moveIndex = rand() % validMoves.size();
                int newRow = validMoves[moveIndex].first;
                int newCol = validMoves[moveIndex].second;

                // Record the change
                changedPositions.emplace_back(oldRow, oldCol);
                changedPositions.emplace_back(newRow, newCol);

                // Move the obstacle
                (*root->maze)[oldRow][oldCol] = FREE_SPACE;
                (*root->maze)[newRow][newCol] = OBSTACLE;
                obstaclePositions[randomIndex] = {newRow, newCol};
            }
        }
    }
}

/*************************************************************************/
struct Node {
    int x, y, g, h;
    bool operator>(const Node &other) const { return (g + h) > (other.g + h); }
};

/*************************************************************************/
int heuristic(const int x1, const int y1, const int x2, const int y2) {
    // Use the Chebyshev distance heuristic
    return max(abs(x1 - x2), abs(y1 - y2));
}

/*************************************************************************/
vector<pair<int, int> > reconstructPath(unordered_map<pair<int, int>, pair<int, int>, HashPair> &cameFrom,
                                        const int startX, const int startY, const int goalX, const int goalY) {
    vector<pair<int, int> > path;
    int x = goalX, y = goalY;
    while (!(x == startX && y == startY)) {
        path.emplace_back(x, y);
        tie(x, y) = cameFrom[{x, y}];
    }
    path.emplace_back(startX, startY);
    reverse(path.begin(), path.end());
    return path;
}

unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> computeAllShortestPaths(
    const vector<vector<int> > &maze) {
    const int rows = maze.size();
    const int cols = maze[0].size();
    vector<pair<int, int> > directions = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
    };
    unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> shortestPaths;
    unordered_set<pair<int, int>, HashPair> processed; // Tracks positions with assigned paths

    // Iterate through all cells in the maze
    for (int startX = 0; startX < rows; ++startX) {
        for (int startY = 0; startY < cols; ++startY) {
            if (maze[startX][startY] == OBSTACLE || processed.count({startX, startY})) {
                continue; // Skip obstacles and processed positions
            }

            priority_queue<Node, vector<Node>, greater<> > openSet;
            unordered_map<pair<int, int>, int, HashPair> gScore;
            unordered_map<pair<int, int>, pair<int, int>, HashPair> cameFrom;

            openSet.push({startX, startY, 0, 0});
            gScore[{startX, startY}] = 0;

            bool found = false;
            pair<int, int> goal;

            // A* search
            while (!openSet.empty() && !found) {
                Node current = openSet.top();
                openSet.pop();

                if (maze[current.x][current.y] == CHARGING_STATION) {
                    goal = {current.x, current.y};
                    found = true;
                    break;
                }

                for (auto [dx, dy]: directions) {
                    const int newX = current.x + dx;
                    const int newY = current.y + dy;

                    if (newX >= 0 && newX < rows && newY >= 0 && newY < cols && maze[newX][newY] != OBSTACLE) {
                        const int newG = gScore[{current.x, current.y}] + 1;
                        if (!gScore.count({newX, newY}) || newG < gScore[{newX, newY}]) {
                            gScore[{newX, newY}] = newG;
                            const int h = heuristic(newX, newY, startX, startY);
                            openSet.push({newX, newY, newG, h});
                            cameFrom[{newX, newY}] = {current.x, current.y};
                        }
                    }
                }
            }

            // Process the path and its sub-paths
            if (found) {
                auto path = reconstructPath(cameFrom, startX, startY, goal.first, goal.second);
                // Store sub-paths for all positions on the path
                for (size_t i = 0; i < path.size(); ++i) {
                    auto [px, py] = path[i];
                    if (maze[px][py] == CHARGING_STATION && i == path.size() - 1) {
                        shortestPaths[{px, py}] = {{px, py}}; // Station to itself
                    } else {
                        // Store suffix as shortest path (from px, py to goal)
                        vector<pair<int, int> > subPath(path.begin() + i, path.end());
                        auto pos = make_pair(px, py);
                        // Only store if no path exists or new path is shorter
                        if (!shortestPaths.count(pos) || subPath.size() < shortestPaths[pos].size()) {
                            shortestPaths[pos] = subPath;
                        }
                    }
                    processed.insert({px, py});
                }
            } else {
                shortestPaths[{startX, startY}] = {};
                processed.insert({startX, startY});
            }
        }
    }

    // Ensure all free positions have a path (in case any were missed)
    for (int x = 0; x < rows; ++x) {
        for (int y = 0; y < cols; ++y) {
            if (maze[x][y] != OBSTACLE && !shortestPaths.count({x, y})) {
                shortestPaths[{x, y}] = {};
            }
        }
    }

    return shortestPaths;
}

/*************************************************************************/
tuple<double, double, double> testAgentAStar(const vector<vector<int> > &maze, const int rows, const int cols,
                                             const unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> &
                                             shortestPaths) {
    // Collect all valid positions to test
    vector<pair<int, int> > positions;
    int totalPositions = 0;
    for (int x1 = 0; x1 < rows; ++x1) {
        for (int y1 = 0; y1 < cols; ++y1) {
            if (maze[x1][y1] != OBSTACLE) {
                positions.emplace_back(x1, y1);
                totalPositions++;
            }
        }
    }

    // Function to process a chunk of positions and return results
    auto processChunk = [&](const size_t startIdx, const size_t endIdx) -> ThreadResult {
        ThreadResult result;
        for (size_t i = startIdx; i < endIdx && i < positions.size(); ++i) {
            int startX = positions[i].first;
            int startY = positions[i].second;

            auto start = chrono::high_resolution_clock::now();
            bool success = false;
            int steps = 0;

            // Check if path exists in the precomputed shortest paths
            auto it = shortestPaths.find({startX, startY});
            if (it != shortestPaths.end()) {
                const vector<pair<int, int> > &path = it->second;
                success = !path.empty();
                for (size_t j = 0; j < path.size() && success; ++j) {
                    const int x = path[j].first;
                    const int y = path[j].second;
                    if (maze[x][y] == OBSTACLE) {
                        success = false;
                    }
                }
                if (success) {
                    steps = path.size() - 1;
                }
            }

            auto end = chrono::high_resolution_clock::now();
            result.planningTime += chrono::duration<double>(end - start).count();
            if (success) {
                result.successfulPaths++;
                result.totalSteps += steps;
            }
        }
        return result;
    };

    // Split into chunks and process in parallel
    const size_t totalTasks = positions.size();
    constexpr size_t chunkSize = 50;
    vector<future<ThreadResult> > futures;

    // Determine the number of threads to use
    for (size_t i = 0; i < totalTasks; i += chunkSize) {
        size_t startIdx = i;
        size_t endIdx = min(i + chunkSize, totalTasks);
        futures.push_back(async(launch::async, processChunk, startIdx, endIdx));
    }

    // Aggregate results (single-threaded, after all threads finish)
    double totalPlanningTime = 0.0;
    int successfulPaths = 0;
    int totalSteps = 0;
    for (auto &f: futures) {
        ThreadResult r = f.get();
        totalPlanningTime += r.planningTime;
        successfulPaths += r.successfulPaths;
        totalSteps += r.totalSteps;
    }

    // Compute final metrics
    double successRate = totalPositions > 0 ? static_cast<double>(successfulPaths) / totalPositions : 0.0;
    double avgPathLength = successfulPaths > 0 ? static_cast<double>(totalSteps) / successfulPaths : 0.0;
    double avgPlanningTime = totalPositions > 0 ? totalPlanningTime / totalPositions : 0.0;

    return {avgPlanningTime, successRate, avgPathLength};
}

/*************************************************************************/
void testAStarPerformance(const MazeNode *root, const int numSteps) {
    // Measure A* efficiency (before environment change)
    srand(time(NULL));
    auto start = chrono::high_resolution_clock::now();
    const unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> shortestPaths =
            computeAllShortestPaths(*root->maze);
    auto end = chrono::high_resolution_clock::now();
    const double staticTime = chrono::duration<double>(end - start).count();

    // Simulate environment changes
    srand(42);
    vector<pair<int, int> > changedPositions;
    simulateEnvironmentChanges(root, numSteps, changedPositions);

    // Measure A* efficiency (after environment change)
    srand(time(NULL));
    start = chrono::high_resolution_clock::now();
    const unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> newShortestPaths =
            computeAllShortestPaths(*root->maze);
    end = chrono::high_resolution_clock::now();
    const double dynamicTime = chrono::duration<double>(end - start).count();

    // Test the agent using A* algorithm
    srand(time(NULL));
    auto [planningTime, successRate, avgPath] = testAgentAStar(*root->maze, root->rows, root->cols, newShortestPaths);

    cout << "\n--- A* Algorithm Results ---\n";
    cout << "Static A* Time          : " << staticTime << "s\n";
    cout << "Dynamic A* Time         : " << dynamicTime << "s\n";
    cout << "Average Planning Time   : " << planningTime << "s\n";
    cout << "Success Rate            : " << successRate * 100 << "%\n";
    cout << "Average Path Length     : " << avgPath << " steps\n";
}

/*************************************************************************/
void collectLeafNodes(MazeNode *node, vector<MazeNode *> &leafNodes) {
    if (!node) return;
    if (node->children.empty()) {
        // Leaf node
        leafNodes.push_back(node);
    } else {
        for (MazeNode *child: node->children) {
            collectLeafNodes(child, leafNodes);
        }
    }
}

/*************************************************************************/
double computeNodeSuccessRate(const MazeNode *root, const MazeNode *node) {
    if (!root || !node || !root->maze || !root->qTable)
        return 0.0; // Safety checks

    // Get the sub-environment bounds from the node
    const int startRow = node->startRow;
    const int startCol = node->startCol;
    const int endRow = node->endRow;
    const int endCol = node->endCol;

    // Use the full maze from the root for pathfinding
    const vector<vector<int> > &maze = *root->maze;
    const int rows = root->rows;
    const int cols = root->cols;
    const int maxSteps = rows + cols; // Consistent with testAgent

    int totalPositions = 0;
    int successfulPaths = 0;

    // Iterate over all positions within the node's subenvironment
    for (int x = startRow; x <= endRow; ++x) {
        for (int y = startCol; y <= endCol; ++y) {
            if (maze[x][y] == OBSTACLE) continue; // Skip obstacles
            totalPositions++;

            // Try to find a valid path from this position
            auto [success, steps, path] = findValidPath(root, x, y, maxSteps);
            if (success) {
                successfulPaths++;
            }
        }
    }

    // Compute success rate
    return totalPositions > 0 ? static_cast<double>(successfulPaths) / totalPositions : 0.0;
}


/*************************************************************************/
void fedAsynQ_EqAvg(MazeNode *node, const vector<vector<int> > &maze, const int tau, const int T, const int K = 8) {
    // Create aggregate Q-table
    const int localRows = node->endRow - node->startRow + 1;
    const int localCols = node->endCol - node->startCol + 1;
    auto aggregatedQTable = vector<vector<vector<double> > >(
        localRows, vector<vector<double> >(localCols, vector<double>(ACTION_COUNT, 0.0)));

    // Create previous aggregate Q-table for convergence check
    auto prevAggregatedQTable = vector<vector<vector<double> > >(
        localRows, vector<vector<double> >(localCols, vector<double>(ACTION_COUNT, 0.0)));

    // Local Q-tables for each agent
    vector<vector<vector<vector<double> > > > localQTables(
        K, vector<vector<vector<double> > >(
            localRows, vector<vector<double> >(localCols, vector<double>(ACTION_COUNT, 0.0))));

    // Create a hash map for start statistics
    unordered_map<pair<int, int>, StartStats, HashPair> startStats;
    mutex statsMutex;

    // Random number generators for each agent
    vector<mt19937> rngs(K);
    for (int i = 0; i < K; ++i) {
        rngs[i].seed(random_device{}() + i);
    }

    // Create initial start positions for all agents
    vector<pair<int, int> > agentPositions(K);
    for (int k = 0; k < K; ++k) {
        // Randomly select a start position within the node's bounds
        // auto [x1, y1] = selectFirstPlace(maze, node->startRow, node->startCol, node->endRow, node->endCol);
        auto [x1, y1] = selectFirstPlace(maze, node->startRow, node->startCol, node->endRow, node->endCol, 0,
                                         startStats, rngs[k]);
        agentPositions[k] = {x1, y1};
    }

    // Define epsilon-greedy parameters
    double epsilon = 1.0; // Initial exploration rate

    // Loop for T iterations
    int t = 0;
    while (t < T) {
        // Spawn threads for each agent
        vector<thread> threads;
        for (int k = 0; k < K; ++k) {
            threads.emplace_back(
                [&maze, &node, &agentPositions, &localQTables, &startStats, &statsMutex, epsilon, tau, k ]() {
                    pair<int, int> &agentPosition = agentPositions[k];
                    vector<vector<vector<double> > > &localQTable = localQTables[k];
                    int x1 = agentPosition.first, y1 = agentPosition.second;

                    // Perform tau steps
                    for (int step = 0; step < tau; ++step) {
                        // Select and perform action
                        vector<double> &qValues = getLocalValues(localQTable, x1, y1, node->startRow, node->startCol);
                        int act = selectAction(qValues, x1, y1, epsilon, node->startRow, node->startCol, node->endRow,
                                               node->endCol);

                        int x2, y2, actionReward;
                        tie(x2, y2, act, actionReward) = performAction(maze, node->rows, node->cols, x1, y1, act);

                        // Update Q-value
                        const vector<double> &nextQValues = getLocalValues(
                            localQTable, x2, y2, node->startRow, node->startCol);
                        const double maxNextQ = *max_element(nextQValues.begin(), nextQValues.end());
                        qValues[act] += LEARNING_RATE * (actionReward + DISCOUNT_FACTOR * maxNextQ - qValues[act]);

                        // Update startStats
                        if (step == 0) {
                            lock_guard<mutex> lock(statsMutex);
                            auto &stats = startStats[{x1, y1}];
                            stats.attempts++;
                            if (checkExit(maze, x2, y2)) stats.successes++;
                        }

                        // Move to next position
                        x1 = x2;
                        y1 = y2;
                        agentPosition = {x1, y1};
                    }
                });
        }

        // Join threads to ensure all training is complete
        for (thread &t: threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        // Reset aggregated Q-table
        aggregatedQTable = vector<vector<vector<double> > >(
            localRows, vector<vector<double> >(localCols, vector<double>(ACTION_COUNT, 0.0)));

        // Default alpha for averaging
        double alpha = 1.0 / K;

        // Aggregate Q-values from all local Q-tables
        for (int k = 0; k < K; ++k) {
            for (int row = node->startRow; row <= node->endRow; ++row) {
                for (int col = node->startCol; col <= node->endCol; ++col) {
                    vector<double> &aggregatedQValues = getLocalValues(aggregatedQTable, row, col, node->startRow,
                                                                       node->startCol);
                    vector<double> &localQValues = getLocalValues(localQTables[k], row, col, node->startRow,
                                                                  node->startCol);
                    for (int a = 0; a < ACTION_COUNT; ++a) {
                        aggregatedQValues[a] += alpha * localQValues[a];
                    }
                }
            }
        }

        // Copy the aggregated Q-table back to the local Q-tables
        for (int k = 0; k < K; ++k) {
            localQTables[k] = aggregatedQTable;
        }

        // Compute the maximum difference entry-wise between the aggregated Q-table and the previous Q-table
        double maxDiff = 0.0;
        for (int row = node->startRow; row <= node->endRow; ++row) {
            for (int col = node->startCol; col <= node->endCol; ++col) {
                const vector<double> &currentQ = getLocalValues(aggregatedQTable, row, col, node->startRow,
                                                                node->startCol);
                const vector<double> &prevQ = getLocalValues(prevAggregatedQTable, row, col, node->startRow,
                                                             node->startCol);

                // Compute the difference for each action
                for (int a = 0; a < ACTION_COUNT; ++a) {
                    double diff = abs(currentQ[a] - prevQ[a]);
                    if (diff > maxDiff) {
                        maxDiff = diff;
                    }
                }
            }
        }

        // Log the maximum difference
        // cout << "Iteration " << t << ": Max Q-difference = " << maxDiff << endl;

        // Update the previous Q-table
        prevAggregatedQTable = aggregatedQTable; // Copy current Q-values to previous

        // Select new start positions for all agents
        for (int k = 0; k < K; ++k) {
            // Randomly select a new start position within the node's bounds
            // auto [x1, y1] = selectFirstPlace(maze, node->startRow, node->startCol, node->endRow, node->endCol);
            auto [x1, y1] = selectFirstPlace(maze, node->startRow, node->startCol, node->endRow, node->endCol, t,
                                             startStats, rngs[k]);
            agentPositions[k] = {x1, y1};
        }

        // Increment iteration count by tau
        t += tau;

        // Linear decay of epsilon and learning rate
        // epsilon = 1.0 - static_cast<double>(t) / T; // 1.0 -> 0.0
        // epsilon = max(0.01, epsilon * decayRate);
    }

    // Create final Q-table for the node
    node->qTable = make_unique<vector<vector<vector<double> > > >(aggregatedQTable);
}


/*************************************************************************/
void fedAsynQ_ImAvg(MazeNode *node, const vector<vector<int> > &maze, const int tau, const int T, const int K = 8) {
    // Create aggregate Q-table
    const int localRows = node->endRow - node->startRow + 1;
    const int localCols = node->endCol - node->startCol + 1;
    auto aggregatedQTable = vector<vector<vector<double> > >(
        localRows, vector<vector<double> >(localCols, vector<double>(ACTION_COUNT, 0.0)));

    // Create previous aggregate Q-table for convergence check
    auto prevAggregatedQTable = vector<vector<vector<double> > >(
        localRows, vector<vector<double> >(localCols, vector<double>(ACTION_COUNT, 0.0)));

    // Local Q-tables for each agent
    vector<vector<vector<vector<double> > > > localQTables(
        K, vector<vector<vector<double> > >(
            localRows, vector<vector<double> >(localCols, vector<double>(ACTION_COUNT, 0.0))));

    auto stateActionCounts = vector<vector<vector<vector<double> > > >(
        K, vector<vector<vector<double> > >(
            localRows, vector<vector<double> >(localCols, vector<double>(ACTION_COUNT, 0.0))));

    // Create a hash map for start statistics
    unordered_map<pair<int, int>, StartStats, HashPair> startStats;
    mutex statsMutex;

    // Random number generators for each agent
    vector<mt19937> rngs(K);
    for (int i = 0; i < K; ++i) {
        rngs[i].seed(random_device{}() + i);
    }

    // Create initial start positions for all agents
    vector<pair<int, int> > agentPositions(K);
    for (int k = 0; k < K; ++k) {
        // Randomly select a start position within the node's bounds
        // auto [x1, y1] = selectFirstPlace(maze, node->startRow, node->startCol, node->endRow, node->endCol);
        auto [x1, y1] = selectFirstPlace(maze, node->startRow, node->startCol, node->endRow, node->endCol, 0,
                                         startStats, rngs[k]);
        agentPositions[k] = {x1, y1};
    }

    // Define epsilon-greedy parameters
    double epsilon = 1.0; // Initial exploration rate

    // Loop for T iterations
    int t = 0;
    while (t < T) {
        // Spawn threads for each agent
        vector<thread> threads;
        for (int k = 0; k < K; ++k) {
            threads.emplace_back(
                [&maze, &node, &agentPositions, &localQTables, &stateActionCounts, &startStats, &statsMutex, epsilon,
                    tau, k ]() {
                    pair<int, int> &agentPosition = agentPositions[k];
                    vector<vector<vector<double> > > &localQTable = localQTables[k];

                    // Wrong initialization, but used to avoid compiler errors
                    vector<vector<vector<double> > > &stateActionTable = stateActionCounts[k];
                    int x1 = agentPosition.first, y1 = agentPosition.second;

                    // Perform tau steps
                    for (int step = 0; step < tau; ++step) {
                        // Select and perform action
                        vector<double> &qValues = getLocalValues(localQTable, x1, y1, node->startRow, node->startCol);
                        int act = selectAction(qValues, x1, y1, epsilon, node->startRow, node->startCol, node->endRow,
                                               node->endCol);

                        int x2, y2, actionReward;
                        tie(x2, y2, act, actionReward) = performAction(maze, node->rows, node->cols, x1, y1, act);

                        // Update the state-action count
                        vector<double> &actionCounts = getLocalValues(stateActionTable, x1, y1, node->startRow,
                                                                      node->startCol);
                        actionCounts[act] += 1.0; // Increment action count for this state

                        // Update Q-value
                        const vector<double> &nextQValues = getLocalValues(
                            localQTable, x2, y2, node->startRow, node->startCol);
                        const double maxNextQ = *max_element(nextQValues.begin(), nextQValues.end());
                        qValues[act] += LEARNING_RATE * (actionReward + DISCOUNT_FACTOR * maxNextQ - qValues[act]);

                        // Update startStats
                        if (step == 0) {
                            lock_guard<mutex> lock(statsMutex);
                            auto &stats = startStats[{x1, y1}];
                            stats.attempts++;
                            if (checkExit(maze, x2, y2)) stats.successes++;
                        }

                        // Move to next position
                        x1 = x2;
                        y1 = y2;
                        agentPosition = {x1, y1};
                    }
                });
        }

        // Join threads to ensure all training is complete
        for (thread &t: threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        // Reset aggregated Q-table
        aggregatedQTable = vector<vector<vector<double> > >(
            localRows, vector<vector<double> >(localCols, vector<double>(ACTION_COUNT, 0.0)));

        // Create denominator table for computation of alpha
        auto denominatorTable = vector<vector<vector<double> > >(
            localRows, vector<vector<double> >(localCols, vector<double>(ACTION_COUNT, 0.0)));

        // Compute the denominator for each position in the local Q-tables
        for (int k = 0; k < K; ++k) {
            for (int row = node->startRow; row <= node->endRow; ++row) {
                for (int col = node->startCol; col <= node->endCol; ++col) {
                    vector<double> &denominator = getLocalValues(denominatorTable, row, col, node->startRow,
                                                                 node->startCol);
                    const vector<double> &actionCounts = getLocalValues(stateActionCounts[k], row, col, node->startRow,
                                                                        node->startCol);
                    for (int a = 0; a < ACTION_COUNT; ++a) {
                        denominator[a] += pow(1 - LEARNING_RATE, -1.0 * actionCounts[a]);
                    }
                }
            }
        }

        // Default alpha for averaging
        double alpha = 1.0 / K;

        // Aggregate Q-values from all local Q-tables
        for (int k = 0; k < K; ++k) {
            for (int row = node->startRow; row <= node->endRow; ++row) {
                for (int col = node->startCol; col <= node->endCol; ++col) {
                    vector<double> &aggregatedQValues = getLocalValues(aggregatedQTable, row, col, node->startRow,
                                                                       node->startCol);
                    vector<double> &localQValues = getLocalValues(localQTables[k], row, col, node->startRow,
                                                                  node->startCol);

                    // Compute alpha based on the mode
                    const vector<double> &denominator = getLocalValues(denominatorTable, row, col, node->startRow,
                                                                       node->startCol);
                    const vector<double> &actionCounts = getLocalValues(stateActionCounts[k], row, col, node->startRow,
                                                                        node->startCol);
                    for (int a = 0; a < ACTION_COUNT; ++a) {
                        double nominator = pow(1 - LEARNING_RATE, -1.0 * actionCounts[a]);
                        alpha = nominator / denominator[a]; // Compute alpha
                        aggregatedQValues[a] += alpha * localQValues[a];
                    }
                }
            }
        }

        // Copy the aggregated Q-table back to the local Q-tables
        for (int k = 0; k < K; ++k) {
            localQTables[k] = aggregatedQTable;
        }

        // Compute the maximum difference entry-wise between the aggregated Q-table and the previous Q-table
        double maxDiff = 0.0;
        for (int row = node->startRow; row <= node->endRow; ++row) {
            for (int col = node->startCol; col <= node->endCol; ++col) {
                const vector<double> &currentQ = getLocalValues(aggregatedQTable, row, col, node->startRow,
                                                                node->startCol);
                const vector<double> &prevQ = getLocalValues(prevAggregatedQTable, row, col, node->startRow,
                                                             node->startCol);

                // Compute the difference for each action
                for (int a = 0; a < ACTION_COUNT; ++a) {
                    double diff = abs(currentQ[a] - prevQ[a]);
                    if (diff > maxDiff) {
                        maxDiff = diff;
                    }
                }
            }
        }

        // Log the maximum difference
        // cout << "Iteration " << t << ": Max Q-difference = " << maxDiff << endl;

        // Update the previous Q-table
        prevAggregatedQTable = aggregatedQTable; // Copy current Q-values to previous

        // Reset state-action counts for the next iteration
        stateActionCounts = vector<vector<vector<vector<double> > > >(
            K, vector<vector<vector<double> > >(
                localRows, vector<vector<double> >(localCols, vector<double>(ACTION_COUNT, 0.0))));

        // Select new start positions for all agents
        for (int k = 0; k < K; ++k) {
            // Randomly select a new start position within the node's bounds
            // auto [x1, y1] = selectFirstPlace(maze, node->startRow, node->startCol, node->endRow, node->endCol);
            auto [x1, y1] = selectFirstPlace(maze, node->startRow, node->startCol, node->endRow, node->endCol, t,
                                             startStats, rngs[k]);
            agentPositions[k] = {x1, y1};
        }

        // Increment iteration count by tau
        t += tau;

        // Linear decay of epsilon and learning rate
        // epsilon = 1.0 - static_cast<double>(t) / T; // 1.0 -> 0.0
        // epsilon = max(0.01, epsilon * decayRate);
    }

    // Create final Q-table for the node
    node->qTable = make_unique<vector<vector<vector<double> > > >(aggregatedQTable);
}


/*************************************************************************/
void trainNodesInParallel(MazeNode *root, const vector<MazeNode *> &nodes, const string &mode) {
    // Use threads to train agents concurrently
    vector<thread> threads;

    for (MazeNode *node: nodes) {
        threads.emplace_back([root, node, mode]() {
            if (mode == "Hierarchy") {
                const int maxSteps = (node->endRow - node->startRow + 1) + (node->endCol - node->startCol + 1);
                trainAgentWithStoppingCriterion(node, *root->maze, node->rows, node->cols, node->startRow,
                                                node->startCol, node->endRow, node->endCol, maxSteps);
            } else if (mode == "EqAvg") {
                const int T = (node->endRow - node->startRow + 1) * (node->endCol - node->startCol + 1) * 250;
                fedAsynQ_EqAvg(node, *root->maze, 1000, T, 12);
            } else if (mode == "ImAvg") {
                const int T = (node->endRow - node->startRow + 1) * (node->endCol - node->startCol + 1) * 250;
                fedAsynQ_ImAvg(node, *root->maze, 1000, T, 12);
            }

            // Propagate the Q-table results upwards
            propagateQTableUpwards(node);
            propagateQTableDownwards(node);
        });
    }

    // Join threads to ensure all training is complete
    for (thread &t: threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    cout << "Updating success rates...\n";

    // Recompute success rates for retrained nodes and their descendants
    unordered_set<MazeNode *> visited; // Track nodes to avoid recomputing shared descendants
    for (MazeNode *node: nodes) {
        if (visited.find(node) != visited.end()) continue; // Skip if already processed

        // DFS to recompute success rates for node and descendants
        stack<MazeNode *> toVisit;
        toVisit.push(node);

        while (!toVisit.empty()) {
            MazeNode *current = toVisit.top();
            toVisit.pop();

            // Skip if already visited
            if (visited.find(current) != visited.end()) continue;
            visited.insert(current);

            // Recompute success rate if node has a qTable or was trained
            if (current->qTable) {
                const double newSuccessRate = computeNodeSuccessRate(root, current);
                current->baselineSuccessRate = newSuccessRate;
                cout << "Node (" << current->startRow << ", " << current->startCol << ") -> (" << current->endRow <<
                        ", " << current->endCol << ") " << "Size: " << (current->endRow - current->startRow + 1) << "x"
                        << (current->endCol - current->startCol + 1) << " " << "Success Rate: " << newSuccessRate * 100
                        << "%\n";
            }

            // Add children to visit
            for (MazeNode *child: current->children) {
                toVisit.push(child);
            }
        }
    }
    cout << "Finished updating success rates.\n";
}

/*************************************************************************/
// void runAgentEpisodes(const MazeNode *node, const vector<vector<int> > &maze, const double epsilon,
//                       const int maxStepsPerEpisode, mt19937 &rng,
//                       unordered_map<pair<int, int>, vector<double>, HashPair> &localQTable,
//                       vector<Experience> &localReplayBuffer, const bool useReplay, const int numEpisodes,
//                       unordered_map<pair<int, int>, StartStats, HashPair> &startStats, mutex &statsMutex) {
//
//     // Extract maze dimensions from the node
//     const int startRow = node->startRow;
//     const int startCol = node->startCol;
//     const int endRow = node->endRow;
//     const int endCol = node->endCol;
//
//     // Generate the episodes
//     for (int episode = 0; episode < numEpisodes; ++episode) {
//         int arrival = 0, x2, y2, iteration = 1;
//         double actionReward = 0;
//
//         // Random start position
//         // auto [x1, y1] = selectFirstPlace(maze, startRow, startCol, endRow, endCol);
//         auto [x1, y1] = selectFirstPlace(maze, startRow, startCol, endRow, endCol, 0, startStats, rng);
//         pair<int, int> startPos = {x1, y1};
//
//         // Episode loop
//         while (arrival == 0 && iteration < maxStepsPerEpisode) {
//             // Select and perform action
//             auto qValues = localQTable.find({x1, y1}) != localQTable.end()
//                                ? localQTable[{x1, y1}]
//                                : vector<double>(ACTION_COUNT, 0.0);
//             int act = selectAction(qValues, x1, y1, epsilon, node->rows, node->cols, false, startRow, startCol, endRow,
//                                    endCol);
//             tie(x2, y2, act, actionReward) = performAction(maze, node->rows, node->cols, x1, y1, act);
//
//             // Store experience and update local Q-table
//             localReplayBuffer.push_back({x1, y1, act, actionReward, x2, y2});
//             if (localReplayBuffer.size() > 1000) localReplayBuffer.erase(localReplayBuffer.begin());
//
//             // Simplified updateQTable for local Q-table
//             auto &qValuesCurrent = localQTable[{x1, y1}];
//             if (qValuesCurrent.empty()) qValuesCurrent.resize(ACTION_COUNT, 0.0);
//             auto qValuesNext = localQTable.find({x2, y2}) != localQTable.end()
//                                    ? localQTable[{x2, y2}]
//                                    : vector<double>(ACTION_COUNT, 0.0);
//             const double maxNextQ = *max_element(qValuesNext.begin(), qValuesNext.end());
//             const double oldQ = qValuesCurrent[act];
//             qValuesCurrent[act] = oldQ + LEARNING_RATE * (actionReward + DISCOUNT_FACTOR * maxNextQ - oldQ);
//
//             // Experience replay
//             if (useReplay && localReplayBuffer.size() >= 64) {
//                 for (int i = 0; i < 64; ++i) {
//                     const int idx = rand() % localReplayBuffer.size();
//                     const auto &[x1_r, y1_r, action, reward, x2_r, y2_r] = localReplayBuffer[idx];
//                     auto &qValues_r = localQTable[{x1_r, y1_r}];
//                     if (qValues_r.empty()) qValues_r.resize(ACTION_COUNT, 0.0);
//                     auto qValuesNext_r = localQTable.find({x2_r, y2_r}) != localQTable.end()
//                                              ? localQTable[{x2_r, y2_r}]
//                                              : vector<double>(ACTION_COUNT, 0.0);
//                     const double maxNextQ_r = *max_element(qValuesNext_r.begin(), qValuesNext_r.end());
//                     const double oldQ_r = qValues_r[action];
//                     qValues_r[action] = oldQ_r + LEARNING_RATE * (reward + DISCOUNT_FACTOR * maxNextQ_r - oldQ_r);
//                 }
//             }
//
//             arrival = checkExit(maze, x2, y2);
//             x1 = x2;
//             y1 = y2;
//             iteration++;
//         }
//
//         // Update startStats with episode outcome
//         {
//             lock_guard<mutex> lock(statsMutex);
//             auto& stats = startStats[startPos];
//             stats.attempts++;
//             if (arrival == 1) stats.successes++;
//         }
//     }
//
//     // Remove Q-values for positions outside the node's subenvironment
//     for (auto it = localQTable.begin(); it != localQTable.end();) {
//         const int x = it->first.first;
//         const int y = it->first.second;
//         if (x < startRow || x > endRow || y < startCol || y > endCol) {
//             it = localQTable.erase(it);
//         } else {
//             ++it;
//         }
//     }
// }

/*************************************************************************/
// void trainNodeWithMultiAgents(const MazeNode *root, MazeNode *node, const vector<vector<int> > &maze, double epsilon,
//                               const int maxStepsPerEpisode, int numAgents) {
//     node->initQTable();
//     auto prevQTable = *node->qTable;
//
//     // Determine number of agents based on node size
//     if (node->endRow - node->startRow + 1 >= 40) {
//         numAgents = 8;
//         cout << "Using 8 agents for node size " << (node->endRow - node->startRow + 1) << "x"
//              << (node->endCol - node->startCol + 1) << "\n";
//     }
//
//     // Convergence parameters
//     constexpr double successThreshold = 0.99; // 99% success rate
//     constexpr double stabilityMargin = 0.01; // Allow 1% deviation from best success
//     constexpr int patience = 5; // Wait 5 batches for stability
//     constexpr double decayRate = 0.99; // Epsilon decay per batch
//     constexpr int episodesPerBatch = 50; // Aggregate every 100 episodes
//     int counter = 0, stableEpisodes = 0;
//     bool converged = false;
//     double bestSuccessRate = 0.0; // Track best success rate seen
//
//     // Agent setup
//     vector<thread> threads;
//     vector<mt19937> rngs(numAgents);
//     vector<unordered_map<pair<int, int>, vector<double>, HashPair> > localQTables(numAgents);
//
//     // Initialize all positions in the localQTables
//     // for (int i = node->startRow; i <= node->endRow; ++i) {
//     //     for (int j = node->startCol; j <= node->endCol; ++j) {
//     //         for (auto &qTable: localQTables) {
//     //             qTable[{i, j}] = vector<double>(ACTION_COUNT, 0.0);
//     //         }
//     //     }
//     // }
//
//     // Initialize local Q-tables with shared Q-table
//     for (auto &qTable: localQTables) {
//         qTable = *node->qTable;
//     }
//
//     vector<vector<Experience> > localReplayBuffers(numAgents, vector<Experience>());
//     for (auto &buffer: localReplayBuffers) {
//         buffer.reserve(1000);
//     }
//     for (int i = 0; i < numAgents; ++i) {
//         rngs[i].seed(random_device{}() + i);
//     }
//
//     // Track start position success
//     unordered_map<pair<int, int>, StartStats, HashPair> startStats;
//     mutex statsMutex;
//
//     // Main training loop
//     while (!converged && counter < 10000) {
//         threads.clear();
//
//         // Spawn agent threads for batch of episodes
//         for (int i = 0; i < numAgents; ++i) {
//             threads.emplace_back(runAgentEpisodes, node, ref(maze), epsilon, maxStepsPerEpisode, ref(rngs[i]),
//                                  ref(localQTables[i]),
//                                  ref(localReplayBuffers[i]), false, episodesPerBatch, ref(startStats), ref(statsMutex));
//         }
//
//         // Join threads
//         for (auto &thread: threads) {
//             if (thread.joinable())
//                 thread.join();
//         }
//
//         unordered_map<pair<int, int>, vector<double>, HashPair> newQTable;
//         unordered_map<pair<int, int>, int, HashPair> stateActionCounts;
//
//         // Pre-populate with subenvironment positions
//         for (int r = node->startRow; r <= node->endRow; ++r) {
//             for (int c = node->startCol; c <= node->endCol; ++c) {
//                 newQTable[{r, c}] = vector<double>(ACTION_COUNT, 0.0);
//                 stateActionCounts[{r, c}] = 0;
//             }
//         }
//
//         // Collect all state-action pairs
//         for (const auto &localQTable: localQTables) {
//             for (const auto &[pos, qValues]: localQTable) {
//                 auto &newQValues = newQTable[pos];
//                 auto &counts = stateActionCounts[pos];
//                 counts++;
//                 for (int a = 0; a < ACTION_COUNT; ++a) {
//                     newQValues[a] += qValues[a];
//                 }
//             }
//         }
//
//         // Average Q-values
//         for (auto &[pos, qValues]: newQTable) {
//             if (stateActionCounts[pos] == 0) continue; // Skip if no actions taken
//             for (int a = 0; a < ACTION_COUNT; ++a) {
//                 qValues[a] /= stateActionCounts[pos];
//             }
//         }
//
//         // Update shared Q-table
//         *node->qTable = move(newQTable);
//
//         // Copy shared Q-table back to local Q-tables
//         for (auto &localQTable: localQTables) {
//             localQTable = *node->qTable;
//         }
//
//         // Compute success rate
//         const double successRate = computeNodeSuccessRate(root, node);
//
//         cout << "Batch " << counter << ": "
//              << "Success Rate: " << successRate * 100 << "%, "
//              << "Best Success Rate: " << bestSuccessRate * 100 << "%, "
//              << "Epsilon: " << epsilon << "\n";
//
//         // Check for convergence
//         if (successRate >= successThreshold || fabs(successRate - bestSuccessRate) <= stabilityMargin) {
//             cout << "Incrementing stable episodes...\n";
//             stableEpisodes++;
//             if (stableEpisodes >= patience) {
//                 cout << "Converged after " << counter << " batches.\n";
//                 converged = true;
//             }
//         } else {
//             stableEpisodes = 0;
//         }
//         // Update the best success rate
//         bestSuccessRate = max(bestSuccessRate, successRate);
//
//         // Update epsilon and counter
//         epsilon = max(0.01, epsilon * decayRate);
//         prevQTable = *node->qTable;
//         counter += episodesPerBatch;
//     }
// }

/*************************************************************************/
// void trainNodesInParallelMultiAgents(MazeNode *root, const vector<MazeNode *> &nodes, double epsilon) {
//     constexpr int numAgents = 8; // Adjustable
//     vector<thread> threads;
//
//     for (MazeNode *node: nodes) {
//         threads.emplace_back([root, node, epsilon]() {
//             const int maxSteps = (node->endRow - node->startRow + 1) + (node->endCol - node->startCol + 1);
//             trainNodeWithMultiAgents(root, node, *root->maze, epsilon, maxSteps, numAgents);
//             propagateQTableUpwards(node);
//             propagateQTableDownwards(node);
//         });
//     }
//
//     for (auto &thread: threads) {
//         if (thread.joinable()) {
//             thread.join();
//         }
//     }
//
//     cout << "Updating success rates...\n";
//
//     // Recompute success rates for retrained nodes and their descendants
//     unordered_set<MazeNode *> visited; // Track nodes to avoid recomputing shared descendants
//     for (MazeNode *node: nodes) {
//         if (visited.find(node) != visited.end()) continue; // Skip if already processed
//
//         // DFS to recompute success rates for node and descendants
//         stack<MazeNode *> toVisit;
//         toVisit.push(node);
//
//         while (!toVisit.empty()) {
//             MazeNode *current = toVisit.top();
//             toVisit.pop();
//
//             // Skip if already visited
//             if (visited.find(current) != visited.end()) continue;
//             visited.insert(current);
//
//             // Recompute success rate if node has a qTable or was trained
//             if (current->qTable) {
//                 const double newSuccessRate = computeNodeSuccessRate(root, current);
//                 current->baselineSuccessRate = newSuccessRate;
//                 cout << "Node (" << current->startRow << ", " << current->startCol << ") -> (" << current->endRow <<
//                         ", " << current->endCol << ") " << "Size: " << (current->endRow - current->startRow + 1) << "x"
//                         << (current->endCol - current->startCol + 1) << " " << "Success Rate: " << newSuccessRate * 100
//                         << "%\n";
//             }
//
//             // Add children to visit
//             for (MazeNode *child: current->children) {
//                 toVisit.push(child);
//             }
//         }
//     }
//     cout << "Finished updating success rates.\n";
// }

/*************************************************************************/
void trainNodesSequentially(const MazeNode *root, const vector<MazeNode *> &nodes) {
    for (MazeNode *node: nodes) {
        // Calculate maxSteps dynamically based on leaf size
        const int maxSteps = (node->endRow - node->startRow + 1) + (node->endCol - node->startCol + 1);

        // Train using root's maze and leaf's qTable
        trainAgentWithStoppingCriterion(node, *root->maze, node->rows, node->cols, node->startRow, node->startCol,
                                        node->endRow, node->endCol, maxSteps);

        // Propagate the Q-table results upwards
        propagateQTableUpwards(node);
        propagateQTableDownwards(node);
    }

    cout << "Updating success rates...\n";

    // Recompute success rates for retrained nodes and their descendants
    unordered_set<MazeNode *> visited; // Track nodes to avoid recomputing shared descendants
    for (MazeNode *node: nodes) {
        if (visited.find(node) != visited.end()) continue; // Skip if already processed

        // DFS to recompute success rates for node and descendants
        stack<MazeNode *> toVisit;
        toVisit.push(node);

        while (!toVisit.empty()) {
            MazeNode *current = toVisit.top();
            toVisit.pop();

            // Skip if already visited
            if (visited.find(current) != visited.end()) continue;
            visited.insert(current);

            // Recompute success rate if node has a qTable or was trained
            if (current->qTable) {
                const double newSuccessRate = computeNodeSuccessRate(root, current);
                current->baselineSuccessRate = newSuccessRate;
                cout << "Node (" << current->startRow << ", " << current->startCol << ") -> (" << current->endRow <<
                        ", " << current->endCol << ") " << "Size: " << (current->endRow - current->startRow + 1) << "x"
                        << (current->endCol - current->startCol + 1) << " " << "Success Rate: " << newSuccessRate * 100
                        << "%\n";
            }

            // Add children to visit
            for (MazeNode *child: current->children) {
                toVisit.push(child);
            }
        }
    }
    cout << "Finished updating success rates.\n";
}

/*************************************************************************/
void applyLocalPathPlanning(MazeNode *root, const vector<MazeNode *> &changedLeaves = {}) {
    // Environment is static. Performing global path planning selectively
    if (changedLeaves.empty()) {
        // Train all leaf nodes initially
        vector<MazeNode *> leafNodes;
        collectLeafNodes(root, leafNodes);
        trainNodesInParallel(root, leafNodes, "Hierarchy");
    }
    // Environment changed. Training affected leaf nodes
    else {
        trainNodesInParallel(root, changedLeaves, "Hierarchy");
    }
}

/*************************************************************************/
void trainHierarchy(MazeNode *root, const vector<MazeNode *> &changedLeaves = {}, const int maxLevelsToTrain = 1) {
    if (!root) return;
    const bool isInitialTraining = changedLeaves.empty();

    // Collect leaf nodes to train
    vector<MazeNode *> leafNodesToTrain;
    if (isInitialTraining) {
        collectLeafNodes(root, leafNodesToTrain);
    } else {
        // Use the provided list of affected leaf nodes directly
        leafNodesToTrain = changedLeaves;
    }

    // Train all affected or initial leaf nodes
    trainNodesInParallel(root, leafNodesToTrain, "Hierarchy");

    // Decision mechanism: Count affected children per parent
    unordered_map<MazeNode *, int> parentAffectedCount; // Parent -> # of affected children
    for (const MazeNode *leaf: leafNodesToTrain) {
        if (leaf->parent) {
            parentAffectedCount[leaf->parent]++;
        }
    }

    // Select parents to retrain: >= 1 affected child
    vector<MazeNode *> parentsToTrain;
    for (const auto &[parent, affectedCount]: parentAffectedCount) {
        if (affectedCount >= 1) {
            // Retrain if 1 or more of 4 children are affected
            parentsToTrain.push_back(parent);
        }
    }

    // Train parents hierarchically up to maxLevelsToTrain
    int levelsTrained = 0;
    unordered_set<MazeNode *> currentLevelNodes(parentsToTrain.begin(), parentsToTrain.end());
    while (!currentLevelNodes.empty() && levelsTrained < maxLevelsToTrain) {
        vector<MazeNode *> nodesToTrain(currentLevelNodes.begin(), currentLevelNodes.end());
        trainNodesInParallel(root, nodesToTrain, "Hierarchy");

        // Prepare next level with the same decision rule
        unordered_map<MazeNode *, int> nextLevelAffectedCount;
        for (const MazeNode *node: nodesToTrain) {
            if (node->parent) {
                nextLevelAffectedCount[node->parent]++;
            }
        }

        // Select parents for the next level
        unordered_set<MazeNode *> nextLevelNodes;
        for (const auto &[parent, affectedCount]: nextLevelAffectedCount) {
            if (affectedCount >= 1) {
                // Apply same rule for higher levels
                nextLevelNodes.insert(parent);
            }
        }
        currentLevelNodes = move(nextLevelNodes);
        levelsTrained++;
    }
}

/*************************************************************************/
double getRetrainingThreshold(const int mazeSize) {
    return 0.01;
}

/*************************************************************************/
void trainHierarchySmart(MazeNode *root, const vector<MazeNode *> &changedLeaves = {}) {
    if (!root) return; // Safety check: Exit if root is null

    cout << "\nBegin training...\n";

    // Determine initial training
    const bool isInitialTraining = changedLeaves.empty();

    // Step 1: Collect leaf nodes to train
    vector<MazeNode *> leafNodesToTrain;
    if (isInitialTraining) {
        // Initial training: Gather all leaf nodes in the hierarchy
        collectLeafNodes(root, leafNodesToTrain);
    } else {
        // Dynamic training: Use the list of leaves affected by changes
        leafNodesToTrain = changedLeaves;
    }

    // Step 2: Decide which leaves to train or retrain
    vector<MazeNode *> leavesToRetrain;
    if (isInitialTraining) {
        // For initial training, train all collected leaves
        leavesToRetrain = leafNodesToTrain;
    } else {
        // For changes, check each affected leaf's success rate
        for (MazeNode *leaf: leafNodesToTrain) {
            if (leaf->baselineSuccessRate >= 0) {
                // Only process leaves that were previously trained
                const double baseline = leaf->baselineSuccessRate; // Get the stored success rate
                const double newSuccessRate = computeNodeSuccessRate(root, leaf);

                // Check if the new success rate is significantly lower than the baseline
                if (baseline - newSuccessRate > getRetrainingThreshold(root->rows) || newSuccessRate < 0.9) {
                    leavesToRetrain.push_back(leaf); // Mark leaf for retraining
                } else if (newSuccessRate > baseline) {
                    // Update the baseline success rate for the leaf
                    leaf->baselineSuccessRate = newSuccessRate;
                }
            }
        }
    }

    // Step 3: Train leaves and check for low success
    unordered_set<MazeNode *> parentsToRetrain;
    if (!leavesToRetrain.empty()) {
        // Train all leaves marked for retraining in one batch
        cout << "Training leaves...\n";
        trainNodesInParallel(root, leavesToRetrain, "Hierarchy");
        cout << "Leaves trained.\n";
        for (const MazeNode *leaf: leavesToRetrain) {
            // const double newSuccessRate = computeNodeSuccessRate(root, leaf);
            // leaf->baselineSuccessRate = newSuccessRate;
            if (leaf->baselineSuccessRate < 0.9 && leaf->parent) {
                // If success rate is low, mark parent for retraining
                parentsToRetrain.insert(leaf->parent);
            }
        }
    }

    // Step 4: Propagate retraining upward through the hierarchy
    // Start with parents of retrained leaves
    unordered_set<MazeNode *> currentLevelNodes = parentsToRetrain;
    while (!currentLevelNodes.empty()) {
        // Prepare lists for nodes to train in this level and parents for the next level
        vector<MazeNode *> nodesToTrain;
        unordered_set<MazeNode *> nextLevelNodes;

        // Process each node in the current level
        for (MazeNode *node: currentLevelNodes) {
            if (node->baselineSuccessRate < 0) {
                // Node is untrained
                nodesToTrain.push_back(node);
            } else {
                // Node is already trained
                const double baseline = node->baselineSuccessRate; // Get the stored success rate
                const double newSuccessRate = computeNodeSuccessRate(root, node);

                // Check if the new success rate is significantly lower than the baseline
                if (baseline - newSuccessRate > getRetrainingThreshold(root->rows) || newSuccessRate < 0.9) {
                    nodesToTrain.push_back(node); // Mark node for retraining
                } else if (newSuccessRate > baseline) {
                    // Update the baseline success rate for the leaf
                    node->baselineSuccessRate = newSuccessRate;
                }
            }
        }

        // Step 5: Train the selected nodes in this level
        if (!nodesToTrain.empty()) {
            // Train all marked nodes in one batch
            cout << "Training nodes...\n";
            trainNodesInParallel(root, nodesToTrain, "Hierarchy");
            cout << "Nodes trained.\n";
            // Update each trained node's baseline success rate
            for (const MazeNode *node: nodesToTrain) {
                // const double successRate = computeNodeSuccessRate(root, node);
                // node->baselineSuccessRate = successRate;
                if (node->baselineSuccessRate < 0.9 && node->parent) {
                    // If success rate is low, mark parent for retraining
                    nextLevelNodes.insert(node->parent);
                }
            }
        }

        // Move to the next level of parents to check
        currentLevelNodes = move(nextLevelNodes);
    }
    cout << "Training complete for all levels.\n";
}

/*************************************************************************/
void trainHierarchySmartMultiAgent(const string &mode, MazeNode *root, const vector<MazeNode *> &changedLeaves = {}) {
    if (!root) return; // Safety check: Exit if root is null

    cout << "\nBegin training...\n";

    // Determine initial training
    const bool isInitialTraining = changedLeaves.empty();

    // Step 1: Collect leaf nodes to train
    vector<MazeNode *> leafNodesToTrain;
    if (isInitialTraining) {
        // Initial training: Gather all leaf nodes in the hierarchy
        collectLeafNodes(root, leafNodesToTrain);
    } else {
        // Dynamic training: Use the list of leaves affected by changes
        leafNodesToTrain = changedLeaves;
    }

    // Step 2: Decide which leaves to train or retrain
    vector<MazeNode *> leavesToRetrain;
    if (isInitialTraining) {
        // For initial training, train all collected leaves
        leavesToRetrain = leafNodesToTrain;
    } else {
        // For changes, check each affected leaf's success rate
        for (MazeNode *leaf: leafNodesToTrain) {
            if (leaf->baselineSuccessRate >= 0) {
                // Only process leaves that were previously trained
                const double baseline = leaf->baselineSuccessRate; // Get the stored success rate
                const double newSuccessRate = computeNodeSuccessRate(root, leaf);

                cout << "Leaf (" << leaf->startRow << ", " << leaf->startCol << ") -> (" << leaf->endRow
                        << ", " << leaf->endCol << ") Success Rate: " << newSuccessRate * 100 << "%\n";

                // Check if the new success rate is significantly lower than the baseline
                if (baseline - newSuccessRate > getRetrainingThreshold(root->rows) || newSuccessRate < 0.9) {
                    leavesToRetrain.push_back(leaf); // Mark leaf for retraining
                } else if (newSuccessRate > baseline) {
                    // Update the baseline success rate for the leaf
                    leaf->baselineSuccessRate = newSuccessRate;
                }
            }
        }
    }

    // Step 3: Train leaves and check for low success
    unordered_set<MazeNode *> parentsToRetrain;
    if (!leavesToRetrain.empty()) {
        // Train all leaves marked for retraining in one batch
        cout << "Training leaves...\n";

        // TODO: check this
        trainNodesInParallel(root, leavesToRetrain, mode);
        // if (isInitialTraining) {
        //     trainNodesInParallel(root, leavesToRetrain, 1.0);
        // }
        // } else {
        //     trainNodesInParallelMultiAgents(root, leavesToRetrain, 1.0);
        // }

        cout << "Leaves trained.\n";
        for (const MazeNode *leaf: leavesToRetrain) {
            // const double newSuccessRate = computeNodeSuccessRate(root, leaf);
            // leaf->baselineSuccessRate = newSuccessRate;
            if (leaf->baselineSuccessRate < 0.9 && leaf->parent) {
                // If success rate is low, mark parent for retraining
                parentsToRetrain.insert(leaf->parent);
            }
        }

        // Step 4: Propagate retraining upward through the hierarchy
        // Start with parents of retrained leaves
        unordered_set<MazeNode *> currentLevelNodes = parentsToRetrain;
        while (!currentLevelNodes.empty()) {
            // Prepare lists for nodes to train in this level and parents for the next level
            vector<MazeNode *> nodesToTrain;
            unordered_set<MazeNode *> nextLevelNodes;

            // Process each node in the current level
            for (MazeNode *node: currentLevelNodes) {
                if (node->baselineSuccessRate < 0) {
                    // Node is untrained
                    nodesToTrain.push_back(node);
                } else {
                    // Node is already trained
                    const double baseline = node->baselineSuccessRate; // Get the stored success rate
                    const double newSuccessRate = computeNodeSuccessRate(root, node);

                    // Check if the new success rate is significantly lower than the baseline
                    if (baseline - newSuccessRate > getRetrainingThreshold(root->rows) || newSuccessRate < 0.9) {
                        nodesToTrain.push_back(node); // Mark node for retraining
                    } else if (newSuccessRate > baseline) {
                        // Update the baseline success rate for the leaf
                        node->baselineSuccessRate = newSuccessRate;
                    }
                }
            }

            // Step 5: Train the selected nodes in this level
            if (!nodesToTrain.empty()) {
                // Train all marked nodes in one batch
                cout << "Training nodes...\n";

                // TODO : check this
                trainNodesInParallel(root, nodesToTrain, mode);
                // trainNodesInParallel(root, nodesToTrain, 1.0);
                // trainNodesInParallelMultiAgents(root, nodesToTrain, 1.0);

                cout << "Nodes trained.\n";
                // Update each trained node's baseline success rate
                for (const MazeNode *node: nodesToTrain) {
                    // const double successRate = computeNodeSuccessRate(root, node);
                    // node->baselineSuccessRate = successRate;
                    if (node->baselineSuccessRate < 0.9 && node->parent) {
                        // If success rate is low, mark parent for retraining
                        nextLevelNodes.insert(node->parent);
                    }
                }
            }

            // Move to the next level of parents to check
            currentLevelNodes = move(nextLevelNodes);
        }
        cout << "Training complete for all levels.\n";
    }
}

/*************************************************************************/
void testLocalPathPlanning(MazeNode *root, const int numSteps) {
    // Measure Local Path Planning efficiency (before environment change)
    srand(time(NULL));
    auto start = chrono::high_resolution_clock::now();
    applyLocalPathPlanning(root);
    auto end = chrono::high_resolution_clock::now();
    const double localStaticTime = chrono::duration<double>(end - start).count();

    // Simulate environment changes and reapply local path planning (if needed)
    srand(42);
    vector<pair<int, int> > changedPositions;
    simulateEnvironmentChanges(root, numSteps, changedPositions);

    // Identify changed leaves
    unordered_set<MazeNode *> changedLeaves;
    for (const auto &[r, c]: changedPositions) {
        MazeNode *leaf = root->findSubEnvironment(r, c);
        if (leaf && leaf->children.empty()) changedLeaves.insert(leaf);
    }
    const auto changedLeavesToTrain = vector<MazeNode *>(changedLeaves.begin(), changedLeaves.end());

    // Measure Local Path Planning efficiency (after environment change)
    srand(time(NULL));
    start = chrono::high_resolution_clock::now();
    applyLocalPathPlanning(root, changedLeavesToTrain);
    end = chrono::high_resolution_clock::now();
    const double localDynamicTime = chrono::duration<double>(end - start).count();

    // Test the agent's performance after applying local path planning
    srand(time(NULL));
    auto [localPlanningTime, localSuccessRate, localAvgPath] = testAgent(root);

    cout << "\n--- Local Path Planning Results ---\n";
    cout << "Static Training Time     : " << localStaticTime << "s\n";
    cout << "Dynamic Training Time    : " << localDynamicTime << "s\n";
    cout << "Average Planning Time    : " << localPlanningTime << "s\n";
    cout << "Success Rate             : " << localSuccessRate * 100 << "%\n";
    cout << "Average Path Length      : " << localAvgPath << " steps\n";
}

/*************************************************************************/
void testHierarchicalPathPlanning(MazeNode *root, const int numSteps) {
    // Measure Hierarchical Path Planning efficiency (before environment change)
    srand(time(NULL));
    auto start = chrono::high_resolution_clock::now();
    trainHierarchy(root); // Initial training of the full hierarchy
    auto end = chrono::high_resolution_clock::now();
    const double hierarchyStaticTime = chrono::duration<double>(end - start).count();

    // Simulate environment changes
    srand(42); // Consistent seed for reproducibility
    vector<pair<int, int> > changedPositions;
    simulateEnvironmentChanges(root, numSteps, changedPositions);

    // Identify changed leaves
    unordered_set<MazeNode *> changedLeaves;
    for (const auto &[r, c]: changedPositions) {
        MazeNode *leaf = root->findSubEnvironment(r, c);
        if (leaf && leaf->children.empty()) changedLeaves.insert(leaf);
    }
    const auto changedLeavesToTrain = vector<MazeNode *>(changedLeaves.begin(), changedLeaves.end());

    // Measure Hierarchical Path Planning efficiency (after environment change)
    srand(time(NULL));
    start = chrono::high_resolution_clock::now();
    trainHierarchy(root, changedLeavesToTrain); // Retrain affected nodes and propagate up
    end = chrono::high_resolution_clock::now();
    const double hierarchyDynamicTime = chrono::duration<double>(end - start).count();

    // Test the agent's performance using the root's Q-table (top-level policy)
    srand(time(NULL));
    auto [hierarchyPlanningTime, hierarchySuccessRate, hierarchyAvgPath] = testAgent(root);

    cout << "\n--- Hierarchical Path Planning Results ---\n";
    cout << "Static Training Time     : " << hierarchyStaticTime << "s\n";
    cout << "Dynamic Training Time    : " << hierarchyDynamicTime << "s\n";
    cout << "Average Planning Time    : " << hierarchyPlanningTime << "s\n";
    cout << "Success Rate             : " << hierarchySuccessRate * 100 << "%\n";
    cout << "Average Path Length      : " << hierarchyAvgPath << " steps\n";
}

const vector<pair<int, int> > ACTIONS = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};

/*************************************************************************/
class Agent {
public:
    int row, col;
    MazeNode *subEnv;

    Agent(const int r, const int c, MazeNode *env) : row(r), col(c), subEnv(env) {
    }

    int selectAction(const double epsilon) const {
        // return selectActionWithSoftBoundaries(*subEnv->qTable, subEnv->rows, subEnv->cols, row, col, epsilon);
        return rand() % ACTIONS.size(); // Random action for simplicity
    }

    pair<int, int> step(const int action) {
        const int new_row = row + ACTIONS[action].first;
        const int new_col = col + ACTIONS[action].second;

        // Check if movement is valid
        if (new_row >= subEnv->startRow && new_row <= subEnv->endRow &&
            new_col >= subEnv->startCol && new_col <= subEnv->endCol &&
            (*subEnv->maze)[new_row][new_col] != OBSTACLE &&
            (*subEnv->maze)[new_row][new_col] != AGENT) {
            // **Restore previous position correctly**
            if ((*subEnv->maze)[row][col] != CHARGING_STATION) {
                (*subEnv->maze)[row][col] = FREE_SPACE; // Restore free space only if it wasn't a charging station
            }

            // Move agent
            row = new_row;
            col = new_col;

            // **Do NOT overwrite charging stations**
            if ((*subEnv->maze)[row][col] != CHARGING_STATION) {
                (*subEnv->maze)[row][col] = AGENT; // Mark new position as occupied by agent
            }

            // **Only check for new subenvironment if the agent is NOT in the root**
            if (subEnv->parent) {
                // Find the root node of the hierarchy
                MazeNode *root = subEnv;
                while (root->parent) {
                    root = root->parent;
                }
                MazeNode *newSubEnv = root->findSubEnvironment(row, col);
                if (newSubEnv && newSubEnv != subEnv) {
                    subEnv = newSubEnv; // Update the agent's subenvironment
                }
            }
        }
        return {row, col};
    }

    vector<pair<int, int> > findOptimalPath() {
        vector<pair<int, int> > path;
        while ((*subEnv->maze)[row][col] != CHARGING_STATION) {
            path.emplace_back(row, col);

            // Select action based on the current subenvironment's Q-table
            const int action = selectAction(0.0);
            const pair<int, int> newPos = step(action);

            // Check if the new position falls into a different subenvironment
            MazeNode *newSubEnv = subEnv->parent->findSubEnvironment(newPos.first, newPos.second);
            if (newSubEnv && newSubEnv != subEnv) {
                subEnv = newSubEnv;
            }
        }
        path.emplace_back(row, col);
        return path;
    }
};

/*************************************************************************/
class VDNTrainer {
public:
    vector<Agent *> agents; // List of agents

    // Constructor
    explicit VDNTrainer(MazeNode *root) {
        // For each leaf node, create an agent
        vector<MazeNode *> leafNodes = {};
        collectLeafNodes(root, leafNodes);
        for (MazeNode *leaf: leafNodes) {
            agents.push_back(new Agent(leaf->startRow, leaf->startCol, leaf));
        }
    }

    ~VDNTrainer() {
        for (const Agent *agent: agents) {
            delete agent; // Clean up dynamically allocated agents
        }
    }

    // Function to update global Q-values (already in your trainVDN function)
    void updateGlobalQValues(const vector<double> &rewards, const vector<pair<int, int> > &next_positions,
                             const vector<int> &actions) const {
        double globalQ = 0.0; // Sum of all agents' Q-values (VDN principle)

        // Compute current global Q-value
        for (size_t i = 0; i < agents.size(); i++) {
            const Agent *agent = agents[i];
            const int row = agent->row;
            const int col = agent->col;
            const int action = actions[i];

            if (action == -1) continue; // Skip agents that reached a charging station

            // globalQ += (*agent->subEnv->qTable)[row][col][action]; // Sum local Q-values
        }

        // Compute max Q-value for the next state (using individual max Q-values)
        double maxNextGlobalQ = 0.0;
        for (size_t i = 0; i < agents.size(); i++) {
            const int nextRow = next_positions[i].first;
            const int nextCol = next_positions[i].second;

            // const double maxQ = *max_element((*agents[i]->subEnv->qTable)[nextRow][nextCol].begin(),
            // (*agents[i]->subEnv->qTable)[nextRow][nextCol].end());

            // maxNextGlobalQ += maxQ; // Sum max Q-values for each agent
        }

        // Compute the TD error (same for all agents in VDN)
        double td_error = 0.0;
        for (size_t i = 0; i < agents.size(); i++) {
            td_error += rewards[i]; // Sum of all rewards
        }
        td_error += DISCOUNT_FACTOR * maxNextGlobalQ - globalQ; // Compute TD error

        // Update individual Q-values using the shared TD error
        for (size_t i = 0; i < agents.size(); i++) {
            Agent *agent = agents[i];
            const int row = agent->row;
            const int col = agent->col;
            const int action = actions[i];

            if (action == -1) continue; // Skip agents that reached a charging station

            // Q-learning update rule for each agent
            // double &qValue = (*agent->subEnv->qTable)[row][col][action];
            // qValue += LEARNING_RATE * td_error; // Each agent updates using the same TD error
        }
    }
};

/*************************************************************************/
void trainVDN(VDNTrainer &trainer, double epsilon) {
    // Convergence parameters
    constexpr double threshold = 1e-6; // Convergence threshold
    constexpr int patience = 50; // Required stable episodes
    constexpr int maxEpisodes = 1'000'000; // Safety limit
    constexpr double decayRate = 0.999;

    int stableEpisodes = 0;
    int converged = 0;
    int episode;

    for (episode = 0; !converged; episode++) {
        // **1. Reset the environment and randomly place agents**
        vector<int> reached_goal(trainer.agents.size(), 0); // Track agents reaching charging stations

        for (Agent *agent: trainer.agents) {
            MazeNode *subEnv = agent->subEnv;
            // tie(agent->row, agent->col) = selectFirstPlace(*subEnv->maze, subEnv->startRow, subEnv->startCol,
            //                                                subEnv->endRow, subEnv->endCol);
            (*subEnv->maze)[agent->row][agent->col] = AGENT;
        }

        // **2. Episode execution**
        for (int step = 0; step < maxEpisodes; step++) {
            vector<int> actions;
            vector<pair<int, int> > next_positions;
            vector<double> rewards;
            set<pair<int, int> > occupied_positions;

            for (size_t i = 0; i < trainer.agents.size(); i++) {
                Agent *agent = trainer.agents[i];

                // If this agent has already reached a charging station, it doesn't move anymore
                if (reached_goal[i]) {
                    continue;
                }

                pair<int, int> prev_pos = {agent->row, agent->col};
                const int action = agent->selectAction(epsilon);
                pair<int, int> next_pos = agent->step(action);

                double reward = -1.0; // Default step penalty
                if (occupied_positions.count(next_pos)) {
                    reward = -5.0; // Collision penalty
                    next_pos = prev_pos;
                } else if ((*agent->subEnv->maze)[next_pos.first][next_pos.second] == CHARGING_STATION) {
                    reward = 30.0; // Goal reward
                    reached_goal[i] = 1; // Mark this agent as having reached the goal
                } else if ((*agent->subEnv->maze)[next_pos.first][next_pos.second] == OBSTACLE) {
                    reward = -10.0; // Obstacle penalty
                }

                // Update the agent's q-values
                // updateQTable(*agent->subEnv->qTable, prev_pos.first, prev_pos.second, action, reward, next_pos.first,
                //              next_pos.second);
            }

            // **3. Update Q-values**
            // trainer.updateGlobalQValues(rewards, next_positions, actions);

            // **4. Check if all agents have reached a charging station**
            if (all_of(reached_goal.begin(), reached_goal.end(), [](int x) { return x == 1; })) {
                break; // End episode early
            }
        }

        // **5. Apply exponential decay to epsilon**
        constexpr double lambda = 1e-9;
        constexpr double epsilon_min = 1e-3;
        epsilon = max(0.01, epsilon * decayRate);

        // **6. Clear agents from the environment using the reached_goal vector**
        for (size_t i = 0; i < trainer.agents.size(); i++) {
            Agent *agent = trainer.agents[i];
            // Change position to charging station if agent reached it
            if (reached_goal[i]) {
                (*agent->subEnv->maze)[agent->row][agent->col] = CHARGING_STATION;
                // Otherwise, clear the agent's position
            } else {
                (*agent->subEnv->maze)[agent->row][agent->col] = FREE_SPACE;
            }
        }

        // TODO: Fix the previous Q-table comparison code
        // **7. Check stopping criterion every 10 episodes**
        // if (episode % 10 == 0) {
        //     double maxChange = 0.0;
        //
        //     for (Agent *agent : trainer.agents) {
        //         MazeNode *subEnv = agent->subEnv;
        //
        //         // Iterate over all states in the agent's subenvironment
        //         for (int r = subEnv->startRow; r <= subEnv->endRow; r++) {
        //             for (int c = subEnv->startCol; c <= subEnv->endCol; c++) {
        //                 for (size_t action = 0; action < subEnv->qTable[r][c].size(); action++) {
        //                     // Compute absolute difference in Q-values
        //                     double diff = fabs(subEnv->qTable[r][c][action] - prevQTable[subEnv][r][c][action]);
        //                     maxChange = max(maxChange, diff);
        //                 }
        //             }
        //         }
        //     }
        //
        //     // Store the current Q-table as the new previous Q-table for next iteration
        //     prevQTable = {};
        //     for (Agent *agent : trainer.agents) {
        //         MazeNode *subEnv = agent->subEnv;
        //         for (int r = subEnv->startRow; r <= subEnv->endRow; r++) {
        //             for (int c = subEnv->startCol; c <= subEnv->endCol; c++) {
        //                 prevQTable[subEnv][r][c] = subEnv->qTable[r][c]; // Save Q-table snapshot
        //             }
        //         }
        //     }
        //
        //     if (maxChange < threshold) {
        //         stableEpisodes++;
        //         if (stableEpisodes >= patience) {
        //             converged = true;
        //         }
        //     } else {
        //         stableEpisodes = 0; // Reset stability count if Q-values are still changing
        //     }
        // }

        // **8. Safety stop condition**
        if (++episode >= maxEpisodes) {
            cout << "Training stopped after reaching max episodes (" << maxEpisodes << ").\n";
            break;
        }
    }

    cout << "Training converged after " << episode << " episodes.\n";
}

/*************************************************************************/
void testVDNTraining(MazeNode *root, const int numChanges) {
    constexpr double epsilon = 1.0; // Exploration parameter

    // Create the VDN trainer
    VDNTrainer trainer(root);

    // Measure Training Time (Static Environment)
    srand(time(NULL));
    auto start = chrono::high_resolution_clock::now();
    trainVDN(trainer, epsilon);
    auto end = chrono::high_resolution_clock::now();
    const double staticTrainingTime = chrono::duration<double>(end - start).count();

    // Apply Environment Changes
    srand(42);
    vector<pair<int, int> > changedPositions;
    simulateEnvironmentChanges(root, numChanges, changedPositions);

    // Measure Training Time (Dynamic Environment)
    srand(time(NULL));
    start = chrono::high_resolution_clock::now();
    trainVDN(trainer, epsilon); // Train for fewer episodes after changes
    end = chrono::high_resolution_clock::now();
    const double dynamicTrainingTime = chrono::duration<double>(end - start).count();

    // Evaluate Performance with Path Planning
    srand(time(NULL));
    auto [vdnPlanningTime, vdnSuccessRate, vdnAvgPath] = testAgent(root);

    // Output Results
    cout << "\n--- VDN Training Results ---\n";
    cout << "Static Training Time     : " << staticTrainingTime << "s\n";
    cout << "Dynamic Training Time    : " << dynamicTrainingTime << "s\n";
    cout << "Average Planning Time    : " << vdnPlanningTime << "s\n";
    cout << "Success Rate             : " << vdnSuccessRate * 100 << "%\n";
    cout << "Average Path Length      : " << vdnAvgPath << " steps\n";
}

/*************************************************************************/
struct Metrics {
    double initialTime;
    double adaptTime;
    double successRate;
    double avgPathLength;
};

/*************************************************************************/
class PolicyVisualizer {
private:
    sf::RenderWindow window_;
    const MazeNode* node_;
    int size_;
    string approach_;
    int max_timesteps_;
    int current_timestep_;
    float cell_size_;
    vector<sf::RectangleShape> grid_;
    vector<std::pair<sf::RectangleShape, sf::CircleShape>> arrows_;
    sf::Font font_;

    // Action to arrow direction (dx, dy) for visualization
    const vector<std::pair<float, float>> action_arrows_ = {
        {0.0f, -0.3f},  // 0: Up
        {0.3f, -0.3f},  // 1: Up-right
        {0.3f, 0.0f},   // 2: Right
        {0.3f, 0.3f},   // 3: Down-right
        {0.0f, 0.3f},   // 4: Down
        {-0.3f, 0.3f},  // 5: Down-left
        {-0.3f, 0.0f},  // 6: Left
        {-0.3f, -0.3f}  // 7: Up-left
    };

public:
    PolicyVisualizer(const MazeNode* node, int size, const string& approach, int max_timesteps)
        : node_(node), size_(size), approach_(approach), max_timesteps_(max_timesteps), current_timestep_(0) {
        // Initialize window (800x800 or scaled for large mazes)
        int window_size = std::min(800, size * 20);
        cell_size_ = static_cast<float>(window_size) / size_;
        window_.create(sf::VideoMode(window_size, window_size + 50), "Policy Visualization");
        window_.setFramerateLimit(60);

        // Load font (optional for time step text)
        font_.loadFromFile("arial.ttf"); // Ignore failure for arrows

        // Initialize grid
        grid_.resize(size_ * size_);
        for (int row = 0; row < size_; ++row) {
            for (int col = 0; col < size_; ++col) {
                sf::RectangleShape& cell = grid_[row * size_ + col];
                cell.setSize(sf::Vector2f(cell_size_, cell_size_));
                cell.setPosition(col * cell_size_, row * cell_size_);
                cell.setOutlineThickness(1.0f);
                cell.setOutlineColor(sf::Color(150, 150, 150)); // Gray grid lines
            }
        }
    }

    // Update visualization for the current time step
    void update() {
        if (current_timestep_ >= max_timesteps_) return;

        const auto& maze = *node_->maze;
        const auto& qTable = *node_->qTable;

        // Update grid colors
        for (int row = 0; row < size_; ++row) {
            for (int col = 0; col < size_; ++col) {
                sf::RectangleShape& cell = grid_[row * size_ + col];
                int value = maze[row][col];
                if (value == OBSTACLE) {
                    cell.setFillColor(sf::Color::Black);
                } else if (value == FREE_SPACE) {
                    cell.setFillColor(sf::Color::White);
                } else if (value == CHARGING_STATION) {
                    cell.setFillColor(sf::Color::Yellow);
                }
            }
        }

        // Update policy arrows (only for free spaces)
        arrows_.clear();
        for (int row = 0; row < size_; ++row) {
            for (int col = 0; col < size_; ++col) {
                if (maze[row][col] == FREE_SPACE) {
                    // Convert global to local indices
                    int localRow = row - node_->startRow;
                    int localCol = col - node_->startCol;
                    if (localRow >= 0 && localRow < qTable.size() &&
                        localCol >= 0 && localCol < qTable[localRow].size()) {
                        const auto& q_values = qTable[localRow][localCol];
                        int best_action = std::distance(q_values.begin(),
                            std::max_element(q_values.begin(), q_values.end()));

                        // Create arrow: line + triangular arrowhead
                        auto [dx, dy] = action_arrows_[best_action];
                        float angle = std::atan2(dy, dx) * 180 / 3.14159;
                        float length = 0.3f * cell_size_; // Line length
                        float center_x = (col + 0.5f) * cell_size_;
                        float center_y = (row + 0.5f) * cell_size_;

                        // Line body (rectangle)
                        sf::RectangleShape line(sf::Vector2f(length, 0.03f * cell_size_));
                        line.setOrigin(0.0f, 0.015f * cell_size_);
                        line.setPosition(center_x - length / 2 * dx / 0.3f, center_y - length / 2 * dy / 0.3f);
                        line.setRotation(angle);
                        line.setFillColor(sf::Color::Red);
                        line.setOutlineColor(sf::Color::Black);
                        line.setOutlineThickness(0.5f);

                        // Arrowhead (triangle via CircleShape with 3 points)
                        sf::CircleShape arrowhead(0.1f * cell_size_, 3); // Radius ~4px for 50x50
                        arrowhead.setOrigin(0.1f * cell_size_, 0.1f * cell_size_);
                        arrowhead.setPosition(center_x + length / 2 * dx / 0.35f, center_y + length / 2 * dy / 0.35f);
                        arrowhead.setRotation(angle - 30.0f);
                        arrowhead.setFillColor(sf::Color::Red);
                        arrowhead.setOutlineColor(sf::Color::Black);
                        arrowhead.setOutlineThickness(0.5f);

                        arrows_.emplace_back(line, arrowhead);
                    }
                }
            }
        }

        ++current_timestep_;
    }

    // Render the visualization
    void render() {
        window_.clear(sf::Color::White);

        // Draw grid
        for (const auto& cell : grid_) {
            window_.draw(cell);
        }

        // Draw arrows
        for (const auto& [line, arrowhead] : arrows_) {
            window_.draw(line);
            window_.draw(arrowhead);
        }

        // Draw time step text (if font loaded)
        if (font_.getInfo().family != "") {
            sf::Text text;
            text.setFont(font_);
            text.setString("Time Step: " + std::to_string(current_timestep_) + " - " + approach_);
            text.setCharacterSize(20);
            text.setFillColor(sf::Color::Black);
            text.setPosition(10, size_ * cell_size_ + 10);
            window_.draw(text);
        }

        window_.display();
    }

    // Handle events and check if window is open
    bool isOpen() {
        sf::Event event;
        while (window_.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window_.close();
            }
        }
        return window_.isOpen();
    }
};

/*************************************************************************/
void runFullExperiment() {
    vector<int> sizes = {20, 50, 100, 200, 300, 400};
    vector<tuple<double, double, double> > difficulties = {
        {0.8, 0.18, 0.02}, // Easy
        {0.7, 0.29, 0.01}, // Medium
        {0.6, 0.395, 0.005} // Hard
    };
    // Approaches to test
    vector<string> approaches = {"A* Static", "A* Oracle", "HierarchySmart", "FedAsynQ_EqAvg", "FedAsynQ_ImAvg"};
    // vector<string> approaches = {"A* Static", "A* Oracle", "Local", "Hierarchy", "HierarchySmart"};

    // Detailed output file for per-step data
    ofstream detailedOut("results_incremental_detailed_smart.csv");
    detailedOut << "Approach,Size,Difficulty,TimeStep,NumChanges,AdaptTime,SuccessRate,AvgPathLength\n";

    // Map to store results
    map<string, vector<vector<Metrics> > > results;
    for (const string &name: approaches) {
        results[name].resize(sizes.size());
        for (int s = 0; s < sizes.size(); ++s) {
            results[name][s].resize(difficulties.size());
        }
    }

    // Iterate over maze sizes and difficulties
    for (int s = 0; s < sizes.size(); ++s) {
        int size = sizes[s];
        cout << "\n\nTesting maze size: " << size << "x" << size;

        // Iterate over difficulties
        for (int d = 0; d < difficulties.size(); ++d) {
            srand(d + 50);

            // srand(d + 100); this is the very hard maze, in which the top left corner of the maze (1/4 of the maze)
            // does not contain any charging station. This means that, in a 50x50 maze, some positions have to travel
            // at least half the maze to reach the charging station.

            auto [freeProb, obstProb, chargeProb] = difficulties[d];
            string diffName = (d == 0 ? "Easy" : d == 1 ? "Medium" : "Hard");
            cout << "\n\nDifficulty: " << diffName;

            // Simple scaling: maxTimeSteps proportional to size
            constexpr int k = 2;
            const int maxTimeSteps = k * size;
            cout << " - maxTimeSteps: " << maxTimeSteps;

            // Create initial maze
            auto initialMaze = vector<vector<int> >(size, vector<int>(size, 0));
            createMaze(initialMaze, size, size, freeProb, obstProb, chargeProb);
            vector<pair<int, vector<pair<int, int> > > > changeSequence(maxTimeSteps);
            MazeNode *tempRoot = createSubEnvironments(initialMaze, size, size);

            // Simulate change positions upfront
            for (int t = 0; t < maxTimeSteps; ++t) {
                int r = rand() % 1000;
                int numChanges;
                if (r < 900) numChanges = 1;
                else if (r < 960) numChanges = 2;
                else if (r < 980) numChanges = 3;
                else if (r < 990) numChanges = 4;
                else if (r < 995) numChanges = 5;
                else if (r < 997) numChanges = 6;
                else if (r < 998) numChanges = 7;
                else if (r < 999) numChanges = 8;
                else if (r < 9995) numChanges = 9;
                else numChanges = 10;
                changeSequence[t].first = numChanges;
                simulateEnvironmentChanges(tempRoot, numChanges, changeSequence[t].second);
            }
            delete tempRoot;

            // Iterate over approaches
            for (const string &name: approaches) {
                cout << "\n\nTesting " << name << endl;

                MazeNode *root = createSubEnvironments(initialMaze, size, size);
                unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> shortestPaths;
                double totalInitialTime = 0.0, totalAdaptTime = 0.0, totalSuccessRate = 0.0, totalPathLength = 0.0;
                int stepsCompleted = 0;

                // Inspect the distribution of charging stations across the maze
                printTree(root);

                // Initialize visualization for MultiAgentsFedAsynQ
                // std::unique_ptr<PolicyVisualizer> visualizer;
                // if (name == "FedAsynQ_EqAvg" || name == "FedAsynQ_ImAvg") {
                //     visualizer = std::make_unique<PolicyVisualizer>(root, size, name, maxTimeSteps);
                //     visualizer->update();
                //     visualizer->render();
                // }

                // Initial training
                if (name == "A* Oracle" || name == "A* Static") {
                    auto start = chrono::high_resolution_clock::now();
                    shortestPaths = computeAllShortestPaths(*root->maze);
                    auto end = chrono::high_resolution_clock::now();
                    totalInitialTime = chrono::duration<double>(end - start).count();
                } else {
                    auto start = chrono::high_resolution_clock::now();
                    if (name == "Local") applyLocalPathPlanning(root);
                    else if (name == "Hierarchy") trainHierarchy(root);
                    else if (name == "HierarchySmart") trainHierarchySmart(root);
                    else if (name == "FedAsynQ_EqAvg") trainHierarchySmartMultiAgent("EqAvg", root, {});
                    else if (name == "FedAsynQ_ImAvg") trainHierarchySmartMultiAgent("ImAvg", root, {});
                    auto end = chrono::high_resolution_clock::now();
                    totalInitialTime = chrono::duration<double>(end - start).count();
                }

                // Test initial performance
                auto [_, successRate, avgPath] = (name == "A* Oracle" || name == "A* Static")
                                                     ? testAgentAStar(*root->maze, size, size, shortestPaths)
                                                     : testAgent(root);
                totalSuccessRate += successRate;
                totalPathLength += avgPath;
                stepsCompleted++;

                // Write initial data
                detailedOut << name << "," << size << "," << diffName << ",0,0,"
                        << 0.0 << "," << successRate << "," << avgPath << "\n";

                // Apply changes over time
                vector<vector<int> > currentMaze = initialMaze;
                for (int t = 0; t < maxTimeSteps; ++t) {
                    const auto &[numChanges, changes] = changeSequence[t];
                    for (int i = 0; i < changes.size(); i += 2) {
                        currentMaze[changes[i].first][changes[i].second] = FREE_SPACE;
                        currentMaze[changes[i + 1].first][changes[i + 1].second] = OBSTACLE;
                    }
                    root->maze = make_unique<vector<vector<int> > >(currentMaze);

                    unordered_set<MazeNode *> changedLeaves;
                    for (const auto &[r, c]: changes) {
                        MazeNode *leaf = root->findSubEnvironment(r, c);
                        if (leaf && leaf->children.empty()) changedLeaves.insert(leaf);
                    }
                    vector<MazeNode *> changedLeafSet(changedLeaves.begin(), changedLeaves.end());

                    // Adaptation
                    double adaptTime;
                    if (name == "A* Oracle") {
                        auto start = chrono::high_resolution_clock::now();
                        shortestPaths = computeAllShortestPaths(*root->maze);
                        auto end = chrono::high_resolution_clock::now();
                        adaptTime = chrono::duration<double>(end - start).count();
                    } else if (name == "A* Static") {
                        adaptTime = 0.0; // No adaptation
                    } else {
                        auto start = chrono::high_resolution_clock::now();
                        if (name == "Local") applyLocalPathPlanning(root, changedLeafSet);
                        else if (name == "Hierarchy") trainHierarchy(root, changedLeafSet);
                        else if (name == "HierarchySmart") trainHierarchySmart(root, changedLeafSet);
                        else if (name == "FedAsynQ_EqAvg")
                            trainHierarchySmartMultiAgent(
                                "EqAvg", root, changedLeafSet);
                        else if (name == "FedAsynQ_ImAvg")
                            trainHierarchySmartMultiAgent(
                                "ImAvg", root, changedLeafSet);
                        auto end = chrono::high_resolution_clock::now();
                        adaptTime = chrono::duration<double>(end - start).count();
                    }

                    // Test performance after adaptation
                    auto [_, stepSuccessRate, stepAvgPath] = (name == "A* Oracle" || name == "A* Static")
                                                                 ? testAgentAStar(
                                                                     *root->maze, size, size, shortestPaths)
                                                                 : testAgent(root);
                    totalAdaptTime += adaptTime;
                    totalSuccessRate += stepSuccessRate;
                    totalPathLength += stepAvgPath;
                    stepsCompleted++;

                    // Update visualization
                    // if (visualizer) {
                    //     visualizer->update();
                    //     visualizer->render();
                    //     // Brief delay to ensure smooth rendering
                    //     sf::sleep(sf::milliseconds(500));
                    // }

                    // Write per-step data
                    detailedOut << name << "," << size << "," << diffName << "," << t + 1 << ","
                            << numChanges << "," << adaptTime << "," << stepSuccessRate << ","
                            << stepAvgPath << "\n";

                    // Check if window is still open
                    // if (visualizer && !visualizer->isOpen()) {
                    //     break;
                    // }
                }

                // Finalize results
                cout << "\n" << name << " - Size: " << size << ", Difficulty: " << diffName
                        << ", Initial Time: " << totalInitialTime << "s, Adapt Time: " << totalAdaptTime
                        << "s, Success Rate: " << (totalSuccessRate / stepsCompleted) * 100
                        << "%, Avg Path Length: " << (totalPathLength / stepsCompleted) << " steps";

                results[name][s][d] = {
                    totalInitialTime,
                    totalAdaptTime / maxTimeSteps,
                    totalSuccessRate / stepsCompleted,
                    totalPathLength / stepsCompleted
                };

                delete root;
            }
        }
    }
    detailedOut.close();

    // Save aggregated results
    ofstream out("results_incremental_smart.csv");
    out << "Approach,Size,Difficulty,InitialTime,AdaptTimePerStep,AvgSuccessRate,AvgPathLength\n";
    for (int s = 0; s < sizes.size(); ++s) {
        int size = sizes[s];
        for (int d = 0; d < difficulties.size(); ++d) {
            string diffName = (d == 0 ? "Easy" : d == 1 ? "Medium" : "Hard");
            for (const string &name: approaches) {
                const auto &m = results[name][s][d];
                out << name << "," << size << "," << diffName << ","
                        << m.initialTime << "," << m.adaptTime << "," << m.successRate << "," << m.avgPathLength <<
                        "\n";
            }
        }
    }
    out.close();
}

/*************************************************************************/
int main() {
    runFullExperiment();
    return 0;
}
