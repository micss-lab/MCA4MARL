#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <future>
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

#include "constants.h"
#include "hashpair.h"
#include "maze.h"
#include "policyvisualizer.h"
#include "startstats.h"
#include "table.h"
#include "treenode.h"


using namespace std;

/*************************************************************************/
struct Experience {
    int x1, y1, action;
    double reward;
    int x2, y2;
};

/*************************************************************************/
void trainAgentWithStoppingCriterion(TreeNode *node, const Maze &maze, const int rows, const int cols,
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
    while (!converged && counter < constants::EPISODE_COUNT) {
        auto [x1, y1] = maze.selectFirstPlace(startRow, startCol, endRow, endCol, counter, startStats, rng);
        iteration = 1;

        // Reset episode
        while (arrival == 0 && iteration < maxStepsPerEpisode) {
            // Select action using epsilon-greedy policy and perform it
            int act = node->selectAction(x1, y1, epsilon);
            tie(x2, y2, act, actionReward) = maze.performAction(rows, cols, x1, y1, act);

            // Store experience in replay buffer and update Q-table
            replayBuffer.push_back({x1, y1, act, actionReward, x2, y2});
            if (replayBuffer.size() > bufferSize) replayBuffer.erase(replayBuffer.begin());
            node->updateQTable(x1, y1, act, actionReward, x2, y2);

            // Perform experience replay
            if (replayBuffer.size() >= batchSize && counter > minEpisodes) {
                for (int i = 0; i < batchSize; i++) {
                    const int idx = rand() % replayBuffer.size();
                    const auto &[x1, y1, action, reward, x2, y2] = replayBuffer[idx];
                    node->updateQTable(x1, y1, action, reward, x2, y2);
                }
            }
            arrival = maze.checkExit(x2, y2);
            x1 = x2;
            y1 = y2;
            iteration++;
        }

        arrival = 0;
        epsilon = max(0.01, epsilon * decayRate);

        // Check for convergence every 50 episodes
        if (counter % 50 == 0 && counter >= minEpisodes) {
            Table<double> &qTable = *node->qTable;
            double maxChange = 0.0;
            for (int row = node->startRow; row <= node->endRow; row++) {
                for (int col = node->startCol; col <= node->endCol; col++) {
                    // Get Q-values for the current position
                    vector<double> &qValues = qTable(row, col, startRow, startCol);
                    vector<double> &prevQValues = prevQTable(row, col, startRow, startCol);

                    // Calculate maximum change compared to previous Q-table
                    for (int a = 0; a < constants::ACTION_COUNT; a++) {
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
    for (int i = 0; i < constants::ACTION_COUNT; ++i) {
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
tuple<bool, int, vector<pair<int, int> > > findValidPath(const TreeNode *root, const int startX, const int startY,
                                                         const int maxSteps) {
    // Extract maze and dimensions from the root node
    const Maze &maze = *root->maze;
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
        if (maze(x, y) == constants::CHARGING_STATION) {
            return {true, steps, path};
        }

        // Select top k actions based on Q-values
        const vector<double> &qValues = root->getQValues(x, y, root->startRow, root->startCol);
        vector<int> actions = selectTopKActions(qValues, rows, cols, x, y, 2);
        for (const int act: actions) {
            int newX = x + moves[act].first;
            int newY = y + moves[act].second;
            if (maze(newX, newY) != constants::OBSTACLE && !visited.contains({newX, newY})) {
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
tuple<double, double, double> testAgent(const TreeNode *root) {
    // Extract maze and dimensions from the root node
    const Maze &maze = *root->maze;
    const int rows = root->rows;
    const int cols = root->cols;
    const int maxStepsPerEpisode = rows + cols;

    // Collect all valid positions to test
    vector<pair<int, int> > positions;
    int totalPositions = 0;
    for (int x1 = 0; x1 < rows; ++x1) {
        for (int y1 = 0; y1 < cols; ++y1) {
            if (maze(x1, y1) != constants::OBSTACLE) {
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
void splitMaze(TreeNode *node, const Maze &fullMaze, const int rows, const int cols, const int startRow,
               const int startCol, const int endRow, const int endCol) {
    if ((endRow - startRow + 1) <= 20 && (endCol - startCol + 1) <= 20) {
        return;
    }

    // Split the maze into four quadrants
    const int midRow = (startRow + endRow) / 2;
    const int midCol = (startCol + endCol) / 2;

    // Create child nodes for each quadrant
    auto *child1 = new TreeNode(fullMaze, rows, cols, startRow, startCol, midRow, midCol, node);
    auto *child2 = new TreeNode(fullMaze, rows, cols, startRow, midCol + 1, midRow, endCol, node);
    auto *child3 = new TreeNode(fullMaze, rows, cols, midRow + 1, startCol, endRow, midCol, node);
    auto *child4 = new TreeNode(fullMaze, rows, cols, midRow + 1, midCol + 1, endRow, endCol, node);

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
TreeNode *createSubEnvironments(const Maze &maze, const int rows, const int cols) {
    auto *root = new TreeNode(maze, rows, cols, 0, 0, rows - 1, cols - 1, nullptr, true);
    splitMaze(root, maze, rows, cols, 0, 0, rows - 1, cols - 1);
    return root;
}

/*************************************************************************/
void propagateQTableDownwards(TreeNode *node) {
    if (!node || !node->qTable) return; // Skip if no node or no qTable

    // Propagate to all descendants, updating only those with qTables
    stack<TreeNode *> toVisit;
    toVisit.push(node);

    // DFS to propagate Q-tables
    while (!toVisit.empty()) {
        const TreeNode *current = toVisit.top();
        toVisit.pop();

        for (TreeNode *child: current->children) {
            // If child has no qTable, initialize it
            if (!child->qTable) {
                child->initQTable();
            }
            // Copy Q-values for positions within child's subenvironment
            for (int row = child->startRow; row <= child->endRow; ++row) {
                for (int col = child->startCol; col <= child->endCol; ++col) {
                    vector<double> &childQValues = child->getQValues(row, col, child->startRow, child->startCol);
                    const vector<double> currentQValues = node->getQValues(row, col, node->startRow, node->startCol);
                    childQValues = currentQValues; // Copy all action Q-values
                }
            }
            toVisit.push(child); // Continue to child regardless of qTable
        }
    }
}

/*************************************************************************/
void propagateQTableUpwards(const TreeNode *node) {
    if (!node || !node->qTable || !node->parent) return; // Skip if no node, no qTable, or no parent

    const TreeNode *current = node->parent; // Start at parent
    while (current) {
        // Continue until root (no parent)
        if (current->qTable) {
            // Update only if qTable exists
            // Copy Q-values for positions within node's subenvironment
            for (int row = node->startRow; row <= node->endRow; ++row) {
                for (int col = node->startCol; col <= node->endCol; ++col) {
                    vector<double> &parentQValues = current->getQValues(row, col, current->startRow, current->startCol);
                    const vector<double> nodeQValues = node->getQValues(row, col, node->startRow, node->startCol);
                    parentQValues = nodeQValues; // Copy all action Q-values
                }
            }
        }
        current = current->parent; // Move up, even if no qTable
    }
}

/*************************************************************************/
void simulateEnvironmentChanges(const TreeNode *root, const int numSteps, vector<pair<int, int> > &changedPositions) {
    if (!root) {
        cerr << "Error: Root node is null.\n";
        return;
    }

    // Get the obstacle positions
    const int rows = root->rows, cols = root->cols;
    vector<pair<int, int> > obstaclePositions = root->maze->getObstaclePositions();
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
                    (*root->maze)(newRow, newCol) == constants::FREE_SPACE) {
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
                (*root->maze)(oldRow, oldCol, constants::FREE_SPACE);
                (*root->maze)(newRow, newCol, constants::OBSTACLE);
                obstaclePositions[randomIndex] = {newRow, newCol};
            }
        }
    }
}

/*************************************************************************/
struct AStarNode {
    int x, y, g, h;
    bool operator>(const AStarNode &other) const { return (g + h) > (other.g + h); }
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

unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> computeAllShortestPaths(const Maze &maze) {
    const int rows = maze.getRows();
    const int cols = maze.getCols();
    vector<pair<int, int> > directions = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
    };
    unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> shortestPaths;
    unordered_set<pair<int, int>, HashPair> processed; // Tracks positions with assigned paths

    // Iterate through all cells in the maze
    for (int startX = 0; startX < rows; ++startX) {
        for (int startY = 0; startY < cols; ++startY) {
            if (maze(startX, startY) == constants::OBSTACLE || processed.contains({startX, startY})) {
                continue; // Skip obstacles and processed positions
            }

            priority_queue<AStarNode, vector<AStarNode>, greater<> > openSet;
            unordered_map<pair<int, int>, int, HashPair> gScore;
            unordered_map<pair<int, int>, pair<int, int>, HashPair> cameFrom;

            openSet.push({startX, startY, 0, 0});
            gScore[{startX, startY}] = 0;

            bool found = false;
            pair<int, int> goal;

            // A* search
            while (!openSet.empty() && !found) {
                AStarNode current = openSet.top();
                openSet.pop();

                if (maze(current.x, current.y) == constants::CHARGING_STATION) {
                    goal = {current.x, current.y};
                    found = true;
                    break;
                }

                for (auto [dx, dy]: directions) {
                    const int newX = current.x + dx;
                    const int newY = current.y + dy;

                    if (newX >= 0 && newX < rows && newY >= 0 && newY < cols && maze(newX, newY) !=
                        constants::OBSTACLE) {
                        const int newG = gScore[{current.x, current.y}] + 1;
                        if (!gScore.contains({newX, newY}) || newG < gScore[{newX, newY}]) {
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
                    if (maze(px, py) == constants::CHARGING_STATION && i == path.size() - 1) {
                        shortestPaths[{px, py}] = {{px, py}}; // Station to itself
                    } else {
                        // Store suffix as shortest path (from px, py to goal)
                        vector<pair<int, int> > subPath(path.begin() + i, path.end());
                        auto pos = make_pair(px, py);
                        // Only store if no path exists or new path is shorter
                        if (!shortestPaths.contains(pos) || subPath.size() < shortestPaths[pos].size()) {
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
            if (maze(x, y) != constants::OBSTACLE && !shortestPaths.contains({x, y})) {
                shortestPaths[{x, y}] = {};
            }
        }
    }

    return shortestPaths;
}

/*************************************************************************/
tuple<double, double, double> testAgentAStar(const Maze &maze, const int rows, const int cols,
                                             const unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> &
                                             shortestPaths) {
    // Collect all valid positions to test
    vector<pair<int, int> > positions;
    int totalPositions = 0;
    for (int x1 = 0; x1 < rows; ++x1) {
        for (int y1 = 0; y1 < cols; ++y1) {
            if (maze(x1, y1) != constants::OBSTACLE) {
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
                    if (maze(x, y) == constants::OBSTACLE) {
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
void testAStarPerformance(const TreeNode *root, const int numSteps) {
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
void collectLeafNodes(TreeNode *node, vector<TreeNode *> &leafNodes) {
    if (!node) return;
    if (node->children.empty()) {
        // Leaf node
        leafNodes.push_back(node);
    } else {
        for (TreeNode *child: node->children) {
            collectLeafNodes(child, leafNodes);
        }
    }
}

/*************************************************************************/
double computeNodeSuccessRate(const TreeNode *root, const TreeNode *node) {
    if (!root || !node || !root->maze || !root->qTable)
        return 0.0; // Safety checks

    // Get the sub-environment bounds from the node
    const int startRow = node->startRow;
    const int startCol = node->startCol;
    const int endRow = node->endRow;
    const int endCol = node->endCol;

    // Use the full maze from the root for pathfinding
    const Maze &maze = *root->maze;
    const int rows = root->rows;
    const int cols = root->cols;
    const int maxSteps = rows + cols; // Consistent with testAgent

    int totalPositions = 0;
    int successfulPaths = 0;

    // Iterate over all positions within the node's subenvironment
    for (int x = startRow; x <= endRow; ++x) {
        for (int y = startCol; y <= endCol; ++y) {
            if (maze(x, y) == constants::OBSTACLE) continue; // Skip obstacles
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
void fedAsynQ_EqAvg(TreeNode *node, const Maze &maze, const int tau, const int T, const int K = 8) {
    // Create aggregate Q-table
    const int localRows = node->endRow - node->startRow + 1;
    const int localCols = node->endCol - node->startCol + 1;
    auto aggregatedQTable = Table<double>(localRows, localCols, constants::ACTION_COUNT);

    // Create previous aggregate Q-table for convergence check
    auto prevAggregatedQTable = aggregatedQTable;

    // Initialize the Q-table for the node (if not already initialized)
    node->initQTable();

    // Local Q-tables for each agent
    vector<Table<double> > localQTables(K, *node->qTable);

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
        auto [x1, y1] = maze.selectFirstPlace(node->startRow, node->startCol, node->endRow, node->endCol, 0, startStats,
                                              rngs[k]);
        agentPositions[k] = {x1, y1};
    }

    // Define epsilon-greedy parameters
    double epsilon = 1.0; // Initial exploration rate

    // Loop for at most T iterations (ensuring that t + tau <= T to avoid iterations for which there will be no update)
    int t = 0;
    while (t + tau <= T) {
        // Spawn threads for each agent
        vector<thread> threads;
        for (int k = 0; k < K; ++k) {
            threads.emplace_back(
                [&maze, &node, &agentPositions, &localQTables, &startStats, &statsMutex, epsilon, tau, k]() {
                    pair<int, int> &agentPosition = agentPositions[k];
                    Table<double> &localQTable = localQTables[k];
                    int x1 = agentPosition.first, y1 = agentPosition.second;

                    // Perform tau steps
                    for (int step = 0; step < tau; ++step) {
                        // Select and perform action
                        vector<double> &qValues = localQTable(x1, y1, node->startRow, node->startCol);
                        int act = node->selectAction(x1, y1, epsilon);

                        int x2, y2, actionReward;
                        tie(x2, y2, act, actionReward) = maze.performAction(node->rows, node->cols, x1, y1, act);

                        // Update Q-value
                        const vector<double> &nextQValues = localQTable(x2, y2, node->startRow, node->startCol);
                        const double maxNextQ = *ranges::max_element(nextQValues);
                        qValues[act] += constants::LEARNING_RATE * (
                            actionReward + constants::DISCOUNT_FACTOR * maxNextQ - qValues[act]);

                        // Update startStats
                        if (step == 0) {
                            lock_guard<mutex> lock(statsMutex);
                            auto &stats = startStats[{x1, y1}];
                            stats.incrementAttempts();
                            if (maze.checkExit(x2, y2)) stats.incrementSuccesses();
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
        aggregatedQTable = Table<double>(localRows, localCols, constants::ACTION_COUNT);

        // Default alpha for averaging
        const double alpha = 1.0 / K;

        // Aggregate Q-values from all local Q-tables
        for (int k = 0; k < K; ++k) {
            for (int row = node->startRow; row <= node->endRow; ++row) {
                for (int col = node->startCol; col <= node->endCol; ++col) {
                    vector<double> &aggregatedQValues = aggregatedQTable(row, col, node->startRow, node->startCol);
                    vector<double> &localQValues = localQTables[k](row, col, node->startRow, node->startCol);
                    for (int a = 0; a < constants::ACTION_COUNT; ++a) {
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
                const vector<double> &currentQ = aggregatedQTable(row, col, node->startRow, node->startCol);
                const vector<double> &prevQ = prevAggregatedQTable(row, col, node->startRow, node->startCol);

                // Compute the difference for each action
                for (int a = 0; a < constants::ACTION_COUNT; ++a) {
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
            auto [x1, y1] = maze.selectFirstPlace(node->startRow, node->startCol, node->endRow, node->endCol, t,
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
    node->qTable = make_unique<Table<double> >(aggregatedQTable);
}


/*************************************************************************/
void fedAsynQ_ImAvg(TreeNode *node, const Maze &maze, const int tau, const int T, const int K = 8) {
    // Create aggregate Q-table
    const int localRows = node->endRow - node->startRow + 1;
    const int localCols = node->endCol - node->startCol + 1;
    auto aggregatedQTable = Table<double>(localRows, localCols, constants::ACTION_COUNT);

    // Create previous aggregate Q-table for convergence check
    auto prevAggregatedQTable = Table<double>(localRows, localCols, constants::ACTION_COUNT);

    // Initialize the Q-table for the node (if not already initialized)
    node->initQTable();

    // Local Q-tables for each agent
    vector<Table<double> > localQTables(K, *node->qTable);

    // Create state-action counts for each agent
    auto stateActionCounts = vector<Table<int> >(K, Table<int>(localRows, localCols, constants::ACTION_COUNT));

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
        auto [x1, y1] = maze.selectFirstPlace(node->startRow, node->startCol, node->endRow, node->endCol, 0, startStats,
                                              rngs[k]);
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
                    Table<double> &localQTable = localQTables[k];

                    // Wrong initialization, but used to avoid compiler errors
                    Table<int> &stateActionTable = stateActionCounts[k];
                    int x1 = agentPosition.first, y1 = agentPosition.second;

                    // Perform tau steps
                    for (int step = 0; step < tau; ++step) {
                        // Select and perform action
                        vector<double> &qValues = localQTable(x1, y1, node->startRow, node->startCol);
                        int act = node->selectAction(x1, y1, epsilon);

                        int x2, y2, actionReward;
                        tie(x2, y2, act, actionReward) = maze.performAction(node->rows, node->cols, x1, y1, act);

                        // Update the state-action count
                        vector<int> &actionCounts = stateActionTable(x1, y1, node->startRow, node->startCol);
                        actionCounts[act] += 1; // Increment action count for this state

                        // Update Q-value
                        const vector<double> &nextQValues = localQTable(x2, y2, node->startRow, node->startCol);
                        const double maxNextQ = *ranges::max_element(nextQValues);
                        qValues[act] += constants::LEARNING_RATE * (
                            actionReward + constants::DISCOUNT_FACTOR * maxNextQ - qValues[act]);

                        // Update startStats
                        if (step == 0) {
                            lock_guard<mutex> lock(statsMutex);
                            auto &stats = startStats[{x1, y1}];
                            stats.incrementAttempts();
                            if (maze.checkExit(x2, y2)) stats.incrementSuccesses();
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
        aggregatedQTable = Table<double>(localRows, localCols, constants::ACTION_COUNT);

        // Create denominator table for computation of alpha
        auto denominatorTable = Table<double>(localRows, localCols, constants::ACTION_COUNT);

        // Compute the denominator for each position in the local Q-tables
        for (int k = 0; k < K; ++k) {
            for (int row = node->startRow; row <= node->endRow; ++row) {
                for (int col = node->startCol; col <= node->endCol; ++col) {
                    vector<double> &denominator = denominatorTable(row, col, node->startRow, node->startCol);
                    const vector<int> &actionCounts = stateActionCounts[k](row, col, node->startRow, node->startCol);
                    for (int a = 0; a < constants::ACTION_COUNT; ++a) {
                        denominator[a] += pow(1 - constants::LEARNING_RATE, -1.0 * actionCounts[a]);
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
                    vector<double> &aggregatedQValues = aggregatedQTable(row, col, node->startRow, node->startCol);
                    vector<double> &localQValues = localQTables[k](row, col, node->startRow, node->startCol);

                    // Compute alpha based on the mode
                    const vector<double> &denominator = denominatorTable(row, col, node->startRow, node->startCol);
                    const vector<int> &actionCounts = stateActionCounts[k](row, col, node->startRow, node->startCol);
                    for (int a = 0; a < constants::ACTION_COUNT; ++a) {
                        double nominator = pow(1 - constants::LEARNING_RATE, -1.0 * actionCounts[a]);
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
                const vector<double> &currentQ = aggregatedQTable(row, col, node->startRow, node->startCol);
                const vector<double> &prevQ = prevAggregatedQTable(row, col, node->startRow, node->startCol);

                // Compute the difference for each action
                for (int a = 0; a < constants::ACTION_COUNT; ++a) {
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
        stateActionCounts = vector<Table<int> >(K, Table<int>(localRows, localCols, constants::ACTION_COUNT));

        // Select new start positions for all agents
        for (int k = 0; k < K; ++k) {
            // Randomly select a new start position within the node's bounds
            // auto [x1, y1] = selectFirstPlace(maze, node->startRow, node->startCol, node->endRow, node->endCol);
            auto [x1, y1] = maze.selectFirstPlace(node->startRow, node->startCol, node->endRow, node->endCol, t,
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
    node->qTable = make_unique<Table<double> >(aggregatedQTable);
}


/*************************************************************************/
void trainNodesInParallel(TreeNode *root, const vector<TreeNode *> &nodes, const string &mode) {
    // Use threads to train agents concurrently
    vector<thread> threads;

    for (TreeNode *node: nodes) {
        threads.emplace_back([root, node, mode]() {
            if (mode == "Hierarchy") {
                const int maxSteps = (node->endRow - node->startRow + 1) + (node->endCol - node->startCol + 1);
                trainAgentWithStoppingCriterion(node, *root->maze, node->rows, node->cols, node->startRow,
                                                node->startCol, node->endRow, node->endCol, maxSteps);
            } else if (mode == "EqAvg") {
                const int T = (node->endRow - node->startRow + 1) * (node->endCol - node->startCol + 1) * 200;
                fedAsynQ_EqAvg(node, *root->maze, 1000, T, 12);
            } else if (mode == "ImAvg") {
                const int T = (node->endRow - node->startRow + 1) * (node->endCol - node->startCol + 1) * 200;
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
    unordered_set<TreeNode *> visited; // Track nodes to avoid recomputing shared descendants
    for (TreeNode *node: nodes) {
        if (visited.contains(node)) continue; // Skip if already processed

        // DFS to recompute success rates for node and descendants
        stack<TreeNode *> toVisit;
        toVisit.push(node);

        while (!toVisit.empty()) {
            TreeNode *current = toVisit.top();
            toVisit.pop();

            // Skip if already visited
            if (visited.contains(current)) continue;
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
            for (TreeNode *child: current->children) {
                toVisit.push(child);
            }
        }
    }
    cout << "Finished updating success rates.\n";
}

/*************************************************************************/
// void runAgentEpisodes(const TreeNode *node, const vector<vector<int> > &maze, const double epsilon,
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
//                                : vector<double>(constants::ACTION_COUNT, 0.0);
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
//             if (qValuesCurrent.empty()) qValuesCurrent.resize(constants::ACTION_COUNT, 0.0);
//             auto qValuesNext = localQTable.find({x2, y2}) != localQTable.end()
//                                    ? localQTable[{x2, y2}]
//                                    : vector<double>(constants::ACTION_COUNT, 0.0);
//             const double maxNextQ = *max_element(qValuesNext.begin(), qValuesNext.end());
//             const double oldQ = qValuesCurrent[act];
//             qValuesCurrent[act] = oldQ + constants::LEARNING_RATE * (actionReward + constants::DISCOUNT_FACTOR * maxNextQ - oldQ);
//
//             // Experience replay
//             if (useReplay && localReplayBuffer.size() >= 64) {
//                 for (int i = 0; i < 64; ++i) {
//                     const int idx = rand() % localReplayBuffer.size();
//                     const auto &[x1_r, y1_r, action, reward, x2_r, y2_r] = localReplayBuffer[idx];
//                     auto &qValues_r = localQTable[{x1_r, y1_r}];
//                     if (qValues_r.empty()) qValues_r.resize(constants::ACTION_COUNT, 0.0);
//                     auto qValuesNext_r = localQTable.find({x2_r, y2_r}) != localQTable.end()
//                                              ? localQTable[{x2_r, y2_r}]
//                                              : vector<double>(constants::ACTION_COUNT, 0.0);
//                     const double maxNextQ_r = *max_element(qValuesNext_r.begin(), qValuesNext_r.end());
//                     const double oldQ_r = qValues_r[action];
//                     qValues_r[action] = oldQ_r + constants::LEARNING_RATE * (reward + constants::DISCOUNT_FACTOR * maxNextQ_r - oldQ_r);
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
// void trainNodeWithMultiAgents(const TreeNode *root, TreeNode *node, const vector<vector<int> > &maze, double epsilon,
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
//     //             qTable[{i, j}] = vector<double>(constants::ACTION_COUNT, 0.0);
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
//                 newQTable[{r, c}] = vector<double>(constants::ACTION_COUNT, 0.0);
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
//                 for (int a = 0; a < constants::ACTION_COUNT; ++a) {
//                     newQValues[a] += qValues[a];
//                 }
//             }
//         }
//
//         // Average Q-values
//         for (auto &[pos, qValues]: newQTable) {
//             if (stateActionCounts[pos] == 0) continue; // Skip if no actions taken
//             for (int a = 0; a < constants::ACTION_COUNT; ++a) {
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
// void trainNodesInParallelMultiAgents(TreeNode *root, const vector<TreeNode *> &nodes, double epsilon) {
//     constexpr int numAgents = 8; // Adjustable
//     vector<thread> threads;
//
//     for (TreeNode *node: nodes) {
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
//     unordered_set<TreeNode *> visited; // Track nodes to avoid recomputing shared descendants
//     for (TreeNode *node: nodes) {
//         if (visited.find(node) != visited.end()) continue; // Skip if already processed
//
//         // DFS to recompute success rates for node and descendants
//         stack<TreeNode *> toVisit;
//         toVisit.push(node);
//
//         while (!toVisit.empty()) {
//             TreeNode *current = toVisit.top();
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
//             for (TreeNode *child: current->children) {
//                 toVisit.push(child);
//             }
//         }
//     }
//     cout << "Finished updating success rates.\n";
// }

/*************************************************************************/
void trainNodesSequentially(const TreeNode *root, const vector<TreeNode *> &nodes) {
    for (TreeNode *node: nodes) {
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
    unordered_set<TreeNode *> visited; // Track nodes to avoid recomputing shared descendants
    for (TreeNode *node: nodes) {
        if (visited.contains(node)) continue; // Skip if already processed

        // DFS to recompute success rates for node and descendants
        stack<TreeNode *> toVisit;
        toVisit.push(node);

        while (!toVisit.empty()) {
            TreeNode *current = toVisit.top();
            toVisit.pop();

            // Skip if already visited
            if (visited.contains(current)) continue;
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
            for (TreeNode *child: current->children) {
                toVisit.push(child);
            }
        }
    }
    cout << "Finished updating success rates.\n";
}

/*************************************************************************/
void applyLocalPathPlanning(TreeNode *root, const vector<TreeNode *> &changedLeaves = {}) {
    // Environment is static. Performing global path planning selectively
    if (changedLeaves.empty()) {
        // Train all leaf nodes initially
        vector<TreeNode *> leafNodes;
        collectLeafNodes(root, leafNodes);
        trainNodesInParallel(root, leafNodes, "Hierarchy");
    }
    // Environment changed. Training affected leaf nodes
    else {
        trainNodesInParallel(root, changedLeaves, "Hierarchy");
    }
}

/*************************************************************************/
void trainHierarchy(TreeNode *root, const vector<TreeNode *> &changedLeaves = {}, const int maxLevelsToTrain = 1) {
    if (!root) return;
    const bool isInitialTraining = changedLeaves.empty();

    // Collect leaf nodes to train
    vector<TreeNode *> leafNodesToTrain;
    if (isInitialTraining) {
        collectLeafNodes(root, leafNodesToTrain);
    } else {
        // Use the provided list of affected leaf nodes directly
        leafNodesToTrain = changedLeaves;
    }

    // Train all affected or initial leaf nodes
    trainNodesInParallel(root, leafNodesToTrain, "Hierarchy");

    // Decision mechanism: Count affected children per parent
    unordered_map<TreeNode *, int> parentAffectedCount; // Parent -> # of affected children
    for (const TreeNode *leaf: leafNodesToTrain) {
        if (leaf->parent) {
            parentAffectedCount[leaf->parent]++;
        }
    }

    // Select parents to retrain: >= 1 affected child
    vector<TreeNode *> parentsToTrain;
    for (const auto &[parent, affectedCount]: parentAffectedCount) {
        if (affectedCount >= 1) {
            // Retrain if 1 or more of 4 children are affected
            parentsToTrain.push_back(parent);
        }
    }

    // Train parents hierarchically up to maxLevelsToTrain
    int levelsTrained = 0;
    unordered_set<TreeNode *> currentLevelNodes(parentsToTrain.begin(), parentsToTrain.end());
    while (!currentLevelNodes.empty() && levelsTrained < maxLevelsToTrain) {
        vector<TreeNode *> nodesToTrain(currentLevelNodes.begin(), currentLevelNodes.end());
        trainNodesInParallel(root, nodesToTrain, "Hierarchy");

        // Prepare next level with the same decision rule
        unordered_map<TreeNode *, int> nextLevelAffectedCount;
        for (const TreeNode *node: nodesToTrain) {
            if (node->parent) {
                nextLevelAffectedCount[node->parent]++;
            }
        }

        // Select parents for the next level
        unordered_set<TreeNode *> nextLevelNodes;
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
void trainHierarchySmart(TreeNode *root, const vector<TreeNode *> &changedLeaves = {}) {
    if (!root) return; // Safety check: Exit if root is null

    cout << "\nBegin training...\n";

    // Determine initial training
    const bool isInitialTraining = changedLeaves.empty();

    // Step 1: Collect leaf nodes to train
    vector<TreeNode *> leafNodesToTrain;
    if (isInitialTraining) {
        // Initial training: Gather all leaf nodes in the hierarchy
        collectLeafNodes(root, leafNodesToTrain);
    } else {
        // Dynamic training: Use the list of leaves affected by changes
        leafNodesToTrain = changedLeaves;
    }

    // Step 2: Decide which leaves to train or retrain
    vector<TreeNode *> leavesToRetrain;
    if (isInitialTraining) {
        // For initial training, train all collected leaves
        leavesToRetrain = leafNodesToTrain;
    } else {
        // For changes, check each affected leaf's success rate
        for (TreeNode *leaf: leafNodesToTrain) {
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
    unordered_set<TreeNode *> parentsToRetrain;
    if (!leavesToRetrain.empty()) {
        // Train all leaves marked for retraining in one batch
        cout << "Training leaves...\n";
        trainNodesInParallel(root, leavesToRetrain, "Hierarchy");
        cout << "Leaves trained.\n";
        for (const TreeNode *leaf: leavesToRetrain) {
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
    unordered_set<TreeNode *> currentLevelNodes = parentsToRetrain;
    while (!currentLevelNodes.empty()) {
        // Prepare lists for nodes to train in this level and parents for the next level
        vector<TreeNode *> nodesToTrain;
        unordered_set<TreeNode *> nextLevelNodes;

        // Process each node in the current level
        for (TreeNode *node: currentLevelNodes) {
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
            for (const TreeNode *node: nodesToTrain) {
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
void trainHierarchySmartMultiAgent(const string &mode, TreeNode *root, const vector<TreeNode *> &changedLeaves = {}) {
    if (!root) return; // Safety check: Exit if root is null

    cout << "\nBegin training...\n";

    // Determine initial training
    const bool isInitialTraining = changedLeaves.empty();

    // Step 1: Collect leaf nodes to train
    vector<TreeNode *> leafNodesToTrain;
    if (isInitialTraining) {
        // Initial training: Gather all leaf nodes in the hierarchy
        collectLeafNodes(root, leafNodesToTrain);
    } else {
        // Dynamic training: Use the list of leaves affected by changes
        leafNodesToTrain = changedLeaves;
    }

    // Step 2: Decide which leaves to train or retrain
    vector<TreeNode *> leavesToRetrain;
    if (isInitialTraining) {
        // For initial training, train all collected leaves
        leavesToRetrain = leafNodesToTrain;
    } else {
        // For changes, check each affected leaf's success rate
        for (TreeNode *leaf: leafNodesToTrain) {
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
    unordered_set<TreeNode *> parentsToRetrain;
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
        for (const TreeNode *leaf: leavesToRetrain) {
            // const double newSuccessRate = computeNodeSuccessRate(root, leaf);
            // leaf->baselineSuccessRate = newSuccessRate;
            if (leaf->baselineSuccessRate < 0.9 && leaf->parent) {
                // If success rate is low, mark parent for retraining
                parentsToRetrain.insert(leaf->parent);
            }
        }

        // Step 4: Propagate retraining upward through the hierarchy
        // Start with parents of retrained leaves
        unordered_set<TreeNode *> currentLevelNodes = parentsToRetrain;
        while (!currentLevelNodes.empty()) {
            // Prepare lists for nodes to train in this level and parents for the next level
            vector<TreeNode *> nodesToTrain;
            unordered_set<TreeNode *> nextLevelNodes;

            // Process each node in the current level
            for (TreeNode *node: currentLevelNodes) {
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
                for (const TreeNode *node: nodesToTrain) {
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
void testLocalPathPlanning(TreeNode *root, const int numSteps) {
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
    unordered_set<TreeNode *> changedLeaves;
    for (const auto &[r, c]: changedPositions) {
        TreeNode *leaf = root->findSubEnvironment(r, c);
        if (leaf && leaf->children.empty()) changedLeaves.insert(leaf);
    }
    const auto changedLeavesToTrain = vector<TreeNode *>(changedLeaves.begin(), changedLeaves.end());

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
void testHierarchicalPathPlanning(TreeNode *root, const int numSteps) {
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
    unordered_set<TreeNode *> changedLeaves;
    for (const auto &[r, c]: changedPositions) {
        TreeNode *leaf = root->findSubEnvironment(r, c);
        if (leaf && leaf->children.empty()) changedLeaves.insert(leaf);
    }
    const auto changedLeavesToTrain = vector<TreeNode *>(changedLeaves.begin(), changedLeaves.end());

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
    TreeNode *subEnv;

    Agent(const int r, const int c, TreeNode *env) : row(r), col(c), subEnv(env) {
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
            (*subEnv->maze)(new_row, new_col) != constants::OBSTACLE &&
            (*subEnv->maze)(new_row, new_col) != constants::AGENT) {
            // **Restore previous position correctly**
            if ((*subEnv->maze)(row, col) != constants::CHARGING_STATION) {
                (*subEnv->maze)(row, col, constants::FREE_SPACE);
                // Restore free space only if it wasn't a charging station
            }

            // Move agent
            row = new_row;
            col = new_col;

            // **Do NOT overwrite charging stations**
            if ((*subEnv->maze)(row, col) != constants::CHARGING_STATION) {
                (*subEnv->maze)(row, col, constants::AGENT); // Mark new position as occupied by agent
            }

            // **Only check for new subenvironment if the agent is NOT in the root**
            if (subEnv->parent) {
                // Find the root node of the hierarchy
                TreeNode *root = subEnv;
                while (root->parent) {
                    root = root->parent;
                }
                TreeNode *newSubEnv = root->findSubEnvironment(row, col);
                if (newSubEnv && newSubEnv != subEnv) {
                    subEnv = newSubEnv; // Update the agent's subenvironment
                }
            }
        }
        return {row, col};
    }

    vector<pair<int, int> > findOptimalPath() {
        vector<pair<int, int> > path;
        while ((*subEnv->maze)(row, col) != constants::CHARGING_STATION) {
            path.emplace_back(row, col);

            // Select action based on the current subenvironment's Q-table
            const int action = selectAction(0.0);
            const pair<int, int> newPos = step(action);

            // Check if the new position falls into a different subenvironment
            TreeNode *newSubEnv = subEnv->parent->findSubEnvironment(newPos.first, newPos.second);
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
    explicit VDNTrainer(TreeNode *root) {
        // For each leaf node, create an agent
        vector<TreeNode *> leafNodes = {};
        collectLeafNodes(root, leafNodes);
        for (TreeNode *leaf: leafNodes) {
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
        td_error += constants::DISCOUNT_FACTOR * maxNextGlobalQ - globalQ; // Compute TD error

        // Update individual Q-values using the shared TD error
        for (size_t i = 0; i < agents.size(); i++) {
            Agent *agent = agents[i];
            const int row = agent->row;
            const int col = agent->col;
            const int action = actions[i];

            if (action == -1) continue; // Skip agents that reached a charging station

            // Q-learning update rule for each agent
            // double &qValue = (*agent->subEnv->qTable)[row][col][action];
            // qValue += constants::LEARNING_RATE * td_error; // Each agent updates using the same TD error
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
            TreeNode *subEnv = agent->subEnv;
            // tie(agent->row, agent->col) = selectFirstPlace(*subEnv->maze, subEnv->startRow, subEnv->startCol,
            //                                                subEnv->endRow, subEnv->endCol);
            (*subEnv->maze)(agent->row, agent->col, constants::AGENT);
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
                if (occupied_positions.contains(next_pos)) {
                    reward = -5.0; // Collision penalty
                    next_pos = prev_pos;
                } else if ((*agent->subEnv->maze)(next_pos.first, next_pos.second) == constants::CHARGING_STATION) {
                    reward = 30.0; // Goal reward
                    reached_goal[i] = 1; // Mark this agent as having reached the goal
                } else if ((*agent->subEnv->maze)(next_pos.first, next_pos.second) == constants::OBSTACLE) {
                    reward = -10.0; // Obstacle penalty
                }

                // Update the agent's q-values
                // updateQTable(*agent->subEnv->qTable, prev_pos.first, prev_pos.second, action, reward, next_pos.first,
                //              next_pos.second);
            }

            // **3. Update Q-values**
            // trainer.updateGlobalQValues(rewards, next_positions, actions);

            // **4. Check if all agents have reached a charging station**
            if (ranges::all_of(reached_goal, [](const int x) { return x == 1; })) {
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
                (*agent->subEnv->maze)(agent->row, agent->col, constants::CHARGING_STATION);
                // Otherwise, clear the agent's position
            } else {
                (*agent->subEnv->maze)(agent->row, agent->col, constants::FREE_SPACE);
            }
        }

        // TODO: Fix the previous Q-table comparison code
        // **7. Check stopping criterion every 10 episodes**
        // if (episode % 10 == 0) {
        //     double maxChange = 0.0;
        //
        //     for (Agent *agent : trainer.agents) {
        //         TreeNode *subEnv = agent->subEnv;
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
        //         TreeNode *subEnv = agent->subEnv;
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
void testVDNTraining(TreeNode *root, const int numChanges) {
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

/*************************************************************************/
void runFullExperiment() {
    vector<int> sizes = {50, 100, 200, 300};
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
            Maze initialMaze(size, size, freeProb, obstProb, chargeProb);
            vector<pair<int, vector<pair<int, int> > > > changeSequence(maxTimeSteps);
            TreeNode *tempRoot = createSubEnvironments(initialMaze, size, size);

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

                TreeNode *root = createSubEnvironments(initialMaze, size, size);
                unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> shortestPaths;
                double totalInitialTime = 0.0, totalAdaptTime = 0.0, totalSuccessRate = 0.0, totalPathLength = 0.0;
                int stepsCompleted = 0;

                // Inspect the distribution of charging stations across the maze
                root->printTree();

                // Initialize visualization for MultiAgentsFedAsynQ
                std::unique_ptr<PolicyVisualizer> visualizer;
                if (name == "HierarchySmart" || name == "FedAsynQ_EqAvg" || name == "FedAsynQ_ImAvg") {
                    visualizer = std::make_unique<PolicyVisualizer>(root, size, name, maxTimeSteps);
                    visualizer->update();
                    visualizer->render();
                }

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
                Maze currentMaze = initialMaze;
                for (int t = 0; t < maxTimeSteps; ++t) {
                    const auto &[numChanges, changes] = changeSequence[t];
                    for (int i = 0; i < changes.size(); i += 2) {
                        currentMaze(changes[i].first, changes[i].second, constants::FREE_SPACE);
                        currentMaze(changes[i + 1].first, changes[i + 1].second, constants::OBSTACLE);
                    }
                    root->maze = make_unique<Maze>(currentMaze);

                    unordered_set<TreeNode *> changedLeaves;
                    for (const auto &[r, c]: changes) {
                        TreeNode *leaf = root->findSubEnvironment(r, c);
                        if (leaf && leaf->children.empty()) changedLeaves.insert(leaf);
                    }
                    vector<TreeNode *> changedLeafSet(changedLeaves.begin(), changedLeaves.end());

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
                    if (visualizer) {
                        visualizer->update();
                        visualizer->render();
                        // Brief delay to ensure smooth rendering
                        sf::sleep(sf::milliseconds(500));
                    }

                    // Write per-step data
                    detailedOut << name << "," << size << "," << diffName << "," << t + 1 << ","
                            << numChanges << "," << adaptTime << "," << stepSuccessRate << ","
                            << stepAvgPath << "\n";

                    // Check if window is still open
                    if (visualizer && !visualizer->isOpen()) {
                        break;
                    }
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
