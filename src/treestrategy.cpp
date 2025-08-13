#include "treestrategy.h"

void TreeStrategy::trainTreeNodesInParallel(const TreeNode *root, const vector<TreeNode *> &nodes,
                                            const string &trainingMode) {
    vector<thread> threads;
    for (TreeNode *node: nodes) {
        threads.emplace_back([root, node, trainingMode]() {
            if (trainingMode == "singleAgent") {
                const int maxSteps = (node->endRow - node->startRow + 1) + (node->endCol - node->startCol + 1);
                SingleAgentTraining(node, *root->maze, node->rows, node->cols, node->startRow, node->startCol,
                                    node->endRow, node->endCol, maxSteps);
            } else if (trainingMode == "fedAsynQ_EqAvg") {
                const int T = (node->endRow - node->startRow + 1) * (node->endCol - node->startCol + 1) * 200;
                MultiAgent::fedAsynQ_EqAvg(node, *root->maze, 1000, T, 12);
            } else if (trainingMode == "fedAsynQ_ImAvg") {
                const int T = (node->endRow - node->startRow + 1) * (node->endCol - node->startCol + 1) * 200;
                MultiAgent::fedAsynQ_ImAvg(node, *root->maze, 1000, T, 12);
            }

            // Propagate the Q-table results upwards
            node->propagateQTableUpwards();
            node->propagateQTableDownwards();
        });
    }
    for (thread &t: threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void TreeStrategy::trainTreeNodesSequentially(const TreeNode *root, const vector<TreeNode *> &nodes,
                                              const string &trainingMode) {
    for (TreeNode *node: nodes) {
        if (trainingMode == "singleAgent") {
            const int maxSteps = (node->endRow - node->startRow + 1) + (node->endCol - node->startCol + 1);
            SingleAgentTraining(node, *root->maze, node->rows, node->cols, node->startRow, node->startCol, node->endRow,
                                node->endCol, maxSteps);
        } else if (trainingMode == "fedAsynQ_EqAvg") {
            const int T = (node->endRow - node->startRow + 1) * (node->endCol - node->startCol + 1) * 200;
            MultiAgent::fedAsynQ_EqAvg(node, *root->maze, 1000, T, 12);
        } else if (trainingMode == "fedAsynQ_ImAvg") {
            const int T = (node->endRow - node->startRow + 1) * (node->endCol - node->startCol + 1) * 200;
            MultiAgent::fedAsynQ_ImAvg(node, *root->maze, 1000, T, 12);
        }

        // Propagate the Q-table results upwards
        node->propagateQTableUpwards();
        node->propagateQTableDownwards();
    }
}

void TreeStrategy::trainTreeNodes(const TreeNode *root, const vector<TreeNode *> &nodes, const bool &parallel,
                                  const string &trainingMode) {
    if (parallel) {
        // Train the nodes in parallel
        trainTreeNodesInParallel(root, nodes, trainingMode);
    } else {
        // Train the nodes sequentially
        trainTreeNodesSequentially(root, nodes, trainingMode);
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
                const double newSuccessRate = current->computeSuccessRate(root);
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


void TreeStrategy::onlyTrainLeafNodes(TreeNode *root, const vector<TreeNode *> &changedLeaves) {
    // Environment is static. Performing global path planning selectively
    if (changedLeaves.empty()) {
        // Train all leaf nodes initially
        vector<TreeNode *> leafNodes;
        root->collectLeafNodes(leafNodes);
        trainTreeNodes(root, leafNodes, true, "singleAgent");
    }
    // Environment changed. Training affected leaf nodes
    else {
        trainTreeNodes(root, changedLeaves, true, "singleAgent");
    }
}

double TreeStrategy::getRetrainingThreshold(const int mazeSize) {
    return 0.01;
}

void TreeStrategy::smartHierarchy(TreeNode *root, const vector<TreeNode *> &changedLeaves, const string &trainingMode) {
    if (!root) return; // Safety check: Exit if root is null

    cout << "\nBegin training...\n";

    // Determine initial training
    const bool isInitialTraining = changedLeaves.empty();

    // Step 1: Collect leaf nodes to train
    vector<TreeNode *> leafNodesToTrain;
    if (isInitialTraining) {
        // Initial training: Gather all leaf nodes in the hierarchy
        root->collectLeafNodes(leafNodesToTrain);
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
                const double newSuccessRate = leaf->computeSuccessRate(root);

                cout << "Leaf (" << leaf->startRow << ", " << leaf->startCol << ") -> (" << leaf->endRow << ", " << leaf
                        ->endCol << ") Success Rate: " << newSuccessRate * 100 << "%\n";

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
        trainTreeNodes(root, leavesToRetrain, true, trainingMode);
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
                    const double newSuccessRate = node->computeSuccessRate(root);

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
                trainTreeNodes(root, nodesToTrain, true, trainingMode);
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
