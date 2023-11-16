#pragma once

#include <string>

class Job;

class Trace {
public:
    static void RecordEvent(std::string &&name, const char *category, bool isBegin, unsigned int pid, unsigned int tid,
                            double time);
    static void Flush(const char *fileName);

    static void RecordBeginJob(double time, const Job &job);
    static void RecordEndJob(double time, const Job &job);
    static void RecordBeginStep(double time, const Job &job);
    static void RecordEndStep(double time, const Job &job);
    static void RecordBeginGroup(double time, const Job &job);
    static void RecordEndGroup(double time, const Job &job);
    static void RecordBeginCommOp(double time, const Job &job);
    static void RecordEndCommOp(double time, const Job &job);
    static void RecordBeginTransmission(double time, const Job &job);
    static void RecordEndTransmission(double time, const Job &job);
    static void RecordBeginWaiting(double time, const Job &job);
    static void RecordEndWaiting(double time, const Job &job);
};
