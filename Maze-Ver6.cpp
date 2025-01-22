/****************************************************************************************************/
/*			This Program has been written by Hossein Yarahmadi					                    */
/*          The Path Matrix includes three types of entities                                        */
/*          0: means the obstacle  1:means the feasible moving  2:means the charge station          */
/****************************************************************************************************/

#include <iostream>
#include <iomanip>
#include <cmath>
#include <ctime>

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

#define EPISODE_COUNT 1000000
#define LEARNING_RATE 0.2
#define DISCOUNT_FACTOR 0.9

/**************************************************************************/
using namespace std;

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
                maze[i][j] = 1; // Free space
            } else if (randomValue < freeSpaceProb + obstacleProb) {
                maze[i][j] = 0; // Obstacle
            } else {
                maze[i][j] = 2; // Charging station
                hasChargingStation = true;
            }
        }
    }

    // Ensure there is at least one charging station
    if (!hasChargingStation) {
        const int randomRow = rand() % ROW_COUNT;
        const int randomCol = rand() % COL_COUNT;
        maze[randomRow][randomCol] = 2; // Place a charging station
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
    return (matrix[x][y] == 2) ? 1 : 0;
}

/**************************************************************************/
tuple<int, int> selectFirstPlace(const int maze[ROW_COUNT][COL_COUNT]) {
    int x, y;
    // Keep generating random indices until a free space is found
    do {
        x = rand() % ROW_COUNT; // Generate a random row index within range
        y = rand() % COL_COUNT; // Generate a random column index within range
    } while (maze[x][y] != 1); // Assuming 1 represents a free space
    return make_tuple(x, y);
}

/**************************************************************************/
int selectAction(const double qTable[ROW_COUNT][COL_COUNT][ACTION_COUNT], const int x, const int y, const double epsilon) {
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
tuple<int, int, int, double> performAction(const int maze[ROW_COUNT][COL_COUNT], const int x1, const int y1, const int action) {
    double reward = 0.0;
    int changePos = 0;
    int x2 = x1, y2 = y1;

    // Perform the selected action
    switch (action) {
        case 0: // Move N
            if (x1 > 0 && (maze[x1 - 1][y1] == 1 || maze[x1 - 1][y1] == 2)) {
                x2 = x1 - 1;
                changePos = 1;
            }
            break;
        case 1: // Move NE
            if (x1 > 0 && y1 < COL_COUNT - 1 && (maze[x1 - 1][y1 + 1] == 1 || maze[x1 - 1][y1 + 1] == 2)) {
                x2 = x1 - 1;
                y2 = y1 + 1;
                changePos = 1;
            }
            break;
        case 2: // Move E
            if (y1 < COL_COUNT - 1 && (maze[x1][y1 + 1] == 1 || maze[x1][y1 + 1] == 2)) {
                y2 = y1 + 1;
                changePos = 1;
            }
            break;
        case 3: // Move SE
            if (x1 < ROW_COUNT - 1 && y1 < COL_COUNT - 1 && (maze[x1 + 1][y1 + 1] == 1 || maze[x1 + 1][y1 + 1] == 2)) {
                x2 = x1 + 1;
                y2 = y1 + 1;
                changePos = 1;
            }
            break;
        case 4: // Move S
            if (x1 < ROW_COUNT - 1 && (maze[x1 + 1][y1] == 1 || maze[x1 + 1][y1] == 2)) {
                x2 = x1 + 1;
                changePos = 1;
            }
            break;
        case 5: // Move SW
            if (x1 < ROW_COUNT - 1 && y1 > 0 && (maze[x1 + 1][y1 - 1] == 1 || maze[x1 + 1][y1 - 1] == 2)) {
                x2 = x1 + 1;
                y2 = y1 - 1;
                changePos = 1;
            }
            break;
        case 6: // Move W
            if (y1 > 0 && (maze[x1][y1 - 1] == 1 || maze[x1][y1 - 1] == 2)) {
                y2 = y1 - 1;
                changePos = 1;
            }
            break;
        case 7: // Move NW
            if (x1 > 0 && y1 > 0 && (maze[x1 - 1][y1 - 1] == 1 || maze[x1 - 1][y1 - 1] == 2)) {
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
    if (maze[x2][y2] == 2) {
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
    if (maze[x][y] == 1) path[x][y] = 1;
    if (maze[x][y] == 2) path[x][y] = 2;
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
    qTable[x1][y1][action] = qTable[x1][y1][action] + LEARNING_RATE * (reward + DISCOUNT_FACTOR * maxQNext - qTable[x1][y1][action]);
}

//*************************************************************************/
int selectMaxQ(const double qTable[ROW_COUNT][COL_COUNT][ACTION_COUNT], const int x, const int y, int checkMatrix[ROW_COUNT][COL_COUNT]) {
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
void selectPath(const double qTable[ROW_COUNT][COL_COUNT][ACTION_COUNT], const int xStart, const int yStart, const int maze[ROW_COUNT][COL_COUNT]) {
    int step = 1;
    int checkMatrix[ROW_COUNT][COL_COUNT] = {0}; // Keeps track of visited positions
    int x = xStart, y = yStart; // Current position
    int finalPath[ROW_COUNT][COL_COUNT] = {0}; // Stores the steps taken in the path

    while (maze[x][y] != 2) {
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
void trainAgent(const int maze[ROW_COUNT][COL_COUNT], double qTable[ROW_COUNT][COL_COUNT][ACTION_COUNT], double& epsilon) {
    int path[ROW_COUNT][COL_COUNT] = {0};
    int arrival = 0;
    int x1, y1, x2, y2;
    double actionReward = 0;
    int iteration = 0;

    // Start the training process (generate episodes)
    for (int counter = 1; counter <= EPISODE_COUNT; counter++) {
        tie(x1, y1) = selectFirstPlace(maze);
        // cout << "\n\tThe X is: " << x1 << "\n\tThe Y is: " << y1 << "\n\tThe Item is: " << maze[x1][y1] << "\n";
        iteration = 1;

        // Start generating one episode
        while ((arrival == 0) && (iteration < 5000)) {

            // Select an action using an epsilon-greedy policy
            int act = selectAction(qTable, x1, y1, epsilon);

            // Perform the selected action and get the associated reward
            tie(x2, y2, act, actionReward) = performAction(maze, x1, y1, act);
            detectPath(maze, path, x2, y2);
            // printMatrixInt(maze, "Maze");
            // printMatrixInt(path, "Path");

            // Update the Q-table
            updateQTable(qTable, x1, y1, act, actionReward, x2, y2);

            arrival = checkExit(maze, x2, y2);
            x1 = x2;
            y1 = y2;
            iteration++;
        }

        // cout << "\n\n\t\t ---------The Counter Is: " << counter << "\n";
        epsilon = max(0.0, epsilon - (1.0 / EPISODE_COUNT));
        arrival = 0;
    }
}

//*************************************************************************/
void testAgent(const int maze[ROW_COUNT][COL_COUNT], const double qTable[ROW_COUNT][COL_COUNT][ACTION_COUNT], const int nrTestEpisodes) {
    // Test the agent by selecting a random starting position and finding the path to the charging station
    for (int test = 0; test < nrTestEpisodes; test++) {
        int x1, y1;
        tie(x1, y1) = selectFirstPlace(maze);
        selectPath(qTable, x1, y1, maze);
    }
}

//*************************************************************************/
int main() {
    srand(time(NULL));
    double epsilon = 1.0;

    int maze[ROW_COUNT][COL_COUNT] = {0};
    double qTable[ROW_COUNT][COL_COUNT][ACTION_COUNT] = {0};

    constexpr double freeSpaceProb = 0.7;
    constexpr double obstacleProb = 0.28;
    constexpr double chargingStationProb = 0.02;

    createMaze(maze, freeSpaceProb, obstacleProb, chargingStationProb);
    printMatrixInt(maze, "Maze");

    trainAgent(maze, qTable, epsilon);
    testAgent(maze, qTable, 10);

    return 0;
}