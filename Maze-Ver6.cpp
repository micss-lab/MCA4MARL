/****************************************************************************************************/
/*			This Program has been written by Hossein Yarahmadi					                    */
/*          The Path Matrix includes three types of entities                                        */
/*          0: means the obstacle  1:means the feasible moving  2:means the charge station          */
/****************************************************************************************************/

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
#define DISCOUNT_FACTOR 0.8

/**************************************************************************/
using namespace std;

//*************************************************************************/
int countChargingStations(const vector<vector<int> > &maze, const int startRow, const int startCol, const int endRow,
                          const int endCol) {
    int count = 0;
    for (int i = startRow; i < endRow; ++i) {
        for (int j = startCol; j < endCol; ++j) {
            if (maze[i][j] == CHARGING_STATION) {
                ++count;
            }
        }
    }
    return count;
}

//*************************************************************************/
struct pair_hash {
    size_t operator()(const pair<int, int>& p) const {
        return hash<int>{}(p.first) ^ (hash<int>{}(p.second) << 1);
    }
};

//*************************************************************************/
class MazeNode {
public:
    unique_ptr<vector<vector<int> > > maze; // Optional maze, only at root
    unique_ptr<unordered_map<pair<int, int>, vector<double>, pair_hash>> qTable; // Sparse Q-table
    MazeNode *parent;
    vector<MazeNode *> children;
    int rows, cols;
    int startRow, startCol, endRow, endCol;
    int chargingStationCount;
    double baselineSuccessRate = -1.0; // -1 indicates untrained

    // Constructor
    MazeNode(const vector<vector<int> > &fullMaze, const int rows, const int cols, const int startRow,
             const int startCol,
             const int endRow, const int endCol, MazeNode *parent = nullptr, const bool isRoot = false)
        : parent(parent), rows(rows), cols(cols), startRow(startRow), startCol(startCol),
          endRow(endRow), endCol(endCol) {
        // Only root gets a maze copy
        if (isRoot) {
            maze = make_unique<vector<vector<int> > >(fullMaze);
            initQTable();
        }

        // Q-table and charging station count set later for leaves and parents
        // chargingStationCount = countChargingStations(fullMaze, startRow, startCol, endRow, endCol);
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

    // Initialize Q-table only when needed (leaves and parents)
    // void initQTable() {
    //     if (!qTable) {
    //         qTable = make_unique<vector<vector<vector<double> > > >(
    //             rows, vector<vector<double> >(cols, vector<double>(ACTION_COUNT, 0.0)));
    //     }
    // }

    // Initialize sparse Q-table
    void initQTable() {
        if (!qTable) {
            qTable = make_unique<unordered_map<pair<int, int>, vector<double>, pair_hash>>();
            // Pre-populate with subenvironment positions
            for (int r = startRow; r <= endRow; ++r) {
                for (int c = startCol; c <= endCol; ++c) {
                    (*qTable)[{r, c}] = vector<double>(ACTION_COUNT, 0.0);
                }
            }
        }
    }

    // Access Q-values, defaulting to zeros if not found
    vector<double>& getQValues(int x, int y) {
        const auto it = qTable->find({x, y});
        if (it != qTable->end()) return it->second;
        static vector<double> defaultQ(ACTION_COUNT, 0.0); // Static to avoid reallocation
        return defaultQ;
    }
};

/**************************************************************************/
void createMaze(vector<vector<int> > &maze, const int rows, const int cols, const double freeSpaceProb,
                const double obstacleProb,
                const double chargingStationProb) {
    // Validate that the probabilities sum to 1
    if (abs(freeSpaceProb + obstacleProb + chargingStationProb - 1.0) > 1e-6) {
        cerr << "Error: Probabilities must sum to 1." << endl;
        exit(1);
    }

    // cout << "\n\t\t=== The Maze: 0 means obstacle, 1 means free space, 2 means charging station ===";

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

/**************************************************************************/
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

/**************************************************************************/
void printMatrixDouble(const vector<vector<double> > &matrix, const int rows, const int cols, const char text[]) {
    cout << "\n============= The " << text << " ===============";
    cout << "\n";

    // Print column headers with [j] format
    cout << "\t\t\t\t";
    cout << "         "; // Space for row index
    for (int j = 0; j < cols; j++) {
        cout << "[" << j << "]     ";
    }
    cout << "\n";

    // Print each row with [i] format
    for (int i = 0; i < rows; i++) {
        cout << "\t\t\t\t";
        cout << "[" << i << "] "; // Row index with padding
        for (int j = 0; j < cols; j++) {
            cout << setw(8) << fixed << setprecision(2) << matrix[i][j];
        }
        cout << "\n";
    }
    cout << "\n";
}

/**************************************************************************/
int checkExit(const vector<vector<int> > &matrix, const int x, const int y) {
    return (matrix[x][y] == CHARGING_STATION) ? 1 : 0;
}

/**************************************************************************/
pair<int, int> selectFirstPlace(const vector<vector<int> > &maze, const int startRow, const int startCol,
                                const int endRow,
                                const int endCol) {
    int x, y;
    // Keep generating random indices until a free space is found
    do {
        x = rand() % (endRow - startRow) + startRow;
        y = rand() % (endCol - startCol) + startCol;
    } while (maze[x][y] != FREE_SPACE);
    return make_pair(x, y);
}

/**************************************************************************/
int selectAction(const vector<double>& qValues, const int x, const int y, const double epsilon,
                 const int startRow, const int startCol, const int endRow, const int endCol) {
    int action;
    const double randomValue = static_cast<double>(rand()) / RAND_MAX;

    // Define the potential moves for each action (N, NE, E, SE, S, SW, W, NW)
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

    // Filter valid actions based on boundaries
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
        return -1; // Indicate an error (should not happen in a valid setup)
    }

    if (randomValue < epsilon) {
        // Exploration: Choose a random valid action
        action = validActions[rand() % validActions.size()];
    } else {
        // Exploitation: Choose the action with the highest Q-value among valid actions
        action = validActions[0];
        double maxQValue = qValues[validActions[0]];
        for (const int i : validActions) {
            if (qValues[i] > maxQValue) {
                maxQValue = qValues[i];
                action = i;
            }
        }
    }

    return action;
}

//*************************************************************************/
int selectActionWithSoftBoundaries(const vector<double>& qValues, const int rows, const int cols,
                                   const int x, const int y, const double epsilon) {
    int action;
    const double randomValue = static_cast<double>(rand()) / RAND_MAX;

    // Define the potential moves for each action (N, NE, E, SE, S, SW, W, NW)
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

    vector<int> validActions;
    for (int i = 0; i < ACTION_COUNT; ++i) {
        const int newX = x + moves[i].first;
        const int newY = y + moves[i].second;
        if (newX >= 0 && newX < rows && newY >= 0 && newY < cols) {
            validActions.push_back(i);
        }
    }

    if (validActions.empty()) {
        cerr << "Error: No valid actions available for position (" << x << ", " << y << ")\n";
        return -1;
    }

    if (randomValue < epsilon) {
        action = validActions[rand() % validActions.size()];
    } else {
        action = validActions[0];
        double maxQValue = qValues[validActions[0]];
        for (const int i : validActions) {
            if (qValues[i] > maxQValue) {
                maxQValue = qValues[i];
                action = i;
            }
        }
    }
    return action;
}

/**************************************************************************/
tuple<int, int, int, double> performAction(const vector<vector<int> > &maze, const int rows, const int cols,
                                           const int x1, const int y1, const int action) {
    double reward = 0.0;
    int changePos = 0;
    int x2 = x1, y2 = y1;

    // Perform the selected action (unchanged logic)
    switch (action) {
        case 0: // Move N
            if (x1 > 0 && (maze[x1 - 1][y1] == FREE_SPACE || maze[x1 - 1][y1] == CHARGING_STATION)) {
                x2 = x1 - 1;
                changePos = 1;
            }
            break;
        case 1: // Move NE
            if (x1 > 0 && y1 < cols - 1 && (
                    maze[x1 - 1][y1 + 1] == FREE_SPACE || maze[x1 - 1][y1 + 1] == CHARGING_STATION)) {
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
            if (x1 < rows - 1 && y1 > 0 && (
                    maze[x1 + 1][y1 - 1] == FREE_SPACE || maze[x1 + 1][y1 - 1] == CHARGING_STATION)) {
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

/**************************************************************************/
void detectPath(const vector<vector<int> > &maze, vector<vector<int> > &path, const int x, const int y) {
    if (maze[x][y] == FREE_SPACE) path[x][y] = FREE_SPACE;
    if (maze[x][y] == CHARGING_STATION) path[x][y] = CHARGING_STATION;
}

/**************************************************************************/
void updateQTable(MazeNode* node, const int x1, const int y1, const int action,
                  const double reward, const int x2, const int y2) {
    // Ensure node and qTable exist
    if (!node || !node->qTable) return;

    // Access Q-values for current state (x1, y1)
    vector<double>& qValues = node->getQValues(x1, y1);
    if (qValues.empty()) {
        qValues.resize(ACTION_COUNT, 0.0); // Initialize if not present
    }

    // Access Q-values for next state (x2, y2)
    vector<double>& nextQValues = node->getQValues(x2, y2);
    if (nextQValues.empty()) {
        nextQValues.resize(ACTION_COUNT, 0.0); // Initialize if not present
    }

    // Get the maximum Q-value for the next state
    double maxQNext = nextQValues[0];
    for (int i = 1; i < ACTION_COUNT; i++) {
        if (nextQValues[i] > maxQNext) {
            maxQNext = nextQValues[i];
        }
    }

    // Update the Q-value for the current state and action
    qValues[action] = qValues[action] + LEARNING_RATE * (reward + DISCOUNT_FACTOR * maxQNext - qValues[action]);
}

//*************************************************************************/
int selectMaxQ(const vector<vector<vector<double> > > &qTable, const int rows, const int cols, const int x, const int y,
               vector<vector<int> > &checkMatrix) {
    double maxItem = numeric_limits<double>::lowest(); // Initialize to the lowest possible value
    int selectedAction = -1;

    // Mark the current position as visited in checkMatrix
    checkMatrix[x][y] = 1;

    // Iterate over all possible actions (0 to 7) and find the action with the maximum Q-value
    for (int act = 0; act < ACTION_COUNT; ++act) {
        int nextX = x, nextY = y;

        // Determine the next position based on the action
        switch (act) {
            case 0: // Move N
                if (x > 0) nextX = x - 1;
                break;
            case 1: // Move NE
                if (x > 0 && y < cols - 1) {
                    nextX = x - 1;
                    nextY = y + 1;
                }
                break;
            case 2: // Move E
                if (y < cols - 1) nextY = y + 1;
                break;
            case 3: // Move SE
                if (x < rows - 1 && y < cols - 1) {
                    nextX = x + 1;
                    nextY = y + 1;
                }
                break;
            case 4: // Move S
                if (x < rows - 1) nextX = x + 1;
                break;
            case 5: // Move SW
                if (x < rows - 1 && y > 0) {
                    nextX = x + 1;
                    nextY = y - 1;
                }
                break;
            case 6: // Move W
                if (y > 0) nextY = y - 1;
                break;
            case 7: // Move NW
                if (x > 0 && y > 0) {
                    nextX = x - 1;
                    nextY = y - 1;
                }
                break;
        }

        // Check if the move is valid (within bounds and not visited)
        if (nextX >= 0 && nextX < rows && nextY >= 0 && nextY < cols && checkMatrix[nextX][nextY] == 0) {
            // Compare the Q-value for the current action
            if (qTable[x][y][act] > maxItem) {
                maxItem = qTable[x][y][act];
                selectedAction = act;
            }
        }
    }
    return selectedAction;
}

//*************************************************************************/
void selectPath(const vector<vector<vector<double> > > &qTable, const int rows, const int cols, const int xStart,
                const int yStart, const vector<vector<int> > &maze) {
    int step = 1;
    auto checkMatrix = vector<vector<int> >(rows, vector<int>(cols, 0)); // Keeps track of visited positions
    int x = xStart, y = yStart; // Current position
    auto finalPath = vector<vector<int> >(rows, vector<int>(cols, 0)); // Stores the steps taken in the path

    while (maze[x][y] != CHARGING_STATION) {
        // Continue until a charging station (value 2) is reached
        // cout << "\n\tThe Current Position IS: " << '(' << x << ", " << y << ") ==> " << maze[x][y];

        // Select the best action using the Q-table
        const int selectedAction = selectMaxQ(qTable, rows, cols, x, y, checkMatrix);
        // cout << "\n\tThe Selected Action IS: " << selectedAction;

        // Update position and finalPath based on the selected action
        switch (selectedAction) {
            case 0: // Move N
                finalPath[x - 1][y] = step;
                x = x - 1;
                break;
            case 1: // Move NE
                finalPath[x - 1][y + 1] = step;
                x = x - 1;
                y = y + 1;
                break;
            case 2: // Move E
                finalPath[x][y + 1] = step;
                y = y + 1;
                break;
            case 3: // Move SE
                finalPath[x + 1][y + 1] = step;
                x = x + 1;
                y = y + 1;
                break;
            case 4: // Move S
                finalPath[x + 1][y] = step;
                x = x + 1;
                break;
            case 5: // Move SW
                finalPath[x + 1][y - 1] = step;
                x = x + 1;
                y = y - 1;
                break;
            case 6: // Move W
                finalPath[x][y - 1] = step;
                y = y - 1;
                break;
            case 7: // Move NW
                finalPath[x - 1][y - 1] = step;
                x = x - 1;
                y = y - 1;
                break;
            default:
                cout << "\n\tInvalid action selected. Terminating path selection.\n";
                return; // Exit the function if an invalid action is selected
        }

        step++;
        // cout << "\n\tThe New Position IS: " << '(' << x << ", " << y << ") ==> " << maze[x][y] << "\n";
    }

    // Print the final path
    // printMatrixInt(finalPath, rows, cols, "FinalPath");
}

//*************************************************************************/
void trainAgent(const vector<vector<int>>& maze, MazeNode* node, const int rows,
                const int cols, const int startRow, const int startCol, const int endRow, const int endCol, double epsilon) {
    // Global-sized path array for updates
    auto path = vector<vector<int>>(rows, vector<int>(cols, 0));
    int arrival = 0;
    int x1, y1, x2, y2;
    double actionReward = 0;
    int iteration = 0;
    const int maxEpisodes = rows + cols;
    constexpr double decayRate = 0.999; // Adjust as needed

    // Start the training process (generate episodes)
    for (int counter = 0; counter < EPISODE_COUNT; counter++) {
        // Select a random starting place within the sub-environment
        tie(x1, y1) = selectFirstPlace(maze, startRow, startCol, endRow, endCol);
        iteration = 1;

        // Start generating one episode
        while (arrival == 0 && iteration < maxEpisodes) {
            // Select an action using an epsilon-greedy policy
            int act = selectAction(node->getQValues(x1, y1), x1, y1, epsilon, startRow, startCol, endRow, endCol);

            // Perform the selected action and get the associated reward
            tie(x2, y2, act, actionReward) = performAction(maze, rows, cols, x1, y1, act);
            detectPath(maze, path, x1, y1); // Update the path matrix

            // Update the Q-table using the sparse Q-table in node
            updateQTable(node, x1, y1, act, actionReward, x2, y2);

            // Check if the exit condition is met within the sub-environment
            arrival = checkExit(maze, x2, y2);
            x1 = x2;
            y1 = y2;
            iteration++;
        }

        // Decay epsilon to reduce exploration over time
        epsilon = max(0.01, epsilon * decayRate); // Min epsilon 0.01
        arrival = 0; // Reset arrival for the next episode
    }
}

//*************************************************************************/
void trainAgentWithSoftBoundaries(const vector<vector<int>>& maze, MazeNode* node, const int rows,
    const int cols, const int startRow, const int startCol, const int endRow, const int endCol, double epsilon) {
    // Global-sized path array for updates
    auto path = vector<vector<int> >(rows, vector<int>(cols, 0));
    int arrival = 0;
    int x1, y1, x2, y2;
    double actionReward = 0;
    int iteration = 0;
    const int maxEpisodes = rows + cols;
    const double decayRate = 0.999; // Adjust as needed

    // Start the training process (generate episodes)
    for (int counter = 0; counter < EPISODE_COUNT; counter++) {
        // Select a random starting place within the sub-environment
        tie(x1, y1) = selectFirstPlace(maze, startRow, startCol, endRow, endCol);
        iteration = 1;

        // Start generating one episode
        while (arrival == 0 && iteration < maxEpisodes) {
            // Select an action using an epsilon-greedy policy
            int act = selectActionWithSoftBoundaries(node->getQValues(x1, y1), rows, cols, x1, y1, epsilon);

            // Perform the selected action and get the associated reward
            tie(x2, y2, act, actionReward) = performAction(maze, rows, cols, x1, y1, act);
            detectPath(maze, path, x1, y1); // Update the path matrix

            // Update the Q-table
            updateQTable(node, x1, y1, act, actionReward, x2, y2);

            // Check if the exit condition is met within the sub-environment
            arrival = checkExit(maze, x2, y2);
            x1 = x2;
            y1 = y2;
            iteration++;
        }

        // Decay epsilon to reduce exploration over time
        epsilon = max(0.01, epsilon * decayRate);
        arrival = 0; // Reset arrival for the next episode
    }
}

//*************************************************************************/
void trainAgentWithStoppingCriterion(MazeNode* node, const vector<vector<int>>& maze,
                                     const int rows, const int cols, const int startRow, const int startCol,
                                     const int endRow, const int endCol, double epsilon, int maxStepsPerEpisode) {
    auto path = vector<vector<int>>(rows, vector<int>(cols, 0));
    int arrival = 0, x1, y1, x2, y2, iteration = 0, counter = 0, stableEpisodes = 0;
    double actionReward = 0;
    bool converged = false;

    // Store previous Q-table state for convergence check
    auto prevQTable = node->qTable ? *node->qTable : unordered_map<pair<int, int>, vector<double>, pair_hash>();

    // Convergence parameters
    constexpr double threshold = 5e-4;
    constexpr int patience = 20;
    const double decayRate = 0.999;
    constexpr double epsilon_min = 1e-3;
    constexpr int minEpisodes = 500;

    // Experience replay buffer
    struct Experience {
        int x1, y1, action;
        double reward;
        int x2, y2;
    };
    vector<Experience> replayBuffer;
    constexpr int bufferSize = 1000;
    replayBuffer.reserve(bufferSize);
    constexpr int batchSize = 64;

    while (!converged && counter < EPISODE_COUNT) {
        tie(x1, y1) = selectFirstPlace(maze, startRow, startCol, endRow, endCol);
        iteration = 1;

        while (arrival == 0 && iteration < maxStepsPerEpisode) {
            int act = selectActionWithSoftBoundaries(node->getQValues(x1, y1), rows, cols, x1, y1, epsilon);
            tie(x2, y2, act, actionReward) = performAction(maze, rows, cols, x1, y1, act);
            detectPath(maze, path, x1, y1);

            replayBuffer.push_back({x1, y1, act, actionReward, x2, y2});
            if (replayBuffer.size() > bufferSize) replayBuffer.erase(replayBuffer.begin());
            updateQTable(node, x1, y1, act, actionReward, x2, y2);

            if (replayBuffer.size() >= batchSize && counter > minEpisodes) {
                for (int i = 0; i < batchSize; i++) {
                    int idx = rand() % replayBuffer.size();
                    const auto& exp = replayBuffer[idx];
                    updateQTable(node, exp.x1, exp.y1, exp.action, exp.reward, exp.x2, exp.y2);
                }
            }

            arrival = checkExit(maze, x2, y2);
            x1 = x2;
            y1 = y2;
            iteration++;
        }

        arrival = 0;
        epsilon = max(0.01, epsilon * decayRate);

        if (counter % 50 == 0 && counter >= minEpisodes) {
            double maxChange = 0.0;
            for (const auto& [pos, qValues] : *node->qTable) {
                int i = pos.first;
                int j = pos.second;
                if (i >= startRow && i <= endRow && j >= startCol && j <= endCol) {
                    auto prevIt = prevQTable.find(pos);
                    const vector<double>& prevQ = (prevIt != prevQTable.end()) ? prevIt->second : vector<double>(ACTION_COUNT, 0.0);
                    for (int a = 0; a < ACTION_COUNT; a++) {
                        maxChange = max(maxChange, fabs(qValues[a] - prevQ[a]));
                    }
                }
            }

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

    // Clean up Q-table: Remove entries outside subenvironment bounds
    auto& qTable = *node->qTable;
    for (auto it = qTable.begin(); it != qTable.end();) {
        int x = it->first.first;
        int y = it->first.second;
        if (x < startRow || x > endRow || y < startCol || y > endCol) {
            it = qTable.erase(it); // Remove out-of-bounds entry
        } else {
            ++it; // Move to next entry
        }
    }
}

/**************************************************************************/
vector<int> selectTopTwoActions(const vector<double>& qValues, int rows, int cols, int x, int y) {
    const vector<pair<int, int>> moves = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};
    vector<pair<double, int>> validQValues;

    // Collect valid actions with Q-values
    for (int i = 0; i < ACTION_COUNT; ++i) {
        int newX = x + moves[i].first;
        int newY = y + moves[i].second;
        if (newX >= 0 && newX < rows && newY >= 0 && newY < cols) {
            validQValues.push_back({qValues[i], i});
        }
    }

    if (validQValues.empty()) {
        cerr << "Error: No valid actions at (" << x << ", " << y << ")\n";
        return {};
    }

    // Sort by Q-value descending
    sort(validQValues.begin(), validQValues.end(), greater<pair<double, int>>());

    // Return top two (or one if only one valid)
    vector<int> actions;
    actions.push_back(validQValues[0].second);
    if (validQValues.size() > 1) actions.push_back(validQValues[1].second);
    return actions;
}

/**************************************************************************/
struct PathState {
    int x, y, steps;
    vector<pair<int, int>> path;
};

/**************************************************************************/
tuple<bool, int, vector<pair<int, int>>> findValidPath(const vector<vector<int>>& maze,
                                                       MazeNode* node,
                                                       int rows, int cols, int startX, int startY, int maxSteps) {
    const vector<pair<int, int>> moves = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};
    queue<PathState> toExplore;
    set<pair<int, int>> visited;
    toExplore.push({startX, startY, 0, {{startX, startY}}});
    visited.insert({startX, startY});

    while (!toExplore.empty()) {
        PathState current = toExplore.front();
        toExplore.pop();

        if (current.steps >= maxSteps) continue;
        if (maze[current.x][current.y] == CHARGING_STATION) { // Success
            return {true, current.steps, current.path};
        }

        vector<int> actions = selectTopTwoActions(node->getQValues(current.x, current.y), rows, cols, current.x, current.y);
        for (int act : actions) {
            int newX = current.x + moves[act].first;
            int newY = current.y + moves[act].second;
            if (newX >= 0 && newX < rows && newY >= 0 && newY < cols && maze[newX][newY] != OBSTACLE &&
                visited.find({newX, newY}) == visited.end()) {
                visited.insert({newX, newY});
                vector<pair<int, int>> newPath = current.path;
                newPath.push_back({newX, newY});
                toExplore.push({newX, newY, current.steps + 1, newPath});
            }
        }
    }
    return {false, 0, {}}; // No valid path
}

//*************************************************************************/
tuple<double, double, double> testAgent(const vector<vector<int>>& maze, MazeNode* node,
                                        const int rows, const int cols) {
    const int maxEpisodes = rows + cols;

    // Collect all valid positions to test
    vector<pair<int, int>> positions;
    int totalPositions = 0;
    for (int x1 = 0; x1 < rows; ++x1) {
        for (int y1 = 0; y1 < cols; ++y1) {
            if (maze[x1][y1] != OBSTACLE) {
                positions.emplace_back(x1, y1);
                totalPositions++;
            }
        }
    }

    // Struct to hold results from each thread
    struct ThreadResult {
        double planningTime = 0.0;
        int successfulPaths = 0;
        int totalSteps = 0;
    };

    // Function to process a chunk of positions and return results
    auto processChunk = [&](size_t startIdx, size_t endIdx) -> ThreadResult {
        ThreadResult result;
        for (size_t i = startIdx; i < endIdx && i < positions.size(); ++i) {
            int x1 = positions[i].first;
            int y1 = positions[i].second;

            auto start = chrono::high_resolution_clock::now();
            auto [success, steps, path] = findValidPath(maze, node, rows, cols, x1, y1, maxEpisodes);
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
    vector<future<ThreadResult>> futures;

    for (size_t i = 0; i < totalTasks; i += chunkSize) {
        size_t startIdx = i;
        size_t endIdx = min(i + chunkSize, totalTasks);
        futures.push_back(async(launch::async, processChunk, startIdx, endIdx));
    }

    // Aggregate results (single-threaded, after all threads finish)
    double totalPlanningTime = 0.0;
    int successfulPaths = 0;
    int totalSteps = 0;
    for (auto& f : futures) {
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

//*************************************************************************/
void splitMaze(MazeNode* node, const vector<vector<int>>& fullMaze, const int rows, const int cols,
               const int startRow, const int startCol, const int endRow, const int endCol) {
    if ((endRow - startRow + 1) <= 20 && (endCol - startCol + 1) <= 20) {
        // Leaf node: Initialize Q-table
        node->initQTable(); // Pass fullMaze for charging station count
        return;
    }

    const int midRow = (startRow + endRow) / 2;
    const int midCol = (startCol + endCol) / 2;

    auto* child1 = new MazeNode(fullMaze, rows, cols, startRow, startCol, midRow, midCol, node);
    auto* child2 = new MazeNode(fullMaze, rows, cols, startRow, midCol + 1, midRow, endCol, node);
    auto* child3 = new MazeNode(fullMaze, rows, cols, midRow + 1, startCol, endRow, midCol, node);
    auto* child4 = new MazeNode(fullMaze, rows, cols, midRow + 1, midCol + 1, endRow, endCol, node);

    node->addChild(child1);
    node->addChild(child2);
    node->addChild(child3);
    node->addChild(child4);

    splitMaze(child1, fullMaze, rows, cols, startRow, startCol, midRow, midCol);
    splitMaze(child2, fullMaze, rows, cols, startRow, midCol + 1, midRow, endCol);
    splitMaze(child3, fullMaze, rows, cols, midRow + 1, startCol, endRow, midCol);
    splitMaze(child4, fullMaze, rows, cols, midRow + 1, midCol + 1, endRow, endCol);

    // Parents of leaves: Initialize Q-table if all children are leaves
    if (child1->children.empty() && child2->children.empty() &&
        child3->children.empty() && child4->children.empty()) {
        node->initQTable(); // Pass fullMaze for charging station count
        }
}

//*************************************************************************/
void printTree(const MazeNode *node, const string &prefix = "", const bool isLast = true, const bool isRoot = true) {
    if (!node) return;

    // For the root node, don't add any symbols
    if (isRoot) {
        cout << "Node: Start(" << node->startRow << ", " << node->startCol << "), "
                << "End(" << node->endRow << ", " << node->endCol << "), "
                << "Size(" << (node->endRow - node->startRow + 1) << "x" << (node->endCol - node->startCol + 1) << "), "
                << "Charging Stations: " << node->chargingStationCount << "\n";
    } else {
        // For all other nodes, add the appropriate symbols
        const string currentPrefix = prefix + (isLast ? "└─ " : "├─ ");
        cout << currentPrefix
                << "Node: Start(" << node->startRow << ", " << node->startCol << "), "
                << "End(" << node->endRow << ", " << node->endCol << "), "
                << "Size(" << (node->endRow - node->startRow + 1) << "x" << (node->endCol - node->startCol + 1) << "), "
                << "Charging Stations: " << node->chargingStationCount << "\n";
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

//*************************************************************************/
MazeNode *createSubEnvironments(const vector<vector<int> > &maze, const int rows, const int cols) {
    // Root node with maze copy
    auto *root = new MazeNode(maze, rows, cols, 0, 0, rows - 1, cols - 1, nullptr, true);
    splitMaze(root, maze, rows, cols, 0, 0, rows - 1, cols - 1);
    return root;
}

//*************************************************************************/
void propagateQTableDownwards(MazeNode* node) {
    if (!node || !node->qTable) return; // Skip if no node or no qTable

    // Propagate to all descendants, updating only those with qTables
    stack<MazeNode*> toVisit;
    toVisit.push(node);

    while (!toVisit.empty()) {
        MazeNode* current = toVisit.top();
        toVisit.pop();

        for (MazeNode* child : current->children) {
            if (child->qTable) { // Update only if qTable exists
                // Copy Q-values for positions within child's subenvironment
                for (int i = child->startRow; i <= child->endRow; ++i) {
                    for (int j = child->startCol; j <= child->endCol; ++j) {
                        auto& childQ = (*child->qTable)[{i, j}]; // Access or create child's Q-values
                        auto& currentQ = (*current->qTable)[{i, j}]; // Access current node's Q-values
                        if (childQ.empty()) {
                            childQ.resize(ACTION_COUNT, 0.0); // Initialize if not present
                        }
                        childQ = currentQ; // Copy all action Q-values
                    }
                }
            }
            toVisit.push(child); // Continue to child regardless of qTable
        }
    }
}

//*************************************************************************/
void propagateQTableUpwards(const MazeNode* node) {
    if (!node || !node->qTable || !node->parent) return; // Skip if no node, no qTable, or no parent

    MazeNode* current = node->parent; // Start at parent
    while (current) { // Continue until root (no parent)
        if (current->qTable) { // Update only if qTable exists
            // Copy Q-values for positions within node's subenvironment
            for (int i = node->startRow; i <= node->endRow; ++i) {
                for (int j = node->startCol; j <= node->endCol; ++j) {
                    auto& parentQ = (*current->qTable)[{i, j}]; // Access or create parent's Q-values
                    auto& nodeQ = (*node->qTable)[{i, j}]; // Access node's Q-values
                    if (parentQ.empty()) {
                        parentQ.resize(ACTION_COUNT, 0.0); // Initialize if not present
                    }
                    parentQ = nodeQ; // Copy all action Q-values
                }
            }
        }
        current = current->parent; // Move up, even if no qTable
    }
}

//*************************************************************************/
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

//*************************************************************************/
void masMat(MazeNode *root, const vector<pair<int, int>>& changedPositions, const double epsilon) {
    // If no changes, apply global Q-learning
    if (changedPositions.empty()) {
        cout << "\nEnvironment is static. Applying global Q-learning.\n";

        // Calculate maxEpisodes for the root
        int maxSteps = (root->endRow - root->startRow + 1) + (root->endCol - root->startCol + 1);

        // Train the agent on the root environment using the node's sparse Q-table
        trainAgentWithStoppingCriterion(root, *root->maze, root->rows, root->cols, root->startRow,
                                        root->startCol, root->endRow, root->endCol, epsilon, maxSteps);

        // Propagate the Q-table results from the root to all sub-environments
        propagateQTableDownwards(root);
        return;
    }

    cout << "\nEnvironment changed. Applying local Q-learning.\n";

    unordered_set<MazeNode *> affectedNodes;

    // Find all affected nodes
    for (const auto &pos: changedPositions) {
        MazeNode *node = root->findSubEnvironment(pos.first, pos.second);
        if (node) affectedNodes.insert(node);
    }

    // Train each affected node and propagate the results upwards
    for (MazeNode *node: affectedNodes) {
        // Calculate maxEpisodes for this node
        int maxSteps = (node->endRow - node->startRow + 1) + (node->endCol - node->startCol + 1);

        // Train the agent on the sub-environment
        trainAgentWithStoppingCriterion(node, *root->maze, node->rows, node->cols,
                                        node->startRow, node->startCol, node->endRow, node->endCol, epsilon, maxSteps);

        // Propagate the Q-table results from the node to the root
        propagateQTableUpwards(node);
    }
}

//*************************************************************************/
void simulateEnvironmentChanges(MazeNode *root, const int numSteps, vector<pair<int, int> > &changedPositions) {
    if (!root) {
        cerr << "Error: Root node is null.\n";
        return;
    }

    // Get the obstacle positions
    const int rows = root->rows, cols = root->cols;
    vector<pair<int, int> > obstaclePositions = getObstaclePositions(*root->maze, rows, cols);
    changedPositions.clear(); // Ensure it starts empty

    for (int step = 0; step < numSteps; ++step) {
        // cout << "\nStep " << step + 1 << ":\n";

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

            vector<pair<int, int> > validMoves;
            for (const auto &move: moves) {
                const int newRow = move.first, newCol = move.second;
                if (newRow >= 0 && newRow < rows &&
                    newCol >= 0 && newCol < cols &&
                    (*root->maze)[newRow][newCol] == FREE_SPACE) {
                    validMoves.push_back(move);
                }
            }

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

                // cout << "Moved obstacle from (" << oldRow << ", " << oldCol
                // << ") to (" << newRow << ", " << newCol << ")\n";
            }
        }
        // printMatrixInt(root->maze, rows, cols, "Maze after step:");
    }
}

//*************************************************************************/
struct HashPair {
    size_t operator()(const pair<int, int> &p) const {
        return hash<int>()(p.first) ^ (hash<int>()(p.second) << 1);
    }
};

//*************************************************************************/
struct Node {
    int x, y, g, h;

    bool operator>(const Node &other) const {
        return (g + h) > (other.g + other.h);
    }
};

//*************************************************************************/
int heuristic(const int x1, const int y1, const int x2, const int y2) {
    // Use the Chebyshev distance heuristic
    return max(abs(x1 - x2), abs(y1 - y2));
}

//*************************************************************************/
vector<pair<int, int> > reconstructPath(unordered_map<pair<int, int>, pair<int, int>, HashPair> &cameFrom, int startX,
                                        int startY, const int goalX, const int goalY) {
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

//*************************************************************************/
unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> computeAllShortestPaths(
    const vector<vector<int> > &maze) {
    const int rows = maze.size();
    const int cols = maze[0].size();

    // Directions for moving in 8 possible ways (up, down, left, right, and diagonals)
    vector<pair<int, int> > directions = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1}, // Up, Down, Left, Right
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1} // Diagonal moves
    };
    unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> shortestPaths;

    for (int startX = 0; startX < rows; startX++) {
        for (int startY = 0; startY < cols; startY++) {
            if (maze[startX][startY] == OBSTACLE) continue; // Skip obstacles

            priority_queue<Node, vector<Node>, greater<> > openSet;
            unordered_map<pair<int, int>, int, HashPair> gScore;
            unordered_map<pair<int, int>, pair<int, int>, HashPair> cameFrom;

            openSet.push({startX, startY, 0, 0});
            gScore[{startX, startY}] = 0;

            bool found = false;
            pair<int, int> goal;

            while (!openSet.empty() && !found) {
                Node current = openSet.top();
                openSet.pop();

                if (maze[current.x][current.y] == CHARGING_STATION) {
                    // Charging station
                    goal = {current.x, current.y};
                    found = true;
                    break;
                }

                for (auto [dx, dy]: directions) {
                    int newX = current.x + dx;
                    int newY = current.y + dy;

                    if (newX >= 0 && newX < rows && newY >= 0 && newY < cols && maze[newX][newY] != OBSTACLE) {
                        int newG = gScore[{current.x, current.y}] + 1;
                        if (!gScore.count({newX, newY}) || newG < gScore[{newX, newY}]) {
                            gScore[{newX, newY}] = newG;
                            const int h = heuristic(newX, newY, startX, startY);
                            openSet.push({newX, newY, newG, h});
                            cameFrom[{newX, newY}] = {current.x, current.y};
                        }
                    }
                }
            }

            if (found) {
                shortestPaths[{startX, startY}] = reconstructPath(cameFrom, startX, startY, goal.first, goal.second);
            } else {
                shortestPaths[{startX, startY}] = {};
            }
        }
    }
    return shortestPaths;
}

//*************************************************************************/
tuple<double, double, double> testAgentAStar(const vector<vector<int>>& maze, const int rows, const int cols,
                                             const unordered_map<pair<int, int>, vector<pair<int, int>>, HashPair>& shortestPaths) { // Tunable number of threads
    // Collect all valid positions to test
    vector<pair<int, int>> positions;
    int totalPositions = 0;
    for (int x1 = 0; x1 < rows; ++x1) {
        for (int y1 = 0; y1 < cols; ++y1) {
            if (maze[x1][y1] != OBSTACLE) {
                positions.emplace_back(x1, y1);
                totalPositions++;
            }
        }
    }

    // Struct to hold results from each thread
    struct ThreadResult {
        double planningTime = 0.0;
        int successfulPaths = 0;
        int totalSteps = 0;
    };

    // Function to process a chunk of positions and return results
    auto processChunk = [&](size_t startIdx, size_t endIdx) -> ThreadResult {
        ThreadResult result;
        for (size_t i = startIdx; i < endIdx && i < positions.size(); ++i) {
            int startX = positions[i].first;
            int startY = positions[i].second;

            auto start = chrono::high_resolution_clock::now();
            bool success = false;
            int steps = 0;

            auto it = shortestPaths.find({startX, startY});
            if (it != shortestPaths.end()) {
                const vector<pair<int, int>>& path = it->second;
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
    constexpr size_t chunkSize = 20;
    vector<future<ThreadResult>> futures;

    for (size_t i = 0; i < totalTasks; i += chunkSize) {
        size_t startIdx = i;
        size_t endIdx = min(i + chunkSize, totalTasks);
        futures.push_back(async(launch::async, processChunk, startIdx, endIdx));
    }

    // Aggregate results (single-threaded, after all threads finish)
    double totalPlanningTime = 0.0;
    int successfulPaths = 0;
    int totalSteps = 0;
    for (auto& f : futures) {
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

//*************************************************************************/
void testAStarPerformance(MazeNode *root, const int numSteps) {
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

//*************************************************************************/
// pair<int, vector<pair<int, int> > > shortestPathToChargingStation(const vector<vector<int> > &maze, int startX,
//                                                                   int startY) {
//     const int rows = maze.size();
//     const int cols = maze[0].size();
//
//     // Directions for moving in 8 possible ways (up, down, left, right, and diagonals)
//     vector<pair<int, int> > directions = {
//         {-1, 0}, {1, 0}, {0, -1}, {0, 1}, // Up, Down, Left, Right
//         {-1, -1}, {-1, 1}, {1, -1}, {1, 1} // Diagonal moves
//     };
//
//     // Queue for BFS
//     queue<Node> q;
//     q.push({startX, startY, 0});
//
//     // Visited grid and parent tracking
//     vector<vector<bool> > visited(rows, vector<bool>(cols, false));
//     vector<vector<pair<int, int> > > parent(rows, vector<pair<int, int> >(cols, {-1, -1}));
//
//     visited[startX][startY] = true;
//
//     while (!q.empty()) {
//         Node current = q.front();
//         q.pop();
//
//         // Check if we reached a charging station
//         if (maze[current.x][current.y] == CHARGING_STATION) {
//             // Backtrack to reconstruct the path
//             vector<pair<int, int> > path;
//             int x = current.x, y = current.y;
//             while (x != startX || y != startY) {
//                 path.emplace_back(x, y);
//                 tie(x, y) = parent[x][y];
//             }
//             path.emplace_back(startX, startY); // Add start position
//             reverse(path.begin(), path.end()); // Reverse to get the correct order
//
//             return {current.steps, path}; // Return shortest path length and path itself
//         }
//
//         // Explore all 8 possible moves
//         for (auto [dx, dy]: directions) {
//             const int newX = current.x + dx;
//             const int newY = current.y + dy;
//
//             // Check if the new position is valid
//             if (newX >= 0 && newX < rows && newY >= 0 && newY < cols &&
//                 !visited[newX][newY] && maze[newX][newY] != OBSTACLE) {
//                 // Assuming 1 represents obstacles
//                 visited[newX][newY] = true;
//                 parent[newX][newY] = {current.x, current.y}; // Track parent
//                 q.push({newX, newY, current.steps + 1});
//             }
//         }
//     }
//
//     return {-1, {}}; // No path found to any charging station
// }

//*************************************************************************/
// tuple<double, double, double, double, double, double, double> testAgentWithOptimalComparison(
//     const vector<vector<int> > &maze, const vector<vector<vector<double> > > &qTable, const int rows, const int cols,
//     const int nrTestEpisodes) {
//     double totalPlanningTime = 0.0;
//     int successfulPaths = 0;
//     int totalSteps = 0;
//     int totalExtraSteps = 0;
//     int totalOptimalSteps = 0;
//
//     for (int test = 0; test < nrTestEpisodes; test++) {
//         int x1, y1;
//         tie(x1, y1) = selectFirstPlace(maze, 0, 0, rows - 1, cols - 1);
//
//         auto start = chrono::high_resolution_clock::now();
//         int steps = 0;
//         bool success = false;
//
//         while (steps < maxEpisodes) {
//             int act = selectActionWithSoftBoundaries(qTable, rows, cols, x1, y1, 0.0);
//             int x2, y2;
//             double actionReward;
//             tie(x2, y2, act, actionReward) = performAction(maze, rows, cols, x1, y1, act);
//             steps++;
//
//             if (checkExit(maze, x2, y2)) {
//                 success = true;
//                 break;
//             }
//
//             x1 = x2;
//             y1 = y2;
//         }
//
//         auto end = chrono::high_resolution_clock::now();
//         totalPlanningTime += chrono::duration<double>(end - start).count();
//
//         if (success) {
//             successfulPaths++;
//             totalSteps += steps;
//         }
//     }
//
//     double successRate = static_cast<double>(successfulPaths) / nrTestEpisodes;
//     double avgPathLength = successfulPaths > 0 ? static_cast<double>(totalSteps) / successfulPaths : 0.0;
//     double avgOptimalPathLength = successfulPaths > 0 ? static_cast<double>(totalOptimalSteps) / successfulPaths : 0.0;
//     double avgPlanningTime = totalPlanningTime / nrTestEpisodes;
//     double avgExtraSteps = successfulPaths > 0 ? static_cast<double>(totalExtraSteps) / successfulPaths : 0.0;
//     double pctIncrease = (avgExtraSteps / avgOptimalPathLength) * 100.0;
//
//     return make_tuple(avgPlanningTime, successRate, bfsSuccessRate, avgPathLength, avgOptimalPathLength,
//                       totalExtraSteps, pctIncrease);
// }

//*************************************************************************/
void testMasMat(MazeNode *root, const int numSteps) {
    constexpr double epsilon = 1.0; // Exploration parameter for Q-learning

    // Measure MasMat efficiency (before environment change)
    srand(time(NULL));
    auto start = chrono::high_resolution_clock::now();
    masMat(root, {}, epsilon);
    auto end = chrono::high_resolution_clock::now();
    const double masMatStaticTime = chrono::duration<double>(end - start).count();

    // Simulate environment changes
    srand(42);
    vector<pair<int, int> > changedPositions;
    simulateEnvironmentChanges(root, numSteps, changedPositions);

    // Measure MasMat efficiency (after environment change)
    srand(time(NULL));
    start = chrono::high_resolution_clock::now();
    masMat(root, changedPositions, epsilon);
    end = chrono::high_resolution_clock::now();
    const double masMatDynamicTime = chrono::duration<double>(end - start).count();

    // Test the agent's performance after applying MasMat
    srand(time(NULL));
    auto [masMatPlanningTime, masMatSuccessRate, masMatAvgPath] = testAgent(
        *root->maze, root, root->rows, root->cols);

    cout << "\n--- MasMat Results ---\n";
    cout << "Static Training Time     : " << masMatStaticTime << "s\n";
    cout << "Dynamic Training Time    : " << masMatDynamicTime << "s\n";
    cout << "Average Planning Time    : " << masMatPlanningTime << "s\n";
    cout << "Success Rate             : " << masMatSuccessRate * 100 << "%\n";
    cout << "Average Path Length      : " << masMatAvgPath << " steps\n";
}

//*************************************************************************/
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

//*************************************************************************/
void trainLeafNodesInParallel(MazeNode *root, const vector<MazeNode *> &leafNodes, double epsilon) {
    // Use threads to train agents concurrently
    vector<thread> threads;

    for (MazeNode *leaf: leafNodes) {
        threads.emplace_back([root, leaf, epsilon]() {
            // cout << "Training agent in leaf node: Start(" << leaf->startRow << ", " << leaf->startCol << "), "
            //      << "End(" << leaf->endRow << ", " << leaf->endCol << ")\n";

            // Calculate maxSteps dynamically based on leaf size
            int maxSteps = (leaf->endRow - leaf->startRow + 1) + (leaf->endCol - leaf->startCol + 1);

            // Train using root's maze and leaf's qTable (dereferenced)
            trainAgentWithStoppingCriterion(leaf, *root->maze, leaf->rows, leaf->cols, leaf->startRow,
                                            leaf->startCol, leaf->endRow, leaf->endCol, epsilon, maxSteps);

            // Propagate the Q-table results upwards
            propagateQTableUpwards(leaf);
        });
    }

    // Join threads to ensure all training is complete
    for (thread &t: threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

//*************************************************************************/
void trainLeafNodesSequentially(MazeNode *root, const vector<MazeNode *> &leafNodes, double epsilon) {
    for (MazeNode *leaf: leafNodes) {
        // Logging the start of training
        // cout << "Training agent in leaf node: Start(" << leaf->startRow << ", " << leaf->startCol << "), "
        //      << "End(" << leaf->endRow << ", " << leaf->endCol << ")\n";

        // Calculate maxSteps dynamically based on leaf size
        int maxSteps = (leaf->endRow - leaf->startRow + 1) + (leaf->endCol - leaf->startCol + 1);

        // Train using root's maze and leaf's qTable (dereferenced)
        trainAgentWithStoppingCriterion(leaf, *root->maze, leaf->rows, leaf->cols, leaf->startRow,
                                        leaf->startCol, leaf->endRow, leaf->endCol, epsilon, maxSteps);

        // Propagate the Q-table results upwards
        propagateQTableUpwards(leaf);
    }
}

//*************************************************************************/
void trainLeafNodesInBatches(MazeNode *root, const vector<MazeNode *> &leafNodes, const double epsilon,
                             const int batchSize) {
    const int totalNodes = leafNodes.size();
    for (int i = 0; i < totalNodes; i += batchSize) {
        int end = min(i + batchSize, totalNodes);
        vector<MazeNode *> newLeaves(leafNodes.begin() + i, leafNodes.begin() + end);
        trainLeafNodesInParallel(root, newLeaves, epsilon); // Pass root to access maze
    }
}

//*************************************************************************/
void applyLocalPathPlanning(MazeNode* root, const vector<MazeNode*>& changedLeaves = {}) {
    // Environment is static. Performing global path planning selectively
    if (changedLeaves.empty()) {
        // Train all leaf nodes initially
        vector<MazeNode*> leafNodes;
        collectLeafNodes(root, leafNodes);
        const int nrLeafNodes = leafNodes.size();
        trainLeafNodesInBatches(root, leafNodes, 1.0, nrLeafNodes);
    }

    // Environment changed. Training affected leaf nodes
    else {
        const int nrLeafNodes = changedLeaves.size();
        trainLeafNodesInBatches(root, changedLeaves, 1.0, nrLeafNodes);
        // trainLeafNodesInBatches(root, changedLeaves, 0.5, nrLeafNodes); // Lower epsilon for retraining
    }
}

//*************************************************************************/
void trainHierarchy(MazeNode* root, const vector<MazeNode*>& changedLeaves = {}, const int maxLevelsToTrain = 1) {
    if (!root) return;
    const bool isInitialTraining = changedLeaves.empty();

    // Collect leaf nodes to train
    vector<MazeNode*> leafNodesToTrain;
    if (isInitialTraining) {
        collectLeafNodes(root, leafNodesToTrain);
    } else {
        // Use the provided list of affected leaf nodes directly
        leafNodesToTrain = changedLeaves;
    }

    // Train all affected or initial leaf nodes
    trainLeafNodesInBatches(root, leafNodesToTrain, 1.0, leafNodesToTrain.size());

    // Decision mechanism: Count affected children per parent
    unordered_map<MazeNode*, int> parentAffectedCount; // Parent -> # of affected children
    for (const MazeNode* leaf : leafNodesToTrain) {
        if (leaf->parent) {
            parentAffectedCount[leaf->parent]++;
        }
    }

    // Select parents to retrain: >= 1 affected child
    vector<MazeNode*> parentsToTrain;
    for (const auto& [parent, affectedCount] : parentAffectedCount) {
        if (affectedCount >= 1) { // Retrain if 1 or more of 4 children are affected
            parentsToTrain.push_back(parent);
        }
    }

    // Train parents hierarchically up to maxLevelsToTrain
    int levelsTrained = 0;
    unordered_set<MazeNode*> currentLevelNodes(parentsToTrain.begin(), parentsToTrain.end());
    while (!currentLevelNodes.empty() && levelsTrained < maxLevelsToTrain) {
        vector<MazeNode*> nodesToTrain(currentLevelNodes.begin(), currentLevelNodes.end());
        trainLeafNodesInBatches(root, nodesToTrain, 1.0, nodesToTrain.size());

        // Prepare next level with the same decision rule
        unordered_map<MazeNode*, int> nextLevelAffectedCount;
        for (const MazeNode* node : nodesToTrain) {
            if (node->parent) {
                nextLevelAffectedCount[node->parent]++;
            }
        }

        unordered_set<MazeNode*> nextLevelNodes;
        for (const auto& [parent, affectedCount] : nextLevelAffectedCount) {
            if (affectedCount >= 1) { // Apply same rule for higher levels
                nextLevelNodes.insert(parent);
            }
        }
        currentLevelNodes = move(nextLevelNodes);
        levelsTrained++;
    }
}

//*************************************************************************/
double computeLeafSuccessRate(const MazeNode* root, MazeNode* node) {
    if (!root || !node || !root->maze || !node->qTable) return 0.0; // Safety checks

    // Get the sub-environment bounds from the node
    const int startRow = node->startRow;
    const int startCol = node->startCol;
    const int endRow = node->endRow;
    const int endCol = node->endCol;

    // Use the full maze from the root for pathfinding
    const vector<vector<int>>& maze = *root->maze;
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
            auto [success, steps, path] = findValidPath(maze, node, rows, cols, x, y, maxSteps);
            if (success) {
                successfulPaths++;
            }
        }
    }

    // Compute success rate
    return totalPositions > 0 ? static_cast<double>(successfulPaths) / totalPositions : 0.0;
}

//*************************************************************************/
void trainHierarchySmart(MazeNode* root, const vector<MazeNode*>& changedLeaves = {}) {
    if (!root) return; // Safety check: Exit if root is null

    // Determine if this is the initial training call (no changes provided)
    const bool isInitialTraining = changedLeaves.empty();

    // Step 1: Collect leaf nodes to train
    vector<MazeNode*> leafNodesToTrain;
    if (isInitialTraining) {
        // Initial training: Gather all leaf nodes in the hierarchy
        collectLeafNodes(root, leafNodesToTrain);
    } else {
        // Dynamic training: Use the list of leaves affected by changes
        leafNodesToTrain = changedLeaves;
    }

    // Step 2: Decide which leaves to train or retrain
    vector<MazeNode*> leavesToRetrain;
    if (isInitialTraining) {
        // For initial training, train all collected leaves
        leavesToRetrain = leafNodesToTrain;
    } else {
        // For changes, check each affected leaf's success rate
        for (MazeNode* leaf : leafNodesToTrain) {
            if (leaf->baselineSuccessRate >= 0) { // Only process leaves that were previously trained
                const double baseline = leaf->baselineSuccessRate; // Get the stored success rate
                const double newSuccessRate = computeLeafSuccessRate(root, leaf); // Compute current success using findValidPath
                if (baseline - newSuccessRate > 0.02) { // Check if success dropped by more than 2%
                    leavesToRetrain.push_back(leaf); // Mark leaf for retraining
                }
            }
        }
    }

    // Step 3: Train or retrain the selected leaves
    if (!leavesToRetrain.empty()) {
        // Train all leaves marked for retraining in one batch
        trainLeafNodesInBatches(root, leavesToRetrain, 1.0, leavesToRetrain.size());
        // Update each retrained leaf's baseline success rate
        for (MazeNode* leaf : leavesToRetrain) {
            leaf->baselineSuccessRate = computeLeafSuccessRate(root, leaf); // Set new baseline after training
        }
    }

    // Step 4: Propagate retraining upward through the hierarchy
    // Start with parents of retrained leaves
    unordered_set<MazeNode*> currentLevelNodes;
    for (const MazeNode* leaf : leavesToRetrain) {
        if (leaf->parent) {
            currentLevelNodes.insert(leaf->parent); // Add each retrained leaf's parent to check
        }
    }

    // Continue propagating as long as there are nodes to check
    while (!currentLevelNodes.empty()) {
        // Map to track which parents have affected children in this level
        unordered_map<MazeNode*, vector<MazeNode*>> parentToAffected;
        for (MazeNode* node : currentLevelNodes) {
            if (node->parent) {
                parentToAffected[node->parent].push_back(node); // Group affected children by parent
            }
        }

        // Prepare lists for nodes to train in this level and parents for the next level
        vector<MazeNode*> nodesToTrain;
        unordered_set<MazeNode*> nextLevelNodes;

        // Process each node in the current level
        for (MazeNode* node : currentLevelNodes) {
            if (node->baselineSuccessRate < 0) { // Node is untrained
                // Check if all children of this node are trained
                bool allChildrenTrained = true;
                for (const MazeNode* child : node->children) {
                    if (child->baselineSuccessRate < 0) { // Found an untrained child
                        allChildrenTrained = false;
                        break;
                    }
                }
                if (allChildrenTrained) {
                    // All children are trained, so this node can be trained
                    nodesToTrain.push_back(node);
                }
            } else { // Node is already trained
                const double baseline = node->baselineSuccessRate; // Get the stored success rate
                const double newSuccessRate = computeLeafSuccessRate(root, node); // Compute current success using findValidPath
                if (baseline - newSuccessRate > 0.02) { // Check if success dropped by more than 2%
                    nodesToTrain.push_back(node); // Mark node for retraining
                }
            }

            // If this node is being trained/retrained, consider its parent for the next level
            if (!nodesToTrain.empty() && nodesToTrain.back() == node && node->parent) {
                nextLevelNodes.insert(node->parent); // Add parent to next level for checking
            }
        }

        // Step 5: Train the selected nodes in this level
        if (!nodesToTrain.empty()) {
            // Train all marked nodes in one batch
            trainLeafNodesInBatches(root, nodesToTrain, 1.0, nodesToTrain.size());
            // Update each trained node's baseline success rate
            for (MazeNode* node : nodesToTrain) {
                node->baselineSuccessRate = computeLeafSuccessRate(root, node); // Set new baseline
            }
        }

        // Move to the next level of parents to check
        currentLevelNodes = move(nextLevelNodes);
    }
}

//*************************************************************************/
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
    unordered_set<MazeNode*> changedLeaves;
    for (const auto& [r, c] : changedPositions) {
        MazeNode* leaf = root->findSubEnvironment(r, c);
        if (leaf && leaf->children.empty()) changedLeaves.insert(leaf);
    }
    const auto changedLeavesToTrain = vector<MazeNode*>(changedLeaves.begin(), changedLeaves.end());

    // Measure Local Path Planning efficiency (after environment change)
    srand(time(NULL));
    start = chrono::high_resolution_clock::now();
    applyLocalPathPlanning(root, changedLeavesToTrain);
    end = chrono::high_resolution_clock::now();
    const double localDynamicTime = chrono::duration<double>(end - start).count();

    // Test the agent's performance after applying local path planning
    srand(time(NULL));
    auto [localPlanningTime, localSuccessRate, localAvgPath] = testAgent(
        *root->maze, root, root->rows, root->cols);

    cout << "\n--- Local Path Planning Results ---\n";
    cout << "Static Training Time     : " << localStaticTime << "s\n";
    cout << "Dynamic Training Time    : " << localDynamicTime << "s\n";
    cout << "Average Planning Time    : " << localPlanningTime << "s\n";
    cout << "Success Rate             : " << localSuccessRate * 100 << "%\n";
    cout << "Average Path Length      : " << localAvgPath << " steps\n";
}

//*************************************************************************/
void testHierarchicalPathPlanning(MazeNode *root, const int numSteps) {
    // Measure Hierarchical Path Planning efficiency (before environment change)
    srand(time(NULL));
    auto start = chrono::high_resolution_clock::now();
    trainHierarchy(root); // Initial training of the full hierarchy
    auto end = chrono::high_resolution_clock::now();
    const double hierStaticTime = chrono::duration<double>(end - start).count();

    // Simulate environment changes
    srand(42); // Consistent seed for reproducibility
    vector<pair<int, int> > changedPositions;
    simulateEnvironmentChanges(root, numSteps, changedPositions);

    // Identify changed leaves
    unordered_set<MazeNode*> changedLeaves;
    for (const auto& [r, c] : changedPositions) {
        MazeNode* leaf = root->findSubEnvironment(r, c);
        if (leaf && leaf->children.empty()) changedLeaves.insert(leaf);
    }
    const auto changedLeavesToTrain = vector<MazeNode*>(changedLeaves.begin(), changedLeaves.end());

    // Measure Hierarchical Path Planning efficiency (after environment change)
    srand(time(NULL));
    start = chrono::high_resolution_clock::now();
    trainHierarchy(root, changedLeavesToTrain); // Retrain affected nodes and propagate up
    end = chrono::high_resolution_clock::now();
    const double hierDynamicTime = chrono::duration<double>(end - start).count();

    // Test the agent's performance using the root's Q-table (top-level policy)
    srand(time(NULL));
    auto [hierPlanningTime, hierSuccessRate, hierAvgPath] = testAgent(
        *root->maze, root, root->rows, root->cols);

    cout << "\n--- Hierarchical Path Planning Results ---\n";
    cout << "Static Training Time     : " << hierStaticTime << "s\n";
    cout << "Dynamic Training Time    : " << hierDynamicTime << "s\n";
    cout << "Average Planning Time    : " << hierPlanningTime << "s\n";
    cout << "Success Rate             : " << hierSuccessRate * 100 << "%\n";
    cout << "Average Path Length      : " << hierAvgPath << " steps\n";
}

const vector<pair<int, int> > ACTIONS = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};

//*************************************************************************/
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

//*************************************************************************/
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

//*************************************************************************/
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
            tie(agent->row, agent->col) = selectFirstPlace(*subEnv->maze, subEnv->startRow, subEnv->startCol,
                                                           subEnv->endRow, subEnv->endCol);
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

//*************************************************************************/
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
    auto [vdnPlanningTime, vdnSuccessRate, vdnAvgPath] = testAgent(*root->maze, root, root->rows, root->cols);

    // Output Results
    cout << "\n--- VDN Training Results ---\n";
    cout << "Static Training Time     : " << staticTrainingTime << "s\n";
    cout << "Dynamic Training Time    : " << dynamicTrainingTime << "s\n";
    cout << "Average Planning Time    : " << vdnPlanningTime << "s\n";
    cout << "Success Rate             : " << vdnSuccessRate * 100 << "%\n";
    cout << "Average Path Length      : " << vdnAvgPath << " steps\n";
}

//*************************************************************************/
struct Metrics {
    double initialTime;
    double adaptTime;
    double successRate;
    double avgPathLength;
};

//*************************************************************************/
void runFullExperiment() {
    vector<int> sizes = {20, 50, 100};
    vector<tuple<double, double, double>> difficulties = {
        {0.8, 0.19, 0.01},  // Easy
        {0.7, 0.29, 0.01},  // Medium
        {0.6, 0.395, 0.005} // Hard
    };
    // Approaches to test
    vector<string> approaches = {"A* Static","A* Oracle","Local","Hierarchy","HierarchySmart"};

    // Detailed output file for per-step data
    ofstream detailedOut("results_incremental_detailed_smart.csv");
    detailedOut << "Approach,Size,Difficulty,TimeStep,NumChanges,AdaptTime,SuccessRate,AvgPathLength\n";

    // Map to store results
    map<string, vector<vector<Metrics>>> results;
    for (const string& name : approaches) {
        results[name].resize(sizes.size());
        for (int s = 0; s < sizes.size(); ++s) {
            results[name][s].resize(difficulties.size());
        }
    }

    // Iterate over maze sizes and difficulties
    for (int s = 0; s < sizes.size(); ++s) {
        int size = sizes[s];
        cout << "\n\nTesting maze size: " << size << "x" << size;

        for (int d = 0; d < difficulties.size(); ++d) {
            srand(d);
            auto [freeProb, obstProb, chargeProb] = difficulties[d];
            string diffName = (d == 0 ? "Easy" : d == 1 ? "Medium" : "Hard");
            cout << "\n\nDifficulty: " << diffName;

            // Simple scaling: maxTimeSteps proportional to size
            constexpr int k = 2; // Tune this (1 or 2 recommended)
            const int maxTimeSteps = k * size;
            cout << " - maxTimeSteps: " << maxTimeSteps;

            auto initialMaze = vector<vector<int>>(size, vector<int>(size, 0));
            createMaze(initialMaze, size, size, freeProb, obstProb, chargeProb);
            vector<pair<int, vector<pair<int, int>>>> changeSequence(maxTimeSteps);
            MazeNode* tempRoot = createSubEnvironments(initialMaze, size, size);

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
            for (const string& name : approaches) {
                cout << "\n\nTesting " << name;

                MazeNode* root = createSubEnvironments(initialMaze, size, size);
                unordered_map<pair<int, int>, vector<pair<int, int>>, HashPair> shortestPaths;
                double totalInitialTime = 0.0, totalAdaptTime = 0.0, totalSuccessRate = 0.0, totalPathLength = 0.0;
                int stepsCompleted = 0;

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
                    else trainHierarchySmart(root);
                    auto end = chrono::high_resolution_clock::now();
                    totalInitialTime = chrono::duration<double>(end - start).count();
                }
                auto [_, successRate, avgPath] = (name == "A* Oracle" || name == "A* Static") ?
                    testAgentAStar(*root->maze, size, size, shortestPaths) :
                    testAgent(*root->maze, root, size, size);
                totalSuccessRate += successRate;
                totalPathLength += avgPath;
                stepsCompleted++;
                detailedOut << name << "," << size << "," << diffName << ",0,0,"
                            << 0.0 << "," << successRate << "," << avgPath << "\n";

                // Apply changes over time
                vector<vector<int>> currentMaze = initialMaze;
                for (int t = 0; t < maxTimeSteps; ++t) {
                    const auto& [numChanges, changes] = changeSequence[t];
                    for (int i = 0; i < changes.size(); i += 2) {
                        currentMaze[changes[i].first][changes[i].second] = FREE_SPACE;
                        currentMaze[changes[i + 1].first][changes[i + 1].second] = OBSTACLE;
                    }
                    root->maze = make_unique<vector<vector<int>>>(currentMaze);

                    unordered_set<MazeNode*> changedLeaves;
                    for (const auto& [r, c] : changes) {
                        MazeNode* leaf = root->findSubEnvironment(r, c);
                        if (leaf && leaf->children.empty()) changedLeaves.insert(leaf);
                    }
                    vector<MazeNode*> changedLeafSet(changedLeaves.begin(), changedLeaves.end());

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
                        else trainHierarchySmart(root, changedLeafSet);
                        auto end = chrono::high_resolution_clock::now();
                        adaptTime = chrono::duration<double>(end - start).count();
                    }
                    auto [_, stepSuccessRate, stepAvgPath] = (name == "A* Oracle" || name == "A* Static") ?
                        testAgentAStar(*root->maze, size, size, shortestPaths) :
                        testAgent(*root->maze, root, size, size);
                    totalAdaptTime += adaptTime;
                    totalSuccessRate += stepSuccessRate;
                    totalPathLength += stepAvgPath;
                    stepsCompleted++;

                    // Write per-step data
                    detailedOut << name << "," << size << "," << diffName << "," << t + 1 << ","
                                << numChanges << "," << adaptTime << "," << stepSuccessRate << ","
                                << stepAvgPath << "\n";
                }

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
            for (const string& name : approaches) {
                const auto& m = results[name][s][d];
                out << name << "," << size << "," << diffName << ","
                    << m.initialTime << "," << m.adaptTime << "," << m.successRate << "," << m.avgPathLength << "\n";
            }
        }
    }
    out.close();
}

//*************************************************************************/
int main() {
    runFullExperiment();
    return 0;
}
