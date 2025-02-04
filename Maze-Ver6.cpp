/****************************************************************************************************/
/*			This Program has been written by Hossein Yarahmadi					                    */
/*          The Path Matrix includes three types of entities                                        */
/*          0: means the obstacle  1:means the feasible moving  2:means the charge station          */
/****************************************************************************************************/

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <thread>
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

#define MAX_STEPS_PER_EPISODE 100
#define EPISODE_COUNT 500'000
#define LEARNING_RATE 0.2
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
tuple<int, int> selectFirstPlace(const vector<vector<int> > &maze, const int startRow, const int startCol,
                                 const int endRow,
                                 const int endCol) {
    int x, y;
    // Keep generating random indices until a free space is found
    do {
        x = rand() % (endRow - startRow) + startRow;
        y = rand() % (endCol - startCol) + startCol;
    } while (maze[x][y] != FREE_SPACE);
    return make_tuple(x, y);
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
                                   const int x,
                                   const int y, const double epsilon) {
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
                                           const int x1,
                                           const int y1, const int action) {
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

    // Reward system
    if (maze[x2][y2] == CHARGING_STATION) {
        reward = 30.0; // Larger reward for charging station
    } else if (changePos == 0) {
        reward = -10.0; // Penalty for bumping into an obstacle
    } else {
        reward = -1.0; // Step penalty
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

    // Start the training process (generate episodes)
    for (int counter = 0; counter < EPISODE_COUNT; counter++) {
        // Select a random starting place within the sub-environment
        tie(x1, y1) = selectFirstPlace(maze, startRow, startCol, endRow, endCol);
        iteration = 1;

        // Start generating one episode
        while (arrival == 0 && iteration < MAX_STEPS_PER_EPISODE) {
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

    // Start the training process (generate episodes)
    for (int counter = 0; counter < EPISODE_COUNT; counter++) {
        // Select a random starting place within the sub-environment
        tie(x1, y1) = selectFirstPlace(maze, startRow, startCol, endRow, endCol);
        iteration = 1;

        // Start generating one episode
        while (arrival == 0 && iteration < MAX_STEPS_PER_EPISODE) {
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
                                     const int endRow, const int endCol, double epsilon) {
    // Global-sized path array for updates
    auto path = vector<vector<int> >(rows, vector<int>(cols, 0));
    int arrival = 0, x1, y1, x2, y2, iteration = 0, counter = 0;
    double actionReward = 0;
    int stableEpisodes = 0;
    bool converged = false;

    // Create a backup Q-table for comparison
    vector<vector<vector<double> > > prevQTable = qTable;

    // Epsilon decay parameters
    constexpr double epsilon_min = 0.001;
    constexpr double lambda = 1e-8; // Adjust based on testing

    // Convergence parameters
    constexpr double threshold = 1e-5;
    constexpr int patience = 30;

    // Start the training process (generate episodes)
    for (counter = 0; !converged; counter++) {
        // Select a random starting place within the sub-environment
        tie(x1, y1) = selectFirstPlace(maze, startRow, startCol, endRow, endCol);
        iteration = 1;

        while (arrival == 0 && iteration < MAX_STEPS_PER_EPISODE) {
            // Select an action using an epsilon-greedy policy
            int act = selectActionWithSoftBoundaries(qTable, rows, cols, x1, y1, epsilon);

            // Perform the selected action and get the associated reward
            tie(x2, y2, act, actionReward) = performAction(maze, rows, cols, x1, y1, act);
            detectPath(maze, path, x1, y1);

            // Update the Q-table
            updateQTable(qTable, x1, y1, act, actionReward, x2, y2);

            // Check if the exit condition is met within the sub-environment
            arrival = checkExit(maze, x2, y2);
            x1 = x2;
            y1 = y2;
            iteration++;
        }

        // Apply exponential decay to epsilon
        epsilon = epsilon_min + (epsilon - epsilon_min) * exp(-lambda * counter);
        arrival = 0; // Reset arrival for the next episode

        // Check stopping criterion every 10 episodes (to reduce computation cost)
        if (counter % 10 == 0) {
            double maxChange = 0.0;
            for (int i = startRow; i <= endRow; i++) {
                for (int j = startCol; j <= endCol; j++) {
                    for (int a = 0; a < ACTION_COUNT; a++) {
                        maxChange = max(maxChange, fabs(qTable[i][j][a] - prevQTable[i][j][a]));
                    }
                }
            }

            if (maxChange < threshold) {
                stableEpisodes++;
                if (stableEpisodes >= patience) {
                    converged = true;
                }
            } else {
                stableEpisodes = 0;
            }

            prevQTable = qTable; // Update the previous Q-table
        }
    }
    cout << "The Agent Has Converged After " << counter << " Episodes\n";
}

//*************************************************************************/
tuple<double, double, double> testAgent(const vector<vector<int> > &maze,
                                        const vector<vector<vector<double> > > &qTable,
                                        const int rows, const int cols,
                                        const int nrTestEpisodes) {
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
        while (steps < MAX_STEPS_PER_EPISODE) {
            int act = selectActionWithSoftBoundaries(qTable, rows, cols, x1, y1, 0.0);
            int x2, y2;
            double actionReward;
            tie(x2, y2, act, actionReward) = performAction(maze, rows, cols, x1, y1, act);

            // Check if the exit condition is met
            if (checkExit(maze, x2, y2)) {
                success = true;
                break;
            }

            x1 = x2;
            y1 = y2;
            steps++;
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
    cout << "\n\nMaze Sub-environment Tree:\n\n";
    printTree(root);

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
        auto rootQTable = vector<vector<vector<double> > >(
            root->rows, vector<vector<double> >(root->cols, vector<double>(ACTION_COUNT, 0.0)));

        // Train the agent on the root environment
        trainAgentWithStoppingCriterion(root->maze, rootQTable, root->rows, root->cols, root->startRow, root->startCol,
                                        root->endRow, root->endCol, epsilon);

        // Copy the Q-table results to the leaf node
        copyQTableToNode(root, rootQTable);

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
        auto subQTable = vector<vector<vector<double> > >(
            node->rows, vector<vector<double> >(node->cols, vector<double>(ACTION_COUNT, 0.0)));

        // Train the agent on the sub-environment
        trainAgentWithStoppingCriterion(node->maze, subQTable, node->rows, node->cols, node->startRow, node->startCol,
                                        node->endRow, node->endCol, epsilon);

        // Copy the Q-table results to the leaf node
        copyQTableToNode(node, subQTable);

        // Propagate the Q-table results from the leaf to the root
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
                //         << ") to (" << newRow << ", " << newCol << ")\n";
            }
        }
        // printMatrixInt(root->maze, rows, cols, "Maze after step:");
    }

    propagateMazeDownwards(root);
}

//*************************************************************************/
void testMasMat(MazeNode *root, const int numSteps, const int testEpisodes) {
    // Apply MasMat before any changes in the environment (initial training)
    constexpr double epsilon = 1.0; // Exploration parameter for Q-learning
    masMat(root, {}, epsilon);

    // Simulate environment changes and test MasMat
    for (int step = 0; step < numSteps; ++step) {
        cout << "\nStep " << step + 1 << ":\n";

        // Track changed positions during environment changes
        vector<pair<int, int> > changedPositions;
        simulateEnvironmentChanges(root, 1, changedPositions);

        // Apply the MasMat algorithm
        masMat(root, changedPositions, epsilon);

        // Optionally print the updated maze for debugging
        printMatrixInt(root->maze, root->rows, root->cols, "Maze after step:");
    }

    // Test the agent's performance after applying MasMat
    cout << "Testing the agent's performance:\n";
    testAgent(root->maze, root->qTable, root->rows, root->cols, testEpisodes);
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
            vector<vector<vector<double> > > qTable(
                leaf->rows, vector<vector<double> >(leaf->cols, vector<double>(ACTION_COUNT, 0)));
            cout << "Training agent in leaf node: Start(" << leaf->startRow << ", " << leaf->startCol << "), "
                    << "End(" << leaf->endRow << ", " << leaf->endCol << ")\n";

            // Train the agent in this leaf node's subenvironment
            trainAgentWithStoppingCriterion(leaf->maze, qTable, leaf->rows, leaf->cols, leaf->startRow, leaf->startCol,
                                            leaf->endRow, leaf->endCol, epsilon);

            // Copy the Q-table results to the leaf node
            copyQTableToNode(leaf, qTable);

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
void trainLeafNodesSequentially(const vector<MazeNode *> &leafNodes, const double epsilon) {
    for (MazeNode *leaf: leafNodes) {
        // Create a fresh Q-table for this leaf node
        vector<vector<vector<double> > > qTable(
            leaf->rows, vector<vector<double> >(leaf->cols, vector<double>(ACTION_COUNT, 0)));

        // Logging the start of training
        cout << "Training agent in leaf node: Start(" << leaf->startRow << ", " << leaf->startCol << "), "
                << "End(" << leaf->endRow << ", " << leaf->endCol << ")\n";

        // Train the agent in this leaf node's subenvironment
        trainAgentWithStoppingCriterion(leaf->maze, qTable, leaf->rows, leaf->cols, leaf->startRow, leaf->startCol,
                                        leaf->endRow, leaf->endCol, epsilon);

        // Copy the Q-table results to the leaf node
        copyQTableToNode(leaf, qTable);

        // Propagate the Q-table results upwards
        propagateQTableUpwards(leaf);
    }
}

//*************************************************************************/
void applyLocalPathPlanning(MazeNode *root, const vector<pair<int, int> > &changedPositions, const double epsilon) {
    // Environment is static
    if (changedPositions.empty()) {
        vector<MazeNode *> leafNodes;
        collectLeafNodes(root, leafNodes);
        cout << "\nEnvironment is static. Performing global local path planning for all leaf nodes.\n";
        trainLeafNodesInParallel(leafNodes, epsilon); // Train all leaf nodes in parallel

        // Environment changed
    } else {
        // Collect all affected leaf nodes
        unordered_set<MazeNode *> uniqueAffectedNodes;
        for (const auto &pos: changedPositions) {
            MazeNode *affectedNode = root->findSubEnvironment(pos.first, pos.second);
            if (affectedNode && affectedNode->children.empty()) {
                uniqueAffectedNodes.insert(affectedNode); // Ensure no duplicates
            }
        }

        // Convert the unordered_set to a vector
        vector<MazeNode *> affectedLeafNodes(uniqueAffectedNodes.begin(), uniqueAffectedNodes.end());

        // Apply local path planning to affected leaf nodes
        if (!affectedLeafNodes.empty()) {
            cout << "\nEnvironment changed. Performing local path planning for affected leaf nodes.\n";
            trainLeafNodesInParallel(affectedLeafNodes, epsilon); // Train affected leaf nodes in parallel
        } else {
            cout << "\nNo affected leaf nodes detected for local path planning.\n";
        }
    }
}

//*************************************************************************/
void testLocalPathPlanning(MazeNode *root, const int numSteps, const int testEpisodes) {
    cout << "\nTesting the second approach: Local path planning in leaf nodes\n";

    // Initial local path planning and propagation upwards
    constexpr double epsilon = 1.0; // Exploration parameter for Q-learning
    applyLocalPathPlanning(root, {}, epsilon);

    // Simulate environment changes and reapply local path planning (if needed)
    for (int step = 0; step < numSteps; ++step) {
        cout << "\nStep " << step + 1 << ":\n";

        // Track changed positions during environment changes
        vector<pair<int, int> > changedPositions;
        simulateEnvironmentChanges(root, 1, changedPositions);

        // Apply local path planning based on changes
        applyLocalPathPlanning(root, changedPositions, epsilon);

        // Optionally print the updated maze for debugging
        printMatrixInt(root->maze, root->rows, root->cols, "Maze after step:");
    }

    // Test the agent's performance after applying local path planning
    cout << "Testing the agent's performance:\n";
    testAgent(root->maze, root->qTable, root->rows, root->cols, testEpisodes);
}

//*************************************************************************/
void runExperiment(const int rows, const int cols, const double freeSpaceProb, const double obstacleProb,
                   const double wallProb, const int numChanges) {
    srand(42); // Fixed random seed for fair comparison

    // Create and initialize the maze
    auto maze = vector<vector<int> >(rows, vector<int>(cols, 0));
    createMaze(maze, rows, cols, freeSpaceProb, obstacleProb, wallProb);
    MazeNode *root = createSubEnvironments(maze, rows, cols);

    constexpr int testEpisodes = 100; // Number of test episodes

    // Measure MasMat efficiency (before environment change)
    auto start = chrono::high_resolution_clock::now();
    masMat(root, {}, 1.0); // Static training
    auto end = chrono::high_resolution_clock::now();
    const double masMatStaticTime = chrono::duration<double>(end - start).count();

    // Measure Local Path Planning efficiency (before environment change)
    start = chrono::high_resolution_clock::now();
    applyLocalPathPlanning(root, {}, 1.0); // Static training
    end = chrono::high_resolution_clock::now();
    const double localStaticTime = chrono::duration<double>(end - start).count();

    // Simulate environment change
    vector<pair<int, int> > changedPositions;
    simulateEnvironmentChanges(root, numChanges, changedPositions);

    // Measure MasMat efficiency (after environment change)
    start = chrono::high_resolution_clock::now();
    masMat(root, changedPositions, 1.0);
    end = chrono::high_resolution_clock::now();
    const double masMatDynamicTime = chrono::duration<double>(end - start).count();

    // Measure Local Path Planning efficiency (after environment change)
    start = chrono::high_resolution_clock::now();
    applyLocalPathPlanning(root, changedPositions, 1.0);
    end = chrono::high_resolution_clock::now();
    const double localDynamicTime = chrono::duration<double>(end - start).count();

    // Measure planning time and path effectiveness
    auto [masMatPlanningTime, masMatSuccessRate, masMatAvgPath] = testAgent(
        root->maze, root->qTable, root->rows, root->cols, testEpisodes);
    auto [localPlanningTime, localSuccessRate, localAvgPath] = testAgent(
        root->maze, root->qTable, root->rows, root->cols, testEpisodes);

    // Output results
    cout << "\nMaze Size: " << rows << "x" << cols << "\n";

    cout << "\n--- MasMat Results ---\n";
    cout << "Static Training Time   : " << masMatStaticTime << "s\n";
    cout << "Dynamic Training Time  : " << masMatDynamicTime << "s\n";
    cout << "Average Planning Time  : " << masMatPlanningTime << "s\n";
    cout << "Success Rate           : " << masMatSuccessRate * 100 << "%\n";
    cout << "Average Path Length    : " << masMatAvgPath << " steps\n";

    cout << "\n--- Local Path Planning Results ---\n";
    cout << "Static Training Time   : " << localStaticTime << "s\n";
    cout << "Dynamic Training Time  : " << localDynamicTime << "s\n";
    cout << "Average Planning Time  : " << localPlanningTime << "s\n";
    cout << "Success Rate           : " << localSuccessRate * 100 << "%\n";
    cout << "Average Path Length    : " << localAvgPath << " steps\n";

    delete root;
}

int main() {
    // Run experiments with different maze sizes and obstacle densities
    runExperiment(10, 10, 0.8, 0.18, 0.02, 1);
    runExperiment(10, 10, 0.8, 0.18, 0.02, 2);
    runExperiment(10, 10, 0.6, 0.38, 0.02, 1);
    runExperiment(10, 10, 0.6, 0.38, 0.02, 2);

    runExperiment(20, 20, 0.8, 0.18, 0.02, 1);
    runExperiment(20, 20, 0.8, 0.18, 0.02, 4);
    runExperiment(20, 20, 0.6, 0.38, 0.02, 1);
    runExperiment(20, 20, 0.6, 0.38, 0.02, 4);

    runExperiment(50, 50, 0.8, 0.18, 0.02, 1);
    runExperiment(50, 50, 0.8, 0.18, 0.02, 8);
    runExperiment(50, 50, 0.6, 0.38, 0.02, 1);
    runExperiment(50, 50, 0.6, 0.38, 0.02, 8);

    runExperiment(100, 100, 0.8, 0.18, 0.02, 1);
    runExperiment(100, 100, 0.8, 0.18, 0.02, 10);
    runExperiment(100, 100, 0.6, 0.38, 0.02, 1);
    runExperiment(100, 100, 0.6, 0.38, 0.02, 10);

    runExperiment(150, 150, 0.8, 0.18, 0.02, 1);
    runExperiment(150, 150, 0.8, 0.18, 0.02, 10);
    runExperiment(150, 150, 0.6, 0.38, 0.02, 1);
    runExperiment(150, 150, 0.6, 0.38, 0.02, 10);

    runExperiment(200, 200, 0.8, 0.18, 0.02, 1);
    runExperiment(200, 200, 0.8, 0.18, 0.02, 10);
    runExperiment(200, 200, 0.6, 0.38, 0.02, 1);
    runExperiment(200, 200, 0.6, 0.38, 0.02, 10);

    return 0;
}
