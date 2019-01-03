/*
 * nrProducer.cc
 *
 *  Created on: Jan 12, 2015
 *      Author: chen
 */

#include "nrProducer.h"
#include "NodeSensor.h"
#include "nrUtils.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/string.h"
#include "ns3/log.h"

#include "ns3/ndn-app-face.h"
#include "ns3/ndn-fib.h"

#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"

NS_LOG_COMPONENT_DEFINE("ndn.nrndn.nrProducer");

namespace ns3
{
namespace ndn
{
namespace nrndn
{
NS_OBJECT_ENSURE_REGISTERED(nrProducer);

TypeId nrProducer::GetTypeId()
{
	static TypeId tid = TypeId("ns3::ndn::nrndn::nrProducer")
							.SetGroupName("Nrndn")
							.SetParent<App>()
							.AddConstructor<nrProducer>()
							.AddAttribute("Prefix", "Prefix, for which producer has the data",
										  StringValue("/"),
										  MakeNameAccessor(&nrProducer::m_prefix),
										  MakeNameChecker())
							.AddAttribute("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
										  StringValue("/"),
										  MakeNameAccessor(&nrProducer::m_postfix),
										  MakeNameChecker())
							.AddAttribute("PayloadSize", "Virtual payload size for traffic Content packets",
										  UintegerValue(1024),
										  MakeUintegerAccessor(&nrProducer::m_virtualPayloadSize),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
										  TimeValue(Seconds(0)),
										  MakeTimeAccessor(&nrProducer::m_freshness),
										  MakeTimeChecker())
							.AddAttribute("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
										  UintegerValue(0),
										  MakeUintegerAccessor(&nrProducer::m_signature),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
										  NameValue(),
										  MakeNameAccessor(&nrProducer::m_keyLocator),
										  MakeNameChecker());
	return tid;
}

nrProducer::nrProducer() : m_rand(0, std::numeric_limits<uint32_t>::max()),
						   m_virtualPayloadSize(1024),
						   m_signature(0)
{
	//NS_LOG_FUNCTION(this);
}

nrProducer::~nrProducer()
{
	// TODO Auto-generated destructor stub
}

void nrProducer::OnInterest(Ptr<const Interest> interest)
{
}

//Need to collect the new neighborhood traffic information
void nrProducer::laneChange(std::string oldLane, std::string newLane)
{
	NS_LOG_FUNCTION(this);
	NS_LOG_INFO("Lane change of node " << GetNode()->GetId()
									   << " : move from " << oldLane << " to " << newLane);
	this->SetAttribute("Prefix", StringValue('/' + newLane));
}

void nrProducer::StartApplication()
{
	NS_LOG_FUNCTION_NOARGS();
	NS_ASSERT(GetNode()->GetObject<Fib>() != 0);

	if (m_forwardingStrategy)
		m_forwardingStrategy->Start();

	if (m_CDSBasedForwarding)
		m_CDSBasedForwarding->Start();

	if (m_DistanceForwarding)
		m_DistanceForwarding->Start();
	App::StartApplication();

	NS_LOG_INFO("NodeID: " << GetNode()->GetId());
}

void nrProducer::StopApplication()
{
	NS_LOG_FUNCTION_NOARGS();
	NS_ASSERT(GetNode()->GetObject<Fib>() != 0);

	if (m_forwardingStrategy)
		m_forwardingStrategy->Stop();

	if (m_CDSBasedForwarding)
		m_CDSBasedForwarding->Stop();

	if (m_DistanceForwarding)
		m_DistanceForwarding->Stop();

	App::StopApplication();
}

//Initialize the callback function after the base class initialize
void nrProducer::DoInitialize(void)
{
	if (m_forwardingStrategy == 0)
	{
		Ptr<ForwardingStrategy> forwardingStrategy = m_node->GetObject<ForwardingStrategy>();
		NS_ASSERT_MSG(forwardingStrategy, "nrProducer::DoInitialize cannot find ns3::ndn::fw::ForwardingStrategy");
		if (forwardingStrategy)
		{
			m_forwardingStrategy = DynamicCast<fw::nrndn::NavigationRouteHeuristic>(forwardingStrategy);
			m_CDSBasedForwarding = DynamicCast<fw::nrndn::CDSBasedForwarding>(forwardingStrategy);
			m_DistanceForwarding = DynamicCast<fw::nrndn::DistanceBasedForwarding>(forwardingStrategy);
		}
	}

	if (m_sensor == 0)
	{
		m_sensor = m_node->GetObject<NodeSensor>();

		NS_ASSERT_MSG(m_sensor, "nrProducer::DoInitialize cannot find ns3::ndn::nrndn::NodeSensor");
		// Setup Lane change action
		if (m_sensor != NULL)
		{
			m_sensor->TraceConnectWithoutContext("LaneChange", MakeCallback(&nrProducer::laneChange, this));
		}
	}
	super::DoInitialize();
}

void nrProducer::NotifyNewAggregate()
{
	super::NotifyNewAggregate();
}

void nrProducer::OnSendingTrafficData()
{
	//调整到getLane前面，避免非活跃节点getLane();
	if (!m_active)
		return;
	//std::cout<<"(nrProducer.cc-OnSendingTrafficData) NodeId: "<<GetNode()->GetId()<<endl;
	//Before sending traffic Data, reflash the current lane first!!
	//If not, Let's assume vehicle A is just into lane_2 and previous lane is lane_1,
	//        when A sending traffic data, it's data name may be lane_1 because
	//        the m_sensor->getLane() has not executed. When it happens,
	//        in function nrUtils::GetNodeSizeAndInterestNodeSize, it will reflash all
	//        the lane information and clean the pit entry of lane_1. In that case,
	//        Vehicle A will not find the pit entry of lane_1, so the process crash
	m_sensor->getLane();

	NS_LOG_FUNCTION(this << "Sending Traffic Data:" << m_prefix.toUri());

	Ptr<Data> data = Create<Data>(Create<Packet>(m_virtualPayloadSize));

	//生成数据包名称
	m_prefix = getDataName(m_prefix);
	Ptr<Name> dataName = Create<Name>(m_prefix);

	//2018.12.10 doesn't work
	//dataName->append(m_postfix); //m_postfix is "/", seems OK
	data->SetName(dataName);
	// 2018.1.24
	data->SetFreshness(Seconds(10.0));
	data->SetTimestamp(Simulator::Now());

	data->SetSignature(m_rand.GetValue()); //just generate a random number
	if (m_keyLocator.size() > 0)
	{
		data->SetKeyLocator(Create<Name>(m_keyLocator));
	}

	std::cout << "(nrProducer.cc-OnSendingTrafficData) 源节点 " << GetNode()->GetId() << " at time " << Simulator::Now().GetSeconds()
			  << " 发送数据包:" << m_prefix.toUri() << " Signature " << data->GetSignature() << std::endl;

	NS_LOG_DEBUG("node(" << GetNode()->GetId() << ")\t sending Traffic Data: " << data->GetName() << " \tsignature:" << data->GetSignature());
	FwHopCountTag hopCountTag;
	data->GetPayload()->AddPacketTag(hopCountTag);

	std::pair<uint32_t, uint32_t> size_InterestSize = nrUtils::GetNodeSizeAndInterestNodeSize(GetNode()->GetId(), data->GetSignature(), m_prefix.get(0).toUri());
	//当前活跃节点总数
	nrUtils::SetNodeSize(GetNode()->GetId(), data->GetSignature(), size_InterestSize.first);
	cout << "(nrProducer.cc-OnSendingTrafficData) 当前活跃节点个数为 " << size_InterestSize.first << " 感兴趣的节点总数为 " << size_InterestSize.second << std::endl
		 << std::endl;

	//当前对该数据包感兴趣节点总数
	nrUtils::SetInterestedNodeSize(GetNode()->GetId(), data->GetSignature(), size_InterestSize.second);
	m_face->ReceiveData(data);
	m_transmittedDatas(data, this, m_face);
}

Name nrProducer::getDataName(Name m_prefix)
{
	string m_name = "";
	//获得数据源的坐标
	double x = m_sensor->getX();
	double y = m_sensor->getY();
	string m_x = numToString(x);
	string m_y = numToString(y);

	//随机确定事件类型
	UniformVariable randType(0, 2);
	uint32_t index = randType.GetValue();
	//车辆状态相关
	if (index == 0)
	{
		UniformVariable randDistance(4000, 5000);
		uint32_t distance = randDistance.GetValue();
		string m_distance = numToString(distance);
		m_name = "/vehicle/" + m_distance + "/" + m_x + "/" + m_y;
	}
	else
	{
		m_name = "/road/" + m_x + "/" + m_y;
	}

	m_prefix = Name("/" + m_prefix.get(0).toUri()) + m_name;
	return m_prefix;
}

string nrProducer::numToString(double value)
{
	ostringstream stream;
	stream << value;
	return stream.str();
}

void nrProducer::OnData(Ptr<const Data> contentObject)
{
	NS_LOG_FUNCTION("None its business");
	App::OnData(contentObject);
}

void nrProducer::ScheduleAccident(double t)
{
	m_accidentList.insert(t);
	Simulator::Schedule(Seconds(t), &nrProducer::OnSendingTrafficData, this);
}

void nrProducer::setContentStore(std::string prefix)
{
}

void nrProducer::addAccident()
{
	double start = m_startTime.GetSeconds();
	double end = m_stopTime.GetSeconds();
	double mean = start + (end - start) / 2;
	double varience = (end - start) / 4;

	//Use normal distribution
	SeedManager::SetSeed(15);
	NormalVariable nrnd(mean, varience, (end - start) / 2);
	double t = 0;
	while (true)
	{
		t = nrnd.GetValue();
		if (!m_accidentList.count(t))
		{
			ScheduleAccident(t);
			break;
		}
	}
	NS_LOG_DEBUG(m_node->GetId() << " add accident at " << t);
	return;
}

void nrProducer::addAccident(double iType)
{
	if (iType <= 0.00001)
	{ //产生随机事件
		addAccident();
		return;
	}
	double start = m_startTime.GetSeconds();
	double end = m_stopTime.GetSeconds();

	for (double dTime = 200; dTime < 450; dTime += iType)
	{
		ScheduleAccident(dTime);
	}
	return;
}

bool nrProducer::IsActive()
{
	return m_active;
}

bool nrProducer::IsInterestLane(const std::string &lane)
{
	std::vector<std::string> result;
	Ptr<NodeSensor> sensor = this->GetNode()->GetObject<NodeSensor>();
	const std::string &currentLane = sensor->getLane();
	std::vector<std::string>::const_iterator it;
	std::vector<std::string>::const_iterator it2;
	const std::vector<std::string> &route = sensor->getNavigationRoute();
	//找出当前路段在导航路线中的位置
	it = std::find(route.begin(), route.end(), currentLane);
	//判断lane是否对未来路段感兴趣
	it2 = std::find(it, route.end(), lane);
	return (it2 != route.end());
}

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */
