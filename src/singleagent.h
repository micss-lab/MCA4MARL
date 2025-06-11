#ifndef SINGLEAGENT_H
#define SINGLEAGENT_H

#include "treenode.h"

class Experience {
public:
    Experience(int x1, int y1, int action, double reward, int x2, int y2);

    [[nodiscard]] tuple<int, int, int, double, int, int> getValues() const;

private:
    int x1, y1, action;
    double reward;
    int x2, y2;
};

class SingleAgentTraining {
public:
    SingleAgentTraining(TreeNode *node, const Maze &maze, int rows, int cols, int startRow,
                        int startCol, int endRow, int endCol, int maxStepsPerEpisode);
};


#endif //SINGLEAGENT_H
