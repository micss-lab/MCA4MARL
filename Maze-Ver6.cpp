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
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <random>
#include <set>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
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

#define EPISODE_COUNT 500'000
#define LEARNING_RATE 0.3
#define DISCOUNT_FACTOR 0.9

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
class MazeNode {
public:
    vector<vector<int> > maze; // The subenvironment represented as a matrix
    vector<vector<vector<double> > > qTable; // Q-table for the subenvironment
    MazeNode *parent; // Pointer to the parent node
    vector<MazeNode *> children; // List of child subenvironments
    int rows, cols; // Dimensions of the subenvironment
    int startRow, startCol, endRow, endCol; // Bounds of the subenvironment
    int chargingStationCount; // Number of charging stations in this subenvironment

    // Constructor
    MazeNode(const vector<vector<int> > &maze, const int rows, const int cols, const int startRow, const int startCol,
             const int endRow, const int endCol, MazeNode *parent = nullptr): parent(parent), rows(rows), cols(cols),
                                                                              startRow(startRow), startCol(startCol),
                                                                              endRow(endRow), endCol(endCol) {
        // Copy the maze
        this->maze = maze;

        // Initialize the Q-table
        qTable = vector<vector<vector<double> > >(
            rows, vector<vector<double> >(cols, vector<double>(ACTION_COUNT, 0.0)));

        // Count the number of charging stations in this subenvironment
        chargingStationCount = countChargingStations(maze, startRow, startCol, endRow, endCol);
    }

    // Destructor
    ~MazeNode() {
        // Delete all child nodes
        for (const MazeNode *child: children) {
            delete child;
        }
        children.clear(); // Clear the vector for safety
    }

    // Add a child node
    void addChild(MazeNode *child) {
        children.push_back(child);
    }

    MazeNode *findSubEnvironment(const int row, const int col) {
        if (row < startRow || row > endRow || col < startCol || col > endCol) {
            return nullptr; // Position is outside this node's bounds
        }

        // If this is a leaf node, return it
        if (children.empty()) {
            return this;
        }

        // Recursively search children
        for (MazeNode *child: children) {
            MazeNode *result = child->findSubEnvironment(row, col);
            if (result) {
                return result;
            }
        }

        return nullptr; // Should not reach here if tree is properly constructed
    }
};

//*************************************************************************/
MazeNode *deepCopyMazeTree(MazeNode *node) {
    if (!node) return nullptr;

    // Create a new node and copy data
    auto *newNode = new MazeNode(*node);

    // Recursively copy children
    for (MazeNode *child: node->children) {
        newNode->children.push_back(deepCopyMazeTree(child));
    }

    return newNode;
}

/**************************************************************************/
void createMaze(vector<vector<int> > &maze, const int rows, const int cols, const double freeSpaceProb,
                const double obstacleProb,
                const double chargingStationProb) {
    // Validate that the probabilities sum to 1
    if (abs(freeSpaceProb + obstacleProb + chargingStationProb - 1.0) > 1e-6) {
        cerr << "Error: Probabilities must sum to 1." << endl;
        exit(1);
    }

    cout << "\n\t\t=== The Maze: 0 means obstacle, 1 means free space, 2 means charging station ===";

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
int selectAction(const vector<vector<vector<double> > > &qTable, const int x, const int y, const double epsilon,
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
        double maxQValue = qTable[x][y][validActions[0]];
        for (const int i: validActions) {
            if (qTable[x][y][i] > maxQValue) {
                maxQValue = qTable[x][y][i];
                action = i;
            }
        }
    }

    return action;
}

//*************************************************************************/
int selectActionWithSoftBoundaries(const vector<vector<vector<double> > > &qTable, const int rows, const int cols,
                                   const int x, const int y, const double epsilon) {
    int action;
    const double randomValue = static_cast<double>(rand()) / RAND_MAX;

    // Define the potential moves for each action (N, NE, E, SE, S, SW, W, NW)
    const vector<pair<int, int> > moves = {
        {-1, 0},
        {-1, 1},
        {0, 1},
        {1, 1},
        {1, 0},
        {1, -1},
        {0, -1},
        {-1, -1}
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
        double maxQValue = qTable[x][y][validActions[0]];
        for (const int i: validActions) {
            if (qTable[x][y][i] > maxQValue) {
                maxQValue = qTable[x][y][i];
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
void updateQTable(vector<vector<vector<double> > > &qTable, const int x1, const int y1, const int action,
                  const double reward,
                  const int x2, const int y2) {
    // Get the maximum Q-value for the next state
    double maxQNext = qTable[x2][y2][0];
    for (int i = 1; i < ACTION_COUNT; i++) {
        if (qTable[x2][y2][i] > maxQNext) {
            maxQNext = qTable[x2][y2][i];
        }
    }

    // Update the Q-value for the current state and action
    qTable[x1][y1][action] = qTable[x1][y1][action] + LEARNING_RATE * (
                                 reward + DISCOUNT_FACTOR * maxQNext - qTable[x1][y1][action]);
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
void trainAgent(const vector<vector<int> > &maze, vector<vector<vector<double> > > &qTable, const int rows,
                const int cols, const int startRow,
                const int startCol, const int endRow, const int endCol, double epsilon) {
    // Global-sized path array for updates
    auto path = vector<vector<int> >(rows, vector<int>(cols, 0));
    int arrival = 0;
    int x1, y1, x2, y2;
    double actionReward = 0;
    int iteration = 0;
    const int maxEpisodes = rows + cols;

    // Start the training process (generate episodes)
    for (int counter = 0; counter < EPISODE_COUNT; counter++) {
        // Select a random starting place within the sub-environment
        tie(x1, y1) = selectFirstPlace(maze, startRow, startCol, endRow, endCol);
        iteration = 1;

        // Start generating one episode
        while (arrival == 0 && iteration < maxEpisodes) {
            // Select an action using an epsilon-greedy policy
            int act = selectAction(qTable, x1, y1, epsilon, startRow, startCol, endRow, endCol);

            // Perform the selected action and get the associated reward
            tie(x2, y2, act, actionReward) = performAction(maze, rows, cols, x1, y1, act);
            detectPath(maze, path, x1, y1); // Update the path matrix

            // Update the Q-table
            updateQTable(qTable, x1, y1, act, actionReward, x2, y2);

            // Check if the exit condition is met within the sub-environment
            arrival = checkExit(maze, x2, y2);
            x1 = x2;
            y1 = y2;
            iteration++;
        }

        // Decay epsilon to reduce exploration over time
        epsilon = max(0.0, epsilon - (1.0 / EPISODE_COUNT));
        arrival = 0; // Reset arrival for the next episode
    }
}

//*************************************************************************/
void trainAgentWithSoftBoundaries(const vector<vector<int> > &maze, vector<vector<vector<double> > > &qTable,
                                  const int rows, const int cols,
                                  const int startRow, const int startCol, const int endRow, const int endCol,
                                  double epsilon) {
    // Global-sized path array for updates
    auto path = vector<vector<int> >(rows, vector<int>(cols, 0));
    int arrival = 0;
    int x1, y1, x2, y2;
    double actionReward = 0;
    int iteration = 0;
    const int maxEpisodes = rows + cols;

    // Start the training process (generate episodes)
    for (int counter = 0; counter < EPISODE_COUNT; counter++) {
        // Select a random starting place within the sub-environment
        tie(x1, y1) = selectFirstPlace(maze, startRow, startCol, endRow, endCol);
        iteration = 1;

        // Start generating one episode
        while (arrival == 0 && iteration < maxEpisodes) {
            // Select an action using an epsilon-greedy policy
            int act = selectActionWithSoftBoundaries(qTable, rows, cols, x1, y1, epsilon);

            // Perform the selected action and get the associated reward
            tie(x2, y2, act, actionReward) = performAction(maze, rows, cols, x1, y1, act);
            detectPath(maze, path, x1, y1); // Update the path matrix

            // Update the Q-table
            updateQTable(qTable, x1, y1, act, actionReward, x2, y2);

            // Check if the exit condition is met within the sub-environment
            arrival = checkExit(maze, x2, y2);
            x1 = x2;
            y1 = y2;
            iteration++;
        }

        // Decay epsilon to reduce exploration over time
        epsilon = max(0.0, epsilon - (1.0 / EPISODE_COUNT));
        arrival = 0; // Reset arrival for the next episode
    }
}

//*************************************************************************/
void trainAgentWithStoppingCriterion(const vector<vector<int> > &maze, vector<vector<vector<double> > > &qTable,
                                     const int rows, const int cols, const int startRow, const int startCol,
                                     const int endRow, const int endCol, double epsilon, int maxStepsPerEpisode) {
    auto path = vector<vector<int> >(rows, vector<int>(cols, 0));
    int arrival = 0, x1, y1, x2, y2, iteration = 0, counter = 0, stableEpisodes = 0;
    double actionReward = 0;
    bool converged = false;
    vector<vector<vector<double> > > prevQTable = qTable;

    // Convergence parameters
    constexpr double threshold = 5e-4;
    constexpr int patience = 10;
    constexpr double lambda = 1e-3;
    constexpr double epsilon_min = 1e-3;
    constexpr int minEpisodes = 200;

    // Success tracking
    // constexpr int windowSize = 100;
    // vector<int> successWindow(windowSize, 0);
    // int windowIndex = 0, successCount = 0;

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

    // Validation tracking
    // constexpr int validationSize = 100;
    // int validationCounter = 0; // Tracks when to validate

    while (!converged && counter < EPISODE_COUNT) {
        // Single loop with upper limit
        // Run a training episode
        tie(x1, y1) = selectFirstPlace(maze, startRow, startCol, endRow, endCol);
        iteration = 1;

        while (arrival == 0 && iteration < maxStepsPerEpisode) {
            int act = selectActionWithSoftBoundaries(qTable, rows, cols, x1, y1, epsilon);
            tie(x2, y2, act, actionReward) = performAction(maze, rows, cols, x1, y1, act);
            detectPath(maze, path, x1, y1);

            replayBuffer.push_back({x1, y1, act, actionReward, x2, y2});
            if (replayBuffer.size() > bufferSize) replayBuffer.erase(replayBuffer.begin());
            updateQTable(qTable, x1, y1, act, actionReward, x2, y2);

            if (replayBuffer.size() >= batchSize && counter > minEpisodes) {
                for (int i = 0; i < batchSize; i++) {
                    int idx = rand() % replayBuffer.size();
                    const auto &exp = replayBuffer[idx];
                    updateQTable(qTable, exp.x1, exp.y1, exp.action, exp.reward, exp.x2, exp.y2);
                }
            }

            arrival = checkExit(maze, x2, y2);
            x1 = x2;
            y1 = y2;
            iteration++;
        }

        // Update success window
        // successCount -= successWindow[windowIndex];
        // successWindow[windowIndex] = (arrival == 1) ? 1 : 0;
        // successCount += successWindow[windowIndex];
        // windowIndex = (windowIndex + 1) % windowSize;
        arrival = 0;

        // Update epsilon
        epsilon = epsilon_min + (1.0 - epsilon_min) * exp(-lambda * counter);

        // Check convergence every 50 episodes after minEpisodes
        // if (counter % 50 == 0 && counter >= max(windowSize, minEpisodes)) {
        if (counter % 50 == 0 && counter >= minEpisodes) {
            double maxChange = 0.0;
            for (int i = startRow; i <= endRow; i++) {
                for (int j = startCol; j <= endCol; j++) {
                    for (int a = 0; a < ACTION_COUNT; a++) {
                        maxChange = max(maxChange, fabs(qTable[i][j][a] - prevQTable[i][j][a]));
                    }
                }
            }

            // if (successCount == windowSize || (maxChange < threshold && stableEpisodes >= patience)) {
            if (maxChange < threshold && stableEpisodes >= patience) {
                converged = true; // Assume convergence
                // validationCounter++; // Trigger validation
            } else if (maxChange < threshold) {
                stableEpisodes++;
            } else {
                stableEpisodes = 0;
            }
            prevQTable = qTable;
        }

        // Perform validation after initial convergence
        // if (validationCounter > 0) {
        //     int testSuccesses = 0;
        //     for (int t = 0; t < validationSize; t++) {
        //         tie(x1, y1) = selectFirstPlace(maze, startRow, startCol, endRow, endCol);
        //         iteration = 1;
        //         arrival = 0;
        //         while (arrival == 0 && iteration < maxStepsPerEpisode) {
        //             int act = selectActionWithSoftBoundaries(qTable, rows, cols, x1, y1, 0.0);
        //             tie(x2, y2, act, actionReward) = performAction(maze, rows, cols, x1, y1, act);
        //             arrival = checkExit(maze, x2, y2);
        //             x1 = x2;
        //             y1 = y2;
        //             iteration++;
        //         }
        //         if (arrival == 1) testSuccesses++;
        //     }
        //
        //     if (testSuccesses == validationSize) {
        //         converged = true; // Fully converged only if validation passes
        //     } else {
        //         cout << "Validation failed (only " << testSuccesses << "/" << validationSize
        //              << " successes). Continuing training...\n";
        //         validationCounter = 0; // Reset to require new convergence
        //         stableEpisodes = 0;    // Reset stability for fresh evaluation
        //     }
        // }
        counter++; // Increment episode counter
    }

    // if (converged) {
    //     cout << "The Agent Has Converged After " << counter << " Episodes with Success Rate: "
    //     << (successCount * 100.0 / windowSize) << "%\n";
    // } else {
    //     cout << "Training stopped after " << counter << " episodes without full convergence.\n";
    // }
}

//*************************************************************************/
tuple<double, double, double> testAgent(const vector<vector<int> > &maze,
                                        const vector<vector<vector<double> > > &qTable, const int rows, const int cols,
                                        const int nrTestEpisodes) {
    const int maxEpisodes = rows + cols;

    // Keep track of statistics
    double totalPlanningTime = 0.0;
    int successfulPaths = 0;
    int totalSteps = 0;

    // Test the agent over multiple episodes
    for (int test = 0; test < nrTestEpisodes; test++) {
        int x1, y1;
        tie(x1, y1) = selectFirstPlace(maze, 0, 0, rows - 1, cols - 1);

        // Measure the planning time for this episode
        auto start = chrono::high_resolution_clock::now();
        int steps = 0;
        bool success = false;

        // Start generating one episode
        while (steps < maxEpisodes) {
            int act = selectActionWithSoftBoundaries(qTable, rows, cols, x1, y1, 0.0);
            int x2, y2;
            double actionReward;
            tie(x2, y2, act, actionReward) = performAction(maze, rows, cols, x1, y1, act);
            steps++;

            // Check if the exit condition is met
            if (checkExit(maze, x2, y2)) {
                success = true;
                break;
            }
            x1 = x2;
            y1 = y2;
        }

        // Measure the planning time for this episode
        auto end = chrono::high_resolution_clock::now();
        totalPlanningTime += chrono::duration<double>(end - start).count();

        // Update statistics
        if (success) {
            successfulPaths++;
            totalSteps += steps;
        }
    }

    // Calculate the success rate, average path length, and average planning time
    double successRate = static_cast<double>(successfulPaths) / nrTestEpisodes;
    double avgPathLength = successfulPaths > 0 ? static_cast<double>(totalSteps) / successfulPaths : 0.0;
    double avgPlanningTime = totalPlanningTime / nrTestEpisodes;

    return make_tuple(avgPlanningTime, successRate, avgPathLength);
}

//*************************************************************************/
void splitMaze(MazeNode *node, const vector<vector<int> > &maze, const int rows, const int cols, const int startRow,
               const int startCol, const int endRow, const int endCol) {
    if ((endRow - startRow) <= 3 && (endCol - startCol) <= 3) {
        return; // Stop splitting if subenvironment is smaller than 3x3
    }

    const int midRow = (startRow + endRow) / 2;
    const int midCol = (startCol + endCol) / 2;

    // Create potential child nodes
    auto *child1 = new MazeNode(maze, rows, cols, startRow, startCol, midRow, midCol, node); // Top-left
    auto *child2 = new MazeNode(maze, rows, cols, startRow, midCol + 1, midRow, endCol, node); // Top-right
    auto *child3 = new MazeNode(maze, rows, cols, midRow + 1, startCol, endRow, midCol, node); // Bottom-left
    auto *child4 = new MazeNode(maze, rows, cols, midRow + 1, midCol + 1, endRow, endCol, node); // Bottom-right

    // Check if all four children have at least one charging station
    const bool allHaveChargingStations = (child1->chargingStationCount > 0) &&
                                         (child2->chargingStationCount > 0) &&
                                         (child3->chargingStationCount > 0) &&
                                         (child4->chargingStationCount > 0);

    // If all pieces have charging stations, proceed with the split
    if (allHaveChargingStations) {
        // Add children to the current node
        node->addChild(child1);
        node->addChild(child2);
        node->addChild(child3);
        node->addChild(child4);

        // Recursively split the children
        splitMaze(child1, maze, rows, cols, startRow, startCol, midRow, midCol);
        splitMaze(child2, maze, rows, cols, startRow, midCol, midRow, endCol);
        splitMaze(child3, maze, rows, cols, midRow, startCol, endRow, midCol);
        splitMaze(child4, maze, rows, cols, midRow, midCol, endRow, endCol);
    } else {
        // If any child does not have a charging station, do not split
        // Deallocate memory for the unused children
        delete child1;
        delete child2;
        delete child3;
        delete child4;
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
    // Create the root node for the entire maze
    auto *root = new MazeNode(maze, rows, cols, 0, 0, rows - 1, cols - 1);

    // Split the maze into sub-environments
    splitMaze(root, maze, rows, cols, 0, 0, rows - 1, cols - 1);

    // Print the tree structure
    // cout << "\n\nMaze Sub-environment Tree:\n\n";
    // printTree(root);

    // Return the root node to the caller
    return root;
}

//*************************************************************************/
void copyQTableToNode(MazeNode *node, const vector<vector<vector<double> > > &qTable) {
    for (int i = node->startRow; i <= node->endRow; ++i) {
        for (int j = node->startCol; j <= node->endCol; ++j) {
            for (int action = 0; action < ACTION_COUNT; ++action) {
                node->qTable[i][j][action] = qTable[i][j][action];
            }
        }
    }
}

//*************************************************************************/
void propagateQTableDownwards(MazeNode *node) {
    if (!node) return;

    // If the node has children, propagate its Q-table to each child
    for (MazeNode *child: node->children) {
        // Propagate the current node's Q-table to the child node
        for (int i = child->startRow; i <= child->endRow; ++i) {
            for (int j = child->startCol; j <= child->endCol; ++j) {
                for (int action = 0; action < ACTION_COUNT; ++action) {
                    child->qTable[i][j][action] = node->qTable[i][j][action];
                }
            }
        }

        // Recursively propagate to the next level of children
        propagateQTableDownwards(child);
    }
}

//*************************************************************************/
void propagateQTableUpwards(const MazeNode *node) {
    if (!node || !node->parent) return;

    MazeNode *parent = node->parent;

    // Update parent's Q-table using the child node's Q-table
    for (int i = node->startRow; i <= node->endRow; ++i) {
        for (int j = node->startCol; j <= node->endCol; ++j) {
            for (int action = 0; action < ACTION_COUNT; ++action) {
                parent->qTable[i][j][action] = node->qTable[i][j][action];
            }
        }
    }

    // Recursively propagate upwards
    propagateQTableUpwards(parent);
}

//*************************************************************************/
// Function to propagate maze updates from a node to its children
void propagateMazeDownwards(MazeNode *node) {
    if (!node) return;

    // Propagate maze updates to all children
    for (MazeNode *child: node->children) {
        for (int i = child->startRow; i <= child->endRow; ++i) {
            for (int j = child->startCol; j <= child->endCol; ++j) {
                child->maze[i][j] = node->maze[i][j];
            }
        }

        // Recursively propagate updates to lower levels
        propagateMazeDownwards(child);
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
void masMat(MazeNode *root, const vector<pair<int, int> > &changedPositions, const double epsilon) {
    // If no changes, apply global Q-learning
    if (changedPositions.empty()) {
        cout << "\nEnvironment is static. Applying global Q-learning.\n";

        // Calculate maxEpisodes for the root
        int maxSteps = (root->endRow - root->startRow + 1) + (root->endCol - root->startCol + 1);

        // Train the agent on the root environment
        trainAgentWithStoppingCriterion(root->maze, root->qTable, root->rows, root->cols, root->startRow,
                                        root->startCol, root->endRow, root->endCol, epsilon, maxSteps);

        // Copy the Q-table results to the root node (optional, since qTable is modified in place)
        copyQTableToNode(root, root->qTable);

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
        trainAgentWithStoppingCriterion(node->maze, node->qTable, node->rows, node->cols, node->startRow,
                                        node->startCol, node->endRow, node->endCol, epsilon, maxSteps);

        // Copy the Q-table results to the node (optional, since qTable is modified in place)
        copyQTableToNode(node, node->qTable);

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
    vector<pair<int, int> > obstaclePositions = getObstaclePositions(root->maze, rows, cols);
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
                    root->maze[newRow][newCol] == FREE_SPACE) {
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
                root->maze[oldRow][oldCol] = FREE_SPACE;
                root->maze[newRow][newCol] = OBSTACLE;
                obstaclePositions[randomIndex] = {newRow, newCol};

                // cout << "Moved obstacle from (" << oldRow << ", " << oldCol
                // << ") to (" << newRow << ", " << newCol << ")\n";
            }
        }
        // printMatrixInt(root->maze, rows, cols, "Maze after step:");
    }

    propagateMazeDownwards(root);
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
tuple<double, double, double> testAgentAStar(const vector<vector<int> > &maze, const int rows, const int cols,
                                             const unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> &
                                             shortestPaths, const int nrTestEpisodes) {
    // Statistics
    double totalPlanningTime = 0.0;
    int successfulPaths = 0;
    int totalSteps = 0;

    for (int test = 0; test < nrTestEpisodes; test++) {
        int startX, startY;
        tie(startX, startY) = selectFirstPlace(maze, 0, 0, rows - 1, cols - 1);

        // Measure the planning time
        auto start = chrono::high_resolution_clock::now();
        const vector<pair<int, int> > &path = shortestPaths.at({startX, startY});
        const bool success = !path.empty();

        auto end = chrono::high_resolution_clock::now();
        totalPlanningTime += chrono::duration<double>(end - start).count();

        // Update statistics
        if (success) {
            successfulPaths++;
            totalSteps += (path.size() - 1); // Don't count the start position
        }
    }

    // Calculate the success rate, average path length, and average planning time
    double successRate = static_cast<double>(successfulPaths) / nrTestEpisodes;
    double avgPathLength = successfulPaths > 0 ? static_cast<double>(totalSteps) / successfulPaths : 0.0;
    double avgPlanningTime = totalPlanningTime / nrTestEpisodes;

    return make_tuple(avgPlanningTime, successRate, avgPathLength);
}

//*************************************************************************/
void testAStarPerformance(MazeNode *root, const int numSteps, const int nrTestEpisodes) {
    // Measure A* efficiency (before environment change)
    srand(time(NULL));
    auto start = chrono::high_resolution_clock::now();
    const unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> shortestPaths =
            computeAllShortestPaths(root->maze);
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
            computeAllShortestPaths(root->maze);
    end = chrono::high_resolution_clock::now();
    const double dynamicTime = chrono::duration<double>(end - start).count();

    // Test the agent using A* algorithm
    srand(time(NULL));
    auto [planningTime, successRate, avgPath] = testAgentAStar(root->maze, root->rows, root->cols, newShortestPaths,
                                                               nrTestEpisodes);

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
void testMasMat(MazeNode *root, const int numSteps, const int nrTestEpisodes) {
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
        root->maze, root->qTable, root->rows, root->cols, nrTestEpisodes);

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
void trainLeafNodesInParallel(const vector<MazeNode *> &leafNodes, double epsilon) {
    // Use threads to train agents concurrently
    vector<thread> threads;

    for (MazeNode *leaf: leafNodes) {
        threads.emplace_back([leaf, epsilon]() {
            // cout << "Training agent in leaf node: Start(" << leaf->startRow << ", " << leaf->startCol << "), "
            //      << "End(" << leaf->endRow << ", " << leaf->endCol << ")\n";

            // Calculate maxEpisodes dynamically based on leaf size
            int maxSteps = (leaf->endRow - leaf->startRow + 1) + (leaf->endCol - leaf->startCol + 1);

            // Train the agent in this leaf node's subenvironment
            trainAgentWithStoppingCriterion(leaf->maze, leaf->qTable, leaf->rows, leaf->cols, leaf->startRow,
                                            leaf->startCol, leaf->endRow, leaf->endCol, epsilon, maxSteps);

            // Copy the Q-table results to the leaf node (optional, since qTable is modified in place)
            copyQTableToNode(leaf, leaf->qTable);

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
void trainLeafNodesSequentially(const vector<MazeNode *> &leafNodes, double epsilon) {
    for (MazeNode *leaf: leafNodes) {
        // Logging the start of training
        cout << "Training agent in leaf node: Start(" << leaf->startRow << ", " << leaf->startCol << "), "
                << "End(" << leaf->endRow << ", " << leaf->endCol << ")\n";

        // Calculate maxEpisodes dynamically based on leaf size
        int maxSteps = (leaf->endRow - leaf->startRow + 1) + (leaf->endCol - leaf->startCol + 1);

        // Train the agent in this leaf node's subenvironment
        trainAgentWithStoppingCriterion(leaf->maze, leaf->qTable, leaf->rows, leaf->cols, leaf->startRow,
                                        leaf->startCol, leaf->endRow, leaf->endCol, epsilon, maxSteps);

        // Copy the Q-table results to the leaf node (optional, since qTable is modified in place)
        copyQTableToNode(leaf, leaf->qTable);

        // Propagate the Q-table results upwards
        propagateQTableUpwards(leaf);
    }
}

//*************************************************************************/
void trainLeafNodesInBatches(const vector<MazeNode *> &leafNodes, const double epsilon, const int batchSize) {
    const int totalNodes = leafNodes.size();
    for (int i = 0; i < totalNodes; i += batchSize) {
        int end = min(i + batchSize, totalNodes);
        vector<MazeNode *> newLeaves(leafNodes.begin() + i, leafNodes.begin() + end);
        trainLeafNodesInParallel(newLeaves, epsilon);
    }
}

//*************************************************************************/
void applyLocalPathPlanning(MazeNode *root, const vector<pair<int, int> > &changedPositions) {
    vector<MazeNode *> leafNodes;
    collectLeafNodes(root, leafNodes);

    if (changedPositions.empty()) {
        // cout << "\nEnvironment is static. Performing global path planning selectively.\n";
        // Train in batches
        int nrLeafNodes = leafNodes.size();
        trainLeafNodesInBatches(leafNodes, 1.0, min(nrLeafNodes, 100));
    } else {
        unordered_set<MazeNode *> uniqueAffectedNodes;
        for (const auto &pos: changedPositions) {
            MazeNode *affectedNode = root->findSubEnvironment(pos.first, pos.second);
            if (affectedNode && affectedNode->children.empty()) {
                uniqueAffectedNodes.insert(affectedNode);
            }
        }

        vector<MazeNode *> affectedLeafNodes(uniqueAffectedNodes.begin(), uniqueAffectedNodes.end());

        if (!affectedLeafNodes.empty()) {
            // cout << "\nEnvironment changed. Training affected leaf nodes.\n";
            int nrLeafNodes = affectedLeafNodes.size();
            trainLeafNodesInBatches(affectedLeafNodes, 0.5, min(nrLeafNodes, 100));
        } else {
            // cout << "\nNo affected leaf nodes detected.\n";
        }
    }
}

//*************************************************************************/
void trainHierarchy(MazeNode *node, const vector<vector<int> > &maze,
                    const vector<pair<int, int> > &changedPositions = {}) {
    if (!node) return;

    // Determine if this is initial training (no changes) or dynamic retraining
    bool isInitialTraining = changedPositions.empty();

    // Step 1: Train leaf nodes
    vector<MazeNode *> leafNodesToTrain;
    if (isInitialTraining) {
        // Collect all leaf nodes in the hierarchy
        collectLeafNodes(node, leafNodesToTrain);
    } else {
        // Collect only affected leaf nodes
        unordered_set<MazeNode *> uniqueAffectedNodes;
        for (const auto &pos: changedPositions) {
            MazeNode *affectedNode = node->findSubEnvironment(pos.first, pos.second);
            if (affectedNode && affectedNode->children.empty()) {
                uniqueAffectedNodes.insert(affectedNode);
            }
        }
        leafNodesToTrain = vector<MazeNode *>(uniqueAffectedNodes.begin(), uniqueAffectedNodes.end());
    }

    // Train the leaf nodes
    int nrLeafNodes = leafNodesToTrain.size();
    trainLeafNodesInBatches(leafNodesToTrain, 1.0, min(nrLeafNodes, 100));

    // Step 2: Train affected nodes upwards iteratively
    unordered_set<MazeNode *> currentLevelNodes;
    for (MazeNode *leaf: leafNodesToTrain) {
        if (leaf->parent) {
            currentLevelNodes.insert(leaf->parent);
        }
    }

    // Iterate upwards until no more parents (reached root or no affected nodes)
    while (!currentLevelNodes.empty()) {
        unordered_set<MazeNode *> nextLevelNodes;

        // Train current level
        for (MazeNode *currNode: currentLevelNodes) {
            // Propagate Q-tables from children
            for (MazeNode *child: currNode->children) {
                propagateQTableUpwards(child);
            }

            // Train this node
            int maxStepsPerEpisode = (currNode->endRow - currNode->startRow + 1) +
                                     (currNode->endCol - currNode->startCol + 1);
            double initialEpsilon = 0.6;
            trainAgentWithStoppingCriterion(maze, currNode->qTable, currNode->rows, currNode->cols,
                                            currNode->startRow, currNode->startCol,
                                            currNode->endRow, currNode->endCol,
                                            initialEpsilon, maxStepsPerEpisode);
            // cout << (isInitialTraining ? "Trained" : "Retrained") << " node: Start("
            //      << currNode->startRow << "," << currNode->startCol << "), End("
            //      << currNode->endRow << "," << currNode->endCol << ")\n";

            // Add parent to next level if it exists
            if (currNode->parent) {
                nextLevelNodes.insert(currNode->parent);
            }
        }

        // Move to next level
        currentLevelNodes = move(nextLevelNodes);
    }
}

//*************************************************************************/
void testLocalPathPlanning(MazeNode *root, const int numSteps, const int nrTestEpisodes) {
    // Measure Local Path Planning efficiency (before environment change)
    srand(time(NULL));
    auto start = chrono::high_resolution_clock::now();
    applyLocalPathPlanning(root, {});
    auto end = chrono::high_resolution_clock::now();
    const double localStaticTime = chrono::duration<double>(end - start).count();

    // Simulate environment changes and reapply local path planning (if needed)
    srand(42);
    vector<pair<int, int> > changedPositions;
    simulateEnvironmentChanges(root, numSteps, changedPositions);

    // Measure Local Path Planning efficiency (after environment change)
    srand(time(NULL));
    start = chrono::high_resolution_clock::now();
    applyLocalPathPlanning(root, changedPositions);
    end = chrono::high_resolution_clock::now();
    const double localDynamicTime = chrono::duration<double>(end - start).count();

    // Test the agent's performance after applying local path planning
    srand(time(NULL));
    auto [localPlanningTime, localSuccessRate, localAvgPath] = testAgent(
        root->maze, root->qTable, root->rows, root->cols, nrTestEpisodes);

    cout << "\n--- Local Path Planning Results ---\n";
    cout << "Static Training Time     : " << localStaticTime << "s\n";
    cout << "Dynamic Training Time    : " << localDynamicTime << "s\n";
    cout << "Average Planning Time    : " << localPlanningTime << "s\n";
    cout << "Success Rate             : " << localSuccessRate * 100 << "%\n";
    cout << "Average Path Length      : " << localAvgPath << " steps\n";
}

//*************************************************************************/
void testHierarchicalPathPlanning(MazeNode *root, const int numSteps, const int nrTestEpisodes) {
    // Measure Hierarchical Path Planning efficiency (before environment change)
    srand(time(NULL));
    auto start = chrono::high_resolution_clock::now();
    trainHierarchy(root, root->maze); // Initial training of the full hierarchy
    auto end = chrono::high_resolution_clock::now();
    const double hierStaticTime = chrono::duration<double>(end - start).count();

    // Simulate environment changes
    srand(42); // Consistent seed for reproducibility
    vector<pair<int, int> > changedPositions;
    simulateEnvironmentChanges(root, numSteps, changedPositions);

    // Measure Hierarchical Path Planning efficiency (after environment change)
    srand(time(NULL));
    start = chrono::high_resolution_clock::now();
    trainHierarchy(root, root->maze, changedPositions); // Retrain affected nodes and propagate up
    end = chrono::high_resolution_clock::now();
    const double hierDynamicTime = chrono::duration<double>(end - start).count();

    // Test the agent's performance using the root's Q-table (top-level policy)
    srand(time(NULL));
    auto [hierPlanningTime, hierSuccessRate, hierAvgPath] = testAgent(
        root->maze, root->qTable, root->rows, root->cols, nrTestEpisodes);

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
        return selectActionWithSoftBoundaries(subEnv->qTable, subEnv->rows, subEnv->cols, row, col, epsilon);
    }

    pair<int, int> step(const int action) {
        const int new_row = row + ACTIONS[action].first;
        const int new_col = col + ACTIONS[action].second;

        // Check if movement is valid
        if (new_row >= subEnv->startRow && new_row <= subEnv->endRow &&
            new_col >= subEnv->startCol && new_col <= subEnv->endCol &&
            subEnv->maze[new_row][new_col] != OBSTACLE &&
            subEnv->maze[new_row][new_col] != AGENT) {
            // **Restore previous position correctly**
            if (subEnv->maze[row][col] != CHARGING_STATION) {
                subEnv->maze[row][col] = FREE_SPACE; // Restore free space only if it wasn't a charging station
            }

            // Move agent
            row = new_row;
            col = new_col;

            // **Do NOT overwrite charging stations**
            if (subEnv->maze[row][col] != CHARGING_STATION) {
                subEnv->maze[row][col] = AGENT; // Mark new position as occupied by agent
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
        while (subEnv->maze[row][col] != CHARGING_STATION) {
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

            globalQ += agent->subEnv->qTable[row][col][action]; // Sum local Q-values
        }

        // Compute max Q-value for the next state (using individual max Q-values)
        double maxNextGlobalQ = 0.0;
        for (size_t i = 0; i < agents.size(); i++) {
            const int nextRow = next_positions[i].first;
            const int nextCol = next_positions[i].second;

            const double maxQ = *max_element(agents[i]->subEnv->qTable[nextRow][nextCol].begin(),
                                             agents[i]->subEnv->qTable[nextRow][nextCol].end());

            maxNextGlobalQ += maxQ; // Sum max Q-values for each agent
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
            double &qValue = agent->subEnv->qTable[row][col][action];
            qValue += LEARNING_RATE * td_error; // Each agent updates using the same TD error
        }
    }
};

//*************************************************************************/
void trainVDN(VDNTrainer &trainer, double epsilon) {
    // Convergence parameters
    constexpr double threshold = 1e-6; // Convergence threshold
    constexpr int patience = 50; // Required stable episodes
    constexpr int maxEpisodes = 1'000'000; // Safety limit

    int stableEpisodes = 0;
    int converged = 0;
    int episode;

    for (episode = 0; !converged; episode++) {
        // **1. Reset the environment and randomly place agents**
        vector<int> reached_goal(trainer.agents.size(), 0); // Track agents reaching charging stations

        for (Agent *agent: trainer.agents) {
            MazeNode *subEnv = agent->subEnv;
            tie(agent->row, agent->col) = selectFirstPlace(subEnv->maze, subEnv->startRow, subEnv->startCol,
                                                           subEnv->endRow, subEnv->endCol);
            subEnv->maze[agent->row][agent->col] = AGENT;
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
                } else if (agent->subEnv->maze[next_pos.first][next_pos.second] == CHARGING_STATION) {
                    reward = 30.0; // Goal reward
                    reached_goal[i] = 1; // Mark this agent as having reached the goal
                } else if (agent->subEnv->maze[next_pos.first][next_pos.second] == OBSTACLE) {
                    reward = -10.0; // Obstacle penalty
                }

                // Update the agent's q-values
                updateQTable(agent->subEnv->qTable, prev_pos.first, prev_pos.second, action, reward, next_pos.first,
                             next_pos.second);
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
        epsilon = epsilon_min + (epsilon - epsilon_min) * exp(-lambda * episode);

        // **6. Clear agents from the environment using the reached_goal vector**
        for (size_t i = 0; i < trainer.agents.size(); i++) {
            Agent *agent = trainer.agents[i];
            // Change position to charging station if agent reached it
            if (reached_goal[i]) {
                agent->subEnv->maze[agent->row][agent->col] = CHARGING_STATION;
                // Otherwise, clear the agent's position
            } else {
                agent->subEnv->maze[agent->row][agent->col] = FREE_SPACE;
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
void testVDNTraining(MazeNode *root, const int numChanges, const int testEpisodes) {
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
    auto [vdnPlanningTime, vdnSuccessRate, vdnAvgPath] = testAgent(root->maze, root->qTable, root->rows, root->cols,
                                                                   testEpisodes);

    // Output Results
    cout << "\n--- VDN Training Results ---\n";
    cout << "Static Training Time     : " << staticTrainingTime << "s\n";
    cout << "Dynamic Training Time    : " << dynamicTrainingTime << "s\n";
    cout << "Average Planning Time    : " << vdnPlanningTime << "s\n";
    cout << "Success Rate             : " << vdnSuccessRate * 100 << "%\n";
    cout << "Average Path Length      : " << vdnAvgPath << " steps\n";
}

//*************************************************************************/
// void runExperiment(const int rows, const int cols, const double freeSpaceProb, const double obstacleProb,
//                    const double chargingStationProb, const int numChanges) {
//     // Create and initialize the maze once
//     auto maze = vector<vector<int> >(rows, vector<int>(cols, 0));
//     createMaze(maze, rows, cols, freeSpaceProb, obstacleProb, chargingStationProb);
//
//     // Output Maze size
//     cout << "\nMaze Size: " << rows << "x" << cols << "\n";
//
//     constexpr int nrTestEpisodes = 100'000; // Number of test episodes
//
//     // List of test functions to run
//     vector<function<void(MazeNode *, int, int)> > tests = {
//         testAStarPerformance,
//         // testMasMat,
//         testLocalPathPlanning,
//         testHierarchicalPathPlanning,
//         // testVDNTraining
//     };
//
//     // Run each test sequentially
//     for (const auto &test: tests) {
//         // Create a new root for this test
//         MazeNode *root = createSubEnvironments(maze, rows, cols);
//
//         // Run the test
//         test(root, numChanges, nrTestEpisodes);
//
//         // Clean up the root
//         delete root;
//     }
// }

//*************************************************************************/
struct Metrics {
    double initialTime;
    double adaptTime;
    double successRate;
    double avgPathLength;
};

//*************************************************************************/
void runFullExperiment() {
    vector<int> sizes = {10, 20, 50, 100, 200, 300};
    vector<tuple<double, double, double> > difficulties = {
        {0.8, 0.19, 0.01},
        {0.6, 0.39, 0.01},
        {0.6, 0.395, 0.005}
    };
    map<int, vector<int> > changeLevels = {
        {10, {1, 5}},
        {20, {1, 5}},
        {50, {1, 5, 10, 20}},
        {100, {1, 5, 10, 20}},
        {200, {1, 5, 10, 20, 30, 50}},
        {300, {1, 5, 10, 20, 30, 50}}
    };
    vector<pair<string, function<void(MazeNode *, int, int)> > > approaches = {
        {"A*", testAStarPerformance},
        {"Local", testLocalPathPlanning},
        {"Hierarchy", testHierarchicalPathPlanning}
    };
    constexpr int nrTestEpisodes = 10'000;

    // Store results: [approach][size][difficulty][change]
    map<string, vector<vector<vector<Metrics> > > > results; // 3D structure
    for (const auto &approach: approaches) {
        results[approach.first].resize(sizes.size()); // Resize for sizes
        for (int s = 0; s < sizes.size(); ++s) {
            results[approach.first][s].resize(difficulties.size()); // Resize for difficulties
        }
    }

    // Run the full experiment
    for (int s = 0; s < sizes.size(); ++s) {
        int size = sizes[s];
        cout << "\nTesting maze size: " << size << "x" << size;

        // Run the experiment for each difficulty level
        for (int d = 0; d < difficulties.size(); ++d) {
            srand(42 + d); // Re-seed per difficulty
            auto [freeProb, obstProb, chargeProb] = difficulties[d];
            string diffName = (d == 0 ? "Easy" : d == 1 ? "Medium" : "Hard");
            cout << "\n\nDifficulty: " << diffName;

            // Create the maze
            auto maze = vector<vector<int> >(size, vector<int>(size, 0));
            createMaze(maze, size, size, freeProb, obstProb, chargeProb);

            // Simulate environment changes
            vector<vector<pair<int, int> > > changeSets(changeLevels[size].size());
            MazeNode *tempRoot = createSubEnvironments(maze, size, size);
            for (int c = 0; c < changeLevels[size].size(); ++c) {
                simulateEnvironmentChanges(tempRoot, changeLevels[size][c], changeSets[c]);
            }
            delete tempRoot;

            // Run each approach
            for (const auto &[name, testFunc]: approaches) {
                cout << "\nTesting " << name;

                MazeNode *root = createSubEnvironments(maze, size, size);
                Metrics initialMetrics;

                // Run initial test
                if (name == "A*") {
                    auto start = chrono::high_resolution_clock::now();
                    auto shortestPaths = computeAllShortestPaths(root->maze);
                    auto end = chrono::high_resolution_clock::now();
                    auto [planTime, successRate, avgPath] = testAgentAStar(
                        root->maze, size, size, shortestPaths, nrTestEpisodes);
                    initialMetrics = {chrono::duration<double>(end - start).count(), 0.0, successRate, avgPath};
                } else {
                    auto start = chrono::high_resolution_clock::now();
                    if (name == "Local") applyLocalPathPlanning(root, {});
                    else trainHierarchy(root, root->maze);
                    auto end = chrono::high_resolution_clock::now();
                    auto [_, successRate, avgPath] = testAgent(root->maze, root->qTable, size, size, nrTestEpisodes);
                    initialMetrics = {chrono::duration<double>(end - start).count(), 0.0, successRate, avgPath};
                }
                results[name][s][d].push_back(initialMetrics);

                // Run adaptive tests
                for (int c = 0; c < changeLevels[size].size(); ++c) {
                    vector<pair<int, int> > changes = changeSets[c];

                    // Reset the maze
                    root->maze = maze;

                    // Apply changes
                    for (int i = 0; i < changes.size(); i += 2) {
                        root->maze[changes[i].first][changes[i].second] = FREE_SPACE;
                        root->maze[changes[i + 1].first][changes[i + 1].second] = OBSTACLE;
                    }
                    propagateMazeDownwards(root);

                    Metrics adaptMetrics;
                    if (name == "A*") {
                        auto start = chrono::high_resolution_clock::now();
                        auto newShortestPaths = computeAllShortestPaths(root->maze);
                        auto end = chrono::high_resolution_clock::now();
                        auto [planTime, successRate, avgPath] = testAgentAStar(
                            root->maze, size, size, newShortestPaths, nrTestEpisodes);
                        adaptMetrics = {
                            initialMetrics.initialTime, chrono::duration<double>(end - start).count(), successRate,
                            avgPath
                        };
                    } else {
                        auto start = chrono::high_resolution_clock::now();
                        if (name == "Local") applyLocalPathPlanning(root, changes);
                        else trainHierarchy(root, root->maze, changes);
                        auto end = chrono::high_resolution_clock::now();
                        auto [_, successRate, avgPath] =
                                testAgent(root->maze, root->qTable, size, size, nrTestEpisodes);
                        adaptMetrics = {
                            initialMetrics.initialTime, chrono::duration<double>(end - start).count(), successRate,
                            avgPath
                        };
                    }
                    results[name][s][d].push_back(adaptMetrics);
                }
                delete root;
            }
        }
    }

    // Save results to file
    ofstream out("results.csv");
    out << "Approach,Size,Difficulty,Changes,InitialTime,AdaptTime,SuccessRate,AvgPathLength\n";
    for (int s = 0; s < sizes.size(); ++s) {
        int size = sizes[s];
        for (int d = 0; d < difficulties.size(); ++d) {
            string diffName = (d == 0 ? "Easy" : d == 1 ? "Medium" : "Hard");
            for (const auto &[name, _]: approaches) {
                for (int c = 0; c <= changeLevels[size].size(); ++c) {
                    int changes = (c == 0 ? 0 : changeLevels[size][c - 1]);
                    const auto &m = results[name][s][d][c]; // Correct 3D access
                    out << name << "," << size << "," << diffName << "," << changes << ","
                            << m.initialTime << "," << m.adaptTime << "," << m.successRate << "," << m.avgPathLength <<
                            "\n";
                }
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
