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

#define ROW_COUNT 50 // The number of rows in the maze
#define COL_COUNT 50 // The number of columns in the maze
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

#define MAX_STEPS_PER_EPISODE 5000
#define EPISODE_COUNT 1'000'000
#define LEARNING_RATE 0.2
#define DISCOUNT_FACTOR 0.9

/**************************************************************************/
using namespace std;

//*************************************************************************/
int countChargingStations(const int maze[ROW_COUNT][COL_COUNT], const int startRow, const int startCol,
                          const int endRow, const int endCol) {
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
    int maze[ROW_COUNT][COL_COUNT] = {0}; // Subenvironment's maze
    double qTable[ROW_COUNT][COL_COUNT][ACTION_COUNT] = {0}; // Q-table for the subenvironment
    MazeNode *parent; // Pointer to the parent node
    vector<MazeNode *> children; // List of child subenvironments
    int startRow, startCol, endRow, endCol; // Bounds of the subenvironment
    int chargingStationCount; // Number of charging stations in this subenvironment

    // Constructor
    MazeNode(const int maze[ROW_COUNT][COL_COUNT], const int startRow, const int startCol, const int endRow,
             const int endCol, MazeNode *parent = nullptr): parent(parent), startRow(startRow), startCol(startCol),
                                                            endRow(endRow), endCol(endCol) {
        // Copy the maze
        for (int i = 0; i < ROW_COUNT; i++) {
            for (int j = 0; j < COL_COUNT; j++) {
                this->maze[i][j] = maze[i][j];
            }
        }

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
        if (row < startRow || row >= endRow || col < startCol || col >= endCol) {
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
void createMaze(int maze[ROW_COUNT][COL_COUNT], const double freeSpaceProb, const double obstacleProb,
                const double chargingStationProb) {
    // Validate that the probabilities sum to 1
    if (abs(freeSpaceProb + obstacleProb + chargingStationProb - 1.0) > 1e-6) {
        cerr << "Error: Probabilities must sum to 1." << endl;
        exit(1);
    }

    cout << "\n\t\t=== The Maze: 0 means obstacle, 1 means free space, 2 means charging station ===";

    bool hasChargingStation = false;

    for (int i = 0; i < ROW_COUNT; i++) {
        for (int j = 0; j < COL_COUNT; j++) {
            double randomValue = static_cast<double>(rand()) / RAND_MAX;
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
        const int randomRow = rand() % ROW_COUNT;
        const int randomCol = rand() % COL_COUNT;
        maze[randomRow][randomCol] = CHARGING_STATION; // Place a charging station
    }
}

/**************************************************************************/
void printMatrixInt(const int matrix[ROW_COUNT][COL_COUNT], const char text[]) {
    cout << "\n\t\t******* This is The: " << text << "  *******\n\n";

    // Print column headers with [j] format
    cout << "\t\t\t\t";
    cout << "      "; // Space for row index
    for (int j = 0; j < COL_COUNT; j++) {
        cout << "[" << j << "]  ";
    }
    cout << "\n";

    // Print each row with [i] format
    for (int i = 0; i < ROW_COUNT; i++) {
        cout << "\t\t\t\t";
        cout << "[" << i << "] "; // Row index with padding
        for (int j = 0; j < COL_COUNT; j++) {
            cout << setw(4) << matrix[i][j] << " ";
        }
        cout << "\n";
    }
    cout << "\n";
}

/**************************************************************************/
void printMatrixFloat(const double matrix[ROW_COUNT][COL_COUNT], const char text[]) {
    cout << "\n============= The " << text << " ===============";
    cout << "\n";

    // Print column headers with [j] format
    cout << "\t\t\t\t";
    cout << "         "; // Space for row index
    for (int j = 0; j < COL_COUNT; j++) {
        cout << "[" << j << "]     ";
    }
    cout << "\n";

    // Print each row with [i] format
    for (int i = 0; i < ROW_COUNT; i++) {
        cout << "\t\t\t\t";
        cout << "[" << i << "] "; // Row index with padding
        for (int j = 0; j < COL_COUNT; j++) {
            cout << setw(8) << fixed << setprecision(2) << matrix[i][j];
        }
        cout << "\n";
    }
    cout << "\n";
}

/**************************************************************************/
int checkExit(const int matrix[ROW_COUNT][COL_COUNT], const int x, const int y) {
    return (matrix[x][y] == CHARGING_STATION) ? 1 : 0;
}

/**************************************************************************/
tuple<int, int> selectFirstPlace(const int maze[ROW_COUNT][COL_COUNT], const int startRow, const int startCol,
                                 const int endRow, const int endCol) {
    int x, y;
    // Keep generating random indices until a free space is found
    do {
        x = rand() % (endRow - startRow) + startRow;
        y = rand() % (endCol - startCol) + startCol;
    } while (maze[x][y] != FREE_SPACE);
    return make_tuple(x, y);
}

/**************************************************************************/
int selectAction(const double qTable[ROW_COUNT][COL_COUNT][ACTION_COUNT], const int x, const int y,
                 const double epsilon) {
    int action;
    double randomValue = static_cast<double>(rand()) / RAND_MAX;

    if (randomValue < epsilon) {
        // Exploration: Choose a random action uniformly (between 0 and 7)
        action = rand() % ACTION_COUNT;
    } else {
        // Exploitation: Choose the action with the highest Q-value for the current state
        action = 0;
        double maxQValue = qTable[x][y][0];
        for (int i = 0; i < ACTION_COUNT; i++) {
            if (qTable[x][y][i] > maxQValue) {
                maxQValue = qTable[x][y][i];
                action = i;
            }
        }
    }
    return action;
}

// Function to perform the action and compute the reward
tuple<int, int, int, double> performAction(const int maze[ROW_COUNT][COL_COUNT], const int x1, const int y1,
                                           const int action) {
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
            if (x1 > 0 && y1 < COL_COUNT - 1 && (
                    maze[x1 - 1][y1 + 1] == FREE_SPACE || maze[x1 - 1][y1 + 1] == CHARGING_STATION)) {
                x2 = x1 - 1;
                y2 = y1 + 1;
                changePos = 1;
            }
            break;
        case 2: // Move E
            if (y1 < COL_COUNT - 1 && (maze[x1][y1 + 1] == FREE_SPACE || maze[x1][y1 + 1] == CHARGING_STATION)) {
                y2 = y1 + 1;
                changePos = 1;
            }
            break;
        case 3: // Move SE
            if (x1 < ROW_COUNT - 1 && y1 < COL_COUNT - 1 && (
                    maze[x1 + 1][y1 + 1] == FREE_SPACE || maze[x1 + 1][y1 + 1] == CHARGING_STATION)) {
                x2 = x1 + 1;
                y2 = y1 + 1;
                changePos = 1;
            }
            break;
        case 4: // Move S
            if (x1 < ROW_COUNT - 1 && (maze[x1 + 1][y1] == FREE_SPACE || maze[x1 + 1][y1] == CHARGING_STATION)) {
                x2 = x1 + 1;
                changePos = 1;
            }
            break;
        case 5: // Move SW
            if (x1 < ROW_COUNT - 1 && y1 > 0 && (
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
void detectPath(const int maze[ROW_COUNT][COL_COUNT], int path[ROW_COUNT][COL_COUNT], const int x, const int y) {
    if (maze[x][y] == FREE_SPACE) path[x][y] = FREE_SPACE;
    if (maze[x][y] == CHARGING_STATION) path[x][y] = CHARGING_STATION;
}

/**************************************************************************/
void updateQTable(double qTable[ROW_COUNT][COL_COUNT][ACTION_COUNT], const int x1, const int y1, const int action,
                  const double reward, const int x2, const int y2) {
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
int selectMaxQ(const double qTable[ROW_COUNT][COL_COUNT][ACTION_COUNT], const int x, const int y,
               int checkMatrix[ROW_COUNT][COL_COUNT]) {
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
                if (x > 0 && y < COL_COUNT - 1) {
                    nextX = x - 1;
                    nextY = y + 1;
                }
                break;
            case 2: // Move E
                if (y < COL_COUNT - 1) nextY = y + 1;
                break;
            case 3: // Move SE
                if (x < ROW_COUNT - 1 && y < COL_COUNT - 1) {
                    nextX = x + 1;
                    nextY = y + 1;
                }
                break;
            case 4: // Move S
                if (x < ROW_COUNT - 1) nextX = x + 1;
                break;
            case 5: // Move SW
                if (x < ROW_COUNT - 1 && y > 0) {
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
        if (nextX >= 0 && nextX < ROW_COUNT && nextY >= 0 && nextY < COL_COUNT && checkMatrix[nextX][nextY] == 0) {
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
void selectPath(const double qTable[ROW_COUNT][COL_COUNT][ACTION_COUNT], const int xStart, const int yStart,
                const int maze[ROW_COUNT][COL_COUNT]) {
    int step = 1;
    int checkMatrix[ROW_COUNT][COL_COUNT] = {0}; // Keeps track of visited positions
    int x = xStart, y = yStart; // Current position
    int finalPath[ROW_COUNT][COL_COUNT] = {0}; // Stores the steps taken in the path

    while (maze[x][y] != CHARGING_STATION) {
        // Continue until a charging station (value 2) is reached
        cout << "\n\tThe Current Position IS: " << '(' << x << ", " << y << ") ==> " << maze[x][y];

        // Select the best action using the Q-table
        int selectedAction = selectMaxQ(qTable, x, y, checkMatrix);
        cout << "\n\tThe Selected Action IS: " << selectedAction;

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
        cout << "\n\tThe New Position IS: " << '(' << x << ", " << y << ") ==> " << maze[x][y] << "\n";
    }

    // Print the final path
    printMatrixInt(finalPath, "FinalPath");
}

//*************************************************************************/
void trainAgent(const int maze[ROW_COUNT][COL_COUNT], double qTable[ROW_COUNT][COL_COUNT][ACTION_COUNT],
                const int startRow, const int startCol, const int endRow, const int endCol, double epsilon) {
    // Global-sized path array for updates
    int path[ROW_COUNT][COL_COUNT] = {0};
    int arrival = 0;
    int x1, y1, x2, y2;
    double actionReward = 0;
    int iteration = 0;

    // Start the training process (generate episodes)
    for (int counter = 1; counter <= EPISODE_COUNT; counter++) {
        // Select a random starting place within the sub-environment
        tie(x1, y1) = selectFirstPlace(maze, startRow, startCol, endRow, endCol);
        iteration = 1;

        // Start generating one episode
        while (arrival == 0 && iteration < MAX_STEPS_PER_EPISODE) {
            // Select an action using an epsilon-greedy policy
            int act = selectAction(qTable, x1, y1, epsilon);

            // Perform the selected action and get the associated reward
            tie(x2, y2, act, actionReward) = performAction(maze, x1, y1, act);
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
void testAgent(const int maze[ROW_COUNT][COL_COUNT], const double qTable[ROW_COUNT][COL_COUNT][ACTION_COUNT],
               const int nrTestEpisodes) {
    // Test the agent by selecting a random starting position and finding the path to the charging station
    for (int test = 0; test < nrTestEpisodes; test++) {
        int x1, y1;
        tie(x1, y1) = selectFirstPlace(maze, 0, 0, ROW_COUNT, COL_COUNT);
        selectPath(qTable, x1, y1, maze);
    }
}

//*************************************************************************/
void splitMaze(MazeNode *node, const int maze[ROW_COUNT][COL_COUNT], const int startRow, const int startCol,
               const int endRow, const int endCol) {
    if ((endRow - startRow) <= 3 && (endCol - startCol) <= 3) {
        return; // Stop splitting if subenvironment is smaller than 3x3
    }

    const int midRow = (startRow + endRow) / 2;
    const int midCol = (startCol + endCol) / 2;

    // Create potential child nodes
    MazeNode *child1 = new MazeNode(maze, startRow, startCol, midRow, midCol, node); // Top-left
    MazeNode *child2 = new MazeNode(maze, startRow, midCol, midRow, endCol, node); // Top-right
    MazeNode *child3 = new MazeNode(maze, midRow, startCol, endRow, midCol, node); // Bottom-left
    MazeNode *child4 = new MazeNode(maze, midRow, midCol, endRow, endCol, node); // Bottom-right

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
        splitMaze(child1, maze, startRow, startCol, midRow, midCol);
        splitMaze(child2, maze, startRow, midCol, midRow, endCol);
        splitMaze(child3, maze, midRow, startCol, endRow, midCol);
        splitMaze(child4, maze, midRow, midCol, endRow, endCol);
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
                << "Size(" << (node->endRow - node->startRow) << "x" << (node->endCol - node->startCol) << "), "
                << "Charging Stations: " << node->chargingStationCount << "\n";
    } else {
        // For all other nodes, add the appropriate symbols
        const string currentPrefix = prefix + (isLast ? "└─ " : "├─ ");
        cout << currentPrefix
                << "Node: Start(" << node->startRow << ", " << node->startCol << "), "
                << "End(" << node->endRow << ", " << node->endCol << "), "
                << "Size(" << (node->endRow - node->startRow) << "x" << (node->endCol - node->startCol) << "), "
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
MazeNode *createSubEnvironments(const int maze[ROW_COUNT][COL_COUNT]) {
    // Create the root node for the entire maze
    MazeNode *root = new MazeNode(maze, 0, 0, ROW_COUNT, COL_COUNT);

    // Split the maze into sub-environments
    splitMaze(root, maze, 0, 0, ROW_COUNT, COL_COUNT);

    // Print the tree structure
    cout << "\n\nMaze Sub-environment Tree:\n\n";
    printTree(root);

    // Return the root node to the caller
    return root;
}

//*************************************************************************/
void propagateQTableDownwards(MazeNode *node, const double rootQTable[ROW_COUNT][COL_COUNT][ACTION_COUNT]) {
    if (!node) return;

    // Propagate the Q-table from the root to the current node
    for (int i = node->startRow; i < node->endRow; ++i) {
        for (int j = node->startCol; j < node->endCol; ++j) {
            for (int action = 0; action < ACTION_COUNT; ++action) {
                node->qTable[i][j][action] = rootQTable[i][j][action];
            }
        }
    }

    // Recursively propagate to all children
    for (MazeNode *child: node->children) {
        propagateQTableDownwards(child, rootQTable);
    }
}

//*************************************************************************/
void propagateQTableUpwards(const MazeNode *node) {
    if (!node || !node->parent) return;

    MazeNode *parent = node->parent;

    // Update parent's Q-table using the child node's Q-table
    for (int i = node->startRow; i < node->endRow; ++i) {
        for (int j = node->startCol; j < node->endCol; ++j) {
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
        for (int i = child->startRow; i < child->endRow; ++i) {
            for (int j = child->startCol; j < child->endCol; ++j) {
                child->maze[i][j] = node->maze[i][j];
            }
        }

        // Recursively propagate updates to lower levels
        propagateMazeDownwards(child);
    }
}

//*************************************************************************/
void performPathPlanningAndPropagateDownwards(MazeNode *root) {
    if (!root) {
        cerr << "Error: Root node is null.\n";
        return;
    }

    // Perform Q-learning on the entire environment (root node)
    double rootQTable[ROW_COUNT][COL_COUNT][ACTION_COUNT] = {0};
    double epsilon = 1.0;
    trainAgent(root->maze, rootQTable, root->startRow, root->startCol, root->endRow, root->endCol, epsilon);

    // Propagate the Q-table results from the root to all sub-environments
    propagateQTableDownwards(root, rootQTable);

    cout << "\nPath planning performed on the root environment and propagated to all sub-environments.\n";
}

//*************************************************************************/
vector<pair<int, int> > getObstaclePositions(const int maze[ROW_COUNT][COL_COUNT]) {
    vector<pair<int, int> > obstaclePositions;

    for (int i = 0; i < ROW_COUNT; ++i) {
        for (int j = 0; j < COL_COUNT; ++j) {
            if (maze[i][j] == OBSTACLE) {
                // Check if the cell is an obstacle
                obstaclePositions.emplace_back(i, j);
            }
        }
    }

    return obstaclePositions;
}

//*************************************************************************/
void masMat(MazeNode *root, const vector<pair<int, int> > &changedPositions, double epsilon) {
    // If no changes, apply global Q-learning
    if (changedPositions.empty()) {
        cout << "Environment is static. Applying global Q-learning.\n";
        double rootQTable[ROW_COUNT][COL_COUNT][ACTION_COUNT] = {0};
        trainAgent(root->maze, rootQTable, root->startRow, root->startCol, root->endRow, root->endCol, epsilon);
        propagateQTableDownwards(root, rootQTable);
        return;
    }

    cout << "Environment changed. Applying local Q-learning.\n";

    unordered_set<MazeNode *> affectedNodes;

    // Find all affected nodes
    for (const auto &pos: changedPositions) {
        MazeNode *node = root->findSubEnvironment(pos.first, pos.second);
        if (node) affectedNodes.insert(node);
    }

    // Train each affected node and propagate the results upwards
    for (const MazeNode *node: affectedNodes) {
        double subQTable[ROW_COUNT][COL_COUNT][ACTION_COUNT] = {0};
        trainAgent(node->maze, subQTable, node->startRow, node->startCol, node->endRow, node->endCol, epsilon);
        propagateQTableUpwards(node);
    }
}

//*************************************************************************/
void simulateEnvironmentChanges(MazeNode *root, const int numSteps, vector<pair<int, int> > &changedPositions) {
    if (!root) {
        cerr << "Error: Root node is null.\n";
        return;
    }

    int (*maze)[COL_COUNT] = root->maze;
    vector<pair<int, int> > obstaclePositions = getObstaclePositions(maze);

    changedPositions.clear(); // Ensure it starts empty

    for (int step = 0; step < numSteps; ++step) {
        cout << "Step " << step + 1 << ":\n";

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
                if (newRow >= 0 && newRow < ROW_COUNT &&
                    newCol >= 0 && newCol < COL_COUNT &&
                    maze[newRow][newCol] == FREE_SPACE) {
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
                maze[oldRow][oldCol] = FREE_SPACE;
                maze[newRow][newCol] = OBSTACLE;
                obstaclePositions[randomIndex] = {newRow, newCol};

                cout << "Moved obstacle from (" << oldRow << ", " << oldCol
                        << ") to (" << newRow << ", " << newCol << ")\n";
            }
        }
        printMatrixInt(maze, "Maze after step:");
    }

    propagateMazeDownwards(root);
}

//*************************************************************************/
void testMasMat(MazeNode *root, int maze[ROW_COUNT][COL_COUNT], const int numSteps, const int testEpisodes) {
    // Perform path planning and propagate the results
    performPathPlanningAndPropagateDownwards(root);

    // Simulate environment changes and test MasMat
    for (int step = 0; step < numSteps; ++step) {
        cout << "Step " << step + 1 << ":\n";

        // Track changed positions during environment changes
        vector<pair<int, int> > changedPositions;
        simulateEnvironmentChanges(root, 1, changedPositions);

        // Apply the MasMat algorithm
        masMat(root, changedPositions, 1);

        // Optionally print the updated maze for debugging
        printMatrixInt(maze, "Maze after step:");
    }

    // Test the agent's performance after applying MasMat
    cout << "Testing the agent's performance:\n";
    testAgent(maze, root->qTable, testEpisodes);
}

//*************************************************************************/
void collectLeafNodes(MazeNode *node, vector<MazeNode *> &leafNodes) {
    if (!node) return;
    if (node->children.empty()) { // Leaf node
        leafNodes.push_back(node);
    } else {
        for (MazeNode *child : node->children) {
            collectLeafNodes(child, leafNodes);
        }
    }
}

//*************************************************************************/
void trainLeafNodesInParallel(const vector<MazeNode *> &leafNodes, double &epsilon) {
    // Use threads to train agents concurrently
    vector<thread> threads;

    for (MazeNode *leaf : leafNodes) {
        threads.emplace_back([leaf, epsilon]() {
            double qTable[ROW_COUNT][COL_COUNT][ACTION_COUNT] = {0};
            trainAgent(leaf->maze, qTable, leaf->startRow, leaf->startCol, leaf->endRow, leaf->endCol, epsilon);
            propagateQTableUpwards(leaf); // Propagate results upwards
        });
    }

    // Join threads to ensure all training is complete
    for (thread &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

//*************************************************************************/
void applyLocalPathPlanning(MazeNode *root, double epsilon) {
    // Collect all leaf nodes
    vector<MazeNode *> leafNodes;
    collectLeafNodes(root, leafNodes);

    // Train agents in parallel for all leaf nodes
    trainLeafNodesInParallel(leafNodes, epsilon);
}

//*************************************************************************/
void testLocalPathPlanning(MazeNode *root, int maze[ROW_COUNT][COL_COUNT], const int numSteps, const int testEpisodes) {
    cout << "Testing the second approach: Local path planning in leaf nodes\n";

    // Initial local path planning and propagation upwards
    double epsilon = 1.0; // Exploration parameter for Q-learning
    applyLocalPathPlanning(root, epsilon);
    cout << "Initial local path planning performed and propagated upwards.\n";

    // Simulate environment changes and reapply local path planning
    for (int step = 0; step < numSteps; ++step) {
        cout << "Step " << step + 1 << ":\n";

        // Track changed positions during environment changes
        vector<pair<int, int>> changedPositions;
        simulateEnvironmentChanges(root, 1, changedPositions);

        // Collect leaf nodes affected by the changes
        vector<MazeNode *> leafNodes;
        for (const auto &pos : changedPositions) {
            MazeNode *affectedNode = root->findSubEnvironment(pos.first, pos.second);
            if (affectedNode && affectedNode->children.empty()) {
                leafNodes.push_back(affectedNode);
            }
        }

        // Perform local path planning for affected leaf nodes in parallel
        if (!leafNodes.empty()) {
            trainLeafNodesInParallel(leafNodes, epsilon);
        } else {
            cout << "No affected leaf nodes detected for local path planning.\n";
        }

        // Optionally print the updated maze for debugging
        printMatrixInt(maze, "Maze after step:");
    }

    // Test the agent's performance after applying the second approach
    cout << "Testing the agent's performance:\n";
    testAgent(maze, root->qTable, testEpisodes);
}

//*************************************************************************/
int main() {
    // Create the maze
    int maze[ROW_COUNT][COL_COUNT];
    createMaze(maze, 0.7, 0.28, 0.02);

    // Create the root node and sub-environments
    MazeNode *root = createSubEnvironments(maze);

    // Test the second approach: Local path planning in leaf nodes
    const int numSteps = 20;    // Number of simulation steps
    const int testEpisodes = 10; // Number of test episodes
    testLocalPathPlanning(root, maze, numSteps, testEpisodes);

    // Cleanup
    delete root;
    return 0;
}
