# ✈️ Flight Network Analyzer

An interactive, graph-based flight network visualization and analysis tool. The application lets you explore a network of **40 airports** and **100 flight routes** through a visual map interface backed by a C++ server that runs classic graph algorithms in real time.

---

## 📖 Table of Contents

- [Overview](#overview)
- [Live Demo](#live-demo)
- [Technologies Used](#technologies-used)
- [Architecture](#architecture)
- [Public API Reference](#public-api-reference)
- [Complexity Analysis Summary](#complexity-analysis-summary)
- [Music Used](#music-used)

---

## Overview

**Flight Network Analyzer** models the flight network as a weighted directed graph and exposes several graph algorithms via a REST API:

| Feature | Algorithm | What it answers |
|---|---|---|
| Cheapest / Fastest Path | Dijkstra's Algorithm | What is the cheapest or fastest route between two airports? |
| K-Hop Reachability | Breadth-First Search (BFS) | Which airports can I reach within *K* connections? |
| Budget-Constrained Travel | Modified Dijkstra | Which airports can I reach without exceeding a cost budget? |
| Critical Hub Detection | Tarjan's DFS (Articulation Points) | Which airports are critical — their removal would disconnect the network? |
| Minimum Spanning Network | Prim's Algorithm | What is the minimum-cost/duration/distance network connecting all airports? |

Results are rendered interactively on a Leaflet.js map with color-coded markers and directed route lines.

---

## 🎞Live Demo

🔗 **[https://vanyan12.github.io/Flight-Network-Analyzer/](https://vanyan12.github.io/Flight-Network-Analyzer/)**

The demo page is served directly from the `docs/` folder via GitHub Pages.

> **Note:** The backend is hosted on Render's free tier and may take ~30 seconds to wake up on the first request after a period of inactivity.

---

## 💻Technologies Used

| Layer | Technology | Purpose |
|---|---|---|
| **Backend** | C++ 17 | Core graph algorithms and HTTP server |
| **HTTP Server** | [cpp-httplib](https://github.com/yhirose/cpp-httplib) | Header-only REST server |
| **Build System** | g++ (C++17) |  |
| **Containerization** | Docker (Ubuntu 24.04) | Reproducible deployment |
| **Frontend** | HTML5 + Vanilla JavaScript | UI and API integration |
| **Maps** | [Leaflet.js 1.9.4](https://leafletjs.com/) | Interactive map rendering |
| **Route Arrows** | [leaflet-polylinedecorator](https://github.com/bbecquet/Leaflet.PolylineDecorator) | Directional arrows on routes |
| **Tile Layer** | OpenStreetMap | Map background |
| **Hosting (frontend)** | GitHub Pages | Serves the `docs/` folder |
| **Hosting (backend)** | [Render.com](https://render.com) | Free-tier container hosting |

---

## 🏗Architecture

```
┌─────────────────────┐          HTTP/JSON           ┌──────────────────────────────┐
│   Browser (docs/)   │ ◄──────────────────────────► │      C++ Backend Server      │
│                     │                              │                              │
│  index.html         │  GET /airports               │  load_airports()             │
│  app.js             │  GET /routes                 │  Graph::Dijkstra()           │
│  Leaflet.js map     │  GET /dijkstra               │  Graph::Reachable()          │
│                     │  GET /reachable              │  Graph::ArticulationPoints() │
│  Airport markers    │  GET /articulation-points    │  Graph::Prim()               │
│  Route polylines    │  GET /mst                    │  Graph::budgetLimited()      │
│  Algorithm results  │  GET /budget-limited         │                              │
└─────────────────────┘                              └──────────────────────────────┘
```

**Data:** 40 airports and 100 routes stored in CSV files (`airports.csv`, `routes.csv`). Each route carries three weights: **cost** (USD), **duration** (hours), and **distance** (km).

---

## 🌐Public API Reference


### `GET /airports`

Returns the full list of airports.

**Response**

```json
[
  { "code": "YVR", "city": "Vancouver", "country": "Canada", "lat": 49.19, "lon": -123.18 },
  ...
]
```

**Complexity**

| | |
|---|---|
| **Time** | O(A) — linear in the number of airports |
| **Space** | O(A) |

---

### `GET /routes`

Returns the full list of flight routes enriched with source and destination coordinates.

**Response**

```json
[
  {
    "src_lat": 49.19, "src_lon": -123.18,
    "dst_lat": 33.94, "dst_lon": -118.41,
    "cost": 210, "duration": 2.7, "distance": 1810
  },
  ...
]
```

**Complexity**

| | |
|---|---|
| **Time** | O(E) — linear in the number of routes |
| **Space** | O(E) |

---

### `GET /dijkstra`

Finds the shortest (cheapest or fastest) path between two airports using **Dijkstra's Algorithm**.

**Query Parameters**

| Parameter | Type | Description |
|---|---|---|
| `src` | string | IATA code of the source airport (e.g. `YVR`) |
| `dst` | string | IATA code of the destination airport (e.g. `LAX`) |
| `weight` | string | Optimisation criterion: `cost`, `duration`, or `distance` |

**Example**

```
GET /dijkstra?src=YVR&dst=JFK&weight=cost
```

**Response**

```json
{
  "found": true,
  "total": 540.0,
  "path": ["YVR", "ORD", "JFK"]
}
```

If no path exists, `"found"` is `false` and `"path"` is an empty array.

**Complexity**

| | |
|---|---|
| **Time** | O((V + E) log V) — standard Dijkstra with a binary-heap priority queue |
| **Space** | O(V + E) — distance map, predecessor map, adjacency list |

Where V = number of airports, E = number of routes.

---

### `GET /reachable`

Returns all airports reachable from a source airport within at most **K** flights, using **Breadth-First Search**.

**Query Parameters**

| Parameter | Type | Description |
|---|---|---|
| `src` | string | IATA code of the source airport |
| `k` | integer | Maximum number of connections (≥ 0) |

**Example**

```
GET /reachable?src=LAX&k=2
```

**Response**

```json
{
  "found": true,
  "reachable_airports": ["SFO", "DEN", "PHX", "LAS", "ORD"]
}
```

**Complexity**

| | |
|---|---|
| **Time** | O(V_k + E_k) — BFS visits only reachable vertices and their edges up to depth K |
| **Space** | O(V) — visited set and BFS queue |

Where V_k and E_k are the number of vertices and edges reachable within K hops.

---

### `GET /articulation-points`

Detects **articulation points** (cut vertices) — airports whose removal would disconnect the flight network — using **Tarjan's DFS-based algorithm**.

**Example**

```
GET /articulation-points
```

**Response**

```json
{
  "articulation_points": ["ORD", "DFW", "ATL"]
}
```

**Complexity**

| | |
|---|---|
| **Time** | O(V + E) — single DFS traversal of the entire graph |
| **Space** | O(V) — discovery time, low-link values, parent map, visited set |

---

### `GET /mst`

Computes the **Minimum Spanning Tree** of the (undirected) flight network by the chosen weight using **Prim's Algorithm**.

**Query Parameters**

| Parameter | Type | Description |
|---|---|---|
| `weight` | string | Edge weight to minimise: `cost`, `duration`, or `distance` |

**Example**

```
GET /mst?weight=cost
```

**Response**

```json
{
  "edges": [
    { "from": "YVR", "to": "SEA", "weight": 89.0 },
    { "from": "SEA", "to": "SFO", "weight": 120.0 },
    ...
  ]
}
```

**Complexity**

| | |
|---|---|
| **Time** | O((V + E) log V) — Prim's with a min-heap priority queue |
| **Space** | O(E + V) — priority queue, in-tree set, MST edge list |

---

### `GET /budget-limited`

Returns all airports reachable from a source without exceeding a **cost budget**, using a budget-constrained variant of **Dijkstra's Algorithm**.

**Query Parameters**

| Parameter | Type | Description |
|---|---|---|
| `src` | string | IATA code of the source airport |
| `budget` | number | Maximum total travel cost in USD |

**Example**

```
GET /budget-limited?src=YVR&budget=500
```

**Response**

```json
{
  "reachable_airports": ["SEA", "SFO", "LAX", "LAS", "PHX"]
}
```

**Complexity**

| | |
|---|---|
| **Time** | O((V + E) log V) — Dijkstra exploring only paths within budget |
| **Space** | O(V + E) — distance map, adjacency list |

---

## 📊Complexity Analysis Summary

| Endpoint | Algorithm | Time Complexity | Space Complexity |
|---|---|---|---|
| `GET /airports` | Linear scan | O(A) | O(A) |
| `GET /routes` | Linear scan | O(E) | O(E) |
| `GET /dijkstra` | Dijkstra | O((V + E) log V) | O(V + E) |
| `GET /reachable` | BFS | O(V_k + E_k) | O(V) |
| `GET /articulation-points` | Tarjan's DFS | O(V + E) | O(V) |
| `GET /mst` | Prim's | O((V + E) log V) | O(V + E) |
| `GET /budget-limited` | Modified Dijkstra | O((V + E) log V) | O(V + E) |

**Legend:** A = airports, E = routes, V = vertices (airports) in the graph, V_k / E_k = vertices/edges reachable within K hops.

---

## 🎧Music Used
<img src="https://pickasso.spotifycdn.com/image/ab67c0de0000deef/dt/v1/img/artistmix/00FQb4jTyendYWaN8pK0wa/en" width="200" />

[Lana Del Rey Late Night Drive Playlist](https://youtu.be/dCn84j3Ijg0?si=0JAkkNIpKLH06Pr1)


