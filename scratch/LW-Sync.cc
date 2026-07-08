#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/uan-module.h"
#include "ns3/acoustic-modem-energy-model-helper.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/energy-source-container.h"
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LW-Sync");

uint32_t seed;
uint32_t run;
char plotNum;

double initVelocity;
double maxVelocity;
double acceleration;

double LWOffsetError = 0;
double LWSkewError = 0;

bool track1 = false;
bool track2 = true;

double dataRate;

class LWBeacon : public Application
{
public:
	LWBeacon ();
	virtual ~LWBeacon ();

	void SetParameters (double beaconInterval, uint32_t beaconCount, double timingError, double velocityError);
	void SetSoundSpeed (double speed);
	void SetCarrierFreq (double freq);
	void SetPacketSize (uint32_t size);
	void SetOrdinaryAddress (Mac8Address address);

private:
	virtual void StartApplication (void);
	virtual void StopApplication (void);

	bool ReceivePacket(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &sender);
	void SendReply();
	void SendBeacons();

	Ptr<UanNetDevice> m_device;
	EventId m_sendEvent;
	bool m_running;
	double m_beaconInterval;
	uint32_t m_beaconSequence;
	uint32_t m_beaconCount;

	double m_estimatedOffset;
	double m_T2;
	double m_T3;
	double m_soundSpeed;
	double m_carrierFreq;
	double m_timeError;
	double m_velocityError;
	uint32_t m_pktSize;
	Mac8Address m_ordinaryAddress;

	uint32_t m_recieveCount = 0;
};

LWBeacon::LWBeacon ()
  : m_running (false),
	m_beaconInterval (5),
	m_beaconSequence (0),
	m_beaconCount (10),
	m_estimatedOffset (0),
	m_T2 (0.0),
	m_T3 (0.0),
    m_soundSpeed (1500.0),
    m_carrierFreq (20000.0),
	m_timeError (20e-6),
	m_velocityError (0.2),
	m_pktSize (60)
{
}

LWBeacon::~LWBeacon ()
{
}

void
LWBeacon::SetParameters (double beaconInterval, uint32_t beaconCount, double timingError, double velocityError)
{
	m_beaconInterval = beaconInterval;
	m_beaconCount = beaconCount;
	m_timeError = timingError;
	m_velocityError = velocityError;
}

void
LWBeacon::SetSoundSpeed (double speed)
{
	m_soundSpeed = speed;
}

void
LWBeacon::SetCarrierFreq (double freq)
{
	m_carrierFreq = freq;
}

void
LWBeacon::SetPacketSize (uint32_t size)
{
	m_pktSize = size;
}

void
LWBeacon::SetOrdinaryAddress (Mac8Address address)
{
	m_ordinaryAddress = address;
}

void
LWBeacon::StartApplication (void)
{
	m_running = true;
	m_beaconSequence = 0;

	m_device = GetNode ()->GetDevice (0)->GetObject<UanNetDevice> ();

	if (m_device)
    {
		m_device->SetReceiveCallback (MakeCallback (&LWBeacon::ReceivePacket, this)); // @suppress("Ambiguous problem") // @suppress("Invalid arguments")
    }
	else
    {
		NS_LOG_ERROR ("LWBeacon: Could not get UAN device!");
    }
}

void
LWBeacon::StopApplication (void)
{
	m_running = false;
	if (m_sendEvent.IsRunning ())
    {
		Simulator::Cancel (m_sendEvent);
    }
}

class LWOrdinary : public Application
{
public:
	LWOrdinary ();
	virtual ~LWOrdinary ();

	void SetParameters (uint32_t beaconCount, double clockSkew, double clockOffset, double timingError, double velocityError);
	void SetSoundSpeed (double speed);
	void SetCarrierFreq (double freq);
	void SetPacketSize (uint32_t size);
	void SetBeaconAddress (Mac8Address address);
	void StopAcceleration();

private:
	virtual void StartApplication (void);
	virtual void StopApplication (void);

	void SendRequest();
	bool ReceivePacket(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &sender);
	void SendReply(double V4);

	Ptr<UanNetDevice> m_device;
	EventId m_sendEvent;
	bool m_running;
	uint32_t m_receiveSequence;
	uint32_t m_beaconCount;

	double m_clockSkew;
	double m_clockOffset;
	double m_estimatedSkew;
	double m_estimatedOffset;
	double m_T1;
	double m_T4;
	double m_T5;
	double m_soundSpeed;
	double m_carrierFreq;
	double m_timeError;
	double m_velocityError;
	uint32_t m_pktSize;
	Mac8Address m_beaconAddress;

	double m_sendCount = 0;
	double m_lastBeaconReceiveTime = 0.0;
	double m_lastBeaconSendTime = 0.0;
	double m_averageSkew = 0;

	double currentVelocity = 0;
	double lastVelocity = 0;
};

LWOrdinary::~LWOrdinary ()
{
}

LWOrdinary::LWOrdinary ()
  : m_running (false),
	m_receiveSequence(0),
	m_beaconCount (0),
	m_clockSkew (200),
	m_clockOffset (0.02),
	m_estimatedSkew (0),
	m_estimatedOffset (0),
	m_T1 (0.0),
	m_T4 (0.0),
	m_T5 (0.0),
    m_soundSpeed (1500.0),
    m_carrierFreq (20000.0),
	m_timeError (20e-6),
	m_velocityError (0.2),
	m_pktSize (60)
{
}

void
LWOrdinary::SetParameters (uint32_t beaconCount, double clockSkew, double clockOffset, double timingError, double velocityError){
	m_beaconCount = beaconCount;
	m_clockSkew = clockSkew;
	m_clockOffset = clockOffset;
	m_timeError = timingError;
	m_velocityError = velocityError;
}

void
LWOrdinary::SetSoundSpeed (double speed){
	m_soundSpeed = speed;
}

void
LWOrdinary::SetCarrierFreq (double freq){
	m_carrierFreq = freq;
}

void
LWOrdinary::SetPacketSize (uint32_t size){
	m_pktSize = size;
}

void
LWOrdinary::SetBeaconAddress (Mac8Address address){
	m_beaconAddress = address;
}

void
LWOrdinary::StopAcceleration(){
	Ptr<ConstantAccelerationMobilityModel> mobility = GetNode()-> GetObject<ConstantAccelerationMobilityModel> ();
	double velocity = mobility->GetVelocity().x;
	mobility->SetVelocityAndAcceleration(Vector(velocity, 0.0, 0.0), Vector(0, 0.0, 0.0));
}

void
LWOrdinary::StartApplication (void)
{
	m_running = true;

	m_receiveSequence = 0;
	m_device = GetNode ()->GetDevice (0)->GetObject<UanNetDevice> ();

	if (m_device)
	{
	 	m_device->SetReceiveCallback (MakeCallback (&LWOrdinary::ReceivePacket, this)); // @suppress("Ambiguous problem") // @suppress("Invalid arguments")
	 	m_sendEvent = Simulator::ScheduleNow(&LWOrdinary::SendRequest, this);

		if(track1){
			double stopAccelerationInterval = (maxVelocity - initVelocity) / acceleration;
			Simulator::Schedule(Seconds(stopAccelerationInterval),&LWOrdinary::StopAcceleration, this);
		}
    }
	else
    {
		NS_LOG_ERROR ("LTOrdinary: Could not get UAN device!");
    }
}

void
LWOrdinary::StopApplication (void)
{
	m_running = false;
	if (m_sendEvent.IsRunning ())
    {
		Simulator::Cancel (m_sendEvent);
    }
}

Ptr<LWBeacon> LWBeaconApp;
Ptr<LWOrdinary> LWOrdinaryApp;

double LWGetVelocity(){
	double radialVelocity = 0;
	if(track1){
		Ptr<ConstantAccelerationMobilityModel> mobility = LWOrdinaryApp->GetNode()-> GetObject<ConstantAccelerationMobilityModel> ();
		radialVelocity = mobility->GetVelocity().x;
	}else if(track2){
		Ptr<MobilityModel> mobNode = LWOrdinaryApp->GetNode()->GetObject<MobilityModel>();
		Ptr<MobilityModel> mobGw = LWBeaconApp->GetNode()->GetObject<MobilityModel>();

		Vector pNode = mobNode->GetPosition();
		Vector pGw   = mobGw->GetPosition();
		Vector vel   = mobNode->GetVelocity();

		double dx = pNode.x - pGw.x;
		double dy = pNode.y - pGw.y;
		double dz = pNode.z - pGw.z;

		double distance = sqrt(dx*dx + dy*dy + dz*dz);

		radialVelocity = (vel.x * dx + vel.y * dy + vel.z * dz) / distance;
	}
	return radialVelocity;
}

void
LWOrdinary::SendRequest(){
	m_sendCount++;
	double tSend = Simulator::Now().GetSeconds() * (1 + (m_clockSkew * 1e-6)) + m_clockOffset;
	m_T1 = tSend;

	struct Request_Data {
		double T1;
	} data;

	data.T1 = m_T1;

	Ptr<Packet> packet = Create<Packet> (reinterpret_cast<const uint8_t*> (&data), sizeof(data));

	if (m_pktSize > packet->GetSize ()) {
		uint32_t paddingSize = m_pktSize - packet->GetSize ();
		Ptr<Packet> padding = Create<Packet> (paddingSize);
		packet->AddAtEnd (padding);
	}
	if (packet)
	{
		m_device->GetMac ()->Enqueue (packet, 0, m_beaconAddress);
		NS_LOG_DEBUG ("LWOrdinary sent request ("<< m_sendCount<<") packet at: " << tSend << " s, Packet size = " << packet->GetSize() << " Bytes");
	}
}

bool
LWBeacon::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &sender)
{
	m_recieveCount++;
	double serialization_time = ((packet->GetSize() + 3) * 8) / dataRate;
	double TReceive = Simulator::Now().GetSeconds() - serialization_time;

	Ptr<NormalRandomVariable> timestampNoiseVar = CreateObject<NormalRandomVariable>();
	timestampNoiseVar->SetAttribute("Mean", DoubleValue(0.0));
	timestampNoiseVar->SetAttribute("Variance", DoubleValue(m_timeError * m_timeError));
	double timestampNoise = timestampNoiseVar->GetValue();

	TReceive = TReceive + timestampNoise;

	Ptr<NormalRandomVariable> velocityNoiseVar = CreateObject<NormalRandomVariable>();
	velocityNoiseVar->SetAttribute("Mean", DoubleValue(0.0));
	velocityNoiseVar->SetAttribute("Variance", DoubleValue(m_velocityError * m_velocityError));
	double velocityNoise = velocityNoiseVar->GetValue();

	Ptr<Packet> p = packet->Copy();
	if(m_recieveCount ==1){
		m_T2 = TReceive;
		struct Request_Data {
			double T1;
		} data;
		p->CopyData (reinterpret_cast<uint8_t*> (&data), sizeof(data));
		NS_LOG_DEBUG ("LWBeacon received request at :"<< TReceive<< " s, Packet size :"<< packet->GetSize()<< " Bytes");
		m_sendEvent = Simulator::ScheduleNow(&LWBeacon::SendReply, this);
	}else if(m_recieveCount == 2){
		struct Packet_Data {
			double V4;
			double T4;
			double T5;
			double T4_hat;
			double T5_hat;
		} data;

		double T6 = TReceive;
		p->CopyData (reinterpret_cast<uint8_t*> (&data), sizeof(data));

		double V6 = LWGetVelocity() + velocityNoise;

		double n = ((data.V4*(data.T5_hat - data.T4_hat)) + ((V6*(T6 - data.T5_hat)))) / (2*m_soundSpeed);

		m_estimatedOffset = (((T6 - data.T4) + (m_T3 - data.T5)) / 2) - n;

		NS_LOG_DEBUG("LWBeacon received reply at :"<< TReceive<< " s, Packet size :"<< packet->GetSize()<< " Bytes");

		Simulator::ScheduleNow(&LWBeacon::SendBeacons, this);
	}
	return true; // Packet was handled
}

void
LWBeacon::SendReply(){
	m_T3 = Simulator::Now().GetSeconds();

	struct Reply_Data {
		double T2;
		double T3;
	} data;
	data.T2 = m_T2;
	data.T3 = m_T3;
	Ptr<Packet> packet = Create<Packet> (reinterpret_cast<const uint8_t*> (&data), sizeof(data));
	if (m_pktSize > packet->GetSize ()) {
		uint32_t paddingSize = m_pktSize - packet->GetSize ();
		Ptr<Packet> padding = Create<Packet> (paddingSize);
		packet->AddAtEnd (padding);
	}

	if (packet)
	{
		m_device->GetMac ()->Enqueue (packet, 0, m_ordinaryAddress);
	}
	NS_LOG_DEBUG ("LWBeacon sent Reply at: " << m_T3 << " s, Packet Size = "<<packet->GetSize()<<" Bytes");
}

void
LWBeacon::SendBeacons()
{
	double TSend = Simulator::Now().GetSeconds();

	m_beaconSequence++;
	Ptr<Packet> packet;
	if(m_beaconSequence == 1){
		struct Beacon_Data {
			double TSend;
			double offset;
		} data;

		data.TSend = TSend;
		data.offset = m_estimatedOffset;

		packet = Create<Packet> (reinterpret_cast<const uint8_t*> (&data), sizeof(data));
		if (m_pktSize > packet->GetSize ()) {
			uint32_t paddingSize = m_pktSize - packet->GetSize ();
			Ptr<Packet> padding = Create<Packet> (paddingSize);
			packet->AddAtEnd (padding);
		}
	}else{
		struct Beacon_Data {
			double TSend;
		} data;

		data.TSend = TSend;

		packet = Create<Packet> (reinterpret_cast<const uint8_t*> (&data), sizeof(data));
		if (m_pktSize > packet->GetSize ()) {
			uint32_t paddingSize = m_pktSize - packet->GetSize ();
			Ptr<Packet> padding = Create<Packet> (paddingSize);
			packet->AddAtEnd (padding);
		}
	}

	if (packet)
	{
		m_device->GetMac ()->Enqueue (packet, 0, m_ordinaryAddress);
	}
	NS_LOG_DEBUG ("LWBeacon sent beacon number ("<<m_beaconSequence<<") at: " << TSend << " s, Packet Size = "<<packet->GetSize()<<" Bytes");
	if(m_beaconSequence < m_beaconCount){
		Simulator::Schedule(Seconds(m_beaconInterval),&LWBeacon::SendBeacons, this);
	}
}

bool
LWOrdinary::ReceivePacket(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &sender){
	Ptr<NormalRandomVariable> timestampNoiseVar = CreateObject<NormalRandomVariable>();
	timestampNoiseVar->SetAttribute("Mean", DoubleValue(0.0));
	timestampNoiseVar->SetAttribute("Variance", DoubleValue(m_timeError * m_timeError));
	double timestampNoise = timestampNoiseVar->GetValue();

	double serialization_time = ((packet->GetSize() + 3) * 8) / 20000.0;
	double tReceive = Simulator::Now().GetSeconds() - serialization_time;
	tReceive = tReceive * (1 + (m_clockSkew * 1e-6)) + m_clockOffset + timestampNoise;

	Ptr<Packet> p = packet->Copy();

	if(m_sendCount == 1){
		m_sendCount++;
		Ptr<NormalRandomVariable> velocityNoiseVar = CreateObject<NormalRandomVariable>();
		velocityNoiseVar->SetAttribute("Mean", DoubleValue(0.0));
		velocityNoiseVar->SetAttribute("Variance", DoubleValue(m_velocityError * m_velocityError));
		double velocityNoise = velocityNoiseVar->GetValue();
		struct Reply_Data {
			double T2;
			double T3;
		} data;
		p->CopyData (reinterpret_cast<uint8_t*> (&data), sizeof(data));

		m_T4 = tReceive;
		double V4 = LWGetVelocity() + velocityNoise;

		m_estimatedOffset = ((data.T2- m_T4) + (data.T3 - m_T1)) / 2;

		NS_LOG_DEBUG ("LWOrdinary received beacon ("<<m_receiveSequence<< ") at "<<tReceive<<" s, Packet size :"<< packet->GetSize()<< " Bytes");

		Simulator::ScheduleNow(&LWOrdinary::SendReply, this, V4);
	}else{
		Ptr<NormalRandomVariable> velocityNoiseVar = CreateObject<NormalRandomVariable>();
		velocityNoiseVar->SetAttribute("Mean", DoubleValue(0.0));
		velocityNoiseVar->SetAttribute("Variance", DoubleValue(m_velocityError * m_velocityError));
		double velocityNoise = velocityNoiseVar->GetValue();
		currentVelocity = LWGetVelocity() + velocityNoise;

		double velocity;
		m_receiveSequence++;
		double tSend=0;

		NS_LOG_DEBUG ("LWOrdinary received beacon ("<<m_receiveSequence<< ") at "<<tReceive<<" s, Packet size :"<< packet->GetSize()<< " Bytes");

		if(m_receiveSequence == 1){
			struct Beacon_Data {
				double TSend;
				double offset;
			} data;
			p->CopyData (reinterpret_cast<uint8_t*> (&data), sizeof(data));
			tSend = data.TSend;

			m_estimatedOffset = data.offset;

			velocity = currentVelocity;
		}else{
			struct Beacon_Data {
				double TSend;
			} data;
			p->CopyData (reinterpret_cast<uint8_t*> (&data), sizeof(data));
			tSend = data.TSend;
			velocity = (currentVelocity + lastVelocity) / 2;
		}

		if (m_lastBeaconReceiveTime > 0) {
			double T = m_lastBeaconSendTime - tSend;
			double delta_t = m_lastBeaconReceiveTime - tReceive;
			double motionDelay = 1 + (velocity/m_soundSpeed);
			double skew = T / delta_t;
			skew = skew * motionDelay ;
			skew = skew - 1 ;
			m_averageSkew = m_averageSkew + skew;
		}

		m_lastBeaconReceiveTime = tReceive;
		m_lastBeaconSendTime = tSend;
		lastVelocity = currentVelocity;

		if (m_receiveSequence == m_beaconCount){
			m_averageSkew = m_averageSkew / (m_beaconCount-1);
			m_estimatedSkew = m_averageSkew;

			LWOffsetError = std::abs(std::abs(m_estimatedOffset) - std::abs(m_clockOffset))*1e6;
			LWSkewError = std::abs((((m_clockSkew*1e-6)) - std::abs(m_estimatedSkew)))*1e6;
		}
	}

	return true;
}

void
LWOrdinary::SendReply(double V4){
	double m_T5 = Simulator::Now().GetSeconds() * (1 + (m_clockSkew * 1e-6)) + m_clockOffset;

	struct Packet_Data {
		double V4;
		double T4;
		double T5;
		double T4_hat;
		double T5_hat;
	} data;

	data.V4 = V4;
	data.T4 = m_T4;
	data.T5 = m_T5;
	data.T4_hat = m_T4 + m_estimatedOffset;
	data.T5_hat = m_T5 + m_estimatedOffset;

	Ptr<Packet> packet = Create<Packet> (reinterpret_cast<const uint8_t*> (&data), sizeof(data));

	if (m_pktSize > packet->GetSize ()) {
		uint32_t paddingSize = m_pktSize - packet->GetSize ();
		Ptr<Packet> padding = Create<Packet> (paddingSize);
		packet->AddAtEnd (padding);
	}

	if (packet)
	{
		m_device->GetMac ()->Enqueue (packet, 0, m_beaconAddress);
		NS_LOG_DEBUG ("LWOrdinary sent reply packet at: " << m_T5 << " s, Packet size = " << packet->GetSize() << " Bytes");
	}
}

UanModesList
CreateUnderwaterModes ()
{
  UanModesList modes;
  UanTxMode mode;

  mode = UanTxModeFactory::CreateMode (UanTxMode::FSK,
                                       dataRate, // 20 kbps data rate
                                       2400,     // 2.4 ksym/s
									   20000,    // 20 kHz carrier
                                       4000,     // 4 kHz bandwidth
                                       2,        // binary FSK
                                       "Underwater mode");
  modes.AppendMode (mode);
  return modes;
}

int
main (int argc, char *argv[])
{
	seed = 1;
	run = 0;

	plotNum = '0';
	track1 = true;
	track2 = false;

	initVelocity = 2;
	acceleration = 0.03;

	dataRate = 20000;

	double soundSpeed = 1500.0;
	double velocity = 2;
	double initDistance = 500.0;
	int angleIndex = 0; //[0,7] [0 = 0, 1 = 45, 2 = 90, 3 = 135, 4 = 180, 5 = 225, 6 = 270, 7 = 315)
	double carrierFreq = 20000.0;
	double beaconCount = 10;
	double LWBeaconInterval = 5; //5 seconds
	double clockSkew = 200;   //200 ppm
	double clockOffset = 0.02; //20 ms
	double timeError = 20e-6; //20us
	double velocityError = 0.2; //0.2 m/s
	uint32_t pktSize = 60;  //60 byte

	double radius = 100.0;
	double omega = velocity / radius; // Result: 0.04 rad/s

	CommandLine cmd;
	cmd.AddValue("seed", "Random seed", seed);
	cmd.AddValue("run", "Run number", run);
	cmd.AddValue("plotNum", "Plot number", plotNum);
	cmd.AddValue("track1", "Track type", track1);
	cmd.AddValue("track2", "Track type", track2);
	cmd.AddValue ("soundSpeed", "Speed of sound in water (m/s)", soundSpeed);
	cmd.AddValue ("clockSkew", "clock skew (ppm)", clockSkew);
	cmd.AddValue ("clockOffset", "clock offset (s)", clockOffset);
	cmd.AddValue ("initVelocity", "Ordinary velocity (m/s)", initVelocity);
	cmd.AddValue ("acceleration", "Ordinary acceleration (m/s^2)", acceleration);
	cmd.AddValue ("timeError", "timestamp noise (us)", timeError);
	cmd.AddValue ("velocityError", "velocity noise (m/s)", velocityError);
	cmd.AddValue ("initDistance", "Initial distance between nodes (m)", initDistance);
	cmd.AddValue ("angleIndex", "Initial angle of track2 (m)", angleIndex);
	cmd.AddValue ("carrierFreq", "Carrier frequency (Hz)", carrierFreq);
	cmd.AddValue ("beaconCount", "Number of beacons", beaconCount);
	cmd.AddValue ("beaconInterval", "Time to between beacons (s)", LWBeaconInterval);
	cmd.AddValue ("pktSize", "Packet size in bytes", pktSize);

	cmd.Parse (argc, argv);

	RngSeedManager::SetSeed(seed);
	RngSeedManager::SetRun(run);

	if(track2){
		initDistance = initDistance - radius;
	}

	maxVelocity = initVelocity + 3;

	double runtime = (beaconCount * LWBeaconInterval) + 10;

	// Enable detailed UAN logging - use INFO level to see timing information
	LogComponentEnable ("UanPhyGen", LOG_LEVEL_INFO);
	LogComponentEnable ("UanMacAloha", LOG_LEVEL_INFO);
	LogComponentEnable ("UanChannel", LOG_LEVEL_INFO);
	LogComponentEnable ("UanNetDevice", LOG_LEVEL_INFO);

	// Enable our application logging
	LogComponentEnable ("LW-Sync", LOG_LEVEL_INFO);

	// Create nodes
	NodeContainer LWNodes;
	LWNodes.Create (2);

	Ptr<ConstantAccelerationMobilityModel> LWSyncMobility1 = CreateObject<ConstantAccelerationMobilityModel>();

	LWSyncMobility1->SetPosition(Vector(0.0, 0.0, -100.0));
	LWSyncMobility1->SetVelocityAndAcceleration(Vector(0, 0.0, 0.0), Vector(0, 0.0, 0.0));

	LWNodes.Get (0)->AggregateObject(LWSyncMobility1);

	if(track1){
		Ptr<ConstantAccelerationMobilityModel> LWSyncMobility2 = CreateObject<ConstantAccelerationMobilityModel>();
		LWSyncMobility2->SetPosition(Vector(initDistance, 0.0, -100.0));
		LWSyncMobility2->SetVelocityAndAcceleration(Vector(initVelocity, 0.0, 0.0), Vector(acceleration, 0.0, 0.0));
		LWNodes.Get (1)->AggregateObject(LWSyncMobility2);
	}else if(track2){
		Ptr<WaypointMobilityModel> LWwaypoints = CreateObject<WaypointMobilityModel>();
		double initialPhase = angleIndex * (M_PI / 4.0);

		for (double t = 0; t < runtime; t += 0.1) {

			double phase = (omega * t) + initialPhase;

		    double LWx = initDistance + radius * cos(phase);
		    double y = radius * sin(phase);

		    LWwaypoints->AddWaypoint(Waypoint(Seconds(t), Vector(LWx, y, -100.0)));
		}

		LWNodes.Get(1)->AggregateObject(LWwaypoints);
	}

	// Create UAN channel with realistic propagation and noise models
	Ptr<UanChannel> channel = CreateObject<UanChannel> ();

	// Use Thorp propagation model for realistic underwater attenuation
	Ptr<UanPropModelThorp> propModel = CreateObject<UanPropModelThorp> ();
	channel->SetPropagationModel (propModel);

	// Add realistic underwater noise model
	Ptr<UanNoiseModelDefault> noiseModel = CreateObject<UanNoiseModelDefault> ();
	// You can adjust noise levels for different sea states
	noiseModel->SetAttribute ("Wind", DoubleValue (5.0)); // 5 m/s wind speed
	noiseModel->SetAttribute ("Shipping", DoubleValue (0.5)); // Shipping activity factor
	channel->SetNoiseModel (noiseModel);

	// Configure UAN helper with realistic parameters
	UanHelper uan;

	UanModesList realisticModes = CreateUnderwaterModes ();

	// Set realistic PHY parameters
	uan.SetPhy ("ns3::UanPhyGen",
             	 "TxPower", DoubleValue (160),      // More realistic 160 dB (instead of 180)
				 "SupportedModes", UanModesListValue (realisticModes),
				 "PerModel", StringValue ("ns3::UanPhyPerGenDefault"),
				 "SinrModel", StringValue ("ns3::UanPhyCalcSinrDefault"));

	uan.SetMac ("ns3::UanMacAloha");

	// Install UAN devices
	NetDeviceContainer LWDevices = uan.Install (LWNodes, channel);

	// Create energy sources for both nodes
	BasicEnergySourceHelper LWEnergySourceHelper;
	LWEnergySourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (1000000)); // 1 MJ initial energy
	LWEnergySourceHelper.Set ("BasicEnergySupplyVoltageV", DoubleValue (12)); // 12V typical for underwater systems

	EnergySourceContainer LWEnergySources = LWEnergySourceHelper.Install (LWNodes);

	// Create UAN energy model helper
	AcousticModemEnergyModelHelper LWAcousticModemEnergyHelper; // @suppress("Abstract class cannot be instantiated")

	// Install energy models on UAN devices
	DeviceEnergyModelContainer LWDeviceEnergyModels = LWAcousticModemEnergyHelper.Install (LWDevices, LWEnergySources);

	auto LWPrintEnergyReport = [&]() {
		std::cout << "\n=== LW DETAILED ENERGY CONSUMPTION REPORT ===" << std::endl;

		for (uint32_t i = 0; i < LWNodes.GetN(); i++)
		{
			Ptr<EnergySource> energySource = LWEnergySources.Get(i);
			Ptr<DeviceEnergyModel> energyModel = LWDeviceEnergyModels.Get(i);

			std::cout << "Node " << i << " (" << (i == 0 ? "LWBeacon" : "LWOrdinary") << "):" << std::endl;
			std::cout << "  Initial energy: " << energySource->GetInitialEnergy() << " J" << std::endl;
			std::cout << "  Remaining energy: " << energySource->GetRemainingEnergy() << " J" << std::endl;
			std::cout << "  Total consumed: " << energyModel->GetTotalEnergyConsumption() << " J" << std::endl;
		}

		// Protocol efficiency analysis
		std::cout << "\n--- Protocol Efficiency ---" << std::endl;
		std::cout << "Total energy for one synchronization: "
				<< (LWDeviceEnergyModels.Get(0)->GetTotalEnergyConsumption() +
						LWDeviceEnergyModels.Get(1)->GetTotalEnergyConsumption()) << " J" << std::endl;
		std::cout << "Beacon/Ordinary energy ratio: "
				<< (LWDeviceEnergyModels.Get(0)->GetTotalEnergyConsumption() /
						LWDeviceEnergyModels.Get(1)->GetTotalEnergyConsumption()) << std::endl;
		std::cout << "==========================================" << std::endl;
	};

	// Get MAC addresses
	Ptr<UanNetDevice> LWBeaconDevice = LWDevices.Get (0)->GetObject<UanNetDevice> ();
	Ptr<UanNetDevice> LWOrdinaryDevice = LWDevices.Get (1)->GetObject<UanNetDevice> ();

	Mac8Address LWBeaconMac = Mac8Address::ConvertFrom (LWBeaconDevice->GetMac ()->GetAddress ());
	Mac8Address LWOrdainryMac = Mac8Address::ConvertFrom (LWOrdinaryDevice->GetMac ()->GetAddress ());

	// Create applications
	// Beacon application
	LWBeaconApp = CreateObject<LWBeacon> ();
	LWBeaconApp->SetParameters (LWBeaconInterval, beaconCount, timeError, velocityError);
	LWBeaconApp->SetSoundSpeed (soundSpeed);
	LWBeaconApp->SetCarrierFreq (carrierFreq);
	LWBeaconApp->SetPacketSize (pktSize);
	LWBeaconApp->SetOrdinaryAddress(LWOrdainryMac);
	LWNodes.Get (0)->AddApplication (LWBeaconApp);
	LWBeaconApp->SetStartTime (Seconds (0));

	// Ordinary application
	LWOrdinaryApp = CreateObject<LWOrdinary> ();
	LWOrdinaryApp->SetParameters (beaconCount, clockSkew, clockOffset, timeError, velocityError);
	LWOrdinaryApp->SetSoundSpeed (soundSpeed);
	LWOrdinaryApp->SetCarrierFreq (carrierFreq);
	LWOrdinaryApp->SetPacketSize (pktSize);
	LWOrdinaryApp->SetBeaconAddress(LWBeaconMac);
	LWNodes.Get (1)->AddApplication (LWOrdinaryApp);
	LWOrdinaryApp->SetStartTime (Seconds (0));

	auto PrintResuLWs = [&]() {
		int angle = 0;
		switch (angleIndex){
			case 0:{
				angle = 0;
				break;
			}
			case 1:{
				angle = 45;
				break;
			}
			case 2:{
				angle = 90;
				break;
			}
			case 3:{
				angle = 135;
				break;
			}
			case 4:{
				angle = 180;
				break;
			}
			case 5:{
				angle = 225;
				break;
			}
			case 6:{
				angle = 270;
				break;
			}
			case 7:{
				angle = 315;
				break;
			}
		}

		switch (plotNum) {
			case'0':{
				LWPrintEnergyReport();
				std::cout << "=== LW Results ===" << std::endl;
				std::cout << "LW offset error = " <<LWOffsetError<<" us"<< std::endl;
				std::cout << "LW skew error = " <<LWSkewError<<" ppm"<< std::endl;
				break;
			}
			case '1':{
				std::ofstream plot("csv/plot1.csv", std::ios::app);
				if (track1) {
				    if (run == 1 && initDistance == 500) {
				        plot << "Track 1,,,,,Track 2\n";
				        plot << "Init. Distance (m),LW-Offset Error (us),LT-Offset Error (us),DC-Offset Error (us),,LW-Offset Error (us),LT-Offset Error (us),DC-Offset Error (us)\n";
				    }
				    plot << initDistance << "," << LWOffsetError << ",";
				}
				else if (track2) {
				    plot << "," << LWOffsetError << ",";
				}
			break;
			}
			case '2':{
				std::ofstream plot("csv/plot2.csv", std::ios::app);
				if (run == 1 && angle == 0) {
					plot << "Init. Node Angle,LW-Offset Error (us),LT-Offset Error (us),DC-Offset Error (us)\n";
				}
				plot << angle << "," << LWOffsetError << ",";
			break;
			}
			case '3':{
				std::ofstream plot("csv/plot3.csv", std::ios::app);
				if (run == 1 && initVelocity == 0) {
					plot << "Init. Radial Velocity (m/s),LW-Offset Error (us),LT-Offset Error (us),DC-Offset Error (us)\n";
				}
				plot << initVelocity << "," << LWOffsetError << ",";
			break;
			}
			case '4':{
				std::ofstream plot("csv/plot4.csv", std::ios::app);
				if (run == 1 && acceleration == 0.01) {
					plot << "Acceleration,LW-Offset Error (us),LT-Offset Error (us),DC-Offset Error (us)\n";
				}
				plot << acceleration << "," << LWOffsetError << ",";
			break;
			}
			case '5':{
				std::ofstream plot("csv/plot5.csv", std::ios::app);
				if(track1){
					if(run == 1 && velocityError == 0.0){
						plot << "Track 1,,,,,Track 2\n";
						plot << "STD. Dev. Velocity Noise (m/s),LW-Offset Error (us),LT-Offset Error (us),DC-Offset Error (us),,LW-Offset Error (us),LT-Offset Error (us),DC-Offset Error (us)\n";
					}
				    plot << velocityError << "," << LWOffsetError << ",";
				}else if (track2){
					plot << "," << LWOffsetError << ",";
				}
				break;
			}
			case '6':{
				std::ofstream plot("csv/plot6.csv", std::ios::app);
				if(track1){
					if(run == 1 && timeError == 10e-6){
						plot << "Track 1,,,,,Track 2\n";
						plot << "STD. Dev. Jitter Noise (us),LW-Offset Error (us),LT-Offset Error (us),DC-Offset Error (us),,LW-Offset Error (us),LT-Offset Error (us),DC-Offset Error (us)\n";
					}
					plot << timeError * 1e6 << "," << LWOffsetError << ",";
				}else if (track2){
					plot << "," << LWOffsetError << ",";
				}
				break;
			}
			case '7':{
				std::ofstream plot("csv/plot7.csv", std::ios::app);
				if(track1){
					if(run == 1 && clockOffset == 0.02){
						plot << "Track 1,,,,,Track 2\n";
						plot << "Init. Offset (s),LW-Offset Error (us),LT-Offset Error (us),DC-Offset Error (us),,LW-Offset Error (us),LT-Offset Error (us),DC-Offset Error (us)\n";
					}
					plot << clockOffset << "," << LWOffsetError << ",";
				}else if (track2){
					plot << "," << LWOffsetError << ",";
				}
				break;
			}
			case '8':{
				std::ofstream plot("csv/plot8.csv", std::ios::app);
				if(track1){
					if(run == 1 && clockSkew == 100){
						plot << "Track 1,,,,,Track 2\n";
						plot << "Init. Skew (ppm),LW-Offset Error (us),LT-Offset Error (us),DC-Offset Error (us),,LW-Offset Error (us),LT-Offset Error (us),DC-Offset Error (us)\n";
					}
					plot << clockSkew << "," << LWOffsetError << ",";
				}else if (track2){
					plot << "," << LWOffsetError << ",";
				}
				break;
			}
			case '9':{
				std::ofstream plot("csv/plot9a.csv", std::ios::app);
				if(track1){
					if(run == 1 && beaconCount == 10){
						plot << "Track 1,,,Track 2\n";
						plot << "One-way Beacons,LW-Skew Error (ppm),,LW-Skew Error (ppm)\n";
					}
					plot << beaconCount << "," << LWSkewError << ",";
				}else if (track2){
					plot << "," << LWSkewError << "\n";
				}
				break;
			}
			default:{
				break;
			}
		}
	};

	Simulator::Schedule (Seconds (runtime), PrintResuLWs);

	Simulator::Stop (Seconds(runtime));
	Simulator::Run ();
	Simulator::Destroy ();

	return 0;
}
