#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mmwave-helper.h"

using namespace ns3;
using namespace mmwave;

int
main (int argc, char *argv[])
{
  double distance = 100.0;

  // common configurations
  Config::SetDefault ("ns3::MmWaveHelper::PathlossModel", StringValue ("ns3::MmWave3gppPropagationLossModel"));
  Config::SetDefault ("ns3::MmWaveHelper::ChannelModel", StringValue ("ns3::MmWave3gppChannel"));
  Config::SetDefault ("ns3::MmWave3gppChannel::DirectBeam", BooleanValue(false));
  Config::SetDefault ("ns3::AntennaArrayModel::IsotropicAntennaElements", BooleanValue(true)); // use the 3gpp radiation model for the antenna elements
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Scenario", StringValue ("UMa")); // set the propagation environment
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::ChannelCondition", StringValue ("l")); // l = LOS, n = NLOS
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Shadowing", BooleanValue (false)); // enable/disable shadowing
  Config::SetDefault ("ns3::MmWaveHelper::NumUePanels", UintegerValue (2)); // number of antenna panels at the UE
  Config::SetDefault ("ns3::MmWaveHelper::NumEnbPanels", UintegerValue (3)); // number of antenna panels at the BS
  Config::SetDefault ("ns3::MmWave3gppChannel::UpdatePeriod", TimeValue (MilliSeconds (1000))); // channel update time
  Config::SetDefault ("ns3::MmWavePhyMacCommon::CenterFreq", DoubleValue (28e9)); // carrier frequency
  Config::SetDefault ("ns3::MmWaveUeNetDevice::AntennaNum", UintegerValue(16)); // dimension of the UE antenna array
  Config::SetDefault ("ns3::MmWaveEnbNetDevice::AntennaNum", UintegerValue(64)); // dimension of the BS antenna array

  // set the command line parameters
  CommandLine cmd;
  cmd.AddValue ("distance", "distance UE-BS", distance);
  cmd.Parse (argc, argv);

  // create a Node for the BS
  NodeContainer bsNodes;
  bsNodes.Create (1);

  // create a Node for the UE
  NodeContainer ueNodes;
  ueNodes.Create (1);

  // create a ListPositionAllocator and add the position of the BS
  Ptr<ListPositionAllocator> bsPositionAlloc = CreateObject<ListPositionAllocator> ();
  bsPositionAlloc->Add (Vector (0.0, 0.0, 10.0));

  // use a MobilityHelper to install the MobilityModel for the BS
  MobilityHelper bsMobility;
  bsMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  bsMobility.SetPositionAllocator (bsPositionAlloc);
  bsMobility.Install (bsNodes);

  // create a ListPositionAllocator and add the position of the UE
  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
  uePositionAlloc->Add (Vector (distance, 0.0, 1.6));

  // use a MobilityHelper to install the MobilityModel for the UE
  MobilityHelper ueMobility;
  ueMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  ueMobility.SetPositionAllocator (uePositionAlloc);
  ueMobility.Install (ueNodes);

  // create the MmWaveHelper
  Ptr<MmWaveHelper> mmWaveHelper = CreateObject<MmWaveHelper> ();

  // install the BS device on the proper Node
  NetDeviceContainer bsNetDev = mmWaveHelper->InstallEnbDevice (bsNodes);

  // install the UE device on the proper Node
  NetDeviceContainer ueNetDev = mmWaveHelper->InstallUeDevice (ueNodes);

  // connet the UE to the BS
  mmWaveHelper->AttachToClosestEnb (ueNetDev, bsNetDev);

  // activate a data radio bearer
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);
  mmWaveHelper->ActivateDataRadioBearer (ueNetDev, bearer);

  // anable the output traces
  mmWaveHelper->EnableTraces ();

  // run the simulation
  Simulator::Stop (MilliSeconds (500));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
