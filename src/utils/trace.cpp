#include "trace.hpp"
#include "job.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

std::string Tracer::GetEventNameFromJob(const Job &job, bool includeStep, bool includeGroup, bool includeCommOp,
                                        bool includeTransmission, bool includeWaiting) const {
    std::string name = "Job #" + std::to_string(job.ID);
    if (includeStep)
        name += " Step #" + std::to_string(job.GetCurrentStepIdx());
    if (includeGroup)
        name += " Group #" + std::to_string(job.GetCurrentGroupIdx());
    if (includeCommOp)
        name += " CommOp #" + std::to_string(job.GetCurrentOpIdx());
    if (includeTransmission)
        name += (job.IsUsingSharp() ? " Transmission(SHARP, " : " Transmission(Non-SHARP, ") +
                std::to_string(job.GetCurrentOpTransmittedMessageSize()) + "B~" +
                std::to_string(job.GetCurrentOpTransmittedMessageSize() + job.GetTransmittingMessageSize()) + "B)";
    if (includeWaiting)
        name += " Waiting";
    return name;
}

Tracer::~Tracer() {
    if (!EnableRecording)
        return;
    std::ofstream file("results/trace.json");
    file << m_RecordedTraces;
}

void Tracer::RecordEvent(std::string &&name, const char *category, bool isBegin, unsigned int pid, unsigned int tid,
                         double time) {
    if (!EnableRecording)
        return;
    m_RecordedTraces.push_back({
        {"name", name},
        {"cat", category},
        {"ph", isBegin ? "B" : "E"},
        {"pid", pid},
        {"tid", tid},
        {"ts", time * 1'000'000},
    });
}

void Tracer::RecordBeginJob(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job), "Job", true, 0, job.ID, time);
}

void Tracer::RecordEndJob(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job), "Job", false, 0, job.ID, time);
}

void Tracer::RecordBeginStep(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true), "Step", true, 0, job.ID, time);
}

void Tracer::RecordEndStep(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true), "Step", false, 0, job.ID, time);
}

void Tracer::RecordBeginGroup(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true, true), "Group", true, 0, job.ID, time);
}

void Tracer::RecordEndGroup(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true, true), "Group", false, 0, job.ID, time);
}

void Tracer::RecordBeginCommOp(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true, true, true), "CommOp", true, 0, job.ID, time);
}

void Tracer::RecordEndCommOp(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true, true, true), "CommOp", false, 0, job.ID, time);
}

void Tracer::RecordBeginTransmission(double time, const Job &job) {
    auto category = job.IsUsingSharp() ? "Transmission,SHARP" : "Transmission,NonSHARP";
    RecordEvent(GetEventNameFromJob(job, true, true, true, true), category, true, 0, job.ID, time);
}

void Tracer::RecordEndTransmission(double time, const Job &job) {
    auto category = job.IsUsingSharp() ? "Transmission,SHARP" : "Transmission,NonSHARP";
    RecordEvent(GetEventNameFromJob(job, true, true, true, true), category, false, 0, job.ID, time);
}

void Tracer::RecordBeginWaiting(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true, true, true, false, true), "Waiting", true, 0, job.ID, time);
}

void Tracer::RecordEndWaiting(double time, const Job &job) {
    RecordEvent(GetEventNameFromJob(job, true, true, true, false, true), "Waiting", false, 0, job.ID, time);
}

Tracer Trace;
