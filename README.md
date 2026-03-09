# Flight Network Analyzer

A flight network visualization tool with a C++ backend API and a browser-based frontend built with [Leaflet](https://leafletjs.com/).

## Prerequisites

- **Windows** (the pre-built backend is a Windows executable)
- **Node.js** and **npm** (for running the frontend)

## Setup & Running

### 1. Download and run the backend

Download the latest `flight_backend.exe` from the [Releases page](https://github.com/vanyan12/Flight-Network-Analyzer/releases) and run it:

```powershell
# Download the executable
curl -L -o flight_backend.exe https://github.com/vanyan12/Flight-Network-Analyzer/releases/download/v0.1/flight_backend.exe

# Run the backend server
.\flight_backend.exe
```

The backend starts an HTTP server on **port 8090**.

### 2. Run the frontend

Open a new terminal, navigate to the `frontend` directory, install dependencies, and start the dev server:

```bash
cd frontend
npm install
npm run dev
```

Then open the URL shown in the terminal (e.g. `http://localhost:5173`) in your browser.

## Usage

1. Click **Load airports** to fetch airport data from the backend.
2. Select a **Source** and **Destination** airport.
3. Use the buttons to find the **cheapest** or **fastest** route (Dijkstra), list airports **reachable within K connections** (BFS), or identify **articulation points** in the network.

cd backend
cmake -B build
cmake --build build --config Release
```

The resulting executable is `build/Release/flight_backend.exe`.
