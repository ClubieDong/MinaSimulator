#include "mean_std_tracker.hpp"
#include <cmath>

void MeanStdTracker::Update(double value) {
    if (std::isnan(value))
        return;
    ++m_Count;
    double delta1 = value - m_Mean;
    m_Mean += delta1 / m_Count;
    double delta2 = value - m_Mean;
    m_M2 += delta1 * delta2;
}

double MeanStdTracker::Std() const {
    if (m_Count == 0)
        return 0.0;
    return std::sqrt(m_M2 / m_Count);
}
