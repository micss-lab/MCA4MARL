#ifndef HASHPAIR_H
#define HASHPAIR_H

#include <functional>

using namespace std;

class HashPair {
public:
    size_t operator()(const pair<int, int> &p) const;
};

#endif //HASHPAIR_H
