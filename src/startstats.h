#ifndef STARTSTATS_H
#define STARTSTATS_H

class StartStats {
public:
    StartStats();

    [[nodiscard]] double getSuccessRate() const;

    void incrementAttempts();

    void incrementSuccesses();

private:
    int attempts;
    int successes;
};

#endif //STARTSTATS_H
