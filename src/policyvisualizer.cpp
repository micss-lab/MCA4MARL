#include "policyvisualizer.h"

PolicyVisualizer::PolicyVisualizer(const TreeNode *node, const int size, string approach,
                                   const int max_timesteps) : node_(node), size_(size), approach_(move(approach)),
                                                              max_timesteps_(max_timesteps), current_timestep_(0) {
    // Initialize window (800x800 or scaled for large mazes)
    const int window_size = min(800, size * 20);
    cell_size_ = static_cast<float>(window_size) / size_;
    window_.create(sf::VideoMode(window_size, window_size + 50), "Policy Visualization");
    window_.setFramerateLimit(60);

    // Load font (optional for time step text)
    font_.loadFromFile("arial.ttf"); // Ignore failure for arrows

    // Initialize grid
    grid_.resize(size_ * size_);
    for (int row = 0; row < size_; ++row) {
        for (int col = 0; col < size_; ++col) {
            sf::RectangleShape &cell = grid_[row * size_ + col];
            cell.setSize(sf::Vector2f(cell_size_, cell_size_));
            cell.setPosition(col * cell_size_, row * cell_size_);
            cell.setOutlineThickness(1.0f);
            cell.setOutlineColor(sf::Color(150, 150, 150)); // Gray grid lines
        }
    }
}

// Update visualization for the current time step
void PolicyVisualizer::update() {
    if (current_timestep_ >= max_timesteps_) return;

    const auto &maze = *node_->maze;
    const auto &qTable = *node_->qTable;

    // Update grid colors
    for (int row = 0; row < size_; ++row) {
        for (int col = 0; col < size_; ++col) {
            sf::RectangleShape &cell = grid_[row * size_ + col];
            const int value = maze(row, col);
            if (value == constants::OBSTACLE) {
                cell.setFillColor(sf::Color::Black);
            } else if (value == constants::FREE_SPACE) {
                cell.setFillColor(sf::Color::White);
            } else if (value == constants::CHARGING_STATION) {
                cell.setFillColor(sf::Color::Yellow);
            }
        }
    }

    // Update policy arrows (only for free spaces)
    arrows_.clear();
    for (int row = 0; row < size_; ++row) {
        for (int col = 0; col < size_; ++col) {
            if (maze(row, col) == constants::FREE_SPACE) {
                // Convert global to local indices
                const int localRow = row - node_->startRow;
                const int localCol = col - node_->startCol;
                if (localRow >= 0 && localRow < qTable.getRows() &&
                    localCol >= 0 && localCol < qTable.getCols()) {
                    const auto &q_values = node_->getQValues(row, col, node_->startRow, node_->startCol);
                    int best_action = static_cast<int>(distance(q_values.begin(), ranges::max_element(q_values)));

                    // Create arrow: line + triangular arrowhead
                    auto [dx, dy] = action_arrows_[best_action];
                    float angle = atan2(dy, dx) * 180 / 3.14159;
                    float length = 0.3f * cell_size_; // Line length
                    float center_x = (col + 0.5f) * cell_size_;
                    float center_y = (row + 0.5f) * cell_size_;

                    // Line body (rectangle)
                    sf::RectangleShape line(sf::Vector2f(length, 0.03f * cell_size_));
                    line.setOrigin(0.0f, 0.015f * cell_size_);
                    line.setPosition(center_x - length / 2 * dx / 0.3f, center_y - length / 2 * dy / 0.3f);
                    line.setRotation(angle);
                    line.setFillColor(sf::Color::Red);
                    line.setOutlineColor(sf::Color::Black);
                    line.setOutlineThickness(0.5f);

                    // Arrowhead (triangle via CircleShape with 3 points)
                    sf::CircleShape arrowhead(0.1f * cell_size_, 3); // Radius ~4px for 50x50
                    arrowhead.setOrigin(0.1f * cell_size_, 0.1f * cell_size_);
                    arrowhead.setPosition(center_x + length / 2 * dx / 0.35f, center_y + length / 2 * dy / 0.35f);
                    arrowhead.setRotation(angle - 30.0f);
                    arrowhead.setFillColor(sf::Color::Red);
                    arrowhead.setOutlineColor(sf::Color::Black);
                    arrowhead.setOutlineThickness(0.5f);

                    arrows_.emplace_back(line, arrowhead);
                }
            }
        }
    }

    ++current_timestep_;
}

// Render the visualization
void PolicyVisualizer::render() {
    window_.clear(sf::Color::White);

    // Draw grid
    for (const auto &cell: grid_) {
        window_.draw(cell);
    }

    // Draw arrows
    for (const auto &[line, arrowhead]: arrows_) {
        window_.draw(line);
        window_.draw(arrowhead);
    }

    // Draw time step text (if font loaded)
    if (!font_.getInfo().family.empty()) {
        sf::Text text;
        text.setFont(font_);
        text.setString("Time Step: " + to_string(current_timestep_) + " - " + approach_);
        text.setCharacterSize(20);
        text.setFillColor(sf::Color::Black);
        text.setPosition(10, size_ * cell_size_ + 10);
        window_.draw(text);
    }

    window_.display();
}

// Handle events and check if window is open
bool PolicyVisualizer::isOpen() {
    sf::Event event{};
    while (window_.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window_.close();
        }
    }
    return window_.isOpen();
}
