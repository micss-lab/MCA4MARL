#include "singleagent.h"

Experience::Experience(const int x1, const int y1, int const action, const double reward, const int x2,
                       const int y2) : x1(x1), y1(y1), action(action), reward(reward), x2(x2), y2(y2) {
}

tuple<int, int, int, double, int, int> Experience::getValues() const {
    return make_tuple(x1, y1, action, reward, x2, y2);
}

SingleAgentTraining::SingleAgentTraining(TreeNode *node, const Maze &maze, const int rows, const int cols,
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
                    const auto &[x1, y1, action, reward, x2, y2] = replayBuffer[idx].getValues();
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
