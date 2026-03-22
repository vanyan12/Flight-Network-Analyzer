#include "Graph.h"
#include <fstream>
#include <sstream>
#include <queue>
#include <limits>
#include <iostream>


class Graph {

private:
    AdjList directed_adj;
    AdjList undirected_adj;
    std::unordered_map<std::string, int> disc;
    std::unordered_map<std::string, int> low;
    std::unordered_map<std::string, std::string> parent;
    int time = 1;

    // O(E) time complexity where E is the number of routes in the file
    // O(V + E) space complexity where V is the number of unique airports and E is the number of routes
    AdjList make_adj_list(const std::string& filename, bool directed = true) {
        
        // O(V) space for the keys in the adjacency list, where V is the number of unique airports
        // O(E) space for the edges in the adjacency list, where E is the number of routes
        // Total space complexity: O(V + E)
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

            if (!directed) {
                adj[destination].push_back({source, cost, duration, distance});
            }
        }

        return adj;

    }


    // O(V_k + E_k) time complexity for BFS where V_k is the number of unique airports reachable within K flights and E_k is the number of routes among those airports
    // O(V) space complexity for visited set, queue, and reachable airports list in the worst case where all airports are reachable within K flights
    std::vector<std::string> bfs(const std::string& src, int K){
        std::unordered_set<std::string> visited;// O(V) space complexity for visited set, where V is the number of unique airports
        std::vector<std::string> reachable_airports; // O(V) space complexity for reachable airports list, where V is the number of unique airports reachable within K flights
        int level = 0;

        std::queue<std::string> q; // O(V) space complexity for the queue in the worst case where all airports are reachable within K flights
        q.push(src);
        visited.insert(src);

        // Traversing all graph nodes which is O(V+E)
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

    // O(V + E) time complexity for DFS where V is the number of unique airports and E is the number of routes
    // O(V) space complexity for visited set, disc, low, parent maps, and recursion stack in the worst case where the graph is a single path (like a linked list)
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

                const bool is_root = parent[node].empty(); // Root node has no parent

                if (is_root && child_count > 1) {
                    ap.insert(node);
                }

                // For non-root nodes, check if subtree rooted at neighbor has a connection back to one of ancestors of node
                if (!is_root && low[neighbor] >= disc[node]) {
                    ap.insert(node);
                }
            }
            else if (neighbor != parent[node]) { // If neighbor is visited and is not parent of node, then there is a back edge
                low[node] = std::min(low[node], disc[neighbor]); // Update low value of node 
            }
        }
    }


public:
    Graph(const std::string& routes_csv) {
        directed_adj = make_adj_list(routes_csv, true);
        undirected_adj = make_adj_list(routes_csv, false);
    }

    // O((V + E) log V) time complexity for Dijkstra's algorithm where V is the number of unique airports and E is the number of routes
    // O(V + E) | pq: O(E) cuz duplicate entries can exist (we add new entries for the same airport when we find a shorter path)
    PathResult Dijkstra(const std::string& src, const std::string& dst, const std::string& weight) {
        std::unordered_map<std::string, double> dist; // O(V)
        std::unordered_map<std::string, std::string> prev; // O(V)
        PathResult result;
        result.found = false;

        std::priority_queue<
            std::pair<double, std::string>,
            std::vector<std::pair<double, std::string>>,
            std::greater<> // a has lowewr weight than b if a > b
        > pq; 

        std::unordered_set<std::string> nodes; // O(V)
        
        // Collect all unique nodes from the adjacency list
        for (const auto& pair : directed_adj) { // O(V + E) as we iterate through all nodes and their edges
            nodes.insert(pair.first);

            for (const Flight& flight : pair.second) {
                nodes.insert(flight.destination);
            }
        }

        // Initialize distances to infinity
        for (const auto& node : nodes) { // O(V) for initializing distances
            dist[node] = std::numeric_limits<double>::infinity();
        }

        dist[src] = 0;
        pq.push({0, src});

        while (!pq.empty()) {
            auto [dist_v, v] = pq.top(); // V * log V pops 
            pq.pop(); // O(log V) for popping from priority queue


            if (dist_v > dist[v]) {
                continue; // Skip if we already found a better path
            }

            auto it = directed_adj.find(v);

            if (it == directed_adj.end()) {
                continue; // No outgoing flights from this airport
            }


            for (const Flight& flight : it->second) { // E*log V for iterating through edges and pushing into priority queue
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
                    
                    // O(log V) for pushing into priority queue
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

    // O(V_K + E_K) time complexity for BFS where V_K is the number of unique airports reachable within K flights and E_K is the number of routes among those airports
    // O(V_K) space complexity for reachable airports list 
    ReachableResult Reachable(const std::string& src, int K) {
        ReachableResult result;
        result.reachable_airports = bfs(src, K);
        result.found = !result.reachable_airports.empty();
        return result;
    }

    // O(V + E) time complexity for DFS where V is the number of unique airports and E is the number of routes
    // O(V) space complexity
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

    // O((V + E) log V) time complexity for Prim's algorithm where V is the number of unique airports and E is the number of routes
    // O(E + V) 
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

    // O((V + E) log V) time complexity for Dijkstra's algorithm with budget constraint where V is the number of unique airports and E is the number of routes
    // O(V + E) | pq: O(E) cuz duplicate entries can exist (we add new entries for the same airport when we find a cheaper path)
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