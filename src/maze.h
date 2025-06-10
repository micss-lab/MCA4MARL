#ifndef MAZE_H
#define MAZE_H

#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

#include "constants.h"
#include "hashpair.h"
#include "startstats.h"

using namespace std;

class Maze : vector<vector<int> > {
public:
    Maze(int rows, int cols, double freeSpaceProb, double obstacleProb, double chargingStationProb);

    int operator()(int row, int col) const;

    void operator()(int row, int col, int value);

    void printMaze() const;

    [[nodiscard]] int getRows() const;

    [[nodiscard]] int getCols() const;

    [[nodiscard]] bool checkExit(int x, int y) const;

    [[nodiscard]] pair<int, int> selectFirstPlace(int startRow, int startCol, int endRow, int endCol) const;

    pair<int, int> selectFirstPlace(int startRow, int startCol, int endRow, int endCol, int counter,
                                    const unordered_map<pair<int, int>, StartStats, HashPair> &startStats,
                                    mt19937 &rng) const;

    [[nodiscard]] tuple<int, int, int, double> performAction(int rows, int cols, int x1, int y1, int action) const;

    [[nodiscard]] vector<pair<int, int> > getObstaclePositions() const;
};

/***********************/
/*     -----------     */
/*    | 7 | 0 | 1 |    */
/*     -----------     */
/*    | 6 | X | 2 |    */
/*     -----------     */
/*    | 5 | 4 | 3 |    */
/*     -----------     */
/*                     */
/*      0: Move N      */
/*      1: Move NE     */
/*      2: Move E      */
/*      3: Move SE     */
/*      4: Move S      */
/*      5: Move SW     */
/*      6: Move W      */
/*      7: Move NW     */
/*                     */
/***********************/

#endif //MAZE_H
