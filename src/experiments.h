#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

#include <chrono>
#include <fstream>
#include <map>

#include "astar.h"
#include "policyvisualizer.h"
#include "testpolicy.h"
#include "treenode.h"
#include "treestrategy.h"

struct Metrics {
    double initialTime;
    double adaptTime;
    double successRate;
    double avgPathLength;
};

class Experiments {
public:
    static void simulateEnvironmentChanges(const TreeNode *root, int numSteps,
                                           vector<pair<int, int> > &changedPositions);

    static void runFullExperiment(bool visualize);
};


#endif //EXPERIMENTS_H
