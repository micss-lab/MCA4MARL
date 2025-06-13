#include "testpolicy.h"

tuple<double, double, double> TestPolicy::testAgent(const TreeNode *root) {
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
            auto [success, steps, path] = root->findValidPath(x1, y1, maxStepsPerEpisode);
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
