#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

struct Flight {
    std::string destination;
    double cost;
    double duration;
    double distance;
};

struct Airport {
    std::string code;
    std::string city;
    std::string country;
    double latitude;
    double longitude;
};

struct CandidateEdge {
    std::string from;
    std::string to;
    double weight;
};

struct PathResult {
    double total;
    std::vector<std::string> path;
    bool found;
};

struct ReachableResult {
    std::vector<std::string> reachable_airports;
    bool found;
};

using AdjList = std::unordered_map<std::string, std::vector<Flight>>;

// ======================== Graph Class ======================================


class Graph {
private:

    AdjList directed_adj;
    AdjList undirected_adj;

    std::unordered_map<std::string, int> disc;
    std::unordered_map<std::string, int> low;
    std::unordered_map<std::string, std::string> parent;

    int time;

    // Private helpers
    AdjList make_adj_list(const std::string& filename, bool directed);
    std::vector<std::string> bfs(const std::string& src, int K);
    void dfs(const std::string& node,
             std::unordered_set<std::string>& visited,
             std::unordered_set<std::string>& ap);

public:
    Graph(const std::string& routes_csv);

    PathResult Dijkstra(const std::string& src, const std::string& dst, const std::string& weight);

    ReachableResult Reachable(const std::string& src, int K);

    std::vector<std::string> ArticulationPoints();

    std::vector<CandidateEdge> Prim(const std::string& weight_type);

    std::vector<std::string> budgetLimited(const std::string& src, const double& budget);
};

#endif // GRAPH_H