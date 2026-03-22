#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <utility>

#include "includes/graph.h"
#include "includes/httplib.h" 


// O(N) time complexity where N is the number of lines in the file
// O(A) space complexity where A is the number of airports loaded into memory
std::vector<Airport> load_airports(const std::string& filename) {
    std::vector<Airport> airports; // O(A)
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