/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store.h"
#include "ns3/mmwave-helper.h"
#include <ns3/buildings-helper.h>
#include "ns3/global-route-manager.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include <map>
#include "ns3/lte-chunk-processor.h"

using namespace ns3;
using namespace mmwave;

/* In this example a single UE is connected with a single MmWave BS. The UE is
 * placed at distance ueDist from the BS and it does not move. The system
 * bandwidth is fixed at 1GHz. If CA is enabled, 2 CCs are used and each of them
 * uses half of the total bandwidth.
 */

int
main (int argc, char *argv[])
{
  double dist = 10;
  double simTime = 0.1;
  // RNG
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (1);

  // set output file names
  std::string filePath;
  Config::SetDefault ("ns3::MmWaveBearerStatsCalculator::DlRlcOutputFilename", StringValue (filePath + "DlRlcStats.txt"));
  Config::SetDefault ("ns3::MmWaveBearerStatsCalculator::UlRlcOutputFilename", StringValue (filePath + "UlRlcStats.txt"));
  Config::SetDefault ("ns3::MmWaveBearerStatsCalculator::DlPdcpOutputFilename", StringValue (filePath + "DlPdcpStats.txt"));
  Config::SetDefault ("ns3::MmWaveBearerStatsCalculator::UlPdcpOutputFilename", StringValue (filePath + "UlPdcpStats.txt"));
  Config::SetDefault ("ns3::MmWavePhyRxTrace::OutputFilename", StringValue (filePath + "RxPacketTrace.txt"));
  Config::SetDefault ("ns3::LteRlcAm::BufferSizeFilename", StringValue (filePath + "RlcAmBufferSize.txt"));

  // create and set the helper
  // first set UseCa = true, then NumberOfComponentCarriers
  Config::SetDefault ("ns3::MmWaveHelper::UseCa",BooleanValue (false));
  Config::SetDefault ("ns3::MmWaveHelper::ChannelModel",StringValue ("ns3::MmWave3gppChannel"));
  Config::SetDefault ("ns3::MmWaveHelper::PathlossModel",StringValue ("ns3::MmWave3gppPropagationLossModel"));

  // TODO try with simpler channel model
  // The available channel scenarios are 'RMa', 'UMa', 'UMi-StreetCanyon', 'InH-OfficeMixed', 'InH-OfficeOpen', 'InH-ShoppingMall'
  std::string scenario = "UMa";
  std::string condition = "l"; // n = NLOS, l = LOS
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::ChannelCondition", StringValue (condition));
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Scenario", StringValue (scenario));
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::OptionalNlos", BooleanValue (false));
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Shadowing", BooleanValue (false)); // enable or disable the shadowing effect
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::InCar", BooleanValue (false)); // enable or disable the shadowing effect

  Config::SetDefault ("ns3::MmWave3gppChannel::UpdatePeriod", TimeValue (MilliSeconds (100))); // Set channel update period, 0 stands for no update.
  Config::SetDefault ("ns3::MmWave3gppChannel::DirectBeam", BooleanValue (true)); // Set true to perform the beam in the exact direction of receiver node.
  Config::SetDefault ("ns3::MmWave3gppChannel::PortraitMode", BooleanValue (true)); // use blockage model with UT in portrait mode
  Config::SetDefault ("ns3::MmWave3gppChannel::NumNonselfBlocking", IntegerValue (4)); // number of non-self blocking obstacles
  Config::SetDefault ("ns3::MmWave3gppChannel::BlockerSpeed", DoubleValue (1)); // speed of non-self blocking obstacles

  Ptr<MmWaveHelper> helper = CreateObject<MmWaveHelper> ();

  // create the enb node
  NodeContainer enbNodes;
  enbNodes.Create (1);

  // set mobility
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  enbPositionAlloc->Add (Vector (0.0, 0.0, 15.0));

  MobilityHelper enbmobility;
  enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbmobility.SetPositionAllocator (enbPositionAlloc);
  enbmobility.Install (enbNodes);
  BuildingsHelper::Install (enbNodes);

  // install enb device
  NetDeviceContainer enbNetDevices = helper->InstallEnbDevice (enbNodes);

  // create ue node
  NodeContainer ueNodes;
  ueNodes.Create (1);

  // set mobility
  MobilityHelper uemobility;
  uemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
  uePositionAlloc->Add (Vector (dist, 0.0, 1.6));
  uemobility.SetPositionAllocator (uePositionAlloc);
  uemobility.Install (ueNodes.Get (0));
  BuildingsHelper::Install (ueNodes);

  // install ue device
  NetDeviceContainer ueNetDevices = helper->InstallUeDevice (ueNodes);

  Ptr<MmWaveUePhy> ue0Phy = ueNetDevices.Get (0)->GetObject<MmWaveUeNetDevice> ()->GetPhy ()->GetObject<MmWaveUePhy> ();
  Ptr<mmWaveChunkProcessor> testDlSinr1 = Create<mmWaveChunkProcessor> ();
  LteSpectrumValueCatcher dlSinr1Catcher;
  // TODO define a function which prints the SINR and assign it as callback for
  // the mmWaveChunkProcessor
  testDlSinr1->AddCallback (MakeCallback (&LteSpectrumValueCatcher::ReportValue, &dlSinr1Catcher));
  ue0Phy->GetDlSpectrumPhy ()->AddDataSinrChunkProcessor (testDlSinr1);

  helper->AttachToClosestEnb (ueNetDevices, enbNetDevices);
  helper->EnableTraces ();

  // activate a data radio bearer
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);
  helper->ActivateDataRadioBearer (ueNetDevices, bearer);

  BuildingsHelper::MakeMobilityModelConsistent ();

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

  std::cout << 10.0 * log10 (dlSinr1Catcher.GetValue ()->operator[] (0)) << std::endl;
  Simulator::Destroy ();

  return 0;
}
