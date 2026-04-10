#include "gradient.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/random-variable-stream.h"
#include "ns3/node.h"

NS_LOG_COMPONENT_DEFINE("GradientRouting");

using namespace ns3;

// ================================================================
//  Helper: Mac16Address <-> uint16_t
// ================================================================
static uint16_t AddrToU16(Mac16Address a)
{
    uint8_t buf[2];
    a.CopyTo(buf);
    return (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
}

static Mac16Address U16ToAddr(uint16_t v)
{
    uint8_t buf[2] = { static_cast<uint8_t>(v >> 8),
                       static_cast<uint8_t>(v & 0xFF) };
    Mac16Address addr;
    addr.CopyFrom(buf);
    return addr;
}

// ================================================================
//  GradientHeader
// ================================================================
NS_OBJECT_ENSURE_REGISTERED(GradientHeader);

GradientHeader::GradientHeader()
    : type(PKT_BEACON), ttl(DEFAULT_TTL),
      src_address(0), dest_address(0), sender_address(0)
{}

TypeId GradientHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::GradientHeader")
        .SetParent<Header>()
        .SetGroupName("Gradient")
        .AddConstructor<GradientHeader>();
    return tid;
}

TypeId GradientHeader::GetInstanceTypeId() const { return GetTypeId(); }

void GradientHeader::Serialize(Buffer::Iterator i) const
{
    i.WriteU8(type);
    i.WriteU8(ttl);
    i.WriteHtonU16(src_address);
    i.WriteHtonU16(dest_address);
    i.WriteHtonU16(sender_address);
}

uint32_t GradientHeader::Deserialize(Buffer::Iterator i)
{
    type           = i.ReadU8();
    ttl            = i.ReadU8();
    src_address    = i.ReadNtohU16();
    dest_address   = i.ReadNtohU16();
    sender_address = i.ReadNtohU16();
    return 8;
}

void GradientHeader::Print(std::ostream &os) const
{
    os << (type == PKT_BEACON ? "BEACON" : "DATA ")
       << " ttl="    << (int)ttl
       << " src=0x"  << std::hex << src_address
       << " dst=0x"  << dest_address
       << " sndr=0x" << sender_address << std::dec;
}

// ================================================================
//  GradientApp — TypeId, constructor, destructor
// ================================================================
NS_OBJECT_ENSURE_REGISTERED(GradientApp);

TypeId GradientApp::GetTypeId()
{
    static TypeId tid = TypeId("ns3::GradientApp")
        .SetParent<Application>()
        .SetGroupName("Gradient")
        .AddConstructor<GradientApp>();
    return tid;
}

GradientApp::GradientApp()  = default;
GradientApp::~GradientApp() = default;

// ================================================================
//  StartApplication / StopApplication
// ================================================================
void GradientApp::StartApplication()
{
    id = GetShortAddr();

    if (isGateway)
        gradient = GRAD_SINK;

    // Đăng ký callback nhận gói
    Ptr<LrWpanNetDevice> dev = GetLrWpanDev();
    NS_ASSERT_MSG(dev, "GradientApp: không tìm thấy LrWpanNetDevice");
    dev->SetReceiveCallback(MakeCallback(&GradientApp::OnReceive, this));

    // Random start delay 0–5 s
    double delay = CreateObject<UniformRandomVariable>()->GetValue(0.0, 10.0);

    beacon_timer.SetFunction(&GradientApp::SendBeacon, this);
    data_timer  .SetFunction(&GradientApp::SendData,   this);

    Simulator::Schedule(Seconds(delay),        &GradientApp::SendBeacon, this);
    Simulator::Schedule(Seconds(delay + 1.0),  &GradientApp::PurgeTable, this);
    if (!isGateway)
        Simulator::Schedule(Seconds(DATA_PERIOD), &GradientApp::SendData, this);

    NS_LOG_INFO("START id=0x" << std::hex << id << " gw=" << isGateway);
}

void GradientApp::StopApplication()
{
    beacon_timer.Cancel();
    data_timer  .Cancel();
}

// ================================================================
//  PurgeTable  — xóa hàng xóm hết hạn
// ================================================================
void GradientApp::PurgeTable()
{
    Time now     = Simulator::Now();
    bool changed = false;

    for (auto it = routing_table.begin(); it != routing_table.end(); )
    {
        if ((now - it->second.last_seen).GetSeconds() > ROUTE_TIMEOUT)
        {
            NS_LOG_INFO("PURGE nb=0x" << std::hex << it->first);
            it = routing_table.erase(it);
            changed = true;
        }
        else ++it;
    }

    if (changed)
    {
        RecomputeGradient();
    }

    Simulator::Schedule(Seconds(1.0), &GradientApp::PurgeTable, this);
}

// ================================================================
//  RecomputeGradient
// ================================================================
void GradientApp::RecomputeGradient()
{
    if (isGateway)
    {
        if (gradient != GRAD_SINK) {
            gradient = GRAD_SINK;
        }
        SendBeacon();
        return;
    }

    bool changed = false;
    uint8_t new_grad;
    if (routing_table.empty())
    {
        new_grad = GRAD_INF;
        changed = true;
    }
    else
    {
        uint8_t min_ng = GRAD_INF;
        for (auto &[_, e] : routing_table)
            if (e.gradient < min_ng) min_ng = e.gradient;

        new_grad = (min_ng >= GRAD_INF - 1) ? GRAD_INF
                                             : static_cast<uint8_t>(min_ng + 1);
    }
    if (gradient != new_grad) {
        changed = true;
    }
    gradient = new_grad;
    
    if (changed) {
        SendBeacon();
    }
}

// ================================================================
//  Unicast / Broadcast
// ================================================================
void GradientApp::Unicast(Ptr<Packet> pkt, uint16_t destId)
{
    Ptr<LrWpanNetDevice> dev = GetLrWpanDev();
    NS_ASSERT_MSG(dev, "Unicast: không có LrWpanNetDevice");
    dev->Send(pkt, U16ToAddr(destId), GRAD_PROTO);
}

void GradientApp::Broadcast(Ptr<Packet> pkt)
{
    Ptr<LrWpanNetDevice> dev = GetLrWpanDev();
    NS_ASSERT_MSG(dev, "Broadcast: không có LrWpanNetDevice");
    dev->Send(pkt, Mac16Address("ff:ff"), GRAD_PROTO);
}

// ================================================================
//  GetLrWpanDev / GetShortAddr
// ================================================================
Ptr<LrWpanNetDevice> GradientApp::GetLrWpanDev() const
{
    auto node = GetNode();
    for (uint32_t i = 0; i < node->GetNDevices(); ++i)
    {
        auto dev = DynamicCast<LrWpanNetDevice>(node->GetDevice(i));
        if (dev) return dev;
    }
    return nullptr;
}

uint16_t GradientApp::GetShortAddr() const
{
    auto dev = GetLrWpanDev();
    if (!dev) return 0xFFFF;
    return AddrToU16(dev->GetMac()->GetShortAddress());
}

// ================================================================
//  FindBestNeighbor — tìm neighbor có gradient nhỏ nhất, RSSI tốt nhất
// ================================================================
uint16_t GradientApp::FindBestNeighbor() const
{
    if (routing_table.empty()) return 0xFFFF;

    uint16_t best_nb = 0xFFFF;
    uint8_t  best_grad = GRAD_INF;
    double   best_rssi = -999.0;

    for (auto &[nb_id, e] : routing_table) {
        // Ưu tiên: gradient nhỏ nhất
        if (e.gradient < best_grad) {
            best_grad = e.gradient;
            best_rssi = e.RSSI;
            best_nb   = nb_id;
        }
        // Nếu gradient bằng, chọn RSSI tốt nhất (lớn nhất)
        else if (e.gradient == best_grad && e.RSSI > best_rssi) {
            best_rssi = e.RSSI;
            best_nb   = nb_id;
        }
    }

    return best_nb;
}

// ================================================================
//  PrintTable
// ================================================================
void GradientApp::PrintTable(Ptr<OutputStreamWrapper> stream) const
{
    auto &os = *stream->GetStream();
    os << "=== RoutingTable id=0x" << std::hex << id
       << " gradient=" << std::dec << (int)gradient << " ===\n";
    for (auto &[nb_id, e] : routing_table)
        os << "  nb=0x"  << std::hex << nb_id
           << "  grad="  << std::dec << (int)e.gradient
           << "  rssi="  << e.RSSI
           << "  last="  << e.last_seen.GetSeconds() << "s\n";
}

// ================================================================
//  SendBeacon
// ================================================================
void GradientApp::SendBeacon()
{
    GradientHeader hdr;
    hdr.SetType    (PKT_BEACON);
    hdr.SetTTL     (0); 
    hdr.SetSrcId   (id);
    hdr.SetDestId  (0xFFFF);
    hdr.SetSenderId(id);
 
    // Payload: 1 byte gradient
    uint8_t grad_val = gradient;
    Ptr<Packet> pkt = Create<Packet>(&grad_val, 1);
    pkt->AddHeader(hdr);
 
    Broadcast(pkt);
 
    NS_LOG_INFO("BEACON  id=0x" << std::hex << id
                << "  grad=" << std::dec << (int)gradient);
 
    beacon_timer.Schedule(Seconds(BEACON_PERIOD));
}

// ================================================================
//  SendData  — BẠN TỰ VIẾT
// ================================================================
void GradientApp::SendData()
{
    GradientHeader hdr;
    hdr.SetType    (PKT_DATA);
    hdr.SetTTL     (64); 
    hdr.SetSrcId   (id);
    hdr.SetDestId  (SINK_ID);
    hdr.SetSenderId(id);
    
    uint32_t data = 0x12345678;
    Ptr<Packet> pkt = Create<Packet>(&data, 4);
    pkt->AddHeader(hdr);

    uint16_t nextHop = FindBestNeighbor();
    if (nextHop != 0xFFFF) {
        Unicast(pkt, nextHop);
        NS_LOG_INFO("DATA SEND id=0x" << std::hex << id 
                    << " to=0x" << nextHop);
    }
    else {
        NS_LOG_INFO("DATA DROP id=0x" << std::hex << id 
                    << " (no route)");
    }
}

// ================================================================
//  OnReceive  — BẠN TỰ VIẾT
// ================================================================
bool GradientApp::OnReceive(Ptr<NetDevice> dev, Ptr<const Packet> pkt,
                             uint16_t proto, const Address &src,
                             const Address &dst, NetDevice::PacketType pktType)
{
    if (proto != GRAD_PROTO) return false;

    uint16_t sender_id = AddrToU16(Mac16Address::ConvertFrom(src));
    if (sender_id == id) return false; // Echo, ignore

    Ptr<Packet> copy = pkt->Copy();
    GradientHeader hdr;
    copy->RemoveHeader(hdr);

    if (hdr.GetType() == PKT_BEACON) {
        // Đọc gradient từ payload (1 byte)
        uint8_t beacon_gradient = 0;
        copy->CopyData(&beacon_gradient, 1);

        uint8_t rssi = dev->GetObject<LrWpanNetDevice>()->GetPhy()->GetRxPower();
        routing_table[sender_id] = { beacon_gradient, rssi, Simulator::Now() };
        RecomputeGradient();
    }
    else if (hdr.GetType() == PKT_DATA) {
        // Đọc dữ liệu từ packet payload (4 bytes)
        uint32_t received_data = 0;
        copy->CopyData((uint8_t*)&received_data, 4);

        if (hdr.GetDestId() == id) {
            // Packet đến đích
            NS_LOG_INFO("DATA RECV id=0x" << std::hex << id
                        << " from=0x" << hdr.GetSrcId()
                        << " data=0x" << received_data);
        }
        else if (hdr.GetTTL() == 0) {
            // TTL hết, drop packet
            NS_LOG_INFO("DATA DROP id=0x" << std::hex << id 
                        << " TTL expired");
        }
        else {
            // Forward packet tới next hop
            GradientHeader new_hdr = hdr;
            new_hdr.SetTTL(hdr.GetTTL() - 1);
            new_hdr.SetSenderId(id);

            Ptr<Packet> fwd_pkt = copy->Copy();
            fwd_pkt->AddHeader(new_hdr);

            // Chọn next hop (neighbor có gradient nhỏ nhất, RSSI tốt nhất)
            uint16_t next_hop = FindBestNeighbor();
            if (next_hop != 0xFFFF) {
                Unicast(fwd_pkt, next_hop);
                NS_LOG_INFO("DATA FWD id=0x" << std::hex << id 
                            << " to=0x" << next_hop);
            }
        }
    }

    return true;
}