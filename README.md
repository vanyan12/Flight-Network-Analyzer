# Flight Network Analyzer

A flight network visualization and analysis tool. The backend exposes a REST API (C++/CMake) and the frontend is a static HTML/JS page that talks to it.

---

## Project Structure

```
Flight-Network-Analyzer/
├── backend/          # C++ REST API (CMake)
│   ├── CMakeLists.txt
│   └── src/
│       ├── main.cpp
│       ├── httplib.h
│       └── data/
└── frontend/         # Static HTML + JavaScript (no build step needed)
    ├── index.html
    └── app.js
```

---

## Backend


The server listens on **http://localhost:8090** by default.

---

## Frontend

The frontend is plain HTML and JavaScript.

### How to run

1. Make sure the backend is running (see above).
2. Open `frontend/index.html` directly in your browser:
   - Double-click the file in your file manager, **or**
   - Drag `frontend/index.html` into an open browser window, **or**
   - Use a simple local file server if your browser blocks requests from a `file://` URL (some browsers apply CORS restrictions that prevent the page from calling the backend API when opened as a local file):


### Features

- **Load airports** — fetches airports and routes from the backend and renders them on an interactive map.
- **Cheapest path (Dijkstra)** — finds the lowest-cost route between two airports.
- **Fastest path (Dijkstra)** — finds the shortest-duration route between two airports.
- **Reachable within K connections (BFS)** — lists all airports reachable from a source within *K* flights.
- **Articulation points** — highlights airports whose removal would disconnect the network.
