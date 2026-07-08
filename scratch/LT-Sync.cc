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

NS_LOG_COMPONENT_DEFINE ("LT-Sync");

uint32_t seed;
uint32_t run;
char plotNum;

double initVelocity;
double maxVelocity;
double acceleration;

double LTOffsetError = 0;
double LTSkewError = 0;

bool track1 = false;
bool track2 = true;

double dataRate;

class LTBeacon : public Application
{
public:
	LTBeacon ();
	virtual ~LTBeacon ();

	void SetParameters (double replyInterval, double timingError, double velocityError);
	void SetSoundSpeed (double speed);
	void SetCarrierFreq (double freq);
	void SetPacketSize (uint32_t size);
	void SetOrdinaryAddress (Mac8Address address);

private:
	virtual void StartApplication (void);
	virtual void StopApplication (void);

	void SendWakeup();
	bool ReceivePacket(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &sender);
	void SendReply();

	Ptr<UanNetDevice> m_device;
	double m_replyInterval;
	EventId m_sendEvent;
	bool m_running;

	double m_soundSpeed;
	double m_carrierFreq;
	double m_T1, m_T4, m_T5;
	double m_estimatedVelocity;
	double m_estimatedDistance;
	double m_timeError;
	double m_velocityError;
	uint32_t m_pktSize;
	Mac8Address m_ordinaryAddress;
};

LTBeacon::LTBeacon ()
  : m_replyInterval (5),
    m_running (false),
    m_soundSpeed (1500.0),
    m_carrierFreq (20000.0),
	m_T1 (0.0),
	m_T4 (0.0),
	m_T5 (0.0),
	m_estimatedVelocity (0.0),
	m_estimatedDistance (0.0),
	m_timeError (20e-6),
	m_velocityError (0.2),
	m_pktSize (60)
{
}

LTBeacon::~LTBeacon ()
{
}

void
LTBeacon::SetParameters (double replyInterval, double timingError, double velocityError)
{
	m_replyInterval = replyInterval;
	m_timeError = timingError;
	m_velocityError = velocityError;
}

void
LTBeacon::SetSoundSpeed (double speed)
{
	m_soundSpeed = speed;
}

void
LTBeacon::SetCarrierFreq (double freq)
{
	m_carrierFreq = freq;
}

void
LTBeacon::SetPacketSize (uint32_t size)
{
	m_pktSize = size;
}

void
LTBeacon::SetOrdinaryAddress (Mac8Address address)
{
	m_ordinaryAddress = address;
}

void
LTBeacon::StartApplication (void)
{
	m_running = true;

	m_device = GetNode ()->GetDevice (0)->GetObject<UanNetDevice> ();

	if (m_device)
    {
		m_device->SetReceiveCallback (MakeCallback (&LTBeacon::ReceivePacket, this)); // @suppress("Ambiguous problem") // @suppress("Invalid arguments")
		m_sendEvent = Simulator::ScheduleNow(&LTBeacon::SendWakeup, this);
    }
	else
    {
		NS_LOG_ERROR ("LTBeacon: Could not get UAN device!");
    }
}

void
LTBeacon::StopApplication (void)
{
	m_running = false;
	if (m_sendEvent.IsRunning ())
    {
		Simulator::Cancel (m_sendEvent);
    }
}

class LTOrdinary : public Application
{
public:
	LTOrdinary ();
	virtual ~LTOrdinary ();

	void SetParameters (double replyInterval, double clockSkew, double clockOffset, double timingError, double velocityError);
	void SetSoundSpeed (double speed);
	void SetCarrierFreq (double freq);
	void SetPacketSize (uint32_t size);
	void SetBeaconAddress (Mac8Address address);
	void StopAcceleration();

private:
	virtual void StartApplication (void);
	virtual void StopApplication (void);

	bool ReceivePacket(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &sender);
	void SendReply();

	Ptr<UanNetDevice> m_device;
	EventId m_sendEvent;
	bool m_running;
	double m_replyInterval;

	double m_clockSkew;
	double m_clockOffset;
	double m_estimatedSkew;
	double m_estimatedOffset;
	double m_T1, m_T2, m_T3, m_T4, m_T5, m_T6;
	double m_soundSpeed;
	double m_carrierFreq;
	double m_timeError;
	double m_velocityError;
	uint32_t m_pktSize;
	Mac8Address m_beaconAddress;

	double receiveCount = 1;
};

LTOrdinary::~LTOrdinary ()
{
}

LTOrdinary::LTOrdinary ()
  : m_running (false),
	m_replyInterval(5),
	m_clockSkew (200),
	m_clockOffset (0.02),
	m_estimatedSkew (0),
	m_estimatedOffset (0),
	m_T1 (0.0),
	m_T2 (0.0),
	m_T3 (0.0),
	m_T4 (0.0),
	m_T5 (0.0),
	m_T6 (0.0),
    m_soundSpeed (1500.0),
    m_carrierFreq (20000.0),
	m_timeError (20e-6),
	m_velocityError (0.2),
	m_pktSize (60)
{
}

void
LTOrdinary::SetParameters (double replyInterval, double clockSkew, double clockOffset, double timingError, double velocityError){
	m_replyInterval = replyInterval;
	m_clockSkew = clockSkew;
	m_clockOffset = clockOffset;
	m_timeError = timingError;
	m_velocityError = velocityError;
}

void
LTOrdinary::SetSoundSpeed (double speed){
	m_soundSpeed = speed;
}

void
LTOrdinary::SetCarrierFreq (double freq){
	m_carrierFreq = freq;
}

void
LTOrdinary::SetPacketSize (uint32_t size){
	m_pktSize = size;
}

void
LTOrdinary::SetBeaconAddress (Mac8Address address){
	m_beaconAddress = address;
}

void
LTOrdinary::StopAcceleration(){
	Ptr<ConstantAccelerationMobilityModel> mobility = GetNode()-> GetObject<ConstantAccelerationMobilityModel> ();
	double velocity = mobility->GetVelocity().x;
	mobility->SetVelocityAndAcceleration(Vector(velocity, 0.0, 0.0), Vector(0, 0.0, 0.0));
}

void
LTOrdinary::StartApplication (void)
{
	m_running = true;

	m_device = GetNode ()->GetDevice (0)->GetObject<UanNetDevice> ();

	if (m_device)
	{
	 	m_device->SetReceiveCallback (MakeCallback (&LTOrdinary::ReceivePacket, this)); // @suppress("Ambiguous problem") // @suppress("Invalid arguments")

		if(track1){
			double stopAccelerationInterval = (maxVelocity - initVelocity) / acceleration;
			Simulator::Schedule(Seconds(stopAccelerationInterval),&LTOrdinary::StopAcceleration, this);
		}
    }
	else
    {
		NS_LOG_ERROR ("LTOrdinary: Could not get UAN device!");
    }
}

void
LTOrdinary::StopApplication (void)
{
	m_running = false;
	if (m_sendEvent.IsRunning ())
    {
		Simulator::Cancel (m_sendEvent);
    }
}

Ptr<LTBeacon> LTBeaconApp;
Ptr<LTOrdinary> LTOrdinaryApp;

double LTGetVelocity(){
	double radialVelocity = 0;
	if(track1){
		Ptr<ConstantAccelerationMobilityModel> mobility = LTOrdinaryApp->GetNode()-> GetObject<ConstantAccelerationMobilityModel> ();
		radialVelocity = mobility->GetVelocity().x;
	}else if(track2){
		Ptr<MobilityModel> mobNode = LTOrdinaryApp->GetNode()->GetObject<MobilityModel>();
		Ptr<MobilityModel> mobGw = LTBeaconApp->GetNode()->GetObject<MobilityModel>();

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
LTBeacon::SendWakeup(){
	m_T1 = Simulator::Now().GetSeconds();

	struct Wakeup_Data {
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
		m_device->GetMac ()->Enqueue (packet, 0, m_ordinaryAddress);
		NS_LOG_DEBUG ("LTBeacon sent wakeup packet at: " << m_T1 << " s, Packet size = " << packet->GetSize() << " Bytes");
	}
}

bool
LTOrdinary::ReceivePacket(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &sender){
	Ptr<NormalRandomVariable> timestampNoiseVar = CreateObject<NormalRandomVariable>();
	timestampNoiseVar->SetAttribute("Mean", DoubleValue(0.0));
	timestampNoiseVar->SetAttribute("Variance", DoubleValue(m_timeError * m_timeError));
	double timestampNoise = timestampNoiseVar->GetValue();

	double serialization_time = ((packet->GetSize() + 3) * 8) / 20000.0;
	double tReceive = Simulator::Now().GetSeconds() - serialization_time;
	tReceive = tReceive * (1 + (m_clockSkew * 1e-6)) + m_clockOffset + timestampNoise;

	Ptr<Packet> p = packet->Copy();
	if(receiveCount == 1){
		m_T2 = tReceive;
		receiveCount++;
		struct Wakeup_Data {
			double T1;
		} data;

		p->CopyData (reinterpret_cast<uint8_t*> (&data), sizeof(data));

		m_T1 = data.T1;

		NS_LOG_DEBUG ("LTOrdinary received wakeup packet at: " << m_T2 << " s, Packet size = " << packet->GetSize() << " Bytes");

		Simulator::Schedule(Seconds(m_replyInterval),&LTOrdinary::SendReply, this);
	}else{
		m_T6 = tReceive;

		struct Reply_Data {
			double T4;
			double T5;
			double estimatedVelocity;
			double estimatedDistance;
		} data;

		p->CopyData (reinterpret_cast<uint8_t*> (&data), sizeof(data));

		NS_LOG_DEBUG ("LTOrdinary received reply packet at: " << m_T6 << " s, Packet size = " << packet->GetSize() << " Bytes");

		m_T4 = data.T4;
		m_T5 = data.T5;
		double v = data.estimatedVelocity;
		double l = data.estimatedDistance;

		double d0 = l / (m_soundSpeed - v);
		double d1 = (l + ((m_T3 - m_T1) * v)) / m_soundSpeed;
		double d2 = (l + ((m_T5 - m_T1) * v)) / (m_soundSpeed - v);

		m_estimatedSkew = (m_T6 - m_T2) / (m_T5 - m_T1 + d2 - d0);

		m_estimatedOffset = ((m_T2 + m_T3) - (m_estimatedSkew * (m_T1 + m_T4 + d0 - d1))) / 2;

		NS_LOG_DEBUG ("estimated skew = " << m_estimatedSkew);
		NS_LOG_DEBUG ("estimated offset = " << m_estimatedOffset);

		LTSkewError = std::abs(std::abs(m_estimatedSkew) - std::abs(1 + (m_clockSkew * 1e-6))) * 1e6;
		LTOffsetError = std::abs(std::abs(m_estimatedOffset) - std::abs(m_clockOffset)) * 1e6;
	}
	return true;
}

void
LTOrdinary::SendReply(){
	double Tsend = Simulator::Now().GetSeconds() * (1 + (m_clockSkew * 1e-6)) + m_clockOffset;
	m_T3 = Tsend;

	struct Reply_Data {
		double T2;
		double T3;
	} data;

	Ptr<Packet> packet = Create<Packet> (reinterpret_cast<const uint8_t*> (&data), sizeof(data));

	if (m_pktSize > packet->GetSize ()) {
		uint32_t paddingSize = m_pktSize - packet->GetSize ();
		Ptr<Packet> padding = Create<Packet> (paddingSize);
		packet->AddAtEnd (padding);
	}
	if (packet)
	{
		m_device->GetMac ()->Enqueue (packet, 0, m_beaconAddress);
		NS_LOG_DEBUG ("LTOrdinary sent reply packet at: " << m_T3 << " s, Packet size = " << packet->GetSize() << " Bytes");
	}
}

bool
LTBeacon::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &sender)
{
	double serialization_time = ((packet->GetSize() + 3) * 8) / dataRate;   // 20 kbps
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

	struct Reply_Data {
		double T2;
		double T3;
	} data;
	p->CopyData (reinterpret_cast<uint8_t*> (&data), sizeof(data));

	m_T4 = TReceive;

	m_estimatedDistance = m_soundSpeed * ((m_T4 - m_T1) - (data.T3 - data.T2)) / 2.0;

	m_estimatedVelocity = LTGetVelocity() + velocityNoise;

	NS_LOG_DEBUG ("LTBeacon received reply at :"<< m_T4 << " s, Packet size :"<< packet->GetSize()<< " Bytes");

	Simulator::Schedule(Seconds(m_replyInterval),&LTBeacon::SendReply, this);

	return true;
}

void
LTBeacon::SendReply(){
	m_T5 = Simulator::Now().GetSeconds();

	struct Reply_Data {
		double T4;
		double T5;
		double estimatedVelocity;
		double estimatedDistance;
	} data;

	data.T4 = m_T4;
	data.T5 = m_T5;
	data.estimatedVelocity = m_estimatedVelocity;
	data.estimatedDistance = m_estimatedDistance;

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
	NS_LOG_DEBUG ("LTBeacon sent Reply at: " << m_T5 << " s, Packet Size = "<<packet->GetSize()<<" Bytes");
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
	double LTReplyInterval = 5; //5 seconds
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
	cmd.AddValue ("replyInterval", "Time to reply (s)", LTReplyInterval);
	cmd.AddValue ("pktSize", "Packet size in bytes", pktSize);

	cmd.Parse (argc, argv);

	RngSeedManager::SetSeed(seed);
	RngSeedManager::SetRun(run);

	if(track2){
		initDistance = initDistance - radius;
	}

	maxVelocity = initVelocity + 3;

	double runtime = 20;

	// Enable detailed UAN logging - use INFO level to see timing information
	LogComponentEnable ("UanPhyGen", LOG_LEVEL_INFO);
	LogComponentEnable ("UanMacAloha", LOG_LEVEL_INFO);
	LogComponentEnable ("UanChannel", LOG_LEVEL_INFO);
	LogComponentEnable ("UanNetDevice", LOG_LEVEL_INFO);

	// Enable our application logging
	LogComponentEnable ("LT-Sync", LOG_LEVEL_INFO);

	// Create nodes
	NodeContainer LTNodes;
	LTNodes.Create (2);

	Ptr<ConstantAccelerationMobilityModel> LTSyncMobility1 = CreateObject<ConstantAccelerationMobilityModel>();

	LTSyncMobility1->SetPosition(Vector(0.0, 0.0, -100.0));
	LTSyncMobility1->SetVelocityAndAcceleration(Vector(0, 0.0, 0.0), Vector(0, 0.0, 0.0));

	LTNodes.Get (0)->AggregateObject(LTSyncMobility1);

	if(track1){
		Ptr<ConstantAccelerationMobilityModel> LTSyncMobility2 = CreateObject<ConstantAccelerationMobilityModel>();
		LTSyncMobility2->SetPosition(Vector(initDistance, 0.0, -100.0));
		LTSyncMobility2->SetVelocityAndAcceleration(Vector(initVelocity, 0.0, 0.0), Vector(acceleration, 0.0, 0.0));
		LTNodes.Get (1)->AggregateObject(LTSyncMobility2);
	}else if(track2){
		Ptr<WaypointMobilityModel> LTwaypoints = CreateObject<WaypointMobilityModel>();
		double initialPhase = angleIndex * (M_PI / 4.0);

		for (double t = 0; t < runtime; t += 0.1) {

			double phase = (omega * t) + initialPhase;

		    double LTx = initDistance + radius * cos(phase);
		    double y = radius * sin(phase);

		    LTwaypoints->AddWaypoint(Waypoint(Seconds(t), Vector(LTx, y, -100.0)));
		}

		LTNodes.Get(1)->AggregateObject(LTwaypoints);
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
             	 "TxPower", DoubleValue (160),      //160 dB
				 "SupportedModes", UanModesListValue (realisticModes),
				 "PerModel", StringValue ("ns3::UanPhyPerGenDefault"),
				 "SinrModel", StringValue ("ns3::UanPhyCalcSinrDefault"));

	uan.SetMac ("ns3::UanMacAloha");

	// Install UAN devices
	NetDeviceContainer LTDevices = uan.Install (LTNodes, channel);

	// Create energy sources for both nodes
	BasicEnergySourceHelper LTEnergySourceHelper;
	LTEnergySourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (1000000)); // 1 MJ initial energy
	LTEnergySourceHelper.Set ("BasicEnergySupplyVoltageV", DoubleValue (12)); // 12V typical for underwater systems

	EnergySourceContainer LTEnergySources = LTEnergySourceHelper.Install (LTNodes);

	// Create UAN energy model helper
	AcousticModemEnergyModelHelper LTAcousticModemEnergyHelper; // @suppress("Abstract class cannot be instantiated")

	// Install energy models on UAN devices
	DeviceEnergyModelContainer LTDeviceEnergyModels = LTAcousticModemEnergyHelper.Install (LTDevices, LTEnergySources);

	auto LTPrintEnergyReport = [&]() {
		std::cout << "\n=== LT DETAILED ENERGY CONSUMPTION REPORT ===" << std::endl;

		for (uint32_t i = 0; i < LTNodes.GetN(); i++)
		{
			Ptr<EnergySource> energySource = LTEnergySources.Get(i);
			Ptr<DeviceEnergyModel> energyModel = LTDeviceEnergyModels.Get(i);

			std::cout << "Node " << i << " (" << (i == 0 ? "LTBeacon" : "LTOrdinary") << "):" << std::endl;
			std::cout << "  Initial energy: " << energySource->GetInitialEnergy() << " J" << std::endl;
			std::cout << "  Remaining energy: " << energySource->GetRemainingEnergy() << " J" << std::endl;
			std::cout << "  Total consumed: " << energyModel->GetTotalEnergyConsumption() << " J" << std::endl;
		}

		// Protocol efficiency analysis
		std::cout << "\n--- Protocol Efficiency ---" << std::endl;
		std::cout << "Total energy for one synchronization: "
				<< (LTDeviceEnergyModels.Get(0)->GetTotalEnergyConsumption() +
						LTDeviceEnergyModels.Get(1)->GetTotalEnergyConsumption()) << " J" << std::endl;
		std::cout << "Beacon/Ordinary energy ratio: "
				<< (LTDeviceEnergyModels.Get(0)->GetTotalEnergyConsumption() /
	                    LTDeviceEnergyModels.Get(1)->GetTotalEnergyConsumption()) << std::endl;
		std::cout << "==========================================" << std::endl;
	};

	// Get MAC addresses
	Ptr<UanNetDevice> LTBeaconDevice = LTDevices.Get (0)->GetObject<UanNetDevice> ();
	Ptr<UanNetDevice> LTOrdinaryDevice = LTDevices.Get (1)->GetObject<UanNetDevice> ();

	Mac8Address LTBeaconMac = Mac8Address::ConvertFrom (LTBeaconDevice->GetMac ()->GetAddress ());
	Mac8Address LTOrdainryMac = Mac8Address::ConvertFrom (LTOrdinaryDevice->GetMac ()->GetAddress ());

	// Create applications
	// Beacon application
	LTBeaconApp = CreateObject<LTBeacon> ();
	LTBeaconApp->SetParameters (LTReplyInterval, timeError, velocityError);
	LTBeaconApp->SetSoundSpeed (soundSpeed);
	LTBeaconApp->SetCarrierFreq (carrierFreq);
	LTBeaconApp->SetPacketSize (pktSize);
	LTBeaconApp->SetOrdinaryAddress(LTOrdainryMac);
	LTNodes.Get (0)->AddApplication (LTBeaconApp);
	LTBeaconApp->SetStartTime (Seconds (0));

	// Ordinary application
	LTOrdinaryApp = CreateObject<LTOrdinary> ();
	LTOrdinaryApp->SetParameters (LTReplyInterval, clockSkew, clockOffset, timeError, velocityError);
	LTOrdinaryApp->SetSoundSpeed (soundSpeed);
	LTOrdinaryApp->SetCarrierFreq (carrierFreq);
	LTOrdinaryApp->SetPacketSize (pktSize);
	LTOrdinaryApp->SetBeaconAddress(LTBeaconMac);
	LTNodes.Get (1)->AddApplication (LTOrdinaryApp);
	LTOrdinaryApp->SetStartTime (Seconds (0));

	auto PrintResults = [&]() {
		switch (plotNum) {
			case'0':{
				LTPrintEnergyReport();
				std::cout << "=== LT Results ===" << std::endl;
				std::cout << "LT offset error = " <<LTOffsetError<<" us"<< std::endl;
				std::cout << "LT skew error = " <<LTSkewError<<" ppm"<< std::endl;
				break;
			}
			case '1':{
				std::ofstream plot("csv/plot1.csv", std::ios::app);
				if (track1) {
				    plot << LTOffsetError << ",";
				}
				else if (track2) {
				    plot << LTOffsetError << ",";
				}
			break;
			}
			case '2':{
				std::ofstream plot("csv/plot2.csv", std::ios::app);
				plot << LTOffsetError << ",";
			break;
			}
			case '3':{
				std::ofstream plot("csv/plot3.csv", std::ios::app);
				plot << LTOffsetError << ",";
			break;
			}
			case '4':{
				std::ofstream plot("csv/plot4.csv", std::ios::app);
				plot << LTOffsetError << ",";
			break;
			}
			case '5':{
				std::ofstream plot("csv/plot5.csv", std::ios::app);
				if (track1) {
				    plot << LTOffsetError << ",";
				}
				else if (track2) {
				    plot << LTOffsetError << ",";
				}
				break;
			}
			case '6':{
				std::ofstream plot("csv/plot6.csv", std::ios::app);
				if (track1) {
				    plot << LTOffsetError << ",";
				}
				else if (track2) {
				    plot << LTOffsetError << ",";
				}
				break;
			}
			case '7':{
				std::ofstream plot("csv/plot7.csv", std::ios::app);
				if (track1) {
				    plot << LTOffsetError << ",";
				}
				else if (track2) {
				    plot << LTOffsetError << ",";
				}
				break;
			}
			case '8':{
				std::ofstream plot("csv/plot8.csv", std::ios::app);
				if (track1) {
				    plot << LTOffsetError << ",";
				}
				else if (track2) {
				    plot << LTOffsetError << ",";
				}
				break;
			}
			case '9':{
				std::ofstream plot("csv/plot9b.csv", std::ios::app);
				if(track1){
					if(run == 1){
						plot << "Track 1,,,Track 2\n";
						plot << "Run,LT-Skew Error (ppm),,LT-Skew Error (ppm)\n";
					}
					plot << run << "," << LTSkewError << ",";
				}else if (track2){
					plot << "," << LTSkewError << "\n";
				}
				break;
			}
			default:{
				break;
			}
		}
	};

	Simulator::Schedule (Seconds (runtime), PrintResults);

	Simulator::Stop (Seconds(runtime));
	Simulator::Run ();
	Simulator::Destroy ();

	return 0;
}


