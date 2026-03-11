#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include <queue>
#include <functional>
#include <utility>
#include <unordered_set>
#include <limits> // Added for std::numeric_limits

#include "httplib.h"  // Ensure httplib.h is in the same folder

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

std::vector<Airport> load_airports(const std::string& filename) {
    std::vector<Airport> airports;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return airports;
    }

    // Skip header
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string id, code, name, city, country, lat_str, lon_str;
        // Parse all columns
        if (!std::getline(ss, id, ',')) continue;
        if (!std::getline(ss, code, ',')) continue;
        if (!std::getline(ss, name, ',')) continue;
        if (!std::getline(ss, city, ',')) continue;
        if (!std::getline(ss, country, ',')) continue;
        if (!std::getline(ss, lat_str, ',')) continue;
        if (!std::getline(ss, lon_str, ',')) continue;
        try {
            double lat = std::stod(lat_str);
            double lon = std::stod(lon_str);

            airports.push_back({code, city, country, lat, lon});
        } catch (const std::exception& e) {
            std::cerr << "Skipping malformed line: " << line << "\n";
            continue;
        }
    }

    return airports;
}

using AdjList = std::unordered_map<std::string, std::vector<Flight>>;

class Graph {

private:
    AdjList directed_adj;
    AdjList undirected_adj;
    std::unordered_map<std::string, int> disc;
    std::unordered_map<std::string, int> low;
    std::unordered_map<std::string, std::string> parent;
    int time = 1;

    AdjList make_adj_list(const std::string& filename, bool directed = true) {
        
        AdjList adj;
        std::ifstream file(filename);
        std::string line;

        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
        }

        // Skip header
        std::getline(file, line);

        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string source, destination, item;
            double cost, duration, distance;

            std::getline(ss, source, ',');
            std::getline(ss, destination, ',');
            std::getline(ss, item, ','); cost = std::stod(item);
            std::getline(ss, item, ','); duration = std::stod(item);
            std::getline(ss, item, ','); distance = std::stod(item);

            adj[source].push_back({destination, cost, duration, distance});
            adj[destination];

            if (!directed) {
                adj[destination].push_back({source, cost, duration, distance});
            }
        }

        return adj;

    }

    std::vector<std::string> bfs(const std::string& src, int K){
        std::unordered_set<std::string> visited;
        std::vector<std::string> reachable_airports;
        int level = 0;

        std::queue<std::string> q;
        q.push(src);
        visited.insert(src);

        while(!q.empty() && level <= K) {
            int cnt = q.size();

            while (cnt > 0) {
                std::string airport = q.front();
                q.pop();
                cnt--;

                if (level > 0) {
                    reachable_airports.push_back(airport);
                }


                for (const Flight& flight : directed_adj[airport]) {
                    if(!visited.count(flight.destination)) {
                        q.push(flight.destination);
                        visited.insert(flight.destination);
                    }
                }


            }

            level++;
        }

        return reachable_airports;
    }

    void dfs(const std::string& node, std::unordered_set<std::string>& visited, std::unordered_set<std::string>& ap) {
        disc[node] = low[node] = time++;
        visited.insert(node);
        int child_count = 0;

        for (const Flight& flight : undirected_adj[node]) {
            const std::string& neighbor = flight.destination;

            if (!visited.count(neighbor)) {
                parent[neighbor] = node;
                child_count++;
                dfs(neighbor, visited, ap);
                low[node] = std::min(low[node], low[neighbor]);

                const bool is_root = parent[node].empty();
                if (is_root && child_count > 1) {
                    ap.insert(node);
                }

                if (!is_root && low[neighbor] >= disc[node]) {
                    ap.insert(node);
                }
            }
            else if (neighbor != parent[node]) {
                low[node] = std::min(low[node], disc[neighbor]);
            }
        }
    }


public:
    Graph(const std::string& routes_csv) {
        directed_adj = make_adj_list(routes_csv, true);
        undirected_adj = make_adj_list(routes_csv, false);
    }

    PathResult Dijkstra(const std::string& src, const std::string& dst, const std::string& weight) {
        std::unordered_map<std::string, double> dist;
        std::unordered_map<std::string, std::string> prev;
        PathResult result;
        result.found = false;

        std::priority_queue<
            std::pair<double, std::string>,
            std::vector<std::pair<double, std::string>>,
            std::greater<> // a has lowewr weight than b if a > b
        > pq;

        std::unordered_set<std::string> nodes;
        
        // Collect all unique nodes from the adjacency list
        for (const auto& pair : directed_adj) {
            nodes.insert(pair.first);

            for (const Flight& flight : pair.second) {
                nodes.insert(flight.destination);
            }
        }

        // Initialize distances to infinity
        for (const auto& node : nodes) {
            dist[node] = std::numeric_limits<double>::infinity();
        }

        dist[src] = 0;
        pq.push({0, src});

        while (!pq.empty()) {
            auto [dist_v, v] = pq.top();
            pq.pop();


            if (dist_v > dist[v]) {
                continue; // Skip if we already found a better path
            }

            auto it = directed_adj.find(v);

            if (it == directed_adj.end()) {
                continue; // No outgoing flights from this airport
            }


            for (const Flight& flight : it->second) {
                double edge_weight = 0;

                if (weight == "cost") {
                    edge_weight = flight.cost;
                } else if (weight == "duration") {
                    edge_weight = flight.duration;
                } else if (weight == "distance") {
                    edge_weight = flight.distance;
                } else {
                    std::cerr << "Invalid weight type: " << weight << std::endl;
                    return result;
                }

                double new_dist = dist[v] + edge_weight;

                if (new_dist < dist[flight.destination]) { //flight.destination is the neighbor
                    dist[flight.destination] = new_dist; // Update distance to shortest path to neighbor
                    prev[flight.destination] = v; // Update previous node for path reconstruction
                    pq.push({new_dist, flight.destination}); // Add neighbor to priority queue with updated distance
                }
            }
        }

        if (src == dst) {
            std::cout << "Shortest path ("<<weight<<"): " << src
                    << "\nTotal "<<weight<<": 0\n";
            return result;
        }

        // Robust path reconstruction
        std::string at = dst;

        while (at != src) {
            result.path.push_back(at);
            auto it = prev.find(at);

            if (it == prev.end()) {
                std::cout << "Path reconstruction failed. No previous node for " << at << std::endl;
                return result; // No path found
            }
            at = it->second;
        }

        result.path.push_back(src);

        result.found = true;
        result.total = dist[dst];

        std::reverse(result.path.begin(), result.path.end());

        return result;
    }

    ReachableResult Reachable(const std::string& src, int K) {
        ReachableResult result;
        result.reachable_airports = bfs(src, K);
        result.found = !result.reachable_airports.empty();
        return result;
    }

    std::vector<std::string> ArticulationPoints() {
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> ap;
        disc.clear();
        low.clear();
        parent.clear();
        time = 1;

        for (const auto& pair : undirected_adj) {
            const std::string& node = pair.first;
            if (!visited.count(node)) {
                parent[node] = "";
                dfs(node, visited, ap);
            }
        }

        return std::vector<std::string>(ap.begin(), ap.end());
    }

    std::vector<CandidateEdge> Prim(const std::string& weight_type) {
        struct CompareEdge {
            bool operator()(const CandidateEdge& a, const CandidateEdge& b) const {
                return a.weight > b.weight;
            }
        };

        auto get_edge_weight = [&](const Flight& flight) -> double {
            if (weight_type == "cost") return flight.cost;
            if (weight_type == "duration") return flight.duration;
            if (weight_type == "distance") return flight.distance;
            throw std::invalid_argument("Invalid weight type: " + weight_type);
        };

        std::unordered_set<std::string> visited;
        std::vector<CandidateEdge> mst_edges;

        std::priority_queue<
            CandidateEdge,
            std::vector<CandidateEdge>,
            CompareEdge
        > pq;

        for (const auto& [start, _] : undirected_adj) {
            if (visited.count(start)) continue;

            visited.insert(start);

            for (const Flight& flight : undirected_adj[start]) {
                pq.push({start, flight.destination, get_edge_weight(flight)});
            }

            while (!pq.empty()) {
                CandidateEdge best = pq.top();
                pq.pop();

                if (visited.count(best.to)) {
                    continue;
                }

                visited.insert(best.to);
                mst_edges.push_back({best.from, best.to, best.weight});

                for (const Flight& flight : undirected_adj[best.to]) {
                    if (!visited.count(flight.destination)) {
                        pq.push({best.to, flight.destination, get_edge_weight(flight)});
                    }
                }
            }
        }

        return mst_edges;
    }

    std::vector<std::string> budgetLimited(const std::string& src, const double& budget) {
        std::unordered_map<std::string, double> dist;
        std::vector<std::string> reachable_airports;

        std::priority_queue<
            std::pair<double, std::string>,
            std::vector<std::pair<double, std::string>>,
            std::greater<> // a has lowewr weight than b if a > b
        > pq;

        std::unordered_set<std::string> nodes;
        
        // Collect all unique nodes from the adjacency list
        for (const auto& pair : directed_adj) {
            nodes.insert(pair.first);

            for (const Flight& flight : pair.second) {
                nodes.insert(flight.destination);
            }
        }

        // Initialize distances to infinity
        for (const auto& node : nodes) {
            dist[node] = std::numeric_limits<double>::infinity();
        }

        dist[src] = 0;
        pq.push({0, src});

        while (!pq.empty()) {
            auto [dist_v, v] = pq.top();
            pq.pop();


            if (dist_v > dist[v]) {
                continue; // Skip if we already found a better path
            }

            auto it = directed_adj.find(v);

            if (it == directed_adj.end()) {
                continue; // No outgoing flights from this airport
            }

            if (dist[v] > budget) {
                continue; // Skip if we already exceed the budget
            }


            for (const Flight& flight : it->second) {
                double edge_weight = flight.cost;

                double new_dist = dist[v] + edge_weight;

                if (new_dist > budget) {
                    continue; // Skip if this path exceeds the budget
                }

                if (new_dist < dist[flight.destination]) { //flight.destination is the neighbor
                    dist[flight.destination] = new_dist; // Update distance to shortest path to neighbor
                    reachable_airports.push_back(flight.destination); // Add to reachable airports if within budget
                    pq.push({new_dist, flight.destination}); // Add neighbor to priority queue with updated distance
                }
            }
        }


        return reachable_airports;
    }
};



int main() {

    httplib::Server app;

    const char* port_env = std::getenv("PORT");
    int port = port_env ? std::stoi(port_env) : 8080;

    Graph graph("src/data/routes.csv");



    auto add_cors = [](httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    };

    app.Options(R"(.*)", [&](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        res.status = 204;
    });



    app.Get("/airports", [&](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        std::vector<Airport> airports = load_airports("src/data/airports.csv");
        if (airports.empty()) {
            // Check if file exists
            std::ifstream f("src/data/airports.csv");
            if (!f.is_open()) {
                res.status = 500;
                res.set_content("{\"error\":\"File not found: src/data/airports.csv\"}", "application/json");
                return;
            }
            res.status = 500;
            res.set_content("{\"error\":\"No airports loaded. File is empty or format is invalid.\"}", "application/json");
            return;
        }
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < airports.size(); ++i) {
            const auto& a = airports[i];
            oss << "{"
                << "\"code\":\"" << a.code << "\"," 
                << "\"city\":\"" << a.city << "\"," 
                << "\"country\":\"" << a.country << "\"," 
                << "\"lat\":" << a.latitude << "," 
                << "\"lon\":" << a.longitude
                << "}";
            if (i + 1 < airports.size()) oss << ",";
        }
        oss << "]";
        res.set_content(oss.str(), "application/json");
    });

    app.Get("/routes", [&](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        // Load airports
        std::vector<Airport> airports = load_airports("src/data/airports.csv");
        // Build lookup map for airport code to lat/lon
        std::unordered_map<std::string, std::pair<double, double>> airport_coords;
        for (const auto& a : airports) {
            airport_coords[a.code] = {a.latitude, a.longitude};
        }

        // Load routes
        std::ifstream file("src/data/routes.csv");
        std::string line;
        if (!file.is_open()) {
            res.status = 500;
            res.set_content("{\"error\":\"File not found: src/data/routes.csv\"}", "application/json");
            return;
        }
        // Skip header
        std::getline(file, line);

        std::ostringstream oss;
        oss << "[";
        bool first = true;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string source, destination, item;
            double cost, duration, distance;

            if (!std::getline(ss, source, ',')) continue;
            if (!std::getline(ss, destination, ',')) continue;

            std::getline(ss, item, ','); cost = std::stod(item);
            std::getline(ss, item, ','); duration = std::stod(item);
            std::getline(ss, item, ','); distance = std::stod(item);

            auto src_it = airport_coords.find(source);
            auto dst_it = airport_coords.find(destination);
            if (src_it == airport_coords.end() || dst_it == airport_coords.end()) continue;

            if (!first) oss << ",";
            first = false;
            oss << "{"
                << "\"src_lat\":" << src_it->second.first << ","
                << "\"src_lon\":" << src_it->second.second << ","
                << "\"dst_lat\":" << dst_it->second.first << ","
                << "\"dst_lon\":" << dst_it->second.second << ","
                << "\"cost\":" << cost << ","
                << "\"duration\":" << duration << ","
                << "\"distance\":" << distance 
                << "}";
        }
        oss << "]";
        res.set_content(oss.str(), "application/json");
    });


    app.Get("/dijkstra", [&](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        // Get query parameters
        auto src_it = req.params.find("src");
        auto dst_it = req.params.find("dst");
        auto weight_it = req.params.find("weight");
        if (src_it == req.params.end() || dst_it == req.params.end() || weight_it == req.params.end()) {
            res.status = 400;
            res.set_content("{\"error\":\"Missing src, dst, or weight parameter\"}", "application/json");
            return;
        }
        std::string src = src_it->second;
        std::string dst = dst_it->second;
        std::string weight = weight_it->second;


        auto result = graph.Dijkstra(src, dst, weight);

        std::ostringstream oss;
        oss << "{";
        oss << "\"found\":" << (result.found ? "true" : "false") << ",";
        oss << "\"total\":" << result.total << ",";
        oss << "\"path\": [";
        for (size_t i = 0; i < result.path.size(); ++i) {
            oss << "\"" << result.path[i] << "\"";
            if (i + 1 < result.path.size()) oss << ",";
        }
        oss << "]";
        oss << "}";
        res.set_content(oss.str(), "application/json");
    });

    app.Get("/reachable", [&](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        auto src_it = req.params.find("src");
        auto K_it = req.params.find("k");
        if (src_it == req.params.end() || K_it == req.params.end()) {
            res.status = 400;
            res.set_content("{\"error\":\"Missing src or K parameter\"}", "application/json");
            return;
        }

        std::string src = src_it->second;
        int K = std::stoi(K_it->second);

        auto result = graph.Reachable(src, K);

        std::ostringstream oss;
        oss << "{"
            << "\"found\":" << (result.found ? "true" : "false") << ","
            << "\"reachable_airports\": [";
        for (size_t i = 0; i < result.reachable_airports.size(); ++i) {
            oss << "\"" << result.reachable_airports[i] << "\"";
            if (i + 1 < result.reachable_airports.size()) oss << ",";
        }
        oss << "]";

        oss << "}";
        res.set_content(oss.str(), "application/json");
    });

    app.Get("/articulation-points", [&](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        auto aps = graph.ArticulationPoints();

        std::ostringstream oss;
        oss << "{"
            << "\"articulation_points\": [";
        for (size_t i = 0; i < aps.size(); ++i) {
            oss << "\"" << aps[i] << "\"";
            if (i + 1 < aps.size()) oss << ",";
        }
        oss << "]";
        oss << "}";
        res.set_content(oss.str(), "application/json");
    });

    app.Get("/mst", [&](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        auto weight_it = req.params.find("weight");
        if (weight_it == req.params.end()) {
            res.status = 400;
            res.set_content("{\"error\":\"Missing weight parameter\"}", "application/json");
            return;
        }
        
        std::string weight = weight_it->second;

        auto mst_edges = graph.Prim(weight);

        std::ostringstream oss;
        oss << "{"
            << "\"edges\": [";
        for (size_t i = 0; i < mst_edges.size(); ++i) {
            const auto& edge = mst_edges[i];
            oss << "{"
                << "\"from\": \"" << edge.from << "\","
                << "\"to\": \"" << edge.to << "\","
                << "\"weight\": " << edge.weight
                << "}";
            if (i + 1 < mst_edges.size()) oss << ",";
        }
        oss << "]";
        oss << "}";
        res.set_content(oss.str(), "application/json");
    });

    app.Get("/budget-limited", [&](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        auto src_it = req.params.find("src");
        auto budget_it = req.params.find("budget");
        
        if (src_it == req.params.end() || budget_it == req.params.end()) {
            res.status = 400;
            res.set_content("{\"error\":\"Missing src or budget parameter\"}", "application/json");
            return;
        }

        std::string src = src_it->second;
        double budget = std::stod(budget_it->second);

        auto reachable_airports = graph.budgetLimited(src, budget);

        std::ostringstream oss;
        oss << "{"
            << "\"reachable_airports\": [";
        for (size_t i = 0; i < reachable_airports.size(); ++i) {
            oss << "\"" << reachable_airports[i] << "\"";
            if (i + 1 < reachable_airports.size()) oss << ",";
        }
        oss << "]";
        oss << "}";
        res.set_content(oss.str(), "application/json");
    });

    std::cout << "Server running on http://localhost:8090\n";
    app.listen("0.0.0.0", port);
}