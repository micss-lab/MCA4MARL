#ifndef TABLE_H
#define TABLE_H

#include <vector>

using namespace std;

template<typename T>
class Table : vector<vector<vector<T> > > {
public:
    Table(int rows, int cols, int actions);

    Table(const Table<T> &other);

    vector<T> &operator()(int globalRow, int globalCol, int startRow, int startCol);

    [[nodiscard]] int getRows() const;

    [[nodiscard]] int getCols() const;
};

#endif //TABLE_H
