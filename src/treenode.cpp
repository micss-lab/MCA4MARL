#include "treenode.h"

TreeNode::TreeNode(const Maze &fullMaze, const int rows, const int cols, const int startRow, const int startCol,
                   const int endRow, const int endCol, TreeNode *parent, const bool isRoot) : parent(parent),
    rows(rows), cols(cols), startRow(startRow), startCol(startCol), endRow(endRow), endCol(endCol),
    baselineSuccessRate(-1.0) {
    if (isRoot) {
        maze = make_unique<Maze>(fullMaze);
        initQTable();
    }
    chargingStationCount = countChargingStations(fullMaze);
}

// Destructor
TreeNode::~TreeNode() {
    for (const TreeNode *child: children) {
        delete child;
    }
    children.clear();
}

// Initialize 3D Q-table array
void TreeNode::initQTable() {
    if (!qTable) {
        const int localRows = endRow - startRow + 1;
        const int localCols = endCol - startCol + 1;
        qTable = make_unique<Table<double> >(localRows, localCols, constants::ACTION_COUNT);
    }
}

vector<double> &TreeNode::getQValues(const int globalRow, const int globalCol, const int startRow,
                                     const int startCol) const {
    // Return the reference to the Q-values for the specified position
    return (*qTable)(globalRow, globalCol, startRow, startCol);
}

void TreeNode::printTree(const string &prefix, const bool isLast, const bool isRoot) const {
    // For the root node, don't add any symbols
    if (isRoot) {
        cout << "Node: Start(" << startRow << ", " << startCol << "), "
                << "End(" << endRow << ", " << endCol << "), "
                << "Size(" << (endRow - startRow + 1) << "x" << (endCol - startCol + 1)
                << "), " << "Charging Stations: " << chargingStationCount << "\n";
    } else {
        // For all other nodes, add the appropriate symbols
        const string currentPrefix = prefix + (isLast ? "└─ " : "├─ ");
        cout << currentPrefix
                << "Node: Start(" << startRow << ", " << startCol << "), "
                << "End(" << endRow << ", " << endCol << "), "
                << "Size(" << (endRow - startRow + 1) << "x" << (endCol - startCol + 1)
                << "), " << "Charging Stations: " << chargingStationCount << "\n";
    }

    // Adjust prefix for children
    string childPrefix;
    if (isRoot) {
        childPrefix = " ";
    } else {
        childPrefix = prefix + (isLast ? "    " : "│   ");
    }

    // Traverse children
    for (size_t i = 0; i < children.size(); ++i) {
        children[i]->printTree(childPrefix, i == children.size() - 1, false);
    }
}

// Count charging stations in the subenvironment
[[nodiscard]] int TreeNode::countChargingStations(const Maze &fullMaze) const {
    int count = 0;
    for (int i = startRow; i <= endRow; ++i) {
        for (int j = startCol; j <= endCol; ++j) {
            if (fullMaze(i, j) == constants::CHARGING_STATION) {
                count++;
            }
        }
    }
    return count;
}

void TreeNode::addChild(TreeNode *child) {
    children.push_back(child);
}

// Find leaf sub-environment for a given position
TreeNode *TreeNode::findSubEnvironment(const int row, const int col) {
    if (row < startRow || row > endRow || col < startCol || col > endCol) {
        return nullptr;
    }
    if (children.empty()) {
        return this;
    }
    for (TreeNode *child: children) {
        TreeNode *result = child->findSubEnvironment(row, col);
        if (result) return result;
    }
    return nullptr;
}

void TreeNode::updateQTable(const int x1, const int y1, const int action, const double reward, const int x2,
                            const int y2) const {
    // Ensure node and qTable exist
    if (!qTable) return;

    // Access Q-values for current state (x1, y1)
    vector<double> &qValues = getQValues(x1, y1, startRow, startCol);

    // Access Q-values for next state (x2, y2)
    const vector<double> &nextQValues = getQValues(x2, y2, startRow, startCol);

    // Get the maximum Q-value for the next state
    const double maxQNext = *ranges::max_element(nextQValues);

    // Update the Q-value for the current state and action
    qValues[action] += constants::LEARNING_RATE * (reward + constants::DISCOUNT_FACTOR * maxQNext - qValues[action]);
}

int TreeNode::selectAction(const int x, const int y, const double epsilon) const {
    const double randomValue = static_cast<double>(rand()) / RAND_MAX;

    // Define possible moves
    const vector<pair<int, int> > moves = {
        {-1, 0}, // N
        {-1, 1}, // NE
        {0, 1}, // E
        {1, 1}, // SE
        {1, 0}, // S
        {1, -1}, // SW
        {0, -1}, // W
        {-1, -1} // NW
    };

    // Filter valid actions based on boundaries of the subenvironment
    vector<int> validActions;
    for (int i = 0; i < constants::ACTION_COUNT; ++i) {
        const int newX = x + moves[i].first;
        const int newY = y + moves[i].second;
        if (newX >= startRow && newX <= endRow && newY >= startCol && newY <= endCol) {
            validActions.push_back(i);
        }
    }

    // Epsilon-greedy action selection
    int action;
    if (randomValue < epsilon) {
        // Exploration: Choose a random valid action
        action = validActions[rand() % validActions.size()];
    } else {
        // Exploitation: Choose the action with the highest Q-value among valid actions
        const vector<double> &qValues = getQValues(x, y, startRow, startCol);
        action = validActions[0];
        double maxQValue = qValues[validActions[0]];
        for (const int i: validActions) {
            if (qValues[i] > maxQValue) {
                maxQValue = qValues[i];
                action = i;
            }
        }
    }
    return action;
}
