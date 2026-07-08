#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/uan-module.h"
#include "ns3/acoustic-modem-energy-model-helper.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/energy-source-container.h"
#include <fstream>
#include <Eigen/Dense>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DC-Sync");

uint32_t seed;
uint32_t run;
char plotNum;

double initVelocity;
double maxVelocity;
double acceleration;

double DCOffsetError = 0;
double DCSkewError = 0;

bool track1 = false;
bool track2 = true;

double dataRate;

class DCBeacon : public Application
{
public:
	DCBeacon ();
	virtual ~DCBeacon ();

	void SetParameters (double messageInterval, uint32_t messagePairsCount, double clockSkew, double clockOffset, double timingError, double velocityError);
	void SetSoundSpeed (double speed);
	void SetCarrierFreq (double freq);
	void SetPacketSize (uint32_t size);
	void SetOrdinaryAddress (Mac8Address address);

private:
	virtual void StartApplication (void);
	virtual void StopApplication (void);

	void SendRequest();
	bool ReceiveReply(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &sender);

	std::vector<double> FitCubicPolynomial(const std::vector<double>& times,
	                                       const std::vector<double>& velocities);
	double  IntegratePolynomial(const std::vector<double>& coeffs,
	                           double t_start, double t_end);
	void DCSyncDopplerCompensation(int iteration);
	void CalculateRegression();   //least squares estimation
	void Calibration();

	Ptr<UanNetDevice> m_device;
	double m_messageInterval;
	EventId m_sendEvent;
	bool m_running;

	// Timestamps
	std::vector<double> m_T1List;
	std::vector<double> m_T2List;
	std::vector<double> m_T3List;
	std::vector<double> m_T4List;
	std::vector<double> m_V1List;
	std::vector<double> m_V2List;
	std::vector<double> m_a_AB_List;
	std::vector<double> m_a_BA_List;
	std::vector<double> m_velocitySamples;
	std::vector<double> m_velocityTimes;
	std::vector<double> m_a_c_simple_List;
	std::vector<double> m_a_c_List;
	double m_soundSpeed;
	double m_carrierFreq;
	double m_estimatedSkew;
	double m_estimatedOffset;
	double m_clockSkew;
	double m_clockOffset;
	double m_timeError;
	double m_velocityError;
	uint32_t m_messageSequence;
	uint32_t m_messagePairsCount;
	uint32_t m_pktSize;
	Mac8Address m_ordinaryAddress;
};

DCBeacon::DCBeacon ()
  : m_messageInterval (5),
    m_running (false),
    m_soundSpeed (1500.0),
    m_carrierFreq (20000.0),
	m_estimatedSkew (0),
	m_estimatedOffset (0),
	m_clockSkew (0),
	m_clockOffset (0),
	m_timeError (20e-6),
	m_velocityError (0.2),
	m_messageSequence (0),
	m_messagePairsCount (20),
	m_pktSize (60)
{
}

DCBeacon::~DCBeacon ()
{
}

void
DCBeacon::SetParameters (double messageInterval, uint32_t messagePairsCount, double clockSkew, double clockOffset, double timingError, double velocityError)
{
	m_messageInterval = messageInterval;
	m_messagePairsCount = messagePairsCount;
	m_clockSkew = clockSkew;
	m_clockOffset = clockOffset;
	m_timeError = timingError;
	m_velocityError = velocityError;
}

void
DCBeacon::SetSoundSpeed (double speed)
{
	m_soundSpeed = speed;
}

void
DCBeacon::SetCarrierFreq (double freq)
{
	m_carrierFreq = freq;
}

void
DCBeacon::SetPacketSize (uint32_t size)
{
	m_pktSize = size;
}

void
DCBeacon::SetOrdinaryAddress (Mac8Address address)
{
	m_ordinaryAddress = address;
}

void
DCBeacon::StartApplication (void)
{
	m_running = true;
	m_messageSequence = 0;

	m_device = GetNode ()->GetDevice (0)->GetObject<UanNetDevice> ();

	if (m_device)
    {
		m_device->SetReceiveCallback (MakeCallback (&DCBeacon::ReceiveReply, this)); // @suppress("Ambiguous problem") // @suppress("Invalid arguments")
		m_sendEvent = Simulator::ScheduleNow(&DCBeacon::SendRequest, this);
    }
	else
    {
		NS_LOG_ERROR ("Master: Could not get UAN device!");
    }
}

void
DCBeacon::StopApplication (void)
{
	m_running = false;
	if (m_sendEvent.IsRunning ())
    {
		Simulator::Cancel (m_sendEvent);
    }
}

class DCOrdinary : public Application
{
public:
	DCOrdinary ();
	virtual ~DCOrdinary ();

	void SetParameters (uint32_t messagePairsCount, double clockSkew, double clockOffset, double timingError, double velocityError);
	void SetSoundSpeed (double speed);
	void SetCarrierFreq (double freq);
	void SetPacketSize (uint32_t size);
	void SetBeaconAddress (Mac8Address address);
	void StopAcceleration();

private:
	virtual void StartApplication (void);
	virtual void StopApplication (void);

	void SendReply();
	bool ReceiveRequest(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &sender);

	Ptr<UanNetDevice> m_device;
	EventId m_sendEvent;
	bool m_running;

	// Timestamps
	double m_soundSpeed;
	double m_carrierFreq;
	double m_estimatedSkew;
	double m_estimatedOffset;
	double m_clockSkew;
	double m_clockOffset;
	double m_timeError;
	double m_velocityError;
	double m_T2;
	double m_T3;
	double m_V1;
	uint32_t m_messageSequence;
	uint32_t m_messagePairsCount;
	uint32_t m_pktSize;
	Mac8Address m_beaconAddress;
};

DCOrdinary::DCOrdinary ()
  : m_running (false),
    m_soundSpeed (1500.0),
    m_carrierFreq (20000.0),
	m_estimatedSkew (0),
	m_estimatedOffset (0),
	m_clockSkew (0),
	m_clockOffset (0),
	m_timeError (20e-6),
	m_velocityError (0.2),
	m_T2 (0),
	m_T3 (0),
	m_V1 (0),
	m_messageSequence (0),
	m_messagePairsCount (20),
	m_pktSize (60)
{
}

DCOrdinary::~DCOrdinary ()
{
}

void
DCOrdinary::SetParameters (uint32_t messagePairsCount, double clockSkew, double clockOffset, double timingError, double velocityError)
{
	m_messagePairsCount = messagePairsCount;
	m_clockSkew = clockSkew;
	m_clockOffset = clockOffset;
	m_timeError = timingError;
	m_velocityError = velocityError;
}

void
DCOrdinary::SetSoundSpeed (double speed)
{
	m_soundSpeed = speed;
}

void
DCOrdinary::SetCarrierFreq (double freq)
{
	m_carrierFreq = freq;
}

void
DCOrdinary::SetPacketSize (uint32_t size)
{
	m_pktSize = size;
}

void
DCOrdinary::StopAcceleration(){
	Ptr<ConstantAccelerationMobilityModel> mobility = GetNode()-> GetObject<ConstantAccelerationMobilityModel> ();
	double velocity = mobility->GetVelocity().x;
	mobility->SetVelocityAndAcceleration(Vector(velocity, 0.0, 0.0), Vector(0, 0.0, 0.0));
}


void
DCOrdinary::SetBeaconAddress (Mac8Address address)
{
	m_beaconAddress = address;
}

void
DCOrdinary::StartApplication (void)
{
	m_running = true;
	m_messageSequence = 0;

	m_device = GetNode ()->GetDevice (0)->GetObject<UanNetDevice> ();

	if (m_device)
    {
		m_device->SetReceiveCallback (MakeCallback (&DCOrdinary::ReceiveRequest, this)); // @suppress("Ambiguous problem") // @suppress("Invalid arguments")
		if(track1){
			double stopAccelerationInterval = (maxVelocity - initVelocity) / acceleration;
			Simulator::Schedule(Seconds(stopAccelerationInterval),&DCOrdinary::StopAcceleration, this);
		}
    }
	else
    {
		NS_LOG_ERROR ("Master: Could not get UAN device!");
    }
}

void
DCOrdinary::StopApplication (void)
{
	m_running = false;
	if (m_sendEvent.IsRunning ())
    {
		Simulator::Cancel (m_sendEvent);
    }
}

Ptr<DCBeacon> DCBeaconApp;
Ptr<DCOrdinary> DCOrdinaryApp;

double DCGetVelocity(){
	double radialVelocity = 0;
	if(track1){
		Ptr<ConstantAccelerationMobilityModel> mobility = DCOrdinaryApp->GetNode()-> GetObject<ConstantAccelerationMobilityModel> ();
		radialVelocity = mobility->GetVelocity().x;
	}else if(track2){
		Ptr<MobilityModel> mobNode = DCOrdinaryApp->GetNode()->GetObject<MobilityModel>();
		Ptr<MobilityModel> mobGw = DCBeaconApp->GetNode()->GetObject<MobilityModel>();

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
DCBeacon::SendRequest()
{
	m_messageSequence++;
	double T1 = Simulator::Now().GetSeconds();
	Ptr<Packet> packet = Create<Packet> (reinterpret_cast<const uint8_t*> (&T1), sizeof(T1));

	if (m_pktSize > packet->GetSize ()) {
		uint32_t paddingSize = m_pktSize - packet->GetSize ();
		Ptr<Packet> padding = Create<Packet> (paddingSize);
		packet->AddAtEnd (padding);
	}

	if (packet)
	{
		m_device->GetMac ()->Enqueue (packet, 0, m_ordinaryAddress);
	}
	m_T1List.push_back(T1);
	NS_LOG_DEBUG ("DCBeacon sent beacon number ("<<m_messageSequence<<") at: " << T1 << " s, Packet Size = "<<packet->GetSize()<<" Bytes");
	if(m_messageSequence < m_messagePairsCount){
		Simulator::Schedule(Seconds(m_messageInterval),&DCBeacon::SendRequest, this);
	}
}

bool
DCOrdinary::ReceiveRequest (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &sender)
{
	Ptr<NormalRandomVariable> timestampNoiseVar = CreateObject<NormalRandomVariable>();
	timestampNoiseVar->SetAttribute("Mean", DoubleValue(0.0));
	timestampNoiseVar->SetAttribute("Variance", DoubleValue(m_timeError * m_timeError));
	double timestampNoise = timestampNoiseVar->GetValue();

	double serialization_time = ((packet->GetSize() + 3) * 8) / dataRate;
	double T2 = Simulator::Now().GetSeconds() - serialization_time;

	T2 = T2 * (1 + (m_clockSkew * 1e-6)) + m_clockOffset + timestampNoise;
	m_T2 = T2;

	Ptr<NormalRandomVariable> velocityNoiseVar = CreateObject<NormalRandomVariable>();
	velocityNoiseVar->SetAttribute("Mean", DoubleValue(0.0));
	velocityNoiseVar->SetAttribute("Variance", DoubleValue(m_velocityError * m_velocityError));
	double velocityNoise = velocityNoiseVar->GetValue();

	m_V1 = DCGetVelocity() + velocityNoise;

	Ptr<Packet> p = packet->Copy();

	double T1;

	p->CopyData (reinterpret_cast<uint8_t*> (&T1), sizeof(T1));

	NS_LOG_DEBUG ("DCOrdinary received request at :"<< T2 << " s, Packet size :"<< packet->GetSize()<< " Bytes");

	Simulator::ScheduleNow(&DCOrdinary::SendReply, this);
	return true;
}

void
DCOrdinary::SendReply()
{
	m_messageSequence++;
	m_T3 = Simulator::Now().GetSeconds() * (1 + (m_clockSkew * 1e-6)) + m_clockOffset;

	struct Reply_Data {
		double t2;
		double t3;
		double v1;
	} data;

	data.t2 = m_T2;
	data.t3 = m_T3;
	data.v1 = m_V1;

	Ptr<Packet> packet = Create<Packet> (reinterpret_cast<const uint8_t*> (&data), sizeof(data));

	if (m_pktSize > packet->GetSize ()) {
		uint32_t paddingSize = m_pktSize - packet->GetSize ();
		Ptr<Packet> padding = Create<Packet> (paddingSize);
		packet->AddAtEnd (padding);
	}

	if (packet)
	{
		m_device->GetMac ()->Enqueue (packet, 0, m_beaconAddress);
	}
	NS_LOG_DEBUG ("DCOrdinary sent reply number ("<<m_messageSequence<<") at: " << m_T3 << " s, Packet Size = "<<packet->GetSize()<<" Bytes");
}

bool
DCBeacon::ReceiveReply (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &sender)
{
	double serialization_time = ((packet->GetSize() + 3) * 8) / dataRate;

	double T4 = Simulator::Now().GetSeconds() - serialization_time;

	Ptr<NormalRandomVariable> timestampNoiseVar = CreateObject<NormalRandomVariable>();
	timestampNoiseVar->SetAttribute("Mean", DoubleValue(0.0));
	timestampNoiseVar->SetAttribute("Variance", DoubleValue(m_timeError * m_timeError));
	double timestampNoise = timestampNoiseVar->GetValue();

	T4 = T4 + timestampNoise;

	Ptr<NormalRandomVariable> velocityNoiseVar = CreateObject<NormalRandomVariable>();
	velocityNoiseVar->SetAttribute("Mean", DoubleValue(0.0));
	velocityNoiseVar->SetAttribute("Variance", DoubleValue(m_velocityError * m_velocityError));
	double velocityNoise = velocityNoiseVar->GetValue();

	double V2 = DCGetVelocity() + velocityNoise;

	Ptr<Packet> p = packet->Copy();

	struct Reply_Data {
		double t2;
		double t3;
		double v1;
	} data;

	p->CopyData (reinterpret_cast<uint8_t*> (&data), sizeof(data));

	m_T2List.push_back(data.t2);
	m_T3List.push_back(data.t3);
	m_T4List.push_back(T4);
	m_V1List.push_back(data.v1);
	m_V2List.push_back(V2);

	NS_LOG_DEBUG ("DCBeacon received reply at :"<< T4 << " s, Packet size :"<< packet->GetSize()<< " Bytes");

	if(m_messageSequence == m_messagePairsCount){
		DCSyncDopplerCompensation(0);
		CalculateRegression();
	    Calibration();
	}
	return true;
}

void
DCBeacon::DCSyncDopplerCompensation(int iteration) {
    m_a_c_List.clear();

    double alpha = (iteration == 0) ? 1.0 : m_estimatedSkew;

    std::vector<double> all_velocities;
    std::vector<double> all_times;

//    double dopplerNoise = m_velocityError / 1500;

    for(uint32_t i = 0; i < m_messagePairsCount; i++) {
        double a_AB, a_BA;
        if (iteration == 0) {
            a_AB = (m_V1List[i] / m_soundSpeed);

            a_BA = (m_V2List[i] / m_soundSpeed);

            if (m_a_AB_List.size() <= i) {
                m_a_AB_List.push_back(a_AB);
                m_a_BA_List.push_back(a_BA);
            }
        } else {
            a_AB = m_a_AB_List[i];
            a_BA = m_a_BA_List[i];
        }

        double v1 = (1.0 - ((1.0 - a_AB) * alpha)) * m_soundSpeed;
        double v2 = (((1.0 + a_BA) * alpha) - 1.0) * m_soundSpeed;

        all_velocities.push_back(v1);
        all_velocities.push_back(v2);

        all_times.push_back(m_T2List[i]);
        all_times.push_back(m_T4List[i]);
    }

    auto poly_coeffs = FitCubicPolynomial(all_times, all_velocities);

    for(uint32_t i = 0; i < m_messagePairsCount; i++) {
        double t2 = m_T2List[i];
        double t4 = m_T4List[i];

        double integral = IntegratePolynomial(poly_coeffs, t2, t4);
        double a_c = integral / (m_soundSpeed * (t4 - t2));

        m_a_c_List.push_back(a_c);
    }
}

std::vector<double>
DCBeacon::FitCubicPolynomial(const std::vector<double>& times,
                                       const std::vector<double>& velocities) {
    int N = times.size();

    if (N < 4) {
        std::cout << "WARNING: Need at least 4 points for cubic fit" << std::endl;
        double avg_v = 0;
        for (double v : velocities) avg_v += v;
        avg_v /= N;
        return {0, 0, 0, avg_v};
    }

    Eigen::MatrixXd X(N, 4);
    Eigen::VectorXd y(N);

    for (int i = 0; i < N; i++) {
        double t = times[i];
        X(i, 0) = t*t*t;     // t³
        X(i, 1) = t*t;       // t²
        X(i, 2) = t;         // t
        X(i, 3) = 1.0;       // 1

        y(i) = velocities[i];
    }

    // Least squares: coeffs = (XᵀX)⁻¹ Xᵀy
    Eigen::Vector4d coeffs = (X.transpose() * X).inverse() * X.transpose() * y; // @suppress("Invalid arguments")

    return {coeffs(0), coeffs(1), coeffs(2), coeffs(3)};
}

double
DCBeacon::IntegratePolynomial(const std::vector<double>& coeffs,
                           double t_start, double t_end) {
    if (coeffs.size() != 4) {
        std::cout << "ERROR: Need 4 coefficients for cubic polynomial" << std::endl;
        return 0.0;
    }

    double a = coeffs[0];
    double b = coeffs[1];
    double c = coeffs[2];
    double d = coeffs[3];

    double integral_end = (a/4.0)*pow(t_end, 4) + (b/3.0)*pow(t_end, 3)
                        + (c/2.0)*pow(t_end, 2) + d*t_end;

    double integral_start = (a/4.0)*pow(t_start, 4) + (b/3.0)*pow(t_start, 3)
                          + (c/2.0)*pow(t_start, 2) + d*t_start;

    return integral_end - integral_start;
}

void
DCBeacon::CalculateRegression(){
	Eigen::MatrixXd H(m_messagePairsCount, 2);
	Eigen::VectorXd Z(m_messagePairsCount);
    for (uint32_t i = 0; i < m_messagePairsCount; i++) {
        double t1 = m_T1List[i];
        double t4 = m_T4List[i];
        double T2 = m_T2List[i];
        double T3 = m_T3List[i];
        double a_e = m_a_c_List[i];

        H(i, 0) = (t4 * (1 - a_e)) + t1;
        H(i, 1) = 2 - a_e;

        Z(i) = T3 + ((1 - a_e) * T2);
    }

    Eigen::VectorXd solution = (H.transpose() * H).inverse() * H.transpose() * Z; // @suppress("Invalid arguments")
    m_estimatedSkew = solution(0);
    m_estimatedOffset = solution(1);
}

void
DCBeacon::Calibration() {
	NS_LOG_DEBUG("=== CALIBRATION PHASE ===");

    const int MAX_ITERATIONS = 5;

    for (int iter = 0; iter < MAX_ITERATIONS; iter++) {
    	NS_LOG_DEBUG("--- Iteration " << iter + 1 << " ---");
    	DCSyncDopplerCompensation(iter+1);
        CalculateRegression();
    }

    // Final results
    NS_LOG_DEBUG("\n=== FINAL RESULTS ===");
    NS_LOG_DEBUG("α = " << m_estimatedSkew <<" (error: " << (m_estimatedSkew - (1+(m_clockSkew*1e-6)))*1e6 << " ppm)");
    NS_LOG_DEBUG("β = " << m_estimatedOffset<< " s (error: " << std::abs(std::abs(m_estimatedOffset) - m_clockOffset)*1e6 << " us)");
    DCOffsetError = std::abs(std::abs(m_estimatedOffset) - std::abs(m_clockOffset))*1e6;
    DCSkewError = std::abs((std::abs(m_estimatedSkew) - std::abs(1+(m_clockSkew*1e-6))))*1e6;
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
	double messagePairsCount = 20;
	double DCMessageInterval = 5; //5 seconds
	double clockSkew = 200;   //200 ppm
	double clockOffset = 0.02; //20 ms
	double timeError = 20e-6; //20us
	double velocityError = 0.2; //0.2 m/s
	uint32_t pktSize = 60;  //60 byte

	double radius = 100.0;
	double omega = velocity / radius; // Result: 0.04 rad/s

	int totalRuns = 1;
	int totalVariations = 1;
	int currentVariationIndex = 2;

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
	cmd.AddValue ("messagePairsCount", "Number of two-way pairs", messagePairsCount);
	cmd.AddValue ("messageInterval", "Time to between messages (s)", DCMessageInterval);
	cmd.AddValue ("pktSize", "Packet size in bytes", pktSize);
	cmd.AddValue ("totalRuns", "Number of runs", totalRuns);
	cmd.AddValue ("totalVariations", "Number of variations", totalVariations);
	cmd.AddValue ("currentVariationIndex", "current variation index", currentVariationIndex);

	cmd.Parse (argc, argv);

	RngSeedManager::SetSeed(seed);
	RngSeedManager::SetRun(run);

	if(track2){
		initDistance = initDistance - radius;
	}

	maxVelocity = initVelocity + 3;

	double runtime = (messagePairsCount * DCMessageInterval) + 10;

	// Enable detailed UAN logging - use INFO level to see timing information
	LogComponentEnable ("UanPhyGen", LOG_LEVEL_INFO);
	LogComponentEnable ("UanMacAloha", LOG_LEVEL_INFO);
	LogComponentEnable ("UanChannel", LOG_LEVEL_INFO);
	LogComponentEnable ("UanNetDevice", LOG_LEVEL_INFO);

	// Enable our application logging
	LogComponentEnable ("DC-Sync", LOG_LEVEL_INFO);

	// Create nodes
	NodeContainer DCNodes;
	DCNodes.Create (2);

	Ptr<ConstantAccelerationMobilityModel> DCSyncMobility1 = CreateObject<ConstantAccelerationMobilityModel>();

	DCSyncMobility1->SetPosition(Vector(0.0, 0.0, -100.0));
	DCSyncMobility1->SetVelocityAndAcceleration(Vector(0, 0.0, 0.0), Vector(0, 0.0, 0.0));

	DCNodes.Get (0)->AggregateObject(DCSyncMobility1);

	if(track1){
		Ptr<ConstantAccelerationMobilityModel> DCSyncMobility2 = CreateObject<ConstantAccelerationMobilityModel>();
		DCSyncMobility2->SetPosition(Vector(initDistance, 0.0, -100.0));
		DCSyncMobility2->SetVelocityAndAcceleration(Vector(initVelocity, 0.0, 0.0), Vector(acceleration, 0.0, 0.0));
		DCNodes.Get (1)->AggregateObject(DCSyncMobility2);
	}else if(track2){
		Ptr<WaypointMobilityModel> DCwaypoints = CreateObject<WaypointMobilityModel>();
		double initialPhase = angleIndex * (M_PI / 4.0);

		for (double t = 0; t < runtime; t += 0.1) {

			double phase = (omega * t) + initialPhase;

		    double DCx = initDistance + radius * cos(phase);
		    double y = radius * sin(phase);

		    DCwaypoints->AddWaypoint(Waypoint(Seconds(t), Vector(DCx, y, -100.0)));
		}

		DCNodes.Get(1)->AggregateObject(DCwaypoints);
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
	NetDeviceContainer DCDevices = uan.Install (DCNodes, channel);

	// Create energy sources for both nodes
	BasicEnergySourceHelper DCEnergySourceHelper;
	DCEnergySourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (1000000)); // 1 MJ initial energy
	DCEnergySourceHelper.Set ("BasicEnergySupplyVoltageV", DoubleValue (12)); // 12V typical for underwater systems

	EnergySourceContainer DCEnergySources = DCEnergySourceHelper.Install (DCNodes);

	// Create UAN energy model helper
	AcousticModemEnergyModelHelper DCAcousticModemEnergyHelper; // @suppress("Abstract class cannot be instantiated")

	// Install energy models on UAN devices
	DeviceEnergyModelContainer DCDeviceEnergyModels = DCAcousticModemEnergyHelper.Install (DCDevices, DCEnergySources);

	auto DCPrintEnergyReport = [&]() {
		std::cout << "\n=== DC DETAILED ENERGY CONSUMPTION REPORT ===" << std::endl;

		for (uint32_t i = 0; i < DCNodes.GetN(); i++)
		{
			Ptr<EnergySource> energySource = DCEnergySources.Get(i);
			Ptr<DeviceEnergyModel> energyModel = DCDeviceEnergyModels.Get(i);

			std::cout << "Node " << i << " (" << (i == 0 ? "DCBeacon" : "DCOrdinary") << "):" << std::endl;
			std::cout << "  Initial energy: " << energySource->GetInitialEnergy() << " J" << std::endl;
			std::cout << "  Remaining energy: " << energySource->GetRemainingEnergy() << " J" << std::endl;
			std::cout << "  Total consumed: " << energyModel->GetTotalEnergyConsumption() << " J" << std::endl;
		}

		// Protocol efficiency analysis
		std::cout << "\n--- Protocol Efficiency ---" << std::endl;
		std::cout << "Total energy for one synchronization: "
				<< (DCDeviceEnergyModels.Get(0)->GetTotalEnergyConsumption() +
						DCDeviceEnergyModels.Get(1)->GetTotalEnergyConsumption()) << " J" << std::endl;
		std::cout << "Beacon/Ordinary energy ratio: "
				<< (DCDeviceEnergyModels.Get(0)->GetTotalEnergyConsumption() /
						DCDeviceEnergyModels.Get(1)->GetTotalEnergyConsumption()) << std::endl;
		std::cout << "==========================================" << std::endl;
	};

	// Get MAC addresses
	Ptr<UanNetDevice> DCBeaconDevice = DCDevices.Get (0)->GetObject<UanNetDevice> ();
	Ptr<UanNetDevice> DCOrdinaryDevice = DCDevices.Get (1)->GetObject<UanNetDevice> ();

	Mac8Address DCBeaconMac = Mac8Address::ConvertFrom (DCBeaconDevice->GetMac ()->GetAddress ());
	Mac8Address DCOrdainryMac = Mac8Address::ConvertFrom (DCOrdinaryDevice->GetMac ()->GetAddress ());

	// Create applications
	// Beacon application
	DCBeaconApp = CreateObject<DCBeacon> ();
	DCBeaconApp->SetParameters (DCMessageInterval, messagePairsCount, clockSkew, clockOffset, timeError, velocityError);
	DCBeaconApp->SetSoundSpeed (soundSpeed);
	DCBeaconApp->SetCarrierFreq (carrierFreq);
	DCBeaconApp->SetPacketSize (pktSize);
	DCBeaconApp->SetOrdinaryAddress(DCOrdainryMac);
	DCNodes.Get (0)->AddApplication (DCBeaconApp);
	DCBeaconApp->SetStartTime (Seconds (0));

	// Ordinary application
	DCOrdinaryApp = CreateObject<DCOrdinary> ();
	DCOrdinaryApp->SetParameters (messagePairsCount, clockSkew, clockOffset, timeError, velocityError);
	DCOrdinaryApp->SetSoundSpeed (soundSpeed);
	DCOrdinaryApp->SetCarrierFreq (carrierFreq);
	DCOrdinaryApp->SetPacketSize (pktSize);
	DCOrdinaryApp->SetBeaconAddress(DCBeaconMac);
	DCNodes.Get (1)->AddApplication (DCOrdinaryApp);
	DCOrdinaryApp->SetStartTime (Seconds (0));

	auto PrintResuDCs = [&]() {
		switch (plotNum) {
			case'0':{
				DCPrintEnergyReport();
				std::cout << "=== DC Results ===" << std::endl;
				std::cout << "DC offset error = " <<DCOffsetError<<" us"<< std::endl;
				std::cout << "DC skew error = " <<DCSkewError<<" ppm"<< std::endl;
				break;
			}
			case '1':{
				std::ofstream plot("csv/plot1.csv", std::ios::app);
				if (track1) {
				    plot << DCOffsetError << ",";
				}
				else if (track2) {
				    plot << DCOffsetError << "\n";
				}
			break;
			}
			case '2':{
				std::ofstream plot("csv/plot2.csv", std::ios::app);
				plot << DCOffsetError << "\n";
			break;
			}
			case '3':{
				std::ofstream plot("csv/plot3.csv", std::ios::app);
				plot << DCOffsetError << "\n";
			break;
			}
			case '4':{
				std::ofstream plot("csv/plot4.csv", std::ios::app);
				plot << DCOffsetError << "\n";
			break;
			}
			case '5':{
				std::ofstream plot("csv/plot5.csv", std::ios::app);
				if (track1) {
				    plot << DCOffsetError << ",";
				}
				else if (track2) {
				    plot << DCOffsetError << "\n";
				}
				break;
			}
			case '6':{
				std::ofstream plot("csv/plot6.csv", std::ios::app);
				if (track1) {
				    plot << DCOffsetError << ",";
				}
				else if (track2) {
				    plot << DCOffsetError << "\n";
				}
				break;
			}
			case '7':{
				std::ofstream plot("csv/plot7.csv", std::ios::app);
				if (track1) {
				    plot << DCOffsetError << ",";
				}
				else if (track2) {
				    plot << DCOffsetError << "\n";
				}
				break;
			}
			case '8':{
				std::ofstream plot("csv/plot8.csv", std::ios::app);
				if (track1) {
				    plot << DCOffsetError << ",";
				}
				else if (track2) {
				    plot << DCOffsetError << "\n";
				}
				break;
			}
			case '9':{
				std::ofstream plot("csv/plot9c.csv", std::ios::app);
				if(track1){
					if(run == 1 && messagePairsCount == 10){
						plot << "Track 1,,,Track 2\n";
						plot << "Two-way Message Pairs,DC-Skew Error (ppm),,DC-Skew Error (ppm)\n";
					}
					plot << messagePairsCount << "," << DCSkewError << ",";
				}else if (track2){
					plot << "," << DCSkewError << "\n";
				}
				break;
			}
			default:{
				break;
			}
		}
	};

	Simulator::Schedule (Seconds (runtime), PrintResuDCs);

	double currentRun = ((currentVariationIndex-1) * totalRuns) + run;
	double allRuns = totalVariations * totalRuns;
	double finishedPrecnetage = (currentRun / allRuns) * 100;
	std::cout << "Starting simulation plot num ("<<plotNum<<"), Finished ("<<currentRun <<"/"<< allRuns <<") "<< finishedPrecnetage <<" %"<<std::endl;

	Simulator::Stop (Seconds(runtime));
	Simulator::Run ();
	Simulator::Destroy ();

	return 0;
}
