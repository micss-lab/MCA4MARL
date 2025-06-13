#ifndef ASTAR_H
#define ASTAR_H

#include <chrono>
#include <future>
#include <unordered_map>
#include <unordered_set>
#include <queue>

#include "constants.h"
#include "hashpair.h"
#include "maze.h"
#include "threadresult.h"

struct AStarNode {
    int x, y, g, h;
    bool operator>(const AStarNode &other) const { return (g + h) > (other.g + h); }
};

class AStar {
public:
    static int heuristic(int x1, int y1, int x2, int y2);

    static vector<pair<int, int> > reconstructPath(unordered_map<pair<int, int>, pair<int, int>, HashPair> &cameFrom,
                                                   int startX, int startY, int goalX, int goalY);

    static unordered_map<pair<int, int>, vector<pair<int, int> >, HashPair> computeAllShortestPaths(const Maze &maze);

    static tuple<double, double, double> testAStar(const Maze &maze, int rows, int cols,
                                                   const unordered_map<pair<int, int>, vector<pair<int, int> >,
                                                       HashPair> &shortestPaths);
};


#endif //ASTAR_H
