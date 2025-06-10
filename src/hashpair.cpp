#include "hashpair.h"

size_t HashPair::operator()(const pair<int, int> &p) const {
    return hash<int>()(p.first) ^ (hash<int>()(p.second) << 1);
}
