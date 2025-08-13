#include "multiagent.h"

void MultiAgent::fedAsynQ_EqAvg(TreeNode *node, const Maze &maze, const int tau, const int T, const int K) {
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
    }

    // Create final Q-table for the node
    node->qTable = make_unique<Table<double> >(aggregatedQTable);
}

void MultiAgent::fedAsynQ_ImAvg(TreeNode *node, const Maze &maze, const int tau, const int T, const int K) {
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
    }

    // Create final Q-table for the node
    node->qTable = make_unique<Table<double> >(aggregatedQTable);
}
