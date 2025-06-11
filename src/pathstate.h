#ifndef PATHSTATE_H
#define PATHSTATE_H

#include <vector>

using namespace std;

struct PathState {
    int x, y, steps;
    vector<pair<int, int> > path;
};


#endif //PATHSTATE_H
