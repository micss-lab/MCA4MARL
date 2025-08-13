#ifndef TREESTRATEGY_H
#define TREESTRATEGY_H

#include <thread>
#include <unordered_set>

#include "multiagent.h"
#include "singleagent.h"
#include "treenode.h"

using namespace std;

class TreeStrategy {
public:
    static void trainTreeNodesInParallel(const TreeNode *root, const vector<TreeNode *> &nodes,
                                         const string &trainingMode);

    static void trainTreeNodesSequentially(const TreeNode *root, const vector<TreeNode *> &nodes,
                                           const string &trainingMode);

    static void trainTreeNodes(const TreeNode *root, const vector<TreeNode *> &nodes, const bool &parallel,
                               const string &trainingMode);

    static void onlyTrainLeafNodes(TreeNode *root, const vector<TreeNode *> &changedLeaves = {});

    static double getRetrainingThreshold(int mazeSize);

    static void smartHierarchy(TreeNode *root, const vector<TreeNode *> &changedLeaves = {},
                               const string &trainingMode = "singleAgent");
};


#endif //TREESTRATEGY_H
