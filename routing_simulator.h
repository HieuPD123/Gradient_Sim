#ifndef ROUTING_SIMULATOR_H
#define ROUTING_SIMULATOR_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lr-wpan-module.h"
#include <string>
#include <map>
#include <vector>
#include <numeric>
#include <iomanip>
#include <sstream>

using namespace ns3;

// ================ Metrics Structure ================
struct RoutingMetrics {
    std::string protocol;
    uint32_t    grid_size;
    double      duration;
    uint32_t    num_flows;
    uint32_t    packets_sent;
    uint32_t    packets_received;
    uint32_t    control_packets;  // overhead
    double      pdr;              // PDR = packets_received / packets_sent
    double      avg_rtt;          // Average Round Trip Time (ms)
    double      avg_hop_count;
    double      avg_energy;       // estimate

    // Constructor
    RoutingMetrics(std::string proto = "")
        : protocol(proto), grid_size(0), duration(0.0),
          num_flows(0), packets_sent(0), packets_received(0),
          control_packets(0), pdr(0.0), avg_rtt(0.0),
          avg_hop_count(0.0), avg_energy(0.0)
    {}

    // Print to CSV format
    std::string ToCSV() const {
        std::ostringstream oss;
        oss << protocol << ","
            << grid_size << ","
            << duration << ","
            << num_flows << ","
            << packets_sent << ","
            << packets_received << ","
            << control_packets << ","
            << std::fixed << std::setprecision(4)
            << pdr << ","
            << avg_rtt << ","
            << avg_hop_count << ","
            << avg_energy;
        return oss.str();
    }
};

// ================ Base Class ================
class RoutingSimulator {
public:
    RoutingSimulator(uint32_t grid_size, double sim_time)
        : m_grid_size(grid_size), m_sim_time(sim_time),
          m_num_flows(0), m_packets_sent(0), m_packets_received(0),
          m_control_packets(0)
    {}

    virtual ~RoutingSimulator() = default;

    // Main simulation runner
    virtual RoutingMetrics Run() = 0;

    // Setup functions
    virtual void SetupNetwork();
    virtual void SetupRouting() = 0;
    virtual void SetupApplications();
    virtual void SetupMetricsCollection();

    // Callback functions for metrics
    virtual void OnPacketTx(Ptr<const Packet> pkt);
    virtual void OnPacketRx(Ptr<const Packet> pkt);
    virtual void OnControlPacket(Ptr<const Packet> pkt);

    // Getters
    uint32_t GetPacketsSent() const    { return m_packets_sent; }
    uint32_t GetPacketsReceived() const { return m_packets_received; }
    uint32_t GetControlPackets() const { return m_control_packets; }

    // Utility
    double CalculatePDR() const {
        if (m_packets_sent == 0) return 0.0;
        return static_cast<double>(m_packets_received) / m_packets_sent;
    }

protected:
    uint32_t m_grid_size;
    double   m_sim_time;
    uint32_t m_num_flows;

    NodeContainer m_nodes;
    NetDeviceContainer m_devices;
    Ipv4InterfaceContainer m_ipv4_interfaces;

    // Metrics counters
    uint32_t m_packets_sent;
    uint32_t m_packets_received;
    uint32_t m_control_packets;

    std::map<uint32_t, Time> m_tx_timestamps;  // For RTT calculation
    std::vector<double> m_rtt_samples;
};

#endif // ROUTING_SIMULATOR_H
