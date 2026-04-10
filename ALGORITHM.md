# Gradient Routing Algorithm - Updated

## Algorithm Flow

### 1. UPLINK Traffic (dest = SINK)

When a data packet arrives with `dest = SINK_ID`:

```
for each neighbor in routing_table:
    find neighbor with minimum gradient (break ties by best RSSI)

forward_packet to best_neighbor
routing_table[best_neighbor].backprop_dest.insert(src_id)  // Record reverse path

LOG: "DATA FWD UPLINK from src_id via best_neighbor (record backprop)"
```

**Example:**
```
Node 5 sends to sink (node 0)
- forwards to neighbor 3 (gradient=2, RSSI=-50)
- routing_table[3].backprop_dest = {5}  // Node 3 knows Node 5 reachable via uplink
```

### 2. DOWNLINK Traffic (dest ≠ SINK)

When a data packet arrives with `dest ≠ SINK_ID`:

```
for each neighbor in routing_table:
    if dest in neighbor.backprop_dest:
        forward_packet to that neighbor
        return

// If no neighbor has the dest in backprop_dest:
drop packet (no reverse path recorded)

LOG: "DATA FWD DOWNLINK to neighbor_with_dest" OR "DATA DROP: no backprop route"
```

**Example:**
```
Sink wants to send to node 5 (dest = 5)
- Check neighbors... neighbor 3 has {5} in backprop_dest
- forward to node 3
- routing_table[3].backprop_dest contains 5

Node 3 receives downlink packet (dest = 5)
- Check neighbors... neighbor 1 has {5} in backprop_dest  
- forward to node 1
- ...eventually reaches node 5
```

### 3. Beacon Messages (Gradient Establishment)

```
every BEACON_PERIOD (5 seconds):
  broadcast BEACON packet
  
on BEACON received:
  extract sender.gradient from payload
  update: routing_table[sender] = {
    gradient: sender.gradient,
    rssi: measured_rssi,
    last_seen: now
  }
  if gradient changed:
    recompute gradient
    if changed: send BEACON
```

---

## Key Data Structures

```cpp
struct GradientEntry {
    uint8_t gradient;              // Distance to sink
    double rssi;                   // Signal strength
    std::set<uint16_t> backprop_dest;  // Downlink destinations
    Time last_seen;                // For timeout check
};

routing_table: std::map<uint16_t, GradientEntry>
  key: neighbor_id
  value: routing entry with backprop_dest
```

---

## Simulation Scenarios

### Scenario 1: Uplink (All → Sink)

- **Duration:** 10 minutes
- **Pattern:** Each node 1..N sends packets to node 0 (sink) every 2 seconds
- **Result:** routing_table.backprop_dest populated with all source nodes
- **Metrics:** PDR (how many packets reached sink)

```
Node 1 → Node 0 (every 2s)
Node 2 → Node 0 (every 2s)
...
Node 24 → Node 0 (every 2s)
```

### Scenario 2: Downlink (Sink ← → All via Ping-Pong)

- **Duration:** 10 minutes
- **Pattern:** All nodes 1..N send echo requests to sink every 2 seconds
- **Sink behavior:** Replies with echo responses via reverse path
- **Result:** Tests both uplink (requests) + downlink (echo replies)
- **Metrics:** PDR includes both request and reply packets

```
Node 1 → Sink (request every 2s)
Sink → Node 1 (reply, routed via uplink reverse path)
Node 2 → Sink (request every 2s)
Sink → Node 2 (reply)
...all nodes participate simultaneously
```

**Implementation:** UDP Echo Server on sink + UDP Echo Clients on all other nodes
- Requests flow uplink via Gradient (toward sink)
- Replies flow downlink via backprop_dest (away from sink)
- Effective bidirectional link test

---

## Expected Behavior

### 5×5 Grid Uplink

```
        [SINK at (0,0)]
        
        [0] [1] [2] [3] [4]
        [5] [6] [7] [8] [9]
        ...
        [20][21][22][23][24]
```

- Nodes far from sink (e.g., node 24) need more hops
- gradient(24) = gradient(17) + 1 = ... (increases from sink)
- Uplink: 24 → ... → 6 → 0
- Downlink: 0 → 6 → ... → 24

### Backprop Path Formation

After uplink beacons stabilize (≈30 seconds):

```
routing_table[6].backprop_dest = {24, 23, 22, ...}  (all nodes using 6 as next hop to sink)

Example: When 24 sends:
  24 → 17 → ... → 6 → 0
  routing_table[17].backprop_dest.insert(24)
  routing_table[6].backprop_dest.insert(24)
```

Later, downlink from 0 to 24:
```
  0 looks for 24 in neighbors' backprop_dest → finds in routing_table[6]
  0 → 6 (because backprop_dest[6].contains(24))
  
  6 receives packet(dest=24) → finds in routing_table[17]
  6 → 17
  
  17 receives packet(dest=24) → finds in routing_table[24]
  17 → 24 ✓ DELIVERED
```

---

## Important Notes

1. **Backprop_dest is per-neighbor:** Each neighbor maintains its own set of reachable destinations via uplink

2. **Multiple sources:** Same backprop_dest can have multiple source nodes (all using same path to sink)

3. **Path updates:** If topology changes or routes shift, backprop_dest updates dynamically when new uplink packets arrive

4. **No downlink guarantee:** If no uplink path exists to a node, that node won't be in any backprop_dest set, so downlink fails gracefully with PDR=0%

5. **Beacon convergence:** First beacon flooding (≈5s), then gradient stabilization (≈10-30s), then traffic patterns established

---

## Packet Processing Timeline

```
T=0s    : Start beacons
T=0-5s  : First beacon receives distant nodes, forms basic gradient
T=5-15s : Gradient stabilization through repeated beacons
T=15s   : Traffic begins (uplink scenario) or downlink starts

T=15-600s (10 min): Sustained traffic
- Uplink: packets flow toward sink, backprop_dest records paths
- Downlink: packets follow backprop paths backward

T=600s  : Simulation ends
         Report: PDR, overhead (beacons+data packets), RTT
```

---

## CSV Output Columns

```
Protocol,Scenario,GridSize,Duration,Flows,PacketsSent,PacketsRcvd,
ControlPackets,PDR,AvgRTT_ms,AvgHopCount,AvgEnergy

Gradient,Uplink,5,600.0,24,7200,6840,300,0.9500,45.2,4.5,0.0
Gradient,Downlink,5,600.0,24,7200,6750,150,0.9375,48.1,4.5,0.0
```

- **ControlPackets** = beacon count
- **PDR** = packets_received / packets_sent
- **AvgRTT** = estimated round-trip time
- **AvgHopCount** = average path length

---

## Verification Commands (Linux)

```bash
# After simulation:
grep Uplink routing_comparison.csv | head -3
grep Downlink routing_comparison.csv | head -3

# Extract just PDR
awk -F, '{if(NR>1) print $1, $2, $3, $8}' routing_comparison.csv
```

Expected output shows:
- Uplink PDR ≈ 90-98% (good path to sink)
- Downlink PDR ≈ 85-95% (follows backprop paths)
- Downlink usually lower than uplink (reverse paths less optimized)
