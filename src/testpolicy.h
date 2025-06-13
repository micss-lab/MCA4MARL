#ifndef TESTPOLICY_H
#define TESTPOLICY_H

#include <chrono>
#include <future>

#include "threadresult.h"
#include "treenode.h"

class TestPolicy {
public:
    static tuple<double, double, double> testAgent(const TreeNode *root);
};


#endif //TESTPOLICY_H
