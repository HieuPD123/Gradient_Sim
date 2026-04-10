#ifndef GRADIENT_H
#define GRADIENT_H

#include "ns3/core-module.h"
#include "ns3/header.h"
#include "ns3/application.h"
#include "ns3/net-device.h"
#include "ns3/lr-wpan-net-device.h"
#include "ns3/lr-wpan-mac.h"
#include "ns3/mac16-address.h"
#include "ns3/timer.h"
#include "ns3/nstime.h"
#include "ns3/output-stream-wrapper.h"
#include <map>
#include <set>

using namespace ns3;

// ================== CONSTANTS ==========================
constexpr uint8_t  GRAD_SINK   = 0;
constexpr uint8_t  GRAD_INF    = 8;
constexpr uint8_t  PKT_BEACON  = 0x01;
constexpr uint8_t  PKT_DATA    = 0x02;
constexpr uint16_t GRAD_PROTO  = 0x6789;
constexpr uint8_t  DEFAULT_TTL = 64;

#define SINK_ID        0x0001
#define BEACON_PERIOD  5.0
#define ROUTE_TIMEOUT  15.0
#define DATA_PERIOD    30.0
#define HOLD_DOWN_TIME 1.0

// ================= PACKET HEADER ======================
//  Byte  Field
//  ----  ---------------
//  0     type            (1 byte)
//  1     ttl             (1 byte)
//  2-3   src_address     (2 byte)
//  4-5   dest_address    (2 byte)
//  6-7   sender_address  (2 byte)
//  Total: 8 byte

class GradientHeader : public Header
{
public:
    GradientHeader ();

    static TypeId GetTypeId ();
    TypeId GetInstanceTypeId () const override;

    void     Serialize         (Buffer::Iterator i) const override;
    uint32_t Deserialize       (Buffer::Iterator i)       override;
    uint32_t GetSerializedSize () const override { return 8; }  // FIX #7: 8 byte, không phải 6
    void     Print             (std::ostream &os) const override;

    // Setters
    void SetType     (uint8_t  type)      { this->type           = type;      }
    void SetTTL      (uint8_t  ttl)       { this->ttl            = ttl;       }
    void SetSrcId    (uint16_t src_id)    { this->src_address    = src_id;    }
    void SetDestId   (uint16_t dest_id)   { this->dest_address   = dest_id;   }
    void SetSenderId (uint16_t sender_id) { this->sender_address = sender_id; }

    // FIX #1 + #2: kiểu trả về đúng, tên biến đúng
    uint8_t  GetType     () const { return type;           }
    uint8_t  GetTTL      () const { return ttl;            }
    uint16_t GetSrcId    () const { return src_address;    }
    uint16_t GetDestId   () const { return dest_address;   }
    uint16_t GetSenderId () const { return sender_address; }

private:
    uint8_t  type;
    uint8_t  ttl;
    uint16_t src_address;
    uint16_t dest_address;
    uint16_t sender_address;
};


// ================== ROUTE ENTRY ========================
struct GradientEntry {
    uint8_t           gradient;
    double            RSSI;
    std::set<uint16_t> backprop_dest;
    Time              last_seen;
};  // FIX #3: thêm dấu chấm phẩy


// ================== GRADIENT APP =======================
class GradientApp : public Application
{
public:
    static TypeId GetTypeId ();
    GradientApp ();
    ~GradientApp () override;

    void PrintTable (Ptr<OutputStreamWrapper> stream) const;

protected:
    void StartApplication () override;
    void StopApplication  () override;

private:
    // Timer callbacks
    void SendBeacon ();
    void SendData   ();          // Uplink to sink
    void SendUnsolicited ();     // Periodic unsolicited data (builds backprop)
    void SendDownlinkToNode (uint16_t dest_id);  // Downlink to specific node

    void PurgeTable        ();
    void RecomputeGradient ();

    // Helper: tìm neighbor tốt nhất (gradient nhỏ nhất, RSSI tốt nhất)
    uint16_t FindBestNeighbor () const;

    // Messaging
    void Unicast   (Ptr<Packet> pkt, uint16_t destId);
    void Broadcast (Ptr<Packet> pkt);

    // Receive callback
    bool OnReceive (Ptr<NetDevice> dev, Ptr<const Packet> pkt,
                    uint16_t proto, const Address &src,
                    const Address &dst, NetDevice::PacketType pktType);

    // LR-WPAN helpers
    Ptr<LrWpanNetDevice> GetLrWpanDev () const;
    uint16_t             GetShortAddr () const;

    uint16_t id        {0xFFFF};
    uint16_t sinkId    {SINK_ID};
    bool     isGateway {false};
    uint8_t  gradient  {GRAD_INF};

    std::map<uint16_t, GradientEntry> routing_table;

    Timer beacon_timer;
    Timer data_timer;
    Timer unsolicited_timer;

    // Unsolicited data transmission (for backprop building)
    double m_unsolicited_period {5.0};      // Start at 5s
    uint32_t last_routing_table_size {0};   // Track changes
    static constexpr double MAX_UNSOLICITED_PERIOD = 10800.0;  // 3 hours in seconds
};  // FIX #4: thêm dấu chấm phẩy


#endif // GRADIENT_H