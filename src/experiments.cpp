#include "experiments.h"

void Experiments::simulateEnvironmentChanges(const TreeNode *root, const int numSteps,
                                             vector<pair<int, int> > &changedPositions) {
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

void Experiments::runFullExperiment(bool visualize) {
    vector<int> sizes = {20, 50, 100, 200, 300};
    vector<tuple<double, double, double> > difficulties = {
        {0.8, 0.18, 0.02}, // Easy
        {0.7, 0.29, 0.01}, // Medium
        {0.6, 0.395, 0.005} // Hard
    };
    // Approaches to test
    vector<string> approaches = {
        "A* Static",
        "A* Oracle",
        "onlyTrainLeafNodes",
        "singleAgent",
        "fedAsynQ_EqAvg",
        "fedAsynQ_ImAvg"
    };

    // Detailed output file for per-step data
    ofstream detailedOut("results_detailed.csv");
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
            auto *tempRoot = new TreeNode(initialMaze, size, size, 0, 0, size - 1, size - 1, nullptr, true);
            tempRoot->createSubEnvironments(initialMaze);

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

                // Create the root node for the current approach
                auto *root = new TreeNode(initialMaze, size, size, 0, 0, size - 1, size - 1, nullptr, true);
                root->createSubEnvironments(initialMaze);

                unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> shortestPaths;
                double totalInitialTime = 0.0, totalAdaptTime = 0.0, totalSuccessRate = 0.0, totalPathLength = 0.0;
                int stepsCompleted = 0;

                // Inspect the distribution of charging stations across the maze
                root->printTree();

                // Initialize visualization
                unique_ptr<PolicyVisualizer> visualizer;
                if (visualize) {
                    if (name == "singleAgent" || name == "fedAsynQ_EqAvg" || name == "fedAsynQ_ImAvg") {
                        visualizer = make_unique<PolicyVisualizer>(root, size, name, maxTimeSteps);
                        visualizer->update();
                        visualizer->render();
                    }
                }

                // Initial training
                if (name == "A* Oracle" || name == "A* Static") {
                    auto start = chrono::high_resolution_clock::now();
                    shortestPaths = AStar::computeAllShortestPaths(*root->maze);
                    auto end = chrono::high_resolution_clock::now();
                    totalInitialTime = chrono::duration<double>(end - start).count();
                } else {
                    auto start = chrono::high_resolution_clock::now();
                    if (name == "onlyTrainLeafNodes") TreeStrategy::onlyTrainLeafNodes(root);
                    else if (name == "singleAgent") TreeStrategy::smartHierarchy(root, {}, "singleAgent");
                    else if (name == "fedAsynQ_EqAvg") TreeStrategy::smartHierarchy(root, {}, "fedAsynQ_EqAvg");
                    else if (name == "fedAsynQ_ImAvg") TreeStrategy::smartHierarchy(root, {}, "fedAsynQ_ImAvg");
                    auto end = chrono::high_resolution_clock::now();
                    totalInitialTime = chrono::duration<double>(end - start).count();
                }

                // Test initial performance
                auto [_, successRate, avgPath] = (name == "A* Oracle" || name == "A* Static")
                                                     ? AStar::testAStar(*root->maze, size, size, shortestPaths)
                                                     : TestPolicy::testAgent(root);
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
                        shortestPaths = AStar::computeAllShortestPaths(*root->maze);
                        auto end = chrono::high_resolution_clock::now();
                        adaptTime = chrono::duration<double>(end - start).count();
                    } else if (name == "A* Static") {
                        adaptTime = 0.0; // No adaptation
                    } else {
                        auto start = chrono::high_resolution_clock::now();
                        if (name == "onlyTrainLeafNodes") TreeStrategy::onlyTrainLeafNodes(root, changedLeafSet);
                        else if (name == "singleAgent")
                            TreeStrategy::smartHierarchy(
                                root, changedLeafSet, "singleAgent");
                        else if (name == "fedAsynQ_EqAvg")
                            TreeStrategy::smartHierarchy(
                                root, changedLeafSet, "fedAsynQ_EqAvg");
                        else if (name == "fedAsynQ_ImAvg")
                            TreeStrategy::smartHierarchy(
                                root, changedLeafSet, "fedAsynQ_ImAvg");
                        auto end = chrono::high_resolution_clock::now();
                        adaptTime = chrono::duration<double>(end - start).count();
                    }

                    // Test performance after adaptation
                    auto [_, stepSuccessRate, stepAvgPath] = (name == "A* Oracle" || name == "A* Static")
                                                                 ? AStar::testAStar(
                                                                     *root->maze, size, size, shortestPaths)
                                                                 : TestPolicy::testAgent(root);
                    totalAdaptTime += adaptTime;
                    totalSuccessRate += stepSuccessRate;
                    totalPathLength += stepAvgPath;
                    stepsCompleted++;

                    // Update visualization
                    if (visualize) {
                        if (visualizer) {
                            visualizer->update();
                            visualizer->render();
                            // Brief delay to ensure smooth rendering
                            sf::sleep(sf::milliseconds(500));
                        }
                    }

                    // Write per-step data
                    detailedOut << name << "," << size << "," << diffName << "," << t + 1 << ","
                            << numChanges << "," << adaptTime << "," << stepSuccessRate << ","
                            << stepAvgPath << "\n";

                    // Check if window is still open
                    if (visualize) {
                        if (visualizer && !visualizer->isOpen()) {
                            break;
                        }
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
    ofstream out("results.csv");
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
