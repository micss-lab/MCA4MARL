#ifndef POLICYVISUALIZER_H
#define POLICYVISUALIZER_H

#include <SFML/Graphics.hpp>

#include "treenode.h"

class PolicyVisualizer {
public:
    PolicyVisualizer(const TreeNode *node, int size, string approach, int max_timesteps);

    void update();

    void render();

    bool isOpen();

private:
    sf::RenderWindow window_;
    const TreeNode *node_;
    int size_;
    string approach_;
    int max_timesteps_;
    int current_timestep_;
    float cell_size_;
    vector<sf::RectangleShape> grid_;
    vector<pair<sf::RectangleShape, sf::CircleShape> > arrows_;
    sf::Font font_;

    // Action to arrow direction (dx, dy) for visualization
    const vector<pair<float, float> > action_arrows_ = {
        {0.0f, -0.3f}, // 0: Up
        {0.3f, -0.3f}, // 1: Up-right
        {0.3f, 0.0f}, // 2: Right
        {0.3f, 0.3f}, // 3: Down-right
        {0.0f, 0.3f}, // 4: Down
        {-0.3f, 0.3f}, // 5: Down-left
        {-0.3f, 0.0f}, // 6: Left
        {-0.3f, -0.3f} // 7: Up-left
    };
};

#endif //POLICYVISUALIZER_H
