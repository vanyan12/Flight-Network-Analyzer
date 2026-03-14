# ✈️ Flight Network Analyzer

An interactive, graph-based flight network visualization and analysis tool. The application lets you explore a network of **40 airports** and **100 flight routes** through a visual map interface backed by a C++ server that runs classic graph algorithms in real time.

---

## 📖 Table of Contents

- [Overview](#overview)
- [Live Demo](#live-demo)
- [How to Use the Demo](#how-to-use-the-demo)
- [Technologies Used](#technologies-used)
- [Architecture](#architecture)
- [Public API Reference](#public-api-reference)
  - [GET /airports](#get-airports)
  - [GET /routes](#get-routes)
  - [GET /dijkstra](#get-dijkstra)
  - [GET /reachable](#get-reachable)
  - [GET /articulation-points](#get-articulation-points)
  - [GET /mst](#get-mst)
  - [GET /budget-limited](#get-budget-limited)
- [Complexity Analysis Summary](#complexity-analysis-summary)
- [Running Locally](#running-locally)

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

## Live Demo

🌐 **[https://vanyan12.github.io/Flight-Network-Analyzer/](https://vanyan12.github.io/Flight-Network-Analyzer/)**

The demo page is served directly from the `docs/` folder via GitHub Pages. The backend API runs at `https://flight-network-analyzer.onrender.com`.

> **Note:** The backend is hosted on Render's free tier and may take ~30 seconds to wake up on the first request after a period of inactivity.

---

## How to Use the Demo

### 1. Load the Network

Click **"Load airports"** to fetch all airports and routes from the backend. Airports appear as blue circle markers on the world map and all flight routes are drawn as directed arrows.

### 2. Find the Cheapest or Fastest Path (Dijkstra)

1. Select a **Source** and a **Destination** airport from the dropdowns.
2. Click **Cheapest** to find the minimum-cost path, or **Fastest** for minimum-duration.
3. The path is highlighted in **red** on the map. The total cost (or duration) and the ordered list of airports appear in the result panel below the map controls.

### 3. Explore K-Hop Reachability (BFS)

1. Select a **Source** airport.
2. Enter a value for **K** (the maximum number of connections, between 0 and 10).
3. Click **"Reachable within K"**.
4. The source airport turns **gold** and all airports reachable within *K* flights are highlighted in **orange**.

### 4. Travel Within a Budget

1. Select a **Source** airport.
2. Enter a **Travel budget** in dollars.
3. Click **"Reachable within budget"**.
4. All airports reachable without exceeding the budget are highlighted in **orange**.

### 5. Find Critical Hubs (Articulation Points)

Click **"Articulation points"**. The graph is analysed for *cut vertices* — airports whose removal would split the network into disconnected components. These critical hubs are highlighted in **purple** and listed in the message panel.

### 6. Compute the Minimum Spanning Tree

1. Choose a weight from the **MST weight** dropdown: **Cost**, **Duration**, or **Distance**.
2. Click **"Minimum Spanning Tree"**.
3. The MST edges are drawn in **dark green** on the map. Hover over any edge to see its weight.

---

## Technologies Used

| Layer | Technology | Purpose |
|---|---|---|
| **Backend** | C++ 17 | Core graph algorithms and HTTP server |
| **HTTP Server** | [cpp-httplib](https://github.com/yhirose/cpp-httplib) | Header-only REST server |
| **Build System** | CMake 3.20+ | C++ compilation |
| **Containerization** | Docker (Ubuntu 24.04) | Reproducible deployment |
| **Frontend** | HTML5 + Vanilla JavaScript | UI and API integration |
| **Maps** | [Leaflet.js 1.9.4](https://leafletjs.com/) | Interactive map rendering |
| **Route Arrows** | [leaflet-polylinedecorator](https://github.com/bbecquet/Leaflet.PolylineDecorator) | Directional arrows on routes |
| **Tile Layer** | OpenStreetMap | Map background |
| **Hosting (frontend)** | GitHub Pages | Serves the `docs/` folder |
| **Hosting (backend)** | [Render.com](https://render.com) | Free-tier container hosting |

---

## Architecture

```
┌─────────────────────┐          HTTP/JSON          ┌──────────────────────┐
│   Browser (docs/)   │ ◄──────────────────────────► │  C++ Backend Server  │
│                     │                              │                      │
│  index.html         │  GET /airports               │  load_airports()     │
│  app.js             │  GET /routes                 │  Graph::Dijkstra()   │
│  Leaflet.js map     │  GET /dijkstra               │  Graph::Reachable()  │
│                     │  GET /reachable              │  Graph::ArticulationPoints() │
│  Airport markers    │  GET /articulation-points    │  Graph::Prim()       │
│  Route polylines    │  GET /mst                    │  Graph::budgetLimited() │
│  Algorithm results  │  GET /budget-limited         │                      │
└─────────────────────┘                              └──────────────────────┘
```

**Data:** 40 airports and 100 routes stored in CSV files (`airports.csv`, `routes.csv`). Each route carries three weights: **cost** (USD), **duration** (hours), and **distance** (km).

---

## Public API Reference

The base URL for the hosted backend is:

```
https://flight-network-analyzer.onrender.com
```

All endpoints return JSON. All endpoints support CORS.

---

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

## Complexity Analysis Summary

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

## Running Locally

### Backend (C++)

```bash
cd backend
mkdir build && cd build
cmake ..
cmake --build .
./flight_server          # Starts on port 8080 by default
```

Set the `PORT` environment variable to override the default port.

### Backend with Docker

```bash
cd backend
docker build -t flight-server .
docker run -p 8080:8080 flight-server
```

### Frontend

The frontend is pure HTML/JS — no build step needed. Open `docs/index.html` directly in a browser, or serve it with any static file server:

```bash
cd docs
npx serve .
```

> By default, `app.js` points to the hosted Render backend. To use your local backend, change the `BACKEND` constant at the top of `docs/app.js` to `http://localhost:8080`.
