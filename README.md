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

### Requirements

- CMake ≥ 3.20
- A C++17-compatible compiler (GCC, Clang, or MSVC)

### Build & Run

```bash
cd backend
cmake -B build
cmake --build build
./build/flight_backend        # Linux / macOS
build\flight_backend.exe      # Windows
```

> **Note:** With multi-config generators (Visual Studio, Xcode) the binary may be placed in a subdirectory such as `build/Debug/flight_backend` or `build/Release/flight_backend`.

The server listens on **http://localhost:8090** by default.

---

## Frontend

The frontend is plain HTML and JavaScript — **no Node.js or `npm run dev` is needed**.

### How to run

1. Make sure the backend is running (see above).
2. Open `frontend/index.html` directly in your browser:
   - Double-click the file in your file manager, **or**
   - Drag `frontend/index.html` into an open browser window, **or**
   - Use a simple local file server if your browser blocks requests from a `file://` URL (some browsers apply CORS restrictions that prevent the page from calling the backend API when opened as a local file):
     ```bash
     # Python 3 (from the frontend directory)
     cd frontend
     python3 -m http.server 5500
     # then open http://localhost:5500 in your browser
     ```

### Features

- **Load airports** — fetches airports and routes from the backend and renders them on an interactive map.
- **Cheapest path (Dijkstra)** — finds the lowest-cost route between two airports.
- **Fastest path (Dijkstra)** — finds the shortest-duration route between two airports.
- **Reachable within K connections (BFS)** — lists all airports reachable from a source within *K* flights.
- **Articulation points** — highlights airports whose removal would disconnect the network.
