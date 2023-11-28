#include "trace.hpp"
#include "job.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

static nlohmann::json RecordedTraces;

void Trace::RecordEvent(std::string &&name, const char *category, bool isBegin, unsigned int pid, unsigned int tid,
                        double time) {
    if (!EnableRecording)
        return;
    RecordedTraces.push_back({
        {"name", name},
        {"cat", category},
        {"ph", isBegin ? "B" : "E"},
        {"pid", pid},
        {"tid", tid},
        {"ts", time * 1'000'000},
    });
}

void Trace::Flush(const char *fileName) {
    if (!EnableRecording)
        return;
    std::ofstream file(fileName);
    file << RecordedTraces.dump();
}

static std::string GetEventNameFromJob(const Job &job, bool includeStep = false, bool includeGroup = false,
                                       bool includeCommOp = false, bool includeTransmission = false,
                                       bool includeWaiting = false) {
    std::string name = "Job #" + std::to_string(job.ID);
    if (includeStep)
        name += " Step #" + std::to_string(job.GetCurrentStepIdx());
    if (includeGroup)
        name += " Group #" + std::to_string(job.GetCurrentGroupIdx());
    if (includeCommOp)
        name += " CommOp #" + std::to_string(job.GetCurrentOpIdx());
    if (includeTransmission)
        name += " Transmission(" + std::to_string(job.GetCurrentOpTransmittedMessageSize()) + "B~" +
                std::to_string(job.GetCurrentOpTransmittedMessageSize() + job.GetTransmittingMessageSize()) + "B)";
    if (includeWaiting)
        name += " Waiting";
    return name;
}

void Trace::RecordBeginJob(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job), "Job", true, 0, job.ID, time);
}

void Trace::RecordEndJob(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job), "Job", false, 0, job.ID, time);
}

void Trace::RecordBeginStep(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true), "Step", true, 0, job.ID, time);
}

void Trace::RecordEndStep(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true), "Step", false, 0, job.ID, time);
}

void Trace::RecordBeginGroup(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true, true), "Group", true, 0, job.ID, time);
}

void Trace::RecordEndGroup(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true, true), "Group", false, 0, job.ID, time);
}

void Trace::RecordBeginCommOp(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true, true, true), "CommOp", true, 0, job.ID, time);
}

void Trace::RecordEndCommOp(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true, true, true), "CommOp", false, 0, job.ID, time);
}

void Trace::RecordBeginTransmission(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true, true, true, true), "Transmission", true, 0, job.ID, time);
}

void Trace::RecordEndTransmission(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true, true, true, true), "Transmission", false, 0, job.ID, time);
}

void Trace::RecordBeginWaiting(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true, true, true, false, true), "Waiting", true, 0, job.ID, time);
}

void Trace::RecordEndWaiting(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true, true, true, false, true), "Waiting", false, 0, job.ID, time);
}
