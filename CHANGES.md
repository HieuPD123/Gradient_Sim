# Changes Summary - Gradient Routing Enhancement

## 1. Backprop_dest Tracking (gradient.cc)

**Updated OnReceive() method** to track reverse paths for data packets:

```cpp
// When receiving DATA packet:
uint16_t src_id = hdr.GetSrcId();           // Original source
uint16_t sender_id_hdr = hdr.GetSenderId(); // Current forwarder

// Remove src from all neighbors' backprop_dest
for (auto &[nb_id, e] : routing_table) {
    e.backprop_dest.erase(src_id);
}

// Add src to current sender's backprop_dest
routing_table[sender_id_hdr].backprop_dest.insert(src_id);
```

**Logic:**
- When data arrives, we know sender_id_hdr is a good neighbor (already forwarded data)
- We track that src_id can reach back via sender_id_hdr 
- Remove src from other neighbors (ensure single entry point)
- This creates reverse path information for potential reply packets

## 2. Two New Traffic Scenarios (routing_sim.cc)

Added two simulator classes extending GradientSimulator:

### Uplink Scenario
```cpp
class GradientUplinkSimulator : public GradientSimulator
```
- **Configuration:** All nodes send to sink (node 0) every 2 seconds
- **Duration:** 10 minutes (600 seconds)
- **Packets:** 300 per node (1 packet/2s × 300 = 600s)
- **Flows:** N-1 (all nodes except sink)
- **Pattern:** Every node generates traffic independently
- **Routing:** Packets forward via gradient (closest to sink)

### Downlink Scenario (Echo-based)
```cpp
class GradientDownlinkSimulator : public GradientSimulator
```
- **Configuration:** All nodes ping sink every 2 seconds; sink replies
- **Duration:** 10 minutes (600 seconds) 
- **Pattern:** Bidirectional (uplink requests + downlink echo replies)
- **Flows:** N-1 (all nodes except sink, but bidirectional)
- **Total packets:** ~600 per node (300 requests + 300 replies)
- **Routing:**
  - **Uplink requests:** Forward via gradient
  - **Downlink replies:** Forward via backprop_dest (reverse path)

## 3. Updated Main() Program

**Changes in main():**
```cpp
double sim_time = 600.0;  // Changed from 30s to 10 minutes

std::vector<std::string> scenarios = {"Uplink", "Downlink"};

// Loop over scenarios and grid sizes
// For each: run GradientUplinkSimulator or GradientDownlinkSimulator
```

**CSV Output includes new column:**
```
Protocol, Scenario, GridSize, Duration, Flows, PacketsSent, ...
Gradient, Uplink, 5, 600.0, 24, 7200, ...
Gradient, Downlink, 5, 600.0, 24, 7200, ...
```

## Simulation Specifications

| Parameter | Uplink | Downlink |
|-----------|--------|----------|
| **Duration** | 10 min | 10 min |
| **Packet Interval** | 2s | 2s |
| **Sink** | Node 0 | Node 0 |
| **Traffic Sources** | All nodes | Sink only |
| **Total Flows** | N-1 | N-1 |
| **Total Packets/Node** | 300 | 300 |

## Grid Topologies

Three grid sizes tested:
- 5×5 grid = 25 nodes (24 flows)
- 6×6 grid = 36 nodes (35 flows)
- 7×7 grid = 49 nodes (48 flows)

Each grid with both scenarios → Each scenario ≈ 30-60 min simulator time

## Expected Behavior

### Uplink Flow
1. Nodes 1-N send to node 0 (sink)
2. Each node computes route based on gradient
3. Packets travel via intermediate nodes
4. Sink receives and counts delivery
5. Backprop_dest tracks reverse path

### Downlink Flow
1. Sink (node 0) sends to nodes 1-N
2. Each node receives unicast data
3. No forwarding needed (direct transmission should dominate)
4. Good to measure if gradient can support downlink efficiently

## Key Differences from Previous

| Aspect | Before | After |
|--------|--------|-------|
| **Duration** | 30s | 600s (10 min) |
| **Scenario** | Single (ping-pong) | Multiple (uplink, downlink) |
| **Traffic Pattern** | 1→N (corner nodes) | N→1 (all→sink) or 1→N (sink→all) |
| **Backprop_dest** | Not tracked | Tracked per DATA packet |
| **Output** | Simple metrics | Scenario-aware CSV |

## Compilation

New headers needed (already in applications-module):
- `ns3/udp-echo-helper.h` (for UdpEchoServerHelper, UdpEchoClientHelper)
- `ns3/udp-server.h`, `ns3/udp-client.h` (for UdpServerHelper, UdpClientHelper)

## Expected Output

After running on Linux with NS-3:

```
routing_comparison.csv with rows:
Gradient,Uplink,5,600.0,24,7200,XXXX,0.XXXX,XX.XX,4.0,0.0
Gradient,Downlink,5,600.0,24,7200,XXXX,0.XXXX,XX.XX,4.0,0.0
Gradient,Uplink,6,600.0,35,10500,XXXX,0.XXXX,XX.XX,5.0,0.0
Gradient,Downlink,6,600.0,35,10500,XXXX,0.XXXX,XX.XX,5.0,0.0
Gradient,Uplink,7,600.0,48,14400,XXXX,0.XXXX,XX.XX,6.0,0.0
Gradient,Downlink,7,600.0,48,14400,XXXX,0.XXXX,XX.XX,6.0,0.0
```

Total 6 results (2 scenarios × 3 grid sizes)

## Files Modified

1. **gradient.cc** - OnReceive() backprop_dest tracking
2. **routing_sim.cc** - Added UplinkSimulator, DownlinkSimulator, updated main()
3. **routing_simulator.h** - No changes needed
4. **build.sh, run.sh** - No changes needed

## Next Steps on Linux

```bash
./build.sh ~/ns-3-dev
./run.sh ~/ns-3-dev .
# Wait 6-12 hours for simulation to complete
cat routing_comparison.csv
```

Analyze results comparing uplink vs downlink performance on Gradient routing.
