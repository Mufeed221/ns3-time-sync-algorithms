#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/uan-module.h"
#include "ns3/acoustic-modem-energy-model-helper.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/energy-source-container.h"
#include <fstream>
#include <vector>
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("UWGS");

uint32_t seed;
uint32_t run;

uint32_t plotNum;

double dataRate;

double UWGSSkewError  = 0.0;
double UWGSOffsetError = 0.0;

uint32_t iterations = 0;

bool UwgsEstimationDone = false;

Ptr<NormalRandomVariable> timestampNoiseVar;

class UWGSBeacon : public Application
{
public:
	UWGSBeacon ();
	virtual ~UWGSBeacon ();

	void SetParameters (double beaconInterval, uint32_t beaconCount);
	void SetPacketSize (uint32_t size);
	void SetOrdinaryAddress (Mac8Address address);

private:
	virtual void StartApplication (void);
	virtual void StopApplication (void);

	void SendBeacon ();

	Ptr<UanNetDevice> m_device;
	EventId m_sendEvent;
	bool m_running;

	double m_beaconInterval;
	uint32_t m_beaconCount;
	uint32_t m_beaconIndex;
	uint32_t m_pktSize;
	Mac8Address m_ordinaryAddress;
};

UWGSBeacon::UWGSBeacon ()
  : m_running (false),
	m_beaconInterval (10.0),
	m_beaconCount (0),
	m_beaconIndex (0),
    m_pktSize (60)
{
}

UWGSBeacon::~UWGSBeacon ()
{
}

void
UWGSBeacon::SetParameters (double beaconInterval, uint32_t beaconCount)
{
	m_beaconInterval = beaconInterval;
	m_beaconCount = beaconCount;
}

void
UWGSBeacon::SetPacketSize (uint32_t size)
{
	m_pktSize = size;
}

void
UWGSBeacon::SetOrdinaryAddress (Mac8Address address)
{
	m_ordinaryAddress = address;
}

void
UWGSBeacon::StartApplication (void)
{
	m_running = true;
	m_beaconIndex = 0;

	m_device = GetNode ()->GetDevice (0)->GetObject<UanNetDevice> ();

	if (m_device)
	{
		m_sendEvent = Simulator::ScheduleNow (&UWGSBeacon::SendBeacon, this);
	}
	else
	{
		NS_LOG_ERROR ("UWGSBeacon: Could not get UAN device!");
	}
}

void
UWGSBeacon::StopApplication (void)
{
	m_running = false;
	if (m_sendEvent.IsRunning ())
	{
		Simulator::Cancel (m_sendEvent);
	}
}

void
UWGSBeacon::SendBeacon ()
{
	uint32_t h = m_beaconIndex + 1;
	double TSend = Simulator::Now().GetSeconds();

	struct Beacon_Data {
		uint32_t beaconIndex;
		double tSend;
	} data;

	data.beaconIndex = h;
	data.tSend = TSend;

	Ptr<Packet> packet = Create<Packet> (reinterpret_cast<const uint8_t*> (&data), sizeof (data));
	if (m_pktSize > packet->GetSize ())
	{
		uint32_t paddingSize = m_pktSize - packet->GetSize ();
		Ptr<Packet> padding = Create<Packet> (paddingSize);
		packet->AddAtEnd (padding);
	}

	if (packet)
	{
		m_device->GetMac ()->Enqueue (packet, 0, m_ordinaryAddress);
		NS_LOG_DEBUG ("UWGSBeacon sent beacon h=" << h << " at t=" << TSend
		              << " s, size=" << packet->GetSize () << " bytes");
	}
	else
	{
		NS_LOG_DEBUG ("UWGSBeacon DROPPED beacon h=" << h << " (ideal PDR model)");
	}

	++m_beaconIndex;
	if (m_running && (m_beaconCount == 0 || m_beaconIndex < m_beaconCount))
	{
		m_sendEvent = Simulator::Schedule (Seconds (m_beaconInterval), &UWGSBeacon::SendBeacon, this);
	}
}


class UWGSOrdainry : public Application
{
public:
	UWGSOrdainry ();
	virtual ~UWGSOrdainry ();

	void SetParameters (double beaconInterval, double clockSkew, double clockOffset);
	void SetSoundSpeed (double speed);
	void SetTrajectory (double x0, double y0, double velocity, double Ax, double omega, double t0);
	void SetBeaconPosition (double xB, double yB);
	void SetBeaconAddress (Mac8Address address);

private:
	virtual void StartApplication (void);
	virtual void StopApplication (void);

	bool ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet,
	                     uint16_t protocol, const Address &sender);
	void RunEstimation ();

	// Node's own known trajectory (paper eq. 1-2, deterministic part only)
	double NodeX (double t) const { return m_xO + m_Ax * std::sin (m_omega * t); }
	double NodeY (double t) const { return m_yO - m_velocity * (t - m_t0); }
	double DistToRef (double t) const
	{
		double dx = NodeX (t) - m_xB;
		double dy = NodeY (t) - m_yB;
		return std::sqrt (dx * dx + dy * dy);
	}

	Ptr<UanNetDevice> m_device;

	double m_beaconInterval;
	double m_soundSpeed;
	double m_clockSkew;
	double m_clockOffset;

	double m_xO, m_yO, m_velocity, m_Ax, m_omega, m_t0;
	double m_xB, m_yB;

	Mac8Address m_beaconAddress;

	std::vector<uint32_t> m_h;
	std::vector<double> m_tLocal;
};

UWGSOrdainry::UWGSOrdainry ()
  : m_beaconInterval (10.0),
    m_soundSpeed (1500.0),
	m_clockSkew (0.0),
    m_clockOffset (0.0),
    m_xO (100.0), m_yO (-500.0), m_velocity (2.0), m_Ax (3.0), m_omega (0.05), m_t0 (0.0),
    m_xB (0.0), m_yB (0.0)
{
}

UWGSOrdainry::~UWGSOrdainry ()
{
}

void
UWGSOrdainry::SetParameters (double beaconInterval, double clockSkew, double clockOffset)
{
	m_beaconInterval = beaconInterval;
	m_clockSkew = clockSkew;
	m_clockOffset = clockOffset;
}

void
UWGSOrdainry::SetSoundSpeed (double speed){
	m_soundSpeed = speed;
}

void
UWGSOrdainry::SetTrajectory (double xO, double yO, double velocity, double Ax, double omega, double t0)
{
	m_xO = xO; m_yO = yO; m_velocity = velocity; m_Ax = Ax; m_omega = omega; m_t0 = t0;
}

void
UWGSOrdainry::SetBeaconPosition (double xB, double yB)
{
	m_xB = xB; m_yB = yB;
}

void
UWGSOrdainry::SetBeaconAddress (Mac8Address address)
{
	m_beaconAddress = address;
}

void
UWGSOrdainry::StartApplication (void)
{
	m_device = GetNode ()->GetDevice (0)->GetObject<UanNetDevice> ();

	if (m_device)
	{
		m_device->SetReceiveCallback (MakeCallback (&UWGSOrdainry::ReceivePacket, this)); // @suppress("Ambiguous problem") // @suppress("Invalid arguments")
	}
	else
	{
		NS_LOG_ERROR ("UWGSOrdinary: Could not get UAN device!");
	}
}

void
UWGSOrdainry::StopApplication (void)
{
}

bool
UWGSOrdainry::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet,
                          uint16_t protocol, const Address &sender)
{
	double serializationTime = ((packet->GetSize () + 3) * 8) / dataRate;
	double tReceiveTrue = Simulator::Now ().GetSeconds () - serializationTime;

	double timestampNoise = timestampNoiseVar->GetValue ();

	double tLocal = tReceiveTrue * (1.0 + (m_clockSkew * 1e-6)) + m_clockOffset + timestampNoise;

	struct Beacon_Data {
		uint32_t beaconIndex;
		double tSend;
	} data;

	Ptr<Packet> p = packet->Copy ();
	p->CopyData (reinterpret_cast<uint8_t*> (&data), sizeof (data));

	m_h.push_back (data.beaconIndex);
	m_tLocal.push_back (tLocal);

	NS_LOG_DEBUG ("UWGSOrdinary received beacon h=" << data.beaconIndex << ", Tsend = " <<data.tSend
	              << " s, local t'=" << tLocal << " s, size=" << packet->GetSize () << " bytes");

	if (!UwgsEstimationDone && m_h.size () >= 5)
	{
		RunEstimation ();
	}

	return true;
}

void
UWGSOrdainry::RunEstimation ()
{
	// Use exactly the first 5 successfully received packets (Section 3).
	double tp[5], tA[5];
	for (int i = 0; i < 5; ++i)
	{
		tp[i] = m_tLocal[i];
		tA[i] = static_cast<double> (m_h[i] - 1) * m_beaconInterval; // (h_i - 1)*T, t0 = 0
	}

	// Denominators in eq. 22/38/39 use the observed local timestamps as
	// fixed constants -- consistent with the paper's own derivative
	// result d(Delta_i)/da = t'_i (eq. 36), which has no term through
	// these denominators.
	double d21 = tp[1] - tp[0], d32 = tp[2] - tp[1], d31 = tp[2] - tp[0];
	double d42 = tp[3] - tp[1], d43 = tp[3] - tp[2];
	double d53 = tp[4] - tp[2], d54 = tp[4] - tp[3];

	// Initial guess: unskewed, unshifted clock.
	double a = 1.0, b = 0.0;

	const int outerMaxIter = 40;
	const int innerMaxIter = 25;
	const double tolOuter = 1e-14;
	const double tolInner = 1e-16;

	int outerIter = 0;
	for (outerIter = 0; outerIter < outerMaxIter; ++outerIter)
	{
		// --- refresh p_i from the current (a,b) estimate, using the
		// Node's OWN known trajectory model (see class comment) ---
		double p[5];
		for (int i = 0; i < 5; ++i)
		{
			double tHat = a * tp[i] + b; // candidate true reception time
			p[i] = DistToRef (tHat);
//			p[i] = (tHat - 0 - (i * m_beaconInterval)) * m_soundSpeed; // using eq. 8, doesn't solve the system
		}

		double aPrev = a, bPrev = b;

		// --- inner Newton solve of f1(a,b)=0, f2(a,b)=0 with p_i fixed
		// (paper eq. 22 residuals, eq. 38-39 Jacobian) ---
		for (int inner = 0; inner < innerMaxIter; ++inner)
		{
			double dt[5];
			for (int i = 0; i < 5; ++i)
			{
				dt[i] = a * tp[i] + b - tA[i] - p[i] / m_soundSpeed;
			}

			double f1 =
			      (dt[0] * dt[0]) / (d31 * d21)
			    + (dt[2] * dt[2]) / (d31 * d32)
			    - (dt[1] * dt[1]) / (d32 * d21)
			    - (dt[1] * dt[1]) / (d42 * d32)
			    - (dt[3] * dt[3]) / (d42 * d43)
			    + (dt[2] * dt[2]) / (d43 * d32);

			double f2 =
			      (dt[0] * dt[0]) / (d31 * d21)
			    + (dt[2] * dt[2]) / (d31 * d32)
			    - (dt[1] * dt[1]) / (d32 * d21)
			    - (dt[2] * dt[2]) / (d53 * d43)
			    - (dt[4] * dt[4]) / (d53 * d54)
			    + (dt[3] * dt[3]) / (d54 * d43);

			double resNorm = std::sqrt (f1 * f1 + f2 * f2);
			if (resNorm < tolInner) { break; }

			double df1_da =
			      2 * dt[0] * tp[0] / (d31 * d21)
			    + 2 * dt[2] * tp[2] / (d31 * d32)
			    - 2 * dt[1] * tp[1] / (d32 * d21)
			    - 2 * dt[1] * tp[1] / (d42 * d32)
			    - 2 * dt[3] * tp[3] / (d42 * d43)
			    + 2 * dt[2] * tp[2] / (d43 * d32);

			double df1_db =
			      2 * dt[0] / (d31 * d21)
			    + 2 * dt[2] / (d31 * d32)
			    - 2 * dt[1] / (d32 * d21)
			    - 2 * dt[1] / (d42 * d32)
			    - 2 * dt[3] / (d42 * d43)
			    + 2 * dt[2] / (d43 * d32);

			double df2_da =
			      2 * dt[0] * tp[0] / (d31 * d21)
			    + 2 * dt[2] * tp[2] / (d31 * d32)
			    - 2 * dt[1] * tp[1] / (d32 * d21)
			    - 2 * dt[2] * tp[2] / (d53 * d43)
			    - 2 * dt[4] * tp[4] / (d53 * d54)
			    + 2 * dt[3] * tp[3] / (d54 * d43);

			double df2_db =
			      2 * dt[0] / (d31 * d21)
			    + 2 * dt[2] / (d31 * d32)
			    - 2 * dt[1] / (d32 * d21)
			    - 2 * dt[2] / (d53 * d43)
			    - 2 * dt[4] / (d53 * d54)
			    + 2 * dt[3] / (d54 * d43);


			double det = df1_da * df2_db - df1_db * df2_da;
			if (std::fabs (det) < 1e-300) { break; } // singular; bail inner loop

			double da = ( df2_db * f1 - df1_db * f2) / det;
			double db = (-df2_da * f1 + df1_da * f2) / det;

			a -= da;
			b -= db;
		}

		double change = std::fabs (a - aPrev) + std::fabs (b - bPrev);
		if (change < tolOuter) { ++outerIter; break; }
	}

	iterations = outerIter;

	// Recover alpha, beta from a = 1/alpha, b = -beta/alpha (eq. 19/23)
	double estimatedSkew = 1.0 / a;
	double estiamtedOffset = -b / a;

	NS_LOG_DEBUG ("estimated skew = " << estimatedSkew);
	NS_LOG_DEBUG ("estimated offset = " << estiamtedOffset);

	UWGSSkewError = std::abs ((1 + (m_clockSkew * 1e-6)) - estimatedSkew) * 1e6;
	UWGSOffsetError = std::abs (m_clockOffset - estiamtedOffset) * 1e6;
	UwgsEstimationDone = true;
}


UanModesList
CreateUnderwaterModes ()
{
	UanModesList modes;
	UanTxMode mode;

	mode = UanTxModeFactory::CreateMode (UanTxMode::FSK,
	                                      dataRate, // e.g. 20 kbps
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
	run = 1;

	plotNum = 0;

	dataRate = 20000;

	double soundSpeed = 1500.0;     // C
	double beaconInterval = 10;     // transmission interval
	double initDistance = 500;
	double initVelocity = 2;        // Node vertical speed [m/s]
	double clockSkew = 200;         // ppm
	double clockOffset = 0.02;      // s
	double timeError = 20e-6;       // s, additive timestamp jitter
	double trajPosNoiseStd = 0.0;   // m, std. dev. of node trajectory-position noise (0,0.5,1,2)
	uint32_t beaconCount = 5;
	uint32_t pktSize = 60;

	double Ax = 3.0;                // horizontal bending amplitude [m]
	double omega = 0.05;            // bending angular frequency [rad/s]

	CommandLine cmd;
	cmd.AddValue ("seed", "Random seed", seed);
	cmd.AddValue ("run", "Run number", run);
	cmd.AddValue ("plotNum", "Plot number", plotNum);
	cmd.AddValue ("soundSpeed", "Speed of sound in water (m/s)", soundSpeed);
	cmd.AddValue ("beaconInterval", "Beacon transmission interval (s)", beaconInterval);
	cmd.AddValue ("initDistance", "Initial distance between nodes (m)", initDistance);
	cmd.AddValue ("initVelocity", "Node vertical speed (m/s)", initVelocity);
	cmd.AddValue ("clockSkew", "Node clock skew (ppm)", clockSkew);
	cmd.AddValue ("clockOffset", "Node clock offset (s)", clockOffset);
	cmd.AddValue ("timeError", "Timestamp jitter std. dev. (s)", timeError);
	cmd.AddValue ("trajPosNoiseStd", "Node trajectory-position noise std. dev. (m)", trajPosNoiseStd);
	cmd.AddValue ("beaconCount", "Safety cap on number of beacons sent", beaconCount);
	cmd.AddValue ("pktSize", "Packet size in bytes", pktSize);

	cmd.Parse (argc, argv);

	RngSeedManager::SetSeed (seed);
	RngSeedManager::SetRun (run);

	double runtime = (beaconCount * beaconInterval) + 10;

	timestampNoiseVar = CreateObject<NormalRandomVariable>();
	timestampNoiseVar -> SetStream(1);
	timestampNoiseVar -> SetAttribute("Mean", DoubleValue(0.0));
	timestampNoiseVar -> SetAttribute("Variance", DoubleValue(timeError * timeError));

	LogComponentEnable ("UanPhyGen", LOG_LEVEL_INFO);
	LogComponentEnable ("UanMacAloha", LOG_LEVEL_INFO);
	LogComponentEnable ("UanChannel", LOG_LEVEL_INFO);
	LogComponentEnable ("UanNetDevice", LOG_LEVEL_INFO);

	LogComponentEnable ("UWGS", LOG_LEVEL_INFO);

	// ---- Nodes ----
	NodeContainer UWGSNodes;
	UWGSNodes.Create (2); // 0 = Beacon, 1 = Ordinary

	// Ref: stationary.
	Ptr<ConstantPositionMobilityModel> UWGSMobility = CreateObject<ConstantPositionMobilityModel> ();
	UWGSMobility->SetPosition (Vector (0, 0, -100.0));
	UWGSNodes.Get (0)->AggregateObject (UWGSMobility);

	Ptr<WaypointMobilityModel> UWGSWaypoints = CreateObject<WaypointMobilityModel> ();

	Ptr<NormalRandomVariable> xposNoiseVar = CreateObject<NormalRandomVariable> ();
	xposNoiseVar -> SetStream(3);
	xposNoiseVar->SetAttribute ("Mean", DoubleValue (0.0));
	xposNoiseVar->SetAttribute ("Variance", DoubleValue (trajPosNoiseStd * trajPosNoiseStd));

	Ptr<NormalRandomVariable> yposNoiseVar = CreateObject<NormalRandomVariable> ();
	yposNoiseVar -> SetStream(4);
	yposNoiseVar->SetAttribute ("Mean", DoubleValue (0.0));
	yposNoiseVar->SetAttribute ("Variance", DoubleValue (trajPosNoiseStd * trajPosNoiseStd));

	for (double t = 0; t <= runtime; t += 0.5)
	{
		double nx = (trajPosNoiseStd > 0.0) ? xposNoiseVar->GetValue () : 0.0;
		double ny = (trajPosNoiseStd > 0.0) ? yposNoiseVar->GetValue () : 0.0;

		double xt = 100 + Ax * std::sin (omega * t) + nx;
		double yt = -initDistance - initVelocity * t + ny;
		UWGSWaypoints->AddWaypoint (Waypoint (Seconds (t), Vector (xt, yt, -100.0)));
	}
	UWGSNodes.Get (1)->AggregateObject (UWGSWaypoints);

	Ptr<UanChannel> channel = CreateObject<UanChannel> ();

	Ptr<UanPropModelThorp> propModel = CreateObject<UanPropModelThorp> ();
	channel->SetPropagationModel (propModel);

	Ptr<UanNoiseModelDefault> noiseModel = CreateObject<UanNoiseModelDefault> ();
	noiseModel->SetAttribute ("Wind", DoubleValue (5.0));
	noiseModel->SetAttribute ("Shipping", DoubleValue (0.5));
	channel->SetNoiseModel (noiseModel);

	UanHelper uan;
	UanModesList modes = CreateUnderwaterModes ();
	uan.SetPhy ("ns3::UanPhyGen",
	            "TxPower", DoubleValue (160),
	            "SupportedModes", UanModesListValue (modes),
	            "PerModel", StringValue ("ns3::UanPhyPerGenDefault"),
	            "SinrModel", StringValue ("ns3::UanPhyCalcSinrDefault"));
	uan.SetMac ("ns3::UanMacAloha");

	NetDeviceContainer UWGSDevices = uan.Install (UWGSNodes, channel);

	// ---- Addresses ----
	Ptr<UanNetDevice> UWGSBeaconDevice = UWGSDevices.Get (0)->GetObject<UanNetDevice> ();
	Ptr<UanNetDevice> UWGSOrdianryDevice = UWGSDevices.Get (1)->GetObject<UanNetDevice> ();
	Mac8Address UWGSBeaconMac = Mac8Address::ConvertFrom (UWGSBeaconDevice->GetMac ()->GetAddress ());
	Mac8Address UWGSOrdianryMac = Mac8Address::ConvertFrom (UWGSOrdianryDevice->GetMac ()->GetAddress ());

	// ---- Applications ----
	Ptr<UWGSBeacon> UWGSBeaconApp = CreateObject<UWGSBeacon> ();
	UWGSBeaconApp->SetParameters (beaconInterval, beaconCount);
	UWGSBeaconApp->SetPacketSize (pktSize);
	UWGSBeaconApp->SetOrdinaryAddress (UWGSOrdianryMac);
	UWGSNodes.Get (0)->AddApplication (UWGSBeaconApp);
	UWGSBeaconApp->SetStartTime (Seconds (0));

	Ptr<UWGSOrdainry> UWGSOridanryNodeApp = CreateObject<UWGSOrdainry> ();
	UWGSOridanryNodeApp->SetParameters (beaconInterval, clockSkew, clockOffset);
	UWGSOridanryNodeApp->SetSoundSpeed(soundSpeed);
	UWGSOridanryNodeApp->SetTrajectory(100, -initDistance, initVelocity, Ax, omega, 0);
	UWGSOridanryNodeApp->SetBeaconPosition(0, 0);
	UWGSOridanryNodeApp->SetBeaconAddress (UWGSBeaconMac);
	UWGSNodes.Get (1)->AddApplication (UWGSOridanryNodeApp);
	UWGSOridanryNodeApp->SetStartTime (Seconds (0));

	auto UWGSPrintResults = [&] () {
		double parameter = 0;
		switch (plotNum) {
			case 0:{
				std::cout << "=== UWGS Results ===" << std::endl;
				std::cout << "UWGS offset error = " <<UWGSOffsetError<<" us"<< std::endl;
				std::cout << "UWGS skew error = " <<UWGSSkewError<<" ppm"<< std::endl;
				std::cout << "iterations = " <<iterations<< std::endl;
				break;
			}
			case 1:{
				parameter = initVelocity;
				break;
			}
			case 2:{
				parameter = trajPosNoiseStd;
				break;
			}
			default:{
				break;
			}
		}
		std::cout
			<< "CSV_RESULT,"
			<< run << ","
			<< parameter << ","
			<< "UWGS" << ",Offset,"
			<< UWGSOffsetError
			<< std::endl;

		std::cout
			<< "CSV_RESULT,"
			<< run << ","
			<< parameter << ","
			<< "UWGS" << ",Skew,"
			<< UWGSSkewError
			<< std::endl;
	};

	Simulator::Schedule (Seconds (runtime), UWGSPrintResults);

	Simulator::Stop (Seconds (runtime));
	Simulator::Run ();
	Simulator::Destroy ();

	return 0;
}
