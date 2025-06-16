#include "maze.h"

Maze::Maze(const int rows, const int cols, const double freeSpaceProb, const double obstacleProb,
           const double chargingStationProb) : vector<vector<int> >(rows, vector<int>(cols)) {
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
                (*this)[i][j] = constants::FREE_SPACE; // Free space
            } else if (randomValue < freeSpaceProb + obstacleProb) {
                (*this)[i][j] = constants::OBSTACLE; // Obstacle
            } else {
                (*this)[i][j] = constants::CHARGING_STATION; // Charging station
                hasChargingStation = true;
            }
        }
    }

    // Ensure there is at least one charging station
    if (!hasChargingStation) {
        const int randomRow = rand() % rows;
        const int randomCol = rand() % cols;
        (*this)[randomRow][randomCol] = constants::CHARGING_STATION; // Place a charging station
    }
}

int Maze::operator()(const int row, const int col) const {
    // Check bounds
    if (row < 0 || row >= getRows() || col < 0 || col >= getCols()) {
        cerr << "Error: Index out of bounds." << endl;
        exit(1);
    }
    // Check if the position is valid
    if ((*this)[row][col] != constants::FREE_SPACE && (*this)[row][col] != constants::OBSTACLE && (*this)[row][col] !=
        constants::CHARGING_STATION) {
        cerr << "Error: Invalid cell type at (" << row << ", " << col << ")." << endl;
        exit(1);
    }
    return (*this)[row][col];
}

void Maze::operator()(const int row, const int col, const int value) {
    // Check bounds
    if (row < 0 || row >= size() || col < 0 || col >= (*this)[row].size()) {
        cerr << "Error: Index out of bounds." << endl;
        exit(1);
    }
    // Check if the value is valid
    if (value != constants::FREE_SPACE && value != constants::OBSTACLE && value != constants::CHARGING_STATION) {
        cerr << "Error: Invalid cell type value." << endl;
        exit(1);
    }
    // Set the value at the specified position
    (*this)[row][col] = value;
}

void Maze::printMaze() const {
    for (const auto &row: *this) {
        for (const auto &cell: row) {
            switch (cell) {
                case constants::FREE_SPACE:
                    cout << ".";
                    break;
                case constants::OBSTACLE:
                    cout << "#";
                    break;
                case constants::CHARGING_STATION:
                    cout << "C";
                    break;
                default:
                    cout << "?"; // Unknown cell type
            }
        }
        cout << endl;
    }
}

int Maze::getRows() const {
    return this->size();
}

int Maze::getCols() const {
    return this->empty() ? 0 : (*this)[0].size();
}

bool Maze::checkExit(const int x, const int y) const {
    return (*this)[x][y] == constants::CHARGING_STATION;
}

pair<int, int> Maze::selectFirstPlace(const int startRow, const int startCol, const int endRow,
                                      const int endCol) const {
    int x, y;
    // Keep generating random indices until a free space is found
    do {
        x = rand() % (endRow - startRow) + startRow;
        y = rand() % (endCol - startCol) + startCol;
    } while ((*this)[x][y] != constants::FREE_SPACE);
    return make_pair(x, y);
}

pair<int, int> Maze::selectFirstPlace(const int startRow, const int startCol, const int endRow, const int endCol,
                                      const int counter,
                                      const unordered_map<pair<int, int>, StartStats, HashPair> &startStats,
                                      mt19937 &rng) const {
    constexpr int initialRandomEpisodes = 10;
    if (counter < initialRandomEpisodes || startStats.empty()) {
        int r, c;
        do {
            r = startRow + rand() % (endRow - startRow + 1);
            c = startCol + rand() % (endCol - startCol + 1);
        } while ((*this)[r][c] == constants::OBSTACLE);
        return {r, c};
    }

    // Build weights: lower success rate = higher weight
    vector<pair<int, int> > positions;
    vector<double> weights;
    constexpr double epsilon = 0.1; // Ensure non-zero probability
    for (int x = startRow; x <= endRow; ++x) {
        for (int y = startCol; y <= endCol; ++y) {
            if ((*this)[x][y] == constants::OBSTACLE) continue;
            positions.emplace_back(x, y);
            auto it = startStats.find({x, y});
            const double successRate = it != startStats.end() ? it->second.getSuccessRate() : 0.0;
            weights.push_back(1.0 - successRate + epsilon);
        }
    }

    // Weighted random selection
    discrete_distribution<int> dist(weights.begin(), weights.end());
    const auto idx = dist(rng);
    return positions[idx];
}

tuple<int, int, int, double> Maze::performAction(const int rows, const int cols, const int x1, const int y1,
                                                 const int action) const {
    double reward = 0.0;
    int changePos = 0;
    int x2 = x1, y2 = y1;

    // Perform the selected action
    switch (action) {
        case 0: // Move N
            if (x1 > 0 && ((*this)[x1 - 1][y1] == constants::FREE_SPACE || (*this)[x1 - 1][y1] ==
                           constants::CHARGING_STATION)) {
                x2 = x1 - 1;
                changePos = 1;
            }
            break;
        case 1: // Move NE
            if (x1 > 0 && y1 < cols - 1 && ((*this)[x1 - 1][y1 + 1] == constants::FREE_SPACE || (*this)[x1 - 1][y1 + 1]
                                            == constants::CHARGING_STATION)) {
                x2 = x1 - 1;
                y2 = y1 + 1;
                changePos = 1;
            }
            break;
        case 2: // Move E
            if (y1 < cols - 1 && ((*this)[x1][y1 + 1] == constants::FREE_SPACE || (*this)[x1][y1 + 1] ==
                                  constants::CHARGING_STATION)) {
                y2 = y1 + 1;
                changePos = 1;
            }
            break;
        case 3: // Move SE
            if (x1 < rows - 1 && y1 < cols - 1 && (
                    (*this)[x1 + 1][y1 + 1] == constants::FREE_SPACE || (*this)[x1 + 1][y1 + 1] ==
                    constants::CHARGING_STATION)) {
                x2 = x1 + 1;
                y2 = y1 + 1;
                changePos = 1;
            }
            break;
        case 4: // Move S
            if (x1 < rows - 1 && ((*this)[x1 + 1][y1] == constants::FREE_SPACE || (*this)[x1 + 1][y1] ==
                                  constants::CHARGING_STATION)) {
                x2 = x1 + 1;
                changePos = 1;
            }
            break;
        case 5: // Move SW
            if (x1 < rows - 1 && y1 > 0 && ((*this)[x1 + 1][y1 - 1] == constants::FREE_SPACE || (*this)[x1 + 1][y1 - 1]
                                            == constants::CHARGING_STATION)) {
                x2 = x1 + 1;
                y2 = y1 - 1;
                changePos = 1;
            }
            break;
        case 6: // Move W
            if (y1 > 0 && ((*this)[x1][y1 - 1] == constants::FREE_SPACE || (*this)[x1][y1 - 1] ==
                           constants::CHARGING_STATION)) {
                y2 = y1 - 1;
                changePos = 1;
            }
            break;
        case 7: // Move NW
            if (x1 > 0 && y1 > 0 && ((*this)[x1 - 1][y1 - 1] == constants::FREE_SPACE || (*this)[x1 - 1][y1 - 1] ==
                                     constants::CHARGING_STATION)) {
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
    if ((*this)[x2][y2] == constants::CHARGING_STATION) {
        reward = 100.0; // Large reward for reaching the goal
    } else if (changePos == 0) {
        reward = -10.0; // Stronger penalty for hitting obstacles
    } else {
        reward = -1.0; // Increased step penalty to encourage shorter paths
    }

    return make_tuple(x2, y2, action, reward);
}

vector<pair<int, int> > Maze::getObstaclePositions() const {
    vector<pair<int, int> > obstaclePositions;
    for (int i = 0; i < this->size(); ++i) {
        for (int j = 0; j < (*this)[i].size(); ++j) {
            if ((*this)[i][j] == constants::OBSTACLE) {
                obstaclePositions.emplace_back(i, j);
            }
        }
    }
    return obstaclePositions;
}
