#ifndef MULTIAGENT_H
#define MULTIAGENT_H

#include <thread>

#include "treenode.h"

class MultiAgent {
public:
    static void fedAsynQ_EqAvg(TreeNode *node, const Maze &maze, int tau, int T, int K);

    static void fedAsynQ_ImAvg(TreeNode *node, const Maze &maze, int tau, int T, int K);
};


#endif //MULTIAGENT_H
