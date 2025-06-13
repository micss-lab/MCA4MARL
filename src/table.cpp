#include "table.h"

template<typename T>
Table<T>::Table(const int rows, const int cols, const int actions) : vector<vector<vector<T> > >(
    rows, vector<vector<T> >(cols, vector<T>(actions, T(0)))) {
}

template<typename T>
Table<T>::Table(const Table<T> &other) : vector<vector<vector<T> > >(other) {
}

template<typename T>
vector<T> &Table<T>::operator()(const int globalRow, const int globalCol, const int startRow, const int startCol) {
    return (*this)[globalRow - startRow][globalCol - startCol];
}

template<typename T>
int Table<T>::getRows() const {
    return this->size();
}

template<typename T>
int Table<T>::getCols() const {
    return this->empty() ? 0 : (*this)[0].size();
}

// Explicit template instantiations for int and double
template class Table<int>;
template class Table<double>;
