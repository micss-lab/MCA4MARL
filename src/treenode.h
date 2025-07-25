#ifndef TREENODE_H
#define TREENODE_H

#include <algorithm>
#include <iostream>
#include <memory>
#include <set>
#include <vector>
#include <stack>
#include <queue>

#include "constants.h"
#include "maze.h"
#include "pathstate.h"
#include "table.h"

using namespace std;

class TreeNode {
public:
    unique_ptr<Maze> maze; // Optional maze, only at root
    unique_ptr<Table<double> > qTable; // 3D array Q-table
    TreeNode *parent;
    vector<TreeNode *> children;
    int rows, cols;
    int startRow, startCol, endRow, endCol;
    int chargingStationCount;
    double baselineSuccessRate;

    // Constructor
    TreeNode(const Maze &fullMaze, int rows, int cols, int startRow, int startCol, int endRow, int endCol,
             TreeNode *parent = nullptr, bool isRoot = false);

    // Destructor
    ~TreeNode();

    // Initialize 3D Q-table array
    void initQTable();

    [[nodiscard]] vector<double> &getQValues(int globalRow, int globalCol, int startRow, int startCol) const;

    // Print tree structure
    void printTree(const string &prefix = "", bool isLast = true, bool isRoot = true) const;

    [[nodiscard]] int countChargingStations(const Maze &fullMaze) const;

    void addChild(TreeNode *child);

    // Find leaf sub-environment for a given position
    TreeNode *findSubEnvironment(int row, int col);

    void updateQTable(int x1, int y1, int action, double reward, int x2, int y2) const;

    [[nodiscard]] int selectAction(int x, int y, double epsilon) const;

    static vector<int> selectTopKActions(const vector<double> &qValues, int rows, int cols, int x, int y, int k);

    [[nodiscard]] tuple<bool, int, vector<pair<int, int> > > findValidPath(int startX, int startY, int maxSteps) const;

    void createSubEnvironments(const Maze &maze);

    void propagateQTableDownwards();

    void propagateQTableUpwards() const;

    void collectLeafNodes(vector<TreeNode *> &leafNodes);

    double computeSuccessRate(const TreeNode *root) const;
};

#endif //TREENODE_H
