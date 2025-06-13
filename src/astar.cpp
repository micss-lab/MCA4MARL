#include "astar.h"

int AStar::heuristic(const int x1, const int y1, const int x2, const int y2) {
    // Use the Chebyshev distance heuristic
    return max(abs(x1 - x2), abs(y1 - y2));
}

vector<pair<int, int> > AStar::reconstructPath(unordered_map<pair<int, int>, pair<int, int>, HashPair> &cameFrom,
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

unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> AStar::computeAllShortestPaths(const Maze &maze) {
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

tuple<double, double, double> AStar::testAStar(const Maze &maze, const int rows, const int cols,
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
    constexpr size_t chunkSize = 100;
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
