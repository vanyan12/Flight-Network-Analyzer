const BACKEND = "http://localhost:8090";

const out = (msg, tone = "default") => {
  const outEl = document.getElementById("out");
  outEl.classList.toggle("is-error", tone === "error");
  outEl.classList.toggle("is-success", tone === "success");
  outEl.textContent =
    typeof msg === "string" ? msg : JSON.stringify(msg, null, 2);
};

const setDijkstraDisplay = ({ total = "No result yet.", path = "No result yet." } = {}) => {
  document.getElementById("resultTotal").textContent = total;
  document.getElementById("resultPath").textContent = path;
};

const setResultTotalLabel = (label) => {
  document.getElementById("resultTotalLabel").textContent = label;
};

function hasLoadedAirports() {
  return Array.isArray(window._airports) && window._airports.length > 0;
}

function updateKValidation({ showAlert = false } = {}) {
  const kInput = document.getElementById("k");
  const validation = document.getElementById("kValidation");
  const reachButton = document.getElementById("btnReach");
  const articulationButton = document.getElementById("btnArt");
  const kValue = Number(kInput.value);
  const hasNegativeValue = kInput.value !== "" && Number.isFinite(kValue) && kValue < 0;
  const airportsLoaded = hasLoadedAirports();

  if (hasNegativeValue) {
    const message = "Negative values are not allowed for K connections.";
    kInput.setCustomValidity(message);
    validation.textContent = message;
    validation.classList.add("is-visible");
    reachButton.disabled = true;
    articulationButton.disabled = true;

    return false;
  }

  kInput.setCustomValidity("");
  validation.textContent = "";
  validation.classList.remove("is-visible");
  reachButton.disabled = !airportsLoaded;
  articulationButton.disabled = !airportsLoaded;
  return true;
}

// --- Leaflet map ---
const map = L.map("map").setView([40, 20], 2);

map.createPane("routesPane");
map.getPane("routesPane").style.zIndex = 250;

map.createPane("selectedPathPane");
map.getPane("selectedPathPane").style.zIndex = 650;

L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
  maxZoom: 19,
  minZoom: 2,
}).addTo(map);


let markersLayer = L.featureGroup().addTo(map);
let routesLayer = L.featureGroup().addTo(map);
let selectedPathLayer = L.featureGroup().addTo(map);
let airportMarkers = {};
let activeRouteMetric = null;
let activeRouteView = "directed";

let highlightLine = null;

function getRouteArrowSize() {
  const zoom = map.getZoom();
  return Math.max(8, Math.min(18, Math.round(zoom * 1.7)));
}

function formatRouteMetric(route) {
  if (activeRouteMetric === "cost") {
    return `Cost: $${route.cost.toFixed(2)}`;
  }

  if (activeRouteMetric === "duration") {
    return `Duration: ${route.duration.toFixed(2)} hrs`;
  }

  return [
    `Cost: $${route.cost.toFixed(2)}`,
    `Duration: ${route.duration.toFixed(2)} hrs`,
    `Distance: ${route.distance.toFixed(1)} km`
  ].join("<br>");
}

function bindRouteHover(line, route) {
  line.bindTooltip("", {
    sticky: true,
    direction: "top",
    opacity: 0.95,
  });

  line.on("mouseover", () => {
    line.setTooltipContent(formatRouteMetric(route));
    line.openTooltip();
  });

  line.on("mouseout", () => {
    line.closeTooltip();
  });
}

function addDirectedLine(layerGroup, latlngs, lineOptions, arrowOptions = {}) {
  const line = L.polyline(latlngs, lineOptions).addTo(layerGroup);

  L.polylineDecorator(line, {
    patterns: [
      {
        offset: "52%",
        repeat: 0,
        symbol: L.Symbol.arrowHead({
          pixelSize: arrowOptions.pixelSize || 10,
          polygon: true,
          pathOptions: {
            pane: arrowOptions.pane || lineOptions.pane,
            color: arrowOptions.color || lineOptions.color,
            fillOpacity: arrowOptions.fillOpacity ?? 1,
            weight: arrowOptions.weight || 2,
          },
        }),
      },
    ],
  }).addTo(layerGroup);

  return line;
}

function addBidirectionalLine(layerGroup, latlngs, lineOptions, arrowOptions = {}) {
  const line = L.polyline(latlngs, lineOptions).addTo(layerGroup);

  L.polylineDecorator(line, {
    patterns: [
      {
        offset: "46%",
        repeat: 0,
        symbol: L.Symbol.arrowHead({
          pixelSize: arrowOptions.pixelSize || 10,
          polygon: true,
          pathOptions: {
            pane: arrowOptions.pane || lineOptions.pane,
            color: arrowOptions.color || lineOptions.color,
            fillOpacity: arrowOptions.fillOpacity ?? 1,
            weight: arrowOptions.weight || 2,
          },
        }),
      },
      {
        offset: "54%",
        repeat: 0,
        symbol: L.Symbol.arrowHead({
          pixelSize: arrowOptions.pixelSize || 10,
          polygon: true,
          pathOptions: {
            pane: arrowOptions.pane || lineOptions.pane,
            color: arrowOptions.color || lineOptions.color,
            fillOpacity: arrowOptions.fillOpacity ?? 1,
            weight: arrowOptions.weight || 2,
          },
          headAngle: 300,
        }),
      },
    ],
  }).addTo(layerGroup);

  return line;
}

function addUndirectedLine(layerGroup, latlngs, lineOptions) {
  return L.polyline(latlngs, lineOptions).addTo(layerGroup);
}

function clearMap() {
  selectedPathLayer.clearLayers();

  for (const marker of Object.values(airportMarkers)) {
    marker.setRadius(5);
    marker.setStyle({
      color: "#1f4d3a",
      weight: 1,
      fillColor: "#2f69bf",
      fillOpacity: 0.90,
    });
  }
}

function clearSelectedPathResult() {
  clearMap();
  setDijkstraDisplay();
}

function getUndirectedRouteKey(route) {
  const a = `${route.src_lat},${route.src_lon}`;
  const b = `${route.dst_lat},${route.dst_lon}`;
  return [a, b].sort().join("|");
}

function getDirectedRouteKey(route) {
  return `${route.src_lat},${route.src_lon}|${route.dst_lat},${route.dst_lon}`;
}

function renderDirectedRoutes(routes) {
  routesLayer.clearLayers();

  if (highlightLine) {
    map.removeLayer(highlightLine);
    highlightLine = null;
  }

  const arrowSize = getRouteArrowSize();
  const routeMap = new Map();

  for (const route of routes) {
    routeMap.set(getDirectedRouteKey(route), route);
  }

  const rendered = new Set();

  for (const route of routes) {
    const key = getDirectedRouteKey(route);
    if (rendered.has(key)) {
      continue;
    }

    rendered.add(key);
    const latlngs = [
      [route.src_lat, route.src_lon],
      [route.dst_lat, route.dst_lon]
    ];

    const reverseKey = `${route.dst_lat},${route.dst_lon}|${route.src_lat},${route.src_lon}`;
    const reverseRoute = routeMap.get(reverseKey);

    if (reverseRoute) {
      rendered.add(reverseKey);
      highlightLine = addBidirectionalLine(
        routesLayer,
        latlngs,
        { color: 'green', weight: 2, opacity: 0.6, pane: "routesPane" },
        { color: 'green', pixelSize: arrowSize, pane: "routesPane" }
      );
    } else {
      highlightLine = addDirectedLine(
        routesLayer,
        latlngs,
        { color: 'green', weight: 2, opacity: 0.6, pane: "routesPane" },
        { color: 'green', pixelSize: arrowSize, pane: "routesPane" }
      );
    }

    bindRouteHover(highlightLine, route);
  }
}

function renderUndirectedRoutes(routes) {
  routesLayer.clearLayers();

  if (highlightLine) {
    map.removeLayer(highlightLine);
    highlightLine = null;
  }

  const seen = new Set();

  for (const route of routes) {
    const key = getUndirectedRouteKey(route);
    if (seen.has(key)) {
      continue;
    }
    seen.add(key);

    const latlngs = [
      [route.src_lat, route.src_lon],
      [route.dst_lat, route.dst_lon]
    ];

    highlightLine = addUndirectedLine(
      routesLayer,
      latlngs,
      { color: 'green', weight: 2, opacity: 0.6, pane: "routesPane" }
    );

    bindRouteHover(highlightLine, route);
  }
}

function renderRoutes(routes, view = activeRouteView) {
  activeRouteView = view;

  if (view === "undirected") {
    renderUndirectedRoutes(routes);
    return;
  }

  renderDirectedRoutes(routes);
}

function runDijkstra(weight, totalLabel) {
  const src = document.getElementById("src").value;
  const dst = document.getElementById("dst").value;
  activeRouteMetric = weight;
  setResultTotalLabel(totalLabel);

  if (Array.isArray(window._routes)) {
    renderRoutes(window._routes, "directed");
  }

  clearSelectedPathResult();


  const airportMap = {};
  for (const a of window._airports || []) airportMap[a.code] = a;

  fetch(`${BACKEND}/dijkstra?src=${src}&dst=${dst}&weight=${weight}`)
    .then(r => {
      if (!r.ok) throw new Error(`dijkstra failed: ${r.status}`);
      return r.json();
    })
    .then(result => {
      if (!result.found || !Array.isArray(result.path) || result.path.length === 0) {
        setDijkstraDisplay({
          total: "",
          path: `${src} -> ${dst}`
        });

        out("No path found.", "error");
        return;
      }

      out(`Path is found!`, "success");

      setDijkstraDisplay({
        total: String(result.total) + (weight === "cost" ? " $" : " hrs"),
        path: result.path.join(" -> ")
      });

      const path = result.path;
      for (const airportCode of path) {
        const marker = airportMarkers[airportCode];
        if (marker) {
          marker.setRadius(7).setStyle({ fillColor: 'red' });
        }
      }

      for (let i = 0; i < path.length - 1; ++i) {
        const a1 = airportMap[path[i]];
        const a2 = airportMap[path[i + 1]];
        if (a1 && a2) {
          const latlngs = [[a1.lat, a1.lon], [a2.lat, a2.lon]];
          L.polyline(latlngs, {
            color: 'red',
            weight: 4,
            opacity: 0.8,
            pane: "selectedPathPane",
            interactive: false
          }).addTo(selectedPathLayer);
        }
      }

      const bounds = selectedPathLayer.getBounds();
      if (bounds.isValid()) {
        map.fitBounds(bounds, { padding: [32, 32] });
      }
    })
    .catch(e => out("Error: " + e.message));
}

function addAirportMarker(a) {
  const m = L.circleMarker([a.lat, a.lon], {
    radius: 5,
    color: "#1f4d3a",
    weight: 1,
    fillColor: "#2f69bf",
    fillOpacity: 0.90,
  }).addTo(markersLayer);
  m.bindTooltip(`<b>${a.code}</b><br>${a.city}, ${a.country}`, {permanent: false, direction: 'top'});
  // Show tooltip on hover, not on click
  m.on('mouseover', function() { m.openTooltip(); });
  m.on('mouseout', function() { m.closeTooltip(); });
  airportMarkers[a.code] = m;
  return m;
}

function populateSelect(selectEl, airports) {
  selectEl.innerHTML = "";
  for (const a of airports) {
    const opt = document.createElement("option");
    opt.value = a.code;
    opt.textContent = `${a.code} — ${a.city}`;

    if (![...selectEl.options].some(o => o.value === a.code)) {
      selectEl.appendChild(opt);
    }
  }
}

// --- Load airports ---
async function loadAirports() {
  const r = await fetch(`${BACKEND}/airports`);
  if (!r.ok) throw new Error(`airports failed: ${r.status}`);
  const airports = await r.json();

  window._airports = airports; // Save for lookup


  clearSelectedPathResult();
  markersLayer.clearLayers();
  airportMarkers = {};

  // Add markers
  for (const a of airports) addAirportMarker(a);

  // Fill dropdowns
  populateSelect(document.getElementById("src"), airports);
  populateSelect(document.getElementById("dst"), airports);

  // zoom to all markers
  const bounds = markersLayer.getBounds();
  if (bounds.isValid()) {
    map.fitBounds(bounds.pad(-0.1));
  } 


  // Enable buttons once data exists (even if endpoints not done yet)
  document.getElementById("btnCheapest").disabled = false;
  document.getElementById("btnFastest").disabled = false;
  updateKValidation();

}

function markReachableAirports(src, airports) {
  const srcMarker = airportMarkers[src];
  if (srcMarker) {
    srcMarker.setRadius(9).setStyle({
      color: "#7a4b00",
      weight: 6,
      fillColor: "#ffd24a",
      fillOpacity: 1,
    });
    srcMarker.bringToFront();
  }

  for (const code of airports) {
    const marker = airportMarkers[code];
    if (marker) {
      marker.setRadius(7).setStyle({ fillColor: 'orange' });
    }
  }
};

function fitMapToAirportCodes(codes) {
  const bounds = L.latLngBounds([]);

  for (const code of codes) {
    const marker = airportMarkers[code];
    if (marker) {
      bounds.extend(marker.getLatLng());
    }
  }

  if (bounds.isValid()) {
    map.fitBounds(bounds, { padding: [32, 32] });
  }
}

document.getElementById("btnLoad").onclick = () => {
  loadAirports().catch(e => out("Error: " + e.message));
  loadRoutes().catch(e => out("Error: " + e.message));
};

async function loadRoutes() {
  const r = await fetch(`${BACKEND}/routes`);
  if (!r.ok) throw new Error(`routes failed: ${r.status}`);
  const routes = await r.json();

  window._routes = routes;
  renderRoutes(routes, "directed");

}

document.getElementById("btnCheapest").onclick = () => {
  runDijkstra("cost", "Total cost");

};

document.getElementById("btnFastest").onclick = () => {
  runDijkstra("duration", "Total duration");
};

document.getElementById("k").addEventListener("input", () => {
  updateKValidation();
});

document.getElementById("k").addEventListener("change", () => {
  updateKValidation({ showAlert: true });
});

// ===========================================================

document.getElementById("btnReach").onclick = () => {
  if (!updateKValidation({ showAlert: true })) {
    return;
  }

  if (Array.isArray(window._routes)) {
    renderRoutes(window._routes, "directed");
  }

  const src = document.getElementById("src").value;
  const k = document.getElementById("k").value;

  fetch(`${BACKEND}/reachable?src=${src}&k=${k}`)
    .then(r => {
      if (!r.ok) throw new Error(`reachable failed: ${r.status}`);
      return r.json();
    })
    .then(result => {
      console.log(result);
      if (!result.reachable_airports || !Array.isArray(result.reachable_airports)) {
        out("No reachable airports found.");
        return;
      }
      const list = result.reachable_airports.map(r => r).join(", ");
      out(`Airports reachable from ${src} within ${k} flights: ${list}`, "success");

      clearMap();
      markReachableAirports(src, result.reachable_airports);
    })
    .catch(e => out("Error: " + e.message));
};

document.getElementById("btnArt").onclick = () => {
  if (!updateKValidation({ showAlert: true })) {
    return;
  }

  if (Array.isArray(window._routes)) {
    renderRoutes(window._routes, "undirected");
  }


  fetch(`${BACKEND}/articulation-points`)
    .then(r => {
      if (!r.ok) throw new Error(`articulation-points failed: ${r.status}`);
      return r.json();
    })
    .then(result => {

      if (!result.articulation_points || !Array.isArray(result.articulation_points)) {
        out("No articulation points found.");
        return;
      }

      const list = result.articulation_points.map(r => r).join(", ");
      out(`Articulation points: ${list}`, "success");

      clearMap();
      for (const code of result.articulation_points) {
        const marker = airportMarkers[code];
        if (marker) {
          marker.setRadius(9).setStyle({ fillColor: 'purple' });
        }
      }

      fitMapToAirportCodes(result.articulation_points);
    })
    .catch(e => out("Error: " + e.message));

};

updateKValidation();

