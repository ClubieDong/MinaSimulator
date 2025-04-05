#pragma once

class MeanStdTracker {
private:
    unsigned int m_Count = 0;
    double m_Mean = 0.0;
    double m_M2 = 0.0;

public:
    void Update(double value);
    unsigned int Count() const { return m_Count; }
    double Mean() const { return m_Mean; }
    double Std() const;
};
