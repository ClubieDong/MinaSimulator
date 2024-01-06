#pragma once

#include <nlohmann/json.hpp>
#include <string>

class Job;

class Tracer {
private:
    nlohmann::json m_RecordedTraces;

    std::string GetEventNameFromJob(const Job &job, bool includeStep = false, bool includeGroup = false,
                                    bool includeCommOp = false, bool includeTransmission = false,
                                    bool includeWaiting = false) const;

public:
    bool EnableRecording = false;

    ~Tracer();

    void RecordEvent(std::string &&name, const char *category, bool isBegin, unsigned int pid, unsigned int tid,
                     double time);

    void RecordBeginJob(double time, const Job &job);
    void RecordEndJob(double time, const Job &job);
    void RecordBeginStep(double time, const Job &job);
    void RecordEndStep(double time, const Job &job);
    void RecordBeginGroup(double time, const Job &job);
    void RecordEndGroup(double time, const Job &job);
    void RecordBeginCommOp(double time, const Job &job);
    void RecordEndCommOp(double time, const Job &job);
    void RecordBeginTransmission(double time, const Job &job);
    void RecordEndTransmission(double time, const Job &job);
    void RecordBeginWaiting(double time, const Job &job);
    void RecordEndWaiting(double time, const Job &job);
};

extern Tracer Trace;
