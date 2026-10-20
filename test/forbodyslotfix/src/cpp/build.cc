#include "build.h"

Edge* Plan::FindWork() {
    if (ready_.empty())
        return nullptr;
    Edge* e = ready_.back();
    ready_.pop_back();
    return e;
}

void Plan::EdgeFinished(Edge* edge, bool success) {
    edge->outputs_ready = success;
}

bool Builder::AddTarget(const std::string& name, std::string* err) {
    if (name.empty()) {
        *err = "unknown target ''";
        return false;
    }
    return true;
}

bool Builder::RunCommand(Edge* edge) {
    return !edge->command.empty();
}

// Run every ready edge of the plan; stop once failures_allowed commands have failed.
bool Builder::Build(std::string* err) {
    int failures = 0;
    while (plan_.more_to_do()) {
        Edge* edge = plan_.FindWork();
        if (edge == nullptr) {
            *err = "stuck [this is a bug]: cannot make progress";
            return false;
        }
        bool ok = RunCommand(edge);
        plan_.EdgeFinished(edge, ok);
        if (!ok && ++failures >= failures_allowed_) {
            *err = failures_allowed_ > 1 ? "subcommands failed" : "subcommand failed";
            return false;
        }
    }
    return true;
}
