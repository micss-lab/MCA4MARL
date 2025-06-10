#include "startstats.h"

StartStats::StartStats() : attempts(0), successes(0) {
}

double StartStats::getSuccessRate() const {
    return attempts > 0 ? static_cast<double>(successes) / attempts : 0.0;
}

void StartStats::incrementAttempts() {
    attempts++;
}

void StartStats::incrementSuccesses() {
    successes++;
}
