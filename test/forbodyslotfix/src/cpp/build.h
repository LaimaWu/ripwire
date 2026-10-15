// Builder: runs the edges of a build plan until the requested targets are up to date.
#pragma once
#include <string>
#include <vector>

struct Edge {
    std::string command;
    std::vector<std::string> outputs;
    bool outputs_ready = false;
};

class Plan {
public:
    bool more_to_do() const { return !ready_.empty(); }
    Edge* FindWork();
    void EdgeFinished(Edge* edge, bool success);
private:
    std::vector<Edge*> ready_;
};

class Builder {
public:
    explicit Builder(int failures_allowed) : failures_allowed_(failures_allowed) {}
    bool AddTarget(const std::string& name, std::string* err);
    // Build the plan: run every ready edge's commands until the targets are up to date;
    // the builder stops once failures_allowed commands have failed.
    bool Build(std::string* err);
private:
    Plan plan_;
    int failures_allowed_;
    bool RunCommand(Edge* edge);
};
