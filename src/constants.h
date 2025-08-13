#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace constants {
    // Cell types
    constexpr int OBSTACLE = 0;
    constexpr int FREE_SPACE = 1;
    constexpr int CHARGING_STATION = 2;

    // Action count
    constexpr int ACTION_COUNT = 8; // Number of possible actions (up, down, left, right, and 4 diagonals)

    // Learning parameters
    constexpr int EPISODE_COUNT = 10'000;
    constexpr double LEARNING_RATE = 0.4;
    constexpr double DISCOUNT_FACTOR = 0.9;
} // namespace constants

#endif //CONSTANTS_H
