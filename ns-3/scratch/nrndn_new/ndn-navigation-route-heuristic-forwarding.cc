/*
 * ndn-navigation-route-heuristic-forwarding.cc
 *
 *  Created on: Jan 14, 2015
 *      Author: chen
 */

#include "ndn-navigation-route-heuristic-forwarding.h"
#include "NodeSensor.h"
#include "nrHeader.h"
#include "nrndn-Neighbors.h"
#include "ndn-pit-entry-nrimpl.h"
#include "nrUtils.h"
#include "nrConsumer.h"

#include "ns3/core-module.h"
#include "ns3/ptr.h"
#include "ns3/ndn-interest.h"
#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/node.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/application.h"

#include <algorithm> // std::find

NS_LOG_COMPONENT_DEFINE("ndn.fw.NavigationRouteHeuristic");

namespace ns3
{
namespace ndn
{
namespace fw
{
namespace nrndn
{

NS_OBJECT_ENSURE_REGISTERED(NavigationRouteHeuristic);

TypeId NavigationRouteHeuristic::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::ndn::fw::nrndn::NavigationRouteHeuristic")
							.SetGroupName("Ndn")
							.SetParent<GreenYellowRed>()
							.AddConstructor<NavigationRouteHeuristic>()
							.AddAttribute("HelloInterval", "HELLO messages emission interval.",
										  TimeValue(Seconds(5)),
										  MakeTimeAccessor(&NavigationRouteHeuristic::HelloInterval),
										  MakeTimeChecker())
							.AddAttribute("AllowedHelloLoss", "Number of hello messages which may be loss for valid link.",
										  UintegerValue(2),
										  MakeUintegerAccessor(&NavigationRouteHeuristic::AllowedHelloLoss),
										  MakeUintegerChecker<uint32_t>())

							.AddAttribute("gap", "the time gap between interested nodes and disinterested nodes for sending a data packet.",
										  UintegerValue(20),
										  MakeUintegerAccessor(&NavigationRouteHeuristic::m_gap),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("UniformRv", "Access to the underlying UniformRandomVariable",
										  StringValue("ns3::UniformRandomVariable"),
										  MakePointerAccessor(&NavigationRouteHeuristic::m_uniformRandomVariable),
										  MakePointerChecker<UniformRandomVariable>())
							.AddAttribute("HelloLogEnable", "the switch which can turn on the log on Functions about hello messages",
										  BooleanValue(true),
										  MakeBooleanAccessor(&NavigationRouteHeuristic::m_HelloLogEnable),
										  MakeBooleanChecker())
							.AddAttribute("NoFwStop", "When the PIT covers the nodes behind, no broadcast stop message",
										  BooleanValue(false),
										  MakeBooleanAccessor(&NavigationRouteHeuristic::NoFwStop),
										  MakeBooleanChecker())
							.AddAttribute("TTLMax", "This value indicate that when a data is received by disinterested node, the max hop count it should be forwarded",
										  UintegerValue(3),
										  MakeUintegerAccessor(&NavigationRouteHeuristic::m_TTLMax),
										  MakeUintegerChecker<uint32_t>());
	return tid;
}

NavigationRouteHeuristic::NavigationRouteHeuristic() : HelloInterval(Seconds(1)),
													   AllowedHelloLoss(2),
													   m_htimer(Timer::CANCEL_ON_DESTROY),
													   m_timeSlot(Seconds(0.05)),
													   m_CacheSize(5000), // Cache size can not change. Because if you change the size, the m_interestNonceSeen and m_dataNonceSeen also need to change. It is really unnecessary
													   m_interestNonceSeen(m_CacheSize),
													   m_dataSignatureSeen(m_CacheSize),
													   m_nb(HelloInterval),
													   m_preNB(HelloInterval),
													   m_running(false),
													   m_runningCounter(0),
													   m_HelloLogEnable(true),
													   m_gap(20),
													   m_TTLMax(3),
													   NoFwStop(false),
													   m_sendInterestTime(0),
													   m_sendDataTime(0),
													   m_rand(0, std::numeric_limits<uint32_t>::max())
{
	m_htimer.SetFunction(&NavigationRouteHeuristic::HelloTimerExpire, this);
	m_nb.SetCallback(MakeCallback(&NavigationRouteHeuristic::FindBreaksLinkToNextHop, this));
}

NavigationRouteHeuristic::~NavigationRouteHeuristic()
{
}

void NavigationRouteHeuristic::Start()
{
	NS_LOG_FUNCTION(this);
	if (!m_runningCounter)
	{
		m_running = true;
		m_offset = MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100));
		m_htimer.Schedule(m_offset);
		m_nb.ScheduleTimer();
	}
	m_runningCounter++;
}

void NavigationRouteHeuristic::Stop()
{
	NS_LOG_FUNCTION(this);
	if (m_runningCounter)
		m_runningCounter--;
	else
		return;

	if (m_runningCounter)
		return;
	m_running = false;
	m_htimer.Cancel();
	m_nb.CancelTimer();
}

void NavigationRouteHeuristic::WillSatisfyPendingInterest(
	Ptr<Face> inFace, Ptr<pit::Entry> pitEntry)
{
	NS_LOG_FUNCTION(this);
	NS_LOG_UNCOND(this << " is in unused function");
}

bool NavigationRouteHeuristic::DoPropagateInterest(
	Ptr<Face> inFace, Ptr<const Interest> interest,
	Ptr<pit::Entry> pitEntry)
{
	NS_LOG_FUNCTION(this);
	NS_LOG_UNCOND(this << " is in unused function");
	NS_ASSERT_MSG(m_pit != 0, "PIT should be aggregated with forwarding strategy");

	return true;
}

void NavigationRouteHeuristic::WillEraseTimedOutPendingInterest(
	Ptr<pit::Entry> pitEntry)
{
	NS_LOG_FUNCTION(this);
	NS_LOG_UNCOND(this << " is in unused function");
}

void NavigationRouteHeuristic::AddFace(Ptr<Face> face)
{
	//every time face is added to NDN stack?
	NS_LOG_FUNCTION(this);
	if (Face::APPLICATION == face->GetFlags())
	{
		NS_LOG_DEBUG("Node " << m_node->GetId() << " add application face " << face->GetId());
		m_inFaceList.push_back(face);
	}
	else
	{
		NS_LOG_DEBUG("Node " << m_node->GetId() << " add NOT application face " << face->GetId());
		m_outFaceList.push_back(face);
	}
}

void NavigationRouteHeuristic::RemoveFace(Ptr<Face> face)
{
	NS_LOG_FUNCTION(this);
	if (Face::APPLICATION == face->GetFlags())
	{
		NS_LOG_DEBUG("Node " << m_node->GetId() << " remove application face " << face->GetId());
		m_inFaceList.erase(find(m_inFaceList.begin(), m_inFaceList.end(), face));
	}
	else
	{
		NS_LOG_DEBUG("Node " << m_node->GetId() << " remove NOT application face " << face->GetId());
		m_outFaceList.erase(find(m_outFaceList.begin(), m_outFaceList.end(), face));
	}
}
void NavigationRouteHeuristic::DidReceiveValidNack(
	Ptr<Face> incomingFace, uint32_t nackCode, Ptr<const Interest> nack,
	Ptr<pit::Entry> pitEntry)
{
	NS_LOG_FUNCTION(this);
	NS_LOG_UNCOND(this << " is in unused function");
}

//车辆获取转发优先级列表
std::vector<uint32_t> NavigationRouteHeuristic::VehicleGetPriorityListOfInterest()
{
	std::vector<uint32_t> PriorityList;
	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();
	cout << "(forwarding.cc-VehicleGetPriorityListOfInterest) 当前节点为 " << m_node->GetId() << " 时间为 " << Simulator::Now().GetSeconds() << endl;

	std::multimap<double, uint32_t, std::greater<double>> sortlistRSU;
	std::multimap<double, uint32_t, std::greater<double>> sortlistVehicle;

	// step 1. 寻找位于导航路线前方的一跳邻居列表,m_nb为邻居列表
	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb;

	for (nb = m_nb.getNb().begin(); nb != m_nb.getNb().end(); ++nb)
	{
		//判断车辆与RSU的位置关系
		if (nb->first >= numsofvehicles)
		{
			std::pair<bool, double> result = m_sensor->VehicleGetDistanceWithRSU(nb->second.m_x, nb->second.m_y, nb->first);
			//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			if (result.first && result.second > 0)
			{
				sortlistRSU.insert(std::pair<double, uint32_t>(result.second, nb->first));
			}
		}
		//判断车辆与其他车辆的位置关系
		else
		{
			std::pair<bool, double> result = m_sensor->VehicleGetDistanceWithVehicle(nb->second.m_x, nb->second.m_y);
			//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			if (result.first && result.second > 0)
			{
				sortlistVehicle.insert(std::pair<double, uint32_t>(result.second, nb->first));
			}
		}
	}
	cout << endl
		 << "加入RSU：";
	// step 2. Sort By Distance Descending
	std::multimap<double, uint32_t>::iterator it;
	// 加入RSU
	for (it = sortlistRSU.begin(); it != sortlistRSU.end(); ++it)
	{
		PriorityList.push_back(it->second);
		cout << " " << it->second;
	}
	cout << endl
		 << "加入普通车辆：";
	// 加入普通车辆
	for (it = sortlistVehicle.begin(); it != sortlistVehicle.end(); ++it)
	{
		PriorityList.push_back(it->second);
		cout << " " << it->second;
	}
	cout << endl;
	return PriorityList;
}

/*
 * 2017.12.27
 * lane为兴趣包下一行驶路段
 */
std::vector<uint32_t> NavigationRouteHeuristic::RSUGetPriorityListOfInterest(const std::string lane)
{
	std::vector<uint32_t> PriorityList;
	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();
	//cout<<"(forwarding.cc-RSUGetPriorityListOfInterest) 当前节点为 "<<m_node->GetId()<<" 时间为 "<<Simulator::Now().GetSeconds()<<endl;
	std::pair<std::string, std::string> junctions = m_sensor->GetLaneJunction(lane);
	//cout<<"(forwarding.cc-RSUGetPriorityListOfInterest) 道路的起点为 "<<junctions.first<<" 终点为 "<<junctions.second<<endl;
	std::multimap<double, uint32_t, std::greater<double>> sortlistVehicle;
	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb;

	for (nb = m_nb.getNb().begin(); nb != m_nb.getNb().end(); ++nb)
	{
		//判断RSU与RSU的位置关系
		if (nb->first >= numsofvehicles)
		{
			//获取另一RSU所在的交点id
			std::string junction = m_sensor->RSUGetJunctionId(nb->first);
			//cout<<endl<<"(forwarding.cc-RSUGetPriorityListOfInterest) RSU所在的交点为 "<<junction<<endl;
			if (junction == junctions.second)
			{
				PriorityList.push_back(nb->first);
				cout << "(forwarding.cc-RSUGetPriorityListOfInterest) 加入RSU " << nb->first << endl;
			}
		}
		//判断RSU与其他车辆的位置关系
		else
		{
			//车辆位于兴趣包下一行驶路段
			if (lane == nb->second.m_lane)
			{
				std::pair<bool, double> result = m_sensor->RSUGetDistanceWithVehicle(m_node->GetId(), nb->second.m_x, nb->second.m_y);
				//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
				if (result.first && result.second > 0)
				{
					sortlistVehicle.insert(std::pair<double, uint32_t>(result.second, nb->first));
				}
			}
		}
	}
	cout << endl
		 << "加入普通车辆：";
	for (std::multimap<double, uint32_t>::iterator it = sortlistVehicle.begin(); it != sortlistVehicle.end(); ++it)
	{
		PriorityList.push_back(it->second);
		cout << " " << it->second;
	}
	cout << endl;
	return PriorityList;
}

void NavigationRouteHeuristic::OnInterest(Ptr<Face> face,
										  Ptr<Interest> interest)
{

	if (!m_running)
		return;

	//2017.12.12 added by sy
	//分情况处理普通车辆和RSU
	const std::string &currentType = m_sensor->getType();
	if (currentType == "RSU")
	{
		OnInterest_RSU(face, interest);
	}
	else if (currentType == "DEFAULT_VEHTYPE" || currentType == "CarA")
	{
		OnInterest_Car(face, interest);
	}
	else
	{
		NS_ASSERT_MSG(false, "车辆类型出错");
	}
}

void NavigationRouteHeuristic::OnInterest_Car(Ptr<Face> face, Ptr<Interest> interest)
{
	if (!m_running)
		return;

	if (Face::APPLICATION == face->GetFlags())
	{
		//cout << "(forwarding.cc-OnInterest_Car)该兴趣包来自应用层。当前节点为 "<<m_node->GetId() <<endl;
		NS_LOG_DEBUG("Get interest packet from APPLICATION");
		// 1.Set the payload
		interest->SetPayload(GetNrPayload(HeaderHelper::INTEREST_NDNSIM, interest->GetPayload(), 999999999));

		std::string routes = interest->GetRoutes();
		const string &localLane = m_sensor->getLane();
		std::size_t found = routes.find(localLane);
		std::string newroutes = routes.substr(found);
		interest->SetRoutes(newroutes);
		//added by sy
		ndn::nrndn::nrHeader nrheader;
		interest->GetPayload()->PeekHeader(nrheader);
		uint32_t nodeId = nrheader.getSourceId();

		// 2017.12.23 added by sy
		const std::vector<uint32_t> &pri = nrheader.getPriorityList();
		if (pri.empty())
		{
			//cout<<"(forwarding.cc-OnInterest_Car) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<m_node->GetId()<<"准备缓存自身的兴趣包 "<<interest->GetNonce()<<endl;
			CachingInterestPacket(interest->GetNonce(), interest);
			return;
		}

		// 2. record the Interest Packet
		m_interestNonceSeen.Put(interest->GetNonce(), true);

		// 3. Then forward the interest packet directly
		Simulator::Schedule(MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100)),
							&NavigationRouteHeuristic::SendInterestPacket, this, interest);

		//cout<<"(forwarding.cc-OnInterest_Car)来自应用层的兴趣包处理完毕。源节点 "<<nodeId<<endl;
		return;
	}

	if (HELLO_MESSAGE == interest->GetScope())
	{
		ProcessHello(interest);
		return;
	}

	Ptr<const Packet> nrPayload = interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t nodeId = nrheader.getSourceId();
	uint32_t seq = interest->GetNonce();
	uint32_t myNodeId = m_node->GetId();
	uint32_t forwardId = nrheader.getForwardId();

	if (DELETE_MESSAGE == interest->GetScope())
	{
		//cout<<"(forwarding.cc-OnInterest_Car) 车辆收到了删除包 "<<seq<<endl;
	}

	//cout<<endl<<"(forwarding.cc-OnInterest_Car)At Time "<<Simulator::Now().GetSeconds()<<" 当前车辆Id为 "<<myNodeId<<",源节点 "<<nodeId<<",转发节点 "<<forwardId<<" seq "<<seq<<endl;

	//If the interest packet has already been sent, do not proceed the packet
	pair<bool, double> msgdirection = packetFromDirection(interest);
	//cout<<"(forwarding.cc-OnInterest_Car) msgdirection first "<<msgdirection.first<<" second "<<msgdirection.second<<endl;

	if (m_interestNonceSeen.Get(interest->GetNonce()))
	{
		//2018.1.16 从缓存中删除兴趣包
		if (msgdirection.first && msgdirection.second > 0)
		{
			m_cs->DeleteInterest(interest->GetNonce());
		}
		//cout<<"(forwarding.cc-OnInterest_Car) 源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",该兴趣包已经被发送, nonce为 "<<interest->GetNonce()<<endl;
		NS_LOG_DEBUG("The interest packet has already been sent, do not proceed the packet of " << interest->GetNonce());
	}

	//获取优先列表
	//cout << "(forwarding.cc-OnInterest_Car) 兴趣包的转发优先级列表为: ";
	const std::vector<uint32_t> &pri = nrheader.getPriorityList();
	/*for (auto it = pri.begin(); it != pri.end(); it++)
	{
		cout<<*it<<" ";
	}
	cout<<endl;*/

	//Deal with the stop message first
	//避免回环
	if (Interest::NACK_LOOP == interest->GetNack())
	{
		ExpireInterestPacketTimer(nodeId, seq);
		return;
	}

	//If it is not a stop message, prepare to forward
	if (!msgdirection.first ||	// from other direction
		msgdirection.second >= 0) // or from front
	{
		NS_LOG_DEBUG("Get interest packet from front or other direction");
		if (!isDuplicatedInterest(nodeId, seq)) // Is new packet
		{
			//sy:对于从前方收到的兴趣包，若是第一次收到的，直接丢弃即可
			NS_LOG_DEBUG("Get interest packet from front or other direction and it is new packet");
			//cout<<"(forwarding.cc-OnInterest_Car) 该兴趣包从前方或其他路线得到，且该兴趣包是新的。源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",转发节点 "<<forwardId<<endl<<endl;
			DropInterestePacket(interest);
		}
		else // Is old packet
		{
			//wsy:对于从前方收到的兴趣包，若之前已经收到过，则没有必要再转发该兴趣包
			NS_LOG_DEBUG("Get interest packet from front or other direction and it is old packet");
			//	cout<<"(forwarding.cc-OnInterest_Car) 该兴趣包从前方或其他路线得到，且该兴趣包是旧的。源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",转发节点 "<<forwardId<<endl<<endl;
			ExpireInterestPacketTimer(nodeId, seq);
		}
	}
	else // it is from nodes behind
	{
		NS_LOG_DEBUG("Get interest packet from nodes behind");
		cout << "(forwarding.cc-OnInterest_Car) 该兴趣包从后方得到。源节点 " << nodeId << ",当前节点 " << myNodeId << ",转发节点 " << forwardId << endl;

		//evaluate whether receiver's id is in sender's priority list
		bool idIsInPriorityList;
		vector<uint32_t>::const_iterator idit;
		idit = find(pri.begin(), pri.end(), m_node->GetId());
		idIsInPriorityList = (idit != pri.end());
		//evaluate end

		if (idIsInPriorityList)
		{
			NS_LOG_DEBUG("Node id is in PriorityList");

			double index = distance(pri.begin(), idit);
			double random = m_uniformRandomVariable->GetInteger(0, 20);
			Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
			//构造转发优先级列表，并判断前方邻居是否为空
			std::vector<uint32_t> newPriorityList = VehicleGetPriorityListOfInterest();
			if (newPriorityList.empty())
			{
				cout << "(forwarding.cc-OnInterest_Car) At Time " << Simulator::Now().GetSeconds() << " 节点 " << myNodeId << "准备缓存兴趣包 " << seq << endl;
				CachingInterestPacket(seq, interest);
				m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval, &NavigationRouteHeuristic::BroadcastStopInterestMessage, this, interest);
			}
			else
			{
				m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval, &NavigationRouteHeuristic::ForwardInterestPacket, this, interest, newPriorityList);
			}
		}
		else
		{
			NS_LOG_DEBUG("Node id is not in PriorityList");
			DropInterestePacket(interest);
		}
	}
}

void NavigationRouteHeuristic::OnDelete_RSU(Ptr<Face> face, Ptr<Interest> deletepacket)
{
	Ptr<const Packet> nrPayload = deletepacket->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t nodeId = nrheader.getSourceId();
	uint32_t seq = deletepacket->GetNonce();
	uint32_t myNodeId = m_node->GetId();
	uint32_t forwardId = nrheader.getForwardId();
	std::string forwardRoute = deletepacket->GetRoutes();

	/*cout << endl
		 << "(forwarding.cc-OnDelete_RSU)At Time " << Simulator::Now().GetSeconds() << " 当前RSUId为 " << myNodeId << ",源节点 " << nodeId << ",转发节点 " << forwardId << " seq " << seq << endl;
	cout << "删除包实际转发路线为 " << forwardRoute << endl;*/

	//If it is not a stop message, prepare to forward:
	pair<bool, double> msgdirection = packetFromDirection(deletepacket);
	cout << "(forwarding.cc-OnDelete_RSU) msgdirection first " << msgdirection.first << " second " << msgdirection.second << endl;

	//If the deletepacket packet has already been sent, do not proceed the packet
	if (m_interestNonceSeen.Get(deletepacket->GetNonce()))
	{
		//2018.1.16 从缓存中删除兴趣包
		if (msgdirection.first && msgdirection.second > 0)
		{
			m_cs->DeleteInterest(deletepacket->GetNonce());
			//cout<<"(forwarding.cc-OnDelete_RSU) 从缓存中删除兴趣包 "<<deletepacket->GetNonce()<<endl;
		}

		cout << "(forwarding.cc-OnDelete_RSU) 源节点 " << nodeId << ",当前节点 " << myNodeId << ",该删除包已经被发送, nonce为 " << deletepacket->GetNonce() << endl;
		NS_LOG_DEBUG("The interest packet has already been sent, do not proceed the packet of " << deletepacket->GetNonce());
		return;
	}

	//获取优先列表
	//cout << "(forwarding.cc-OnDelete_RSU) 删除包的转发优先级列表为: ";
	const std::vector<uint32_t> &pri = nrheader.getPriorityList();
	/*for (auto it = pri.begin(); it != pri.end(); it++)
	{
		cout << *it << " ";
	}
	cout << endl;*/

	//Deal with the stop message first
	//避免回环
	if (Interest::NACK_LOOP == deletepacket->GetNack())
	{
		cout << "(forwarding.cc-OnDelete_RSU) 该删除包为NACK_LOOP。源节点 " << nodeId << endl;
		ExpireInterestPacketTimer(nodeId, seq);
		return;
	}

	if (!msgdirection.first ||	// from other direction
		msgdirection.second >= 0) // or from front
	{
		NS_LOG_DEBUG("Get deletepacket packet from front or other direction");
		if (!isDuplicatedInterest(nodeId, seq)) // Is new packet
		{
			NS_LOG_DEBUG("Get deletepacket packet from front or other direction and it is new packet");
			//cout<<"(forwarding.cc-OnDelete_RSU) 该兴趣包从前方或其他路线得到，且该兴趣包是新的。源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",转发节点 "<<forwardId<<endl<<endl;
			DropInterestePacket(deletepacket);
		}
		else // Is old packet
		{
			//按理来说，不应该进入该函数；因为RSU具有处理兴趣包的最高优先级
			NS_LOG_DEBUG("Get deletepacket packet from front or other direction and it is old packet");
			//cout<<"(forwarding.cc-OnDelete_RSU) 该兴趣包从前方或其他路线得到，且该兴趣包是旧的。源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",转发节点 "<<forwardId<<endl<<endl;
			ExpireInterestPacketTimer(nodeId, seq);
		}
	}
	else // it is from nodes behind
	{
		NS_LOG_DEBUG("Get deletepacket packet from nodes behind");
		cout << "(forwarding.cc-OnDetect_RSU) 该删除包从后方得到。源节点 " << nodeId << ",当前节点 " << myNodeId << ",转发节点 " << forwardId << endl;

		// routes代表车辆的实际转发路线
		vector<std::string> routes;
		SplitString(forwardRoute, routes, " ");

		//删除副PIT列表
		m_nrpit->DeleteSecondPIT(routes[0], nodeId);

		//evaluate whether receiver's id is in sender's priority list
		bool idIsInPriorityList;
		vector<uint32_t>::const_iterator idit;
		idit = find(pri.begin(), pri.end(), m_node->GetId());
		idIsInPriorityList = (idit != pri.end());

		//evaluate end

		if (idIsInPriorityList)
		{
			if (routes.size() <= 1)
			{
				//std::cout << "(forwarding.cc-OnDetect_RSU) 该删除包已经行驶完了所有的转发路线 " << seq << std::endl;
				BroadcastStopInterestMessage(deletepacket);
				return;
			}

			std::string nextroute = routes[1];
			double index = distance(pri.begin(), idit);
			double random = m_uniformRandomVariable->GetInteger(0, 20);
			Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
			//构造转发优先级列表，并判断前方邻居是否为空
			std::vector<uint32_t> newPriorityList = RSUGetPriorityListOfInterest(nextroute);
			forwardRoute = forwardRoute.substr(nextroute.size() + 1);
			deletepacket->SetRoutes(forwardRoute);

			if (newPriorityList.empty())
			{
				cout << "(forwarding.cc-OnDetect_RSU) At Time " << Simulator::Now().GetSeconds() << " 节点 " << myNodeId << "准备缓存删除包 " << seq << endl;
				CachingInterestPacket(seq, deletepacket);
				m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval, &NavigationRouteHeuristic::BroadcastStopInterestMessage, this, deletepacket);
			}
			else
			{
				m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval, &NavigationRouteHeuristic::ForwardInterestPacket, this, deletepacket, newPriorityList);
			}
		}
		else
		{
			cout << "(forwarding.cc-OnDetect_RSU) Node id is not in PriorityList" << endl;
			NS_LOG_DEBUG("Node id is not in PriorityList");
			DropInterestePacket(deletepacket);
		}
	}
}

void NavigationRouteHeuristic::OnInterest_RSU(Ptr<Face> face, Ptr<Interest> interest)
{
	if (Face::APPLICATION == face->GetFlags())
	{
		NS_ASSERT_MSG(false, "RSU收到了自己发送的兴趣包");
		return;
	}

	//收到心跳包
	if (HELLO_MESSAGE == interest->GetScope())
	{
		ProcessHelloRSU(interest);
		return;
	}

	//收到删除包
	if (DELETE_MESSAGE == interest->GetScope())
	{
		//cout<<"(forwarding.cc-OnInterest_RSU) RSU收到了删除包 "<<interest->GetNonce()<<endl;
		OnDelete_RSU(face, interest);
		return;
	}

	Ptr<const Packet> nrPayload = interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t nodeId = nrheader.getSourceId();
	uint32_t seq = interest->GetNonce();
	uint32_t myNodeId = m_node->GetId();
	uint32_t forwardId = nrheader.getForwardId();
	std::string forwardRoute = interest->GetRoutes();

	//cout<<endl<<"(forwarding.cc-OnInterest_RSU)At Time "<<Simulator::Now().GetSeconds()<<" 当前RSUId为 "<<myNodeId<<",源节点 "<<nodeId<<",转发节点 "<<forwardId<<" seq "<<seq<<endl;
	//cout<<"兴趣包实际转发路线为 "<<forwardRoute<<endl;

	//If it is not a stop message, prepare to forward:
	pair<bool, double> msgdirection = packetFromDirection(interest);
	//cout<<"(forwarding.cc-OnInterest_RSU) msgdirection first "<<msgdirection.first<<" second "<<msgdirection.second<<endl;

	//If the interest packet has already been sent, do not proceed the packet
	if (m_interestNonceSeen.Get(interest->GetNonce()))
	{
		//2018.1.16 从缓存中删除兴趣包
		if (msgdirection.first && msgdirection.second > 0)
		{
			m_cs->DeleteInterest(interest->GetNonce());
			//cout<<"(forwarding.cc-OnInterest_RSU) 从缓存中删除兴趣包 "<<interest->GetNonce()<<endl;
		}

		//cout<<"(forwarding.cc-OnInterest_RSU) 源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",该兴趣包已经被发送, nonce为 "<<interest->GetNonce()<<endl;
	}

	//获取优先列表
	//cout << "(forwarding.cc-OnInterest_RSU) 兴趣包的转发优先级列表为: ";
	const std::vector<uint32_t> &pri = nrheader.getPriorityList();
	for (auto it = pri.begin(); it != pri.end(); it++)
	{
		//cout<<*it<<" ";
	}
	//cout<<endl;

	//Deal with the stop message first
	//避免回环
	if (Interest::NACK_LOOP == interest->GetNack())
	{
		//cout<<"(forwarding.cc-OnInterest_RSU) 该兴趣包为NACK_LOOP。源节点 "<<nodeId<<endl;
		ExpireInterestPacketTimer(nodeId, seq);
		return;
	}

	if (!msgdirection.first ||	// from other direction
		msgdirection.second >= 0) // or from front
	{
		NS_LOG_DEBUG("Get interest packet from front or other direction");
		if (!isDuplicatedInterest(nodeId, seq)) // Is new packet
		{
			NS_LOG_DEBUG("Get interest packet from front or other direction and it is new packet");
			//cout<<"(forwarding.cc-OnInterest_RSU) 该兴趣包从前方或其他路线得到，且该兴趣包是新的。源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",转发节点 "<<forwardId<<endl<<endl;
			DropInterestePacket(interest);
		}
		else // Is old packet
		{
			//按理来说，不应该进入该函数；因为RSU具有处理兴趣包的最高优先级
			NS_LOG_DEBUG("Get interest packet from front or other direction and it is old packet");
			//cout<<"(forwarding.cc-OnInterest_RSU) 该兴趣包从前方或其他路线得到，且该兴趣包是旧的。源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",转发节点 "<<forwardId<<endl<<endl;
			ExpireInterestPacketTimer(nodeId, seq);
		}
	}
	else // it is from nodes behind
	{
		NS_LOG_DEBUG("Get interest packet from nodes behind");
		//cout<<"(forwarding.cc-OnInterest_RSU) 该兴趣包从后方得到。源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",转发节点 "<<forwardId<<endl;

		vector<string> interestRoute = ExtractRouteFromName(interest->GetName());

		// routes代表车辆的实际转发路线
		vector<std::string> routes;
		SplitString(forwardRoute, routes, " ");

		//判断来时的路段和下一路段
		if (routes.size() > 1)
		{
			std::vector<std::string>::const_iterator itcurrent = std::find(interestRoute.begin(), interestRoute.end(), routes[0]);
			std::vector<std::string>::const_iterator itnext = std::find(interestRoute.begin(), interestRoute.end(), routes[1]);
			//来时的路段不是兴趣路段，下一路段是兴趣路段
			if (itcurrent == interestRoute.end() && itnext != interestRoute.end())
			{
				std::unordered_set<uint32_t>::iterator italready = alreadyPassed.find(nodeId);
				if (italready != alreadyPassed.end())
				{
					cout << "(forwarding.cc-OnInterest_RSU) 该兴趣包 " << seq << "对应的车辆已经通过了该RSU" << endl;
					BroadcastStopInterestMessage(interest);
					return;
				}
			}
		}

		std::string junction = m_sensor->RSUGetJunctionId(myNodeId);

		bool IsExist = true;
		// Update the PIT here
		m_nrpit->UpdateRSUPit(IsExist, junction, forwardRoute, interestRoute, nodeId);
		// Update finish

		//获取当前及之后的兴趣路线
		vector<string> futureinterest = GetLocalandFutureInterest(routes, interestRoute);

		//m_nrpit->showPit();
		//m_nrpit->showSecondPit();

		//cout<<"(forwarding.cc-OnInterest_RSU) 当前及之后的兴趣路线为 ";
		/*for (uint32_t i = 0; i < futureinterest.size(); i++)
		{
			cout<<futureinterest[i]<<" ";
		}*/
		//cout<<endl;
		DetectDatainCache(futureinterest, routes[0]);

		if (IsExist)
		{
			//cout<<"(forwarding.cc-OnInterest_RSU) 兴趣包的兴趣路线已经存在，不用再转发兴趣包"<<endl;
			BroadcastStopInterestMessage(interest);
			return;
		}

		//evaluate whether receiver's id is in sender's priority list
		bool idIsInPriorityList;
		vector<uint32_t>::const_iterator idit;
		idit = find(pri.begin(), pri.end(), m_node->GetId());
		idIsInPriorityList = (idit != pri.end());

		//evaluate end

		if (idIsInPriorityList)
		{
			if (routes.size() <= 1)
			{
				//std::cout<<"(forwarding.cc-OnInterest_RSU) 该兴趣包已经行驶完了所有的兴趣路线 "<<seq<<std::endl;
				BroadcastStopInterestMessage(interest);
				return;
			}

			std::string nextroute = routes[1];
			std::vector<std::string>::const_iterator it = std::find(interestRoute.begin(), interestRoute.end(), nextroute);

			//下一路段为兴趣路段
			if (it != interestRoute.end())
			{
				//cout<<"(OnInterest_RSU) 兴趣包的下一路段为兴趣路段"<<endl;
				Interest_InInterestRoute(interest, routes);
			}
			else
			{
				//cout<<"(OnInterest_RSU) 兴趣包的下一路段不为兴趣路段"<<endl;
				Interest_NotInInterestRoute(interest, routes);
			}
		}
		else
		{
			//cout<<"(forwarding.cc-OnInterest) Node id is not in PriorityList"<<endl;
			NS_LOG_DEBUG("Node id is not in PriorityList");
			DropInterestePacket(interest);
		}
		//cout<<endl;
	}
}

void NavigationRouteHeuristic::Interest_InInterestRoute(Ptr<Interest> interest, vector<std::string> routes)
{
	Ptr<const Packet> nrPayload = interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t nodeId = nrheader.getSourceId();
	uint32_t seq = interest->GetNonce();
	uint32_t myNodeId = m_node->GetId();

	const std::vector<uint32_t> &pri = nrheader.getPriorityList();

	std::string forwardRoute = interest->GetRoutes();
	std::string nextroute = routes[1];

	vector<uint32_t>::const_iterator idit;
	idit = find(pri.begin(), pri.end(), m_node->GetId());
	double index = distance(pri.begin(), idit);
	double random = m_uniformRandomVariable->GetInteger(0, 20);
	Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
	//构造转发优先级列表，并判断前方邻居是否为空
	std::vector<uint32_t> newPriorityList = RSUGetPriorityListOfInterest(nextroute);
	if (newPriorityList.empty())
	{
		vector<string> bestroute = GetShortestPath(routes);
		cout << "源节点 " << nodeId << " 的最佳路线为 " << endl;
		for (uint32_t i = 2; i < bestroute.size(); i++)
		{
			cout << bestroute[i] << " ";
		}
		cout << endl;

		//去除兴趣包来时的路段
		forwardRoute = forwardRoute.substr(nextroute.size() + 1);
		cout << "兴趣包实际转发路线为 " << forwardRoute << endl;
		//更新兴趣包的实际转发路线
		interest->SetRoutes(forwardRoute);
		//cout<<"(forwarding.cc-Interest_InInterestRoute) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<myNodeId<<"准备缓存兴趣包 "<<seq<<endl;
		CachingInterestPacket(seq, interest);

		//2018.5.2
		if (bestroute.size() == 0)
		{
			cout << "(forwarding.cc-Interest_InInterestRoute) 不存在最短路线" << endl;
			m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval, &NavigationRouteHeuristic::BroadcastStopInterestMessage, this, interest);
			return;
		}

		//查看最佳路线的转发优先级列表
		std::vector<uint32_t> anotherNewPriorityList = RSUGetPriorityListOfInterest(bestroute[2]);

		if (anotherNewPriorityList.empty())
		{
			cout << "(forwarding.cc-Interest_InInterestRoute) 重新选择的路段也没有车辆" << endl;
			m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval, &NavigationRouteHeuristic::BroadcastStopInterestMessage, this, interest);
		}
		else
		{
			std::unordered_map<uint32_t, std::string>::iterator itnode = nodeWithRoutes.find(nodeId);
			if (itnode != nodeWithRoutes.end())
			{
				cout << "该节点 " << nodeId << " 已经存在于待删除列表中" << endl;
				m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval, &NavigationRouteHeuristic::BroadcastStopInterestMessage, this, interest);
			}
			else
			{
				//重新生成一个兴趣包
				//1.copy the interest packet
				Ptr<Interest> newinterest = Create<Interest>(*interest);
				string newforwardRoute;
				//更新兴趣包的实际转发路线
				for (uint32_t i = 2; i < bestroute.size(); i++)
				{
					newforwardRoute += bestroute[i] + " ";
				}

				//将节点和借路路线添加至待删除列表中
				nodeWithRoutes[nodeId] = newforwardRoute;
				for (itnode = nodeWithRoutes.begin(); itnode != nodeWithRoutes.end(); ++itnode)
				{
					cout << "(" << itnode->first << " " << itnode->second << ")";
				}
				cout << endl;

				//将后续路线添加至转发路线中
				for (uint32_t i = 2; i < routes.size(); i++)
				{
					newforwardRoute += routes[i] + " ";
				}
				cout << "当前节点 " << myNodeId << " 源节点 " << nodeId << " 重新选择后的实际转发路线为 " << newforwardRoute << endl;
				newinterest->SetRoutes(newforwardRoute);
				m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval, &NavigationRouteHeuristic::ForwardInterestPacket, this, newinterest, anotherNewPriorityList);
			}
		}
	}
	else
	{
		forwardRoute = forwardRoute.substr(nextroute.size() + 1);
		//cout<<"兴趣包实际转发路线为 "<<forwardRoute<<endl;
		//更新兴趣包的实际转发路线
		interest->SetRoutes(forwardRoute);
		m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval, &NavigationRouteHeuristic::ForwardInterestPacket, this, interest, newPriorityList);
	}
}

void NavigationRouteHeuristic::Interest_NotInInterestRoute(Ptr<Interest> interest, vector<std::string> routes)
{
	Ptr<const Packet> nrPayload = interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t nodeId = nrheader.getSourceId();
	uint32_t seq = interest->GetNonce();
	uint32_t myNodeId = m_node->GetId();

	const std::vector<uint32_t> &pri = nrheader.getPriorityList();

	//获取兴趣包的实际转发路线
	std::string forwardRoute = interest->GetRoutes();
	std::string nextroute = routes[1];

	vector<uint32_t>::const_iterator idit;
	idit = find(pri.begin(), pri.end(), m_node->GetId());
	double index = distance(pri.begin(), idit);
	double random = m_uniformRandomVariable->GetInteger(0, 20);
	Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
	//构造转发优先级列表，并判断前方邻居是否为空
	std::vector<uint32_t> newPriorityList = RSUGetPriorityListOfInterest(nextroute);
	forwardRoute = forwardRoute.substr(nextroute.size() + 1);
	interest->SetRoutes(forwardRoute);

	if (newPriorityList.empty())
	{
		//cout<<"(forwarding.cc-Interest_NotInInterestRoute) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<myNodeId<<"准备缓存兴趣包 "<<seq<<endl;
		CachingInterestPacket(seq, interest);
		m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval, &NavigationRouteHeuristic::BroadcastStopInterestMessage, this, interest);
	}
	else
	{
		m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval, &NavigationRouteHeuristic::ForwardInterestPacket, this, interest, newPriorityList);
	}
}

void NavigationRouteHeuristic::DetectDatainCache(vector<string> futureinterest, string currentroute)
{
	//查看缓存中是否有对应的数据包
	std::map<uint32_t, Ptr<const Data>> datacollection = m_cs->GetDataSource(futureinterest);

	if (!datacollection.empty())
	{
		SendDataInCache(datacollection);
	}
}

/*
 * 2017.12 25 added by sy
 * 分割字符串
 */
void NavigationRouteHeuristic::SplitString(const std::string &s, std::vector<std::string> &v, const std::string &c)
{
	std::size_t pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (std::string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));
		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
}

/* 2018.2.23
 * 为兴趣包选择最短路线
 */
vector<string>
NavigationRouteHeuristic::GetShortestPath(vector<string> forwardroutes)
{
	//2018.2.22 为兴趣包重新选择转发路线
	string nextroute = forwardroutes[1];
	//下一条路段的两个交点
	std::pair<std::string, std::string> junctions = m_sensor->GetLaneJunction(nextroute);
	cout << "需要寻找最短路线的两个端点为 " << junctions.first << " " << junctions.second << endl;
	//最短路线集合
	vector<vector<string>> shortroutes;

	string home = getenv("HOME");
	string inputDir = home + "/input";
	string path = inputDir + "/shortestpath.txt";
	ifstream fin(path);
	if (!fin)
	{
		cout << "fail to open the file" << endl;
		NS_ASSERT_MSG(false, "fail to open the file");
	}
	else
	{
		cout << "open the file successfully" << endl;

		string str;
		while (getline(fin, str))
		{
			vector<string> v;
			SplitString(str, v, " ");
			if (v[0] == junctions.first && v[1] == junctions.second)
			{
				//cout<<"(forwarding.cc-OnInterest_RSU) 最短的路线为 "<<endl;
				//cout << str.substr(v[0].size() + v[1].size() + 2) << endl;
				shortroutes.push_back(v);
			}
		}
	}
	fin.close();

	//2018.4.28 判断真实地图中是否存在最短路径
	if (shortroutes.size() == 0)
	{
		vector<string> v;
		return v;
	}

	//判断是否具有多条最短路线
	if (shortroutes.size() > 1)
	{
		//cout<<"(forwarding.cc-GetShortestPath) 具有多个最佳路段"<<endl;

		set<string> originjunction;
		//cout<<"(forwarding.cc-GetShortestPath) 兴趣包下下路段及之后的交点为 "<<endl;

		//获得原路线的交点
		/*
		 * forwardroutes[0]代表当前路段
		 * forwardroutes[1]代表下一路段
		 */
		for (uint32_t i = 2; i < forwardroutes.size(); i++)
		{
			junctions = m_sensor->GetLaneJunction(forwardroutes[i]);
			//cout<<junctions.second<<" ";
			originjunction.insert(junctions.second);
		}
		//	cout<<endl;

		vector<vector<string>> temproutes(shortroutes);

		for (vector<vector<string>>::iterator it = shortroutes.begin(); it != shortroutes.end();)
		{
			set<string> newjunction;
			vector<string> v = *it;
			/*
			 * v的前两项为交点
			 * 最后一条路段的终点不用计算
			 */
			//获得新路段的交点
			//	cout<<"(forwarding.cc-GetShortestPath) 新路段的交点为 "<<endl;
			for (uint32_t j = 2; j < v.size() - 1; j++)
			{
				junctions = m_sensor->GetLaneJunction(v[j]);
				//cout<<junctions.second<<" ";
				newjunction.insert(junctions.second);
			}
			//cout<<endl;

			//判断新旧路段是否有重复交点
			bool HasSameJunction = false;
			for (set<string>::iterator itnew = newjunction.begin(); itnew != newjunction.end(); itnew++)
			{
				set<string>::iterator itorigin = originjunction.find(*itnew);
				if (itorigin != originjunction.end())
				{
					HasSameJunction = true;
					//cout<<"相同的交点为 "<<*itnew<<endl;
					break;
				}
			}

			if (HasSameJunction)
			{
				it++;
			}
			else
			{
				it = shortroutes.erase(it);
			}
		}

		if (shortroutes.size() > 1)
		{
			//2018.5.13 在真实地图中，存在多条最佳路线
			//NS_ASSERT_MSG(false,"具有多个最佳路段");
		}
		else if (shortroutes.size() == 0)
		{
			shortroutes = temproutes;
			//cout<<"无最佳路线，则选择第一个路线"<<endl;
		}
	}
	return shortroutes[0];
}

vector<string>
NavigationRouteHeuristic::GetLocalandFutureInterest(vector<string> forwardroute, vector<string> interestroute)
{
	string currentroute = forwardroute[0];
	vector<string>::iterator it = std::find(interestroute.begin(), interestroute.end(), currentroute);

	vector<string> futureinterest;
	if (it != interestroute.end())
	{
		//cout<<"(forwarding.cc-GetLocalandFutureInterest) 兴趣包来时的路段为兴趣路段"<<endl;
		for (; it != interestroute.end(); it++)
		{
			futureinterest.push_back(*it);
		}
	}
	else
	{
		//cout<<"(forwarding.cc-GetLocalandFutureInterest) 兴趣包来时的路段不是兴趣路段"<<endl;
		uint32_t i = 0;
		for (; i < forwardroute.size(); i++)
		{
			string route = forwardroute[i];
			it = find(interestroute.begin(), interestroute.end(), route);
			if (it != interestroute.end())
			{
				//cout<<"(forwarding.cc-GetLocalandFutureInterest) 在转发路线中找到了兴趣路段"<<endl;
				break;
			}
		}
		//cout<<"(forwarding.cc-GetLocalandFutureInterest) 剩余的兴趣路段为 ";
		for (; it != interestroute.end() && i < forwardroute.size(); it++, i++)
		{
			if (*it == forwardroute[i])
			{
				futureinterest.push_back(*it);
				//cout<<*it<<" ";
			}
			else
			{
				//2018.3.1
				//cout<<"(forwarding.cc-GetLocalandFutureInterest) 兴趣路线与转发路线中的兴趣路线不相符"<<endl;
				futureinterest.clear();
				break;
			}
		}
		//cout<<endl;
		//getchar();;
	}
	return futureinterest;
}

void NavigationRouteHeuristic::OnData(Ptr<Face> face, Ptr<Data> data)
{
	NS_LOG_FUNCTION(this);
	if (!m_running)
		return;

	//2017.12.29 added by sy
	//分情况处理普通车辆和RSU
	const std::string &currentType = m_sensor->getType();
	if (currentType == "RSU")
	{
		OnData_RSU(face, data);
	}
	else if (currentType == "DEFAULT_VEHTYPE" || currentType == "CarA")
	{
		OnData_Car(face, data);
	}
	else
	{
		NS_ASSERT_MSG(false, "车辆类型出错");
	}
}

void NavigationRouteHeuristic::OnData_Car(Ptr<Face> face, Ptr<Data> data)
{
	NS_LOG_FUNCTION(this);
	//收到了来自应用层的数据包
	if (Face::APPLICATION & face->GetFlags())
	{
		// This is the source data from the upper node application (eg, nrProducer) of itself
		NS_LOG_DEBUG("Get data packet from APPLICATION");

		// 1.Set the payload
		Ptr<Packet> payload = GetNrPayload(HeaderHelper::CONTENT_OBJECT_NDNSIM, data->GetPayload(), 999999999, data->GetName());

		data->SetPayload(payload);

		ndn::nrndn::nrHeader nrheader;
		data->GetPayload()->PeekHeader(nrheader);
		const std::vector<uint32_t> &pri = nrheader.getPriorityList();

		//若转发优先级列表为空，缓存数据包
		if (pri.empty())
		{
			cout << "(forwarding.cc-OnData_Car) At Time " << Simulator::Now().GetSeconds() << " 节点 " << m_node->GetId() << "准备缓存自身的数据包" << endl;
			// 2018.1.11 added by sy
			NotifyUpperLayer(data);

			CachingDataPacket(data->GetSignature(), data);

			Simulator::Schedule(
				MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100)),
				&NavigationRouteHeuristic::SendDataPacket, this, data);

			return;
		}

		// 2. record the Data Packet(only record the forwarded packet)
		m_dataSignatureSeen.Put(data->GetSignature(), true);

		// 3. Then forward the data packet directly
		Simulator::Schedule(
			MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100)),
			&NavigationRouteHeuristic::SendDataPacket, this, data);

		// 4. Although it is from itself, include into the receive record
		NotifyUpperLayer(data);

		uint32_t myNodeId = m_node->GetId();
		//cout<<"(forwarding.cc-OnData_Car) 应用层的数据包事件设置成功，源节点 "<<myNodeId<<endl<<endl;
		//getchar();
		return;
	}

	Ptr<const Packet> nrPayload = data->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t nodeId = nrheader.getSourceId();
	uint32_t signature = data->GetSignature();
	uint32_t myNodeId = m_node->GetId();
	uint32_t forwardId = nrheader.getForwardId();

	cout << endl
		 << "(forwarding.cc-OnData_Car) 源节点 " << nodeId << " 转发节点 " << forwardId << " 当前节点 " << myNodeId << " Signature " << data->GetSignature() << endl;

	const std::vector<uint32_t> &pri = nrheader.getPriorityList();
	cout << "(forwarding.cc-OnData_Car) 数据包的转发优先级列表为 ";
	for (uint32_t i = 0; i < pri.size(); i++)
	{
		cout << pri[i] << " ";
	}
	cout << endl;

	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();
	pair<bool, double> msgdirection;

	//获取车辆上一跳节点
	uint32_t remoteId = (forwardId == 999999999) ? nodeId : forwardId;

	if (remoteId >= numsofvehicles)
	{
		msgdirection = m_sensor->VehicleGetDistanceWithRSU(nrheader.getX(), nrheader.getY(), remoteId);
	}
	else
	{
		msgdirection = m_sensor->VehicleGetDistanceWithVehicle(nrheader.getX(), nrheader.getY());
	}

	cout << "(forwarding.cc-OnData_Car) 数据包的方向为 " << msgdirection.first << " " << msgdirection.second << endl;

	//该数据包已经被转发过
	// 车辆应该不会重复转发数据包，但是RSU有可能重复转发数据包
	if (m_dataSignatureSeen.Get(data->GetSignature()))
	{
		//cout<<"该数据包已经被发送"<<endl;
		//2018.1.24 从缓存中删除数据包
		if (msgdirection.first && msgdirection.second < 0)
		{
			m_cs->DeleteData(data->GetSignature());
		}
		NS_LOG_DEBUG("The Data packet has already been sent, do not proceed the packet of " << data->GetSignature());
		// 2018.1.18
		return;
	}

	//Deal with the stop message first. Stop message contains an empty priority list
	if (pri.empty())
	{
		// 2018.1.11
		if (!isDuplicatedData(nodeId, signature) && IsInterestData(data->GetName()))
		{
			// 1.Buffer the data in ContentStore
			ToContentStore(data);
			// 2. Notify upper layer
			NotifyUpperLayer(data);
			//cout<<"(forwarding.cc-OnData_Car) 车辆对该数据包感兴趣"<<endl;
		}
		ExpireDataPacketTimer(nodeId, signature);
		return;
	}

	//If it is not a stop message, prepare to forward:

	if (!msgdirection.first || msgdirection.second <= 0) // 数据包位于其他路段或当前路段后方
	{
		//第一次收到该数据包
		if (!isDuplicatedData(nodeId, signature))
		{
			if (IsInterestData(data->GetName()))
			{
				// 1.Buffer the data in ContentStore
				ToContentStore(data);
				// 2. Notify upper layer
				NotifyUpperLayer(data);
				//cout<<"(forwarding.cc-OnData_Car) 车辆第一次从后方或其他路段收到数据包且对该数据包感兴趣"<<endl;
				return;
			}
			else
			{
				//cout<<"(forwarding.cc-OnData_Car) 车辆第一次从后方或其他路段收到数据包且当前节点对该数据包不感兴趣"<<endl;
				DropDataPacket(data);
				return;
			}
		}
		else // duplicated data
		{
			if (!msgdirection.first)
			{
				//cout<<"(forwarding.cc-OnData_Car) 该数据包从其他路段得到，且为重复数据包，不作处理"<<endl;
				return;
			}
			//cout<<"(forwarding.cc-OnData_Car) 该数据包从后方得到且为重复数据包"<<endl<<endl;
			ExpireDataPacketTimer(nodeId, signature);
			return;
		}
	}
	//数据包来源于当前路段前方
	else
	{
		if (isDuplicatedData(nodeId, signature))
		{
			//cout<<"(forwarding.cc-OnData_Car) 该数据包从前方或其他路段得到，重复，丢弃"<<endl;
			ExpireDataPacketTimer(nodeId, signature);
			return;
		}

		if (!IsInterestData(data->GetName()))
		{
			DropDataPacket(data);
			//cout<<"(forwarding.cc-OnData_Car) 车辆对该数据包不感兴趣"<<endl;
		}
		else
		{
			// 1.Buffer the data in ContentStore
			ToContentStore(data);
			// 2. Notify upper layer
			NotifyUpperLayer(data);
			cout << "(forwarding.cc-OnData_Car) 车辆对该数据包感兴趣" << endl;
		}
		//getchar();

		bool idIsInPriorityList;
		std::vector<uint32_t>::const_iterator priorityListIt;
		//找出当前节点是否在优先级列表中
		int size = pri[0];
		priorityListIt = find(pri.begin() + size + 1, pri.end(), m_node->GetId());
		idIsInPriorityList = (priorityListIt != pri.end());

		if (idIsInPriorityList)
		{
			//cout<<"(forwarding.cc-OnData_Car) 车辆在数据包转发优先级列表中"<<endl;
			double index = distance(pri.begin() + size + 1, priorityListIt);
			index = getRealIndex(index, pri);
			double random = m_uniformRandomVariable->GetInteger(0, 20);
			Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
			std::vector<uint32_t> newPriorityList = VehicleGetPriorityListOfData();
			if (newPriorityList.empty())
			{
				//cout<<"(forwarding.cc-OnData_Car) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<myNodeId<<"准备缓存数据包 "<<signature<<endl;
				std::unordered_set<std::string> lastroutes;
				CachingDataPacket(signature, data);
				m_sendingDataEvent[nodeId][signature] =
					Simulator::Schedule(sendInterval,
										&NavigationRouteHeuristic::BroadcastStopDataMessage, this, data);
			}
			else
			{
				m_sendingDataEvent[nodeId][signature] =
					Simulator::Schedule(sendInterval,
										&NavigationRouteHeuristic::ForwardDataPacket, this, data,
										newPriorityList);
				//cout<<"(forwarding.cc-OnData_Car) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<myNodeId<<"准备发送数据包 "<<signature<<endl;
			}
		}
		else
		{
			NS_LOG_DEBUG("Node id is not in PriorityList");
		}
	}
}

double NavigationRouteHeuristic::getRealIndex(double &index, const std::vector<uint32_t> &pri)
{
	int size = pri[0];
	for (uint32_t i = 1; i <= size - 1; i++)
	{
		if (index >= pri[i])
		{
			index = index - pri[i];
		}
		else
			break;
	}
	cout << "(forwarding.cc-getRealIndex) index is " << index << endl;
	return index;
}

std::unordered_set<std::string> NavigationRouteHeuristic::getAllInterestedRoutes(Ptr<pit::Entry> Will, Ptr<pit::Entry> WillSecond)
{
	Ptr<pit::nrndn::EntryNrImpl> entry;
	std::unordered_set<std::string> allinteresRoutes;
	//获取主PIT中感兴趣的上一跳路段
	if (Will)
	{
		entry = DynamicCast<pit::nrndn::EntryNrImpl>(Will);
		const std::unordered_set<std::string> &interestRoutes = entry->getIncomingnbs();
		for (std::unordered_set<std::string>::const_iterator it = interestRoutes.begin(); it != interestRoutes.end(); ++it)
		{
			allinteresRoutes.insert(*it);
		}
	}

	//获取副PIT中感兴趣的上一跳路段
	if (WillSecond)
	{
		entry = DynamicCast<pit::nrndn::EntryNrImpl>(WillSecond);
		const std::unordered_set<std::string> &interestRoutes = entry->getIncomingnbs();
		for (std::unordered_set<std::string>::const_iterator it = interestRoutes.begin(); it != interestRoutes.end(); ++it)
		{
			allinteresRoutes.insert(*it);
		}
	}

	cout << "(forwarding.cc-getAllInterestedRoutes) 感兴趣的路段为 ";
	for (std::unordered_set<std::string>::iterator itroad = allinteresRoutes.begin(); itroad != allinteresRoutes.end(); itroad++)
	{
		cout << *itroad << " ";
	}
	cout << endl;

	return allinteresRoutes;
}

void NavigationRouteHeuristic::OnData_RSU_RSU(const uint32_t remoteId, Ptr<Data> data)
{
	Ptr<const Packet> nrPayload = data->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t nodeId = nrheader.getSourceId();
	uint32_t signature = data->GetSignature();
	uint32_t myNodeId = m_node->GetId();
	const std::vector<uint32_t> &pri = nrheader.getPriorityList();
	string forwardLane = "";
	pair<bool, double> msgdirection;

	set<string> roadCollection = m_sensor->RSUGetRoadWithRSU(remoteId);
	set<string>::iterator itroad = roadCollection.begin();
	for (; itroad != roadCollection.end(); itroad++)
	{
		msgdirection = m_sensor->RSUGetDistanceWithRSU(remoteId, *itroad);
		if (msgdirection.first && msgdirection.second < 0)
		{
			forwardLane = *itroad;
			cout << "(OnData_RSU_RSU) forwardLane is " << forwardLane << endl;
		}
	}

	if (m_dataSignatureSeen.Get(data->GetSignature()) && forwardLane != "")
	{
		RSUForwarded.setForwardedRoads(m_node->GetId(), signature, forwardLane);
		cout << "数据包 " << signature << " 已经转发过，上一跳路段为 " << forwardLane << endl;

		Ptr<pit::Entry> Will = WillInterestedData(data);
		Ptr<pit::Entry> WillSecond = WillInterestedDataInSecondPit(data);
		std::unordered_set<std::string> allinteresRoutes = getAllInterestedRoutes(Will, WillSecond);

		bool isall = RSUForwarded.IsAllForwarded(m_node->GetId(), signature, allinteresRoutes);

		if (isall)
		{
			m_cs->DeleteData(signature);
			cout << "signature " << signature << " 所有感兴趣路段都被转发过，可从缓存中删除" << endl;
		}
		return;
	}

	//第二次收到数据包
	if (isDuplicatedData(nodeId, signature))
	{
		ExpireDataPacketTimer(nodeId, signature);
		cout << "(forwarding.cc-OnData_RSU_RSU) 重复收到数据包, signature " << signature << endl;
		return;
	}

	Ptr<pit::Entry> Will = WillInterestedData(data);
	Ptr<pit::Entry> WillSecond = WillInterestedDataInSecondPit(data);
	if (!Will && !WillSecond)
	{
		//或者改为广播停止转发数据包
		BroadcastStopDataMessage(data);
		cout << "主、副PIT列表中都没有该数据包对应的表项" << endl;
		return;
	}

	std::unordered_set<std::string> allinteresRoutes = getAllInterestedRoutes(Will, WillSecond);

	NS_ASSERT_MSG(allinteresRoutes.size() != 0, "感兴趣的上一跳路段不该为0");

	bool idIsInPriorityList;
	std::vector<uint32_t>::const_iterator priorityListIt;
	//找出当前节点是否在优先级列表中
	int size = pri[0];
	priorityListIt = find(pri.begin() + size + 1, pri.end(), myNodeId);
	idIsInPriorityList = (priorityListIt != pri.end());

	if (idIsInPriorityList)
	{
		//缓存数据包
		//CachingDataSourcePacket(signature, data);

		//cout<<"(forwarding.cc-OnData_RSU_RSU) RSU在数据包转发优先级列表中"<<endl;
		double index = distance(pri.begin() + size + 1, priorityListIt);
		index = getRealIndex(index, pri);
		double random = m_uniformRandomVariable->GetInteger(0, 20);
		Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
		std::pair<std::vector<uint32_t>, std::unordered_set<std::string>> collection = RSUGetPriorityListOfData(data->GetName(), allinteresRoutes);
		std::vector<uint32_t> newPriorityList = collection.first;
		std::unordered_set<std::string> remainroutes = collection.second;

		//2018.2.8
		//有部分路段未被满足
		if (!remainroutes.empty())
		{
			CachingDataPacket(signature, data);
			cout << "(forwarding.cc-OnData_RSU_RSU) 有部分兴趣路段无车辆，缓存该数据包 " << signature << endl;
		}

		// 2018.1.15
		if (newPriorityList.empty())
		{
			m_sendingDataEvent[nodeId][signature] =
				Simulator::Schedule(sendInterval,
									&NavigationRouteHeuristic::BroadcastStopDataMessage, this, data);
			cout << "(forwarding.cc-OnData_RSU_RSU) 广播停止转发数据包的消息" << endl;
		}
		else
		{
			m_sendingDataEvent[nodeId][signature] =
				Simulator::Schedule(sendInterval,
									&NavigationRouteHeuristic::ForwardDataPacket, this, data,
									newPriorityList);
			cout << "(forwarding.cc-OnData_RSU_RSU) At Time " << Simulator::Now().GetSeconds() << " RSU " << myNodeId << "准备发送数据包 " << signature << endl;
		}
	}
	else
	{
		cout << "(forwarding.cc-OnData_RSU_RSU) Node id " << myNodeId << " is not in PriorityList" << endl;
		NS_LOG_DEBUG("Node id is not in PriorityList");
	}
}

void NavigationRouteHeuristic::OnData_RSU(Ptr<Face> face, Ptr<Data> data)
{
	if (Face::APPLICATION == face->GetFlags())
	{
		NS_ASSERT_MSG(false, "RSU收到了自己发送的数据包");
		return;
	}

	//2018.12.20
	string dataType = data->GetName().get(1).toUri();
	int totalDistance = 0;
	double sourceX = 0.0;
	double sourceY = 0.0;

	if (dataType == "vehicle")
	{
		totalDistance = stringToNum(data->GetName().get(2).toUri());
		sourceX = stringToNum(data->GetName().get(3).toUri());
		sourceY = stringToNum(data->GetName().get(4).toUri());
	}
	else
	{
		sourceX = stringToNum(data->GetName().get(2).toUri());
		sourceY = stringToNum(data->GetName().get(3).toUri());
	}

	Vector localPos = GetObject<MobilityModel>()->GetPosition();
	localPos.z = 0; //Just in case
	Vector remotePos(sourceX, sourceY, 0);
	double currentDistance = CalculateDistance(localPos, remotePos);

	int priority = getPriorityOfData(dataType, currentDistance);

	Ptr<const Packet> nrPayload = data->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t nodeId = nrheader.getSourceId();
	uint32_t signature = data->GetSignature();
	uint32_t myNodeId = m_node->GetId();
	uint32_t forwardId = nrheader.getForwardId();
	std::string forwardLane = nrheader.getLane();

	cout << "(forwarding.cc-OnData_RSU) 源节点 " << nodeId << " 转发节点 " << forwardId << " 当前节点 " << myNodeId << " Signature " << data->GetSignature();
	cout << "dataType " << dataType << " totalDistance " << totalDistance << " sourceX " << sourceX << " sourceY " << sourceY << "Priority " << priority << endl;

	const std::vector<uint32_t> &pri = nrheader.getPriorityList();

	//Deal with the stop message first. Stop message contains an empty priority list
	if (pri.empty())
	{
		ExpireDataPacketTimer(nodeId, signature);
		//cout<<"该数据包停止转发 "<<"signature "<<data->GetSignature()<<endl<<endl;
		return;
	}

	//m_nrpit->showPit();
	//m_nrpit->showSecondPit();

	//If it is not a stop message, prepare to forward:
	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();
	pair<bool, double> msgdirection;

	//获取车辆上一跳节点
	uint32_t remoteId = (forwardId == 999999999) ? nodeId : forwardId;

	if (remoteId == myNodeId)
		return;

	//上一跳转发节点为RSU且不为RSU自身
	if (remoteId >= numsofvehicles)
	{
		OnData_RSU_RSU(remoteId, data);
		return;
	}

	msgdirection = m_sensor->RSUGetDistanceWithVehicle(m_node->GetId(), nrheader.getX(), nrheader.getY());
	cout << "(forwarding.cc-OnData_RSU) 数据包的方向为 " << msgdirection.first << " " << msgdirection.second << endl;

	//2018.12.25 低优先级的数据包不用缓存，因此不用记录
	if ((priority < 2) && m_dataSignatureSeen.Get(data->GetSignature()))
	{
		if (msgdirection.first && msgdirection.second < 0)
		{
			RSUForwarded.setForwardedRoads(m_node->GetId(), signature, forwardLane);

			Ptr<pit::Entry> Will = WillInterestedData(data);
			Ptr<pit::Entry> WillSecond = WillInterestedDataInSecondPit(data);
			std::unordered_set<std::string> allinteresRoutes = getAllInterestedRoutes(Will, WillSecond);

			bool isall = RSUForwarded.IsAllForwarded(m_node->GetId(), signature, allinteresRoutes);

			if (isall)
			{
				m_cs->DeleteData(signature);
				cout << "signature " << signature << " 所有感兴趣路段都被转发过，可从缓存中删除" << endl;
			}
		}
		else
		{
			RSUForwarded.clearAllRoads(m_node->GetId(), signature);
			cout << "signature " << signature << "又从前方或其他路段收到该数据包" << endl;
		}
	}

	if (msgdirection.first && msgdirection.second <= 0) // 数据包位于当前路段后方
	{
		//Just For Test
		if ((dataType == "vehicle") && (currentDistance > totalDistance))
		{
			cout << "(OnData_RSU) currentDistance " << currentDistance << " is beyond totalDistance and data comes behind" << endl;
		}

		//第一次收到该数据包
		if (!isDuplicatedData(nodeId, signature))
		{
			Ptr<pit::Entry> Will = WillInterestedData(data);
			Ptr<pit::Entry> WillSecond = WillInterestedDataInSecondPit(data);
			if (Will || WillSecond)
			{
				// 2018.1.6 added by sy 2018.12.24 remove it
				CachingDataSourcePacket(data->GetSignature(), data);
				cout << "该数据包第一次从后方收到数据包且对该数据包感兴趣" << endl;
				return;
			}
			else
			{
				//cout<<"该数据包第一次从后方或其他路段收到数据包且当前节点对该数据包不感兴趣"<<endl;
				return;
			}
		}
		else // duplicated data
		{
			//cout<<"(forwarding.cc-OnData_RSU) 该数据包从后方得到且为重复数据包"<<endl<<endl;
			ExpireDataPacketTimer(nodeId, signature);
			return;
		}
	}
	//数据包来源于当前路段前方或其他路段
	else
	{
		if ((dataType == "vehicle") && (currentDistance > totalDistance))
		{
			cout << "(OnData_RSU) currentDistance " << currentDistance << " is beyond totalDistance" << endl;
			BroadcastStopDataMessage(data);
			return;
		}

		//缓存数据包
		CachingDataSourcePacket(signature, data);

		if (priority == 0)
		{
			ProcessHighPriorityData(data);
			return;
		}

		Ptr<pit::Entry> Will = WillInterestedData(data);
		Ptr<pit::Entry> WillSecond = WillInterestedDataInSecondPit(data);
		if (!Will && !WillSecond)
		{
			BroadcastStopDataMessage(data);
			cout << "主、副PIT列表中都没有该数据包对应的表项" << endl;
			return;
		}

		std::unordered_set<std::string> allinteresRoutes = getAllInterestedRoutes(Will, WillSecond);

		NS_ASSERT_MSG(allinteresRoutes.size() != 0, "感兴趣的上一跳路段不该为0");

		bool idIsInPriorityList;
		std::vector<uint32_t>::const_iterator priorityListIt;
		int size = pri[0];
		priorityListIt = find(pri.begin() + size + 1, pri.end(), m_node->GetId());
		idIsInPriorityList = (priorityListIt != pri.end());

		if (idIsInPriorityList)
		{
			//cout<<"(forwarding.cc-OnData_RSU) 车辆在数据包转发优先级列表中"<<endl;
			double index = distance(pri.begin() + size + 1, priorityListIt);
			index = getRealIndex(index, pri);
			double random = m_uniformRandomVariable->GetInteger(0, 20);
			Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
			std::pair<std::vector<uint32_t>, std::unordered_set<std::string>> collection = RSUGetPriorityListOfData(data->GetName(), allinteresRoutes);
			std::vector<uint32_t> newPriorityList = collection.first;
			std::unordered_set<std::string> remainroutes = collection.second;

			//2018.2.8:有部分路段未被满足
			if ((priority == 1) && !remainroutes.empty())
			{
				CachingDataPacket(signature, data);
				cout << "(forwarding.cc-OnData_RSU) 有部分兴趣路段无车辆，缓存该数据包 " << signature << endl;
			}

			// 2018.1.15
			if (newPriorityList.empty())
			{
				m_sendingDataEvent[nodeId][signature] =
					Simulator::Schedule(sendInterval,
										&NavigationRouteHeuristic::BroadcastStopDataMessage, this, data);
			}
			else
			{
				m_sendingDataEvent[nodeId][signature] =
					Simulator::Schedule(sendInterval,
										&NavigationRouteHeuristic::ForwardDataPacket, this, data,
										newPriorityList);
				//cout<<"(forwarding.cc-OnData_RSU) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<myNodeId<<"准备发送数据包 "<<signature<<endl;
			}
		}
		else
		{
			NS_LOG_DEBUG("Node id is not in PriorityList");
		}
	}
}

void NavigationRouteHeuristic::ProcessHighPriorityData(Ptr<Data> data)
{
	Ptr<const Packet> nrPayload = data->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t nodeId = nrheader.getSourceId();
	uint32_t signature = data->GetSignature();
	const std::vector<uint32_t> &pri = nrheader.getPriorityList();

	bool idIsInPriorityList;
	std::vector<uint32_t>::const_iterator priorityListIt;
	int size = pri[0];
	priorityListIt = find(pri.begin() + size + 1, pri.end(), m_node->GetId());
	idIsInPriorityList = (priorityListIt != pri.end());

	if (idIsInPriorityList)
	{
		double index = distance(pri.begin() + size + 1, priorityListIt);
		index = getRealIndex(index, pri);
		double random = m_uniformRandomVariable->GetInteger(0, 20);
		Time sendInterval(MilliSeconds(random) + index * m_timeSlot);

		Ptr<pit::Entry> Will = WillInterestedData(data);
		Ptr<pit::Entry> WillSecond = WillInterestedDataInSecondPit(data);
		if (!Will && !WillSecond)
		{
			cout << "(forwarding.cc-ProcessHighPriorityData) RSU " << m_node->GetId() << " 对数据包 " << signature << "不感兴趣" << endl;
		}
		

		std::unordered_set<std::string> allinteresRoutes = getAllInterestedRoutes(Will, WillSecond);
		allinteresRoutes = AddDummyInterestedRoutes(allinteresRoutes);

		if (allinteresRoutes.empty())
		{
			cout << "(forwarding.cc-ProcessHighPriorityData) 所有后方路段都无车辆 " << signature << endl;
			m_sendingDataEvent[nodeId][signature] =
				Simulator::Schedule(sendInterval,
									&NavigationRouteHeuristic::BroadcastStopDataMessage, this, data);
			return;
		}

		NS_ASSERT_MSG(allinteresRoutes.size() != 0, "感兴趣的上一跳路段不该为0");
		std::pair<std::vector<uint32_t>, std::unordered_set<std::string>> collection = RSUGetPriorityListOfData(data->GetName(), allinteresRoutes);
		std::vector<uint32_t> newPriorityList = collection.first;
		std::unordered_set<std::string> remainroutes = collection.second;

		//有部分路段未被满足
		if (!remainroutes.empty())
		{
			CachingDataPacket(signature, data);
			cout << "(forwarding.cc-OnData_RSU) 有部分兴趣路段无车辆，缓存该数据包 " << signature << endl;
		}

		if (newPriorityList.empty())
		{
			m_sendingDataEvent[nodeId][signature] =
				Simulator::Schedule(sendInterval,
									&NavigationRouteHeuristic::BroadcastStopDataMessage, this, data);
		}
		else
		{
			m_sendingDataEvent[nodeId][signature] =
				Simulator::Schedule(sendInterval,
									&NavigationRouteHeuristic::ForwardDataPacket, this, data,
									newPriorityList);
		}
	}
}

unordered_set<string> NavigationRouteHeuristic::AddDummyInterestedRoutes(unordered_set<string> allinteresRoutes)
{
	unordered_set<string> behindRoads = m_sensor->GetBehindLinkingRoads(m_node->GetId());
	if (allinteresRoutes.empty())
	{
		return behindRoads;
	}

	if (behindRoads.empty())
	{
		return allinteresRoutes;
	}

	unordered_set<string> newallinteresRoutes(allinteresRoutes);

	for (set<string>::iterator it = behindRoads.begin(); it != behindRoads.end(); it++)
	{
		unordered_set<string>::iterator itfind = allinteresRoutes.find(*it);
		if (itfind == allinteresRoutes.end())
		{
			cout << "(forwarding.cc-AddDummyInterestedRoutes) 路段 " << *it << "对此数据包不感兴趣" << endl;
			newallinteresRoutes.insert(*it);
		}
	}
	return newallinteresRoutes;
}

double NavigationRouteHeuristic::stringToNum(const string &str)
{
	istringstream iss(str);
	double num;
	iss >> num;
	return num;
}

int NavigationRouteHeuristic::getPriorityOfData(const string &dataType, const double &currentDistance)
{
	double sameDistance = currentDistance / 1000;
	double factor = 0.4;
	double highPriority = 2 / 3;
	double lowPriority = 1 / 3;
	if (dataType == "road")
		factor = 0.6;

	double result = exp(-factor * sameDistance);
	cout << "(getPriorityOfData) the result is " << result << endl;
	if (result <= 1 && result > highPriority)
	{
		return 0;
	}
	else if (result <= highPriority && result > lowPriority)
	{
		return 1;
	}
	else if (result <= lowPriority && result > 0)
	{
		return 2;
	}
}

// 2017.12.25 changed by sy
pair<bool, double>
NavigationRouteHeuristic::packetFromDirection(Ptr<Interest> interest)
{
	NS_LOG_FUNCTION(this);
	Ptr<const Packet> nrPayload = interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t sourceId = nrheader.getSourceId();
	uint32_t forwardId = nrheader.getForwardId();
	//cout<<"(forwarding.cc-packetFromDirection) sourceId "<<sourceId<<" forwardId "<<forwardId<<endl;
	uint32_t remoteId = (forwardId == 999999999) ? sourceId : forwardId;
	const double x = nrheader.getX();
	const double y = nrheader.getY();

	std::string routes = interest->GetRoutes();
	//std::cout<<"(forwarding.cc-packetFromDirection) 兴趣包实际转发路线 "<<routes<<std::endl;
	std::size_t found = routes.find(" ");
	std::string currentroute = routes.substr(0, found);
	//std::cout<<"(forwarding.cc-packetFromDirection) 兴趣包当前所在路段 "<<currentroute<<std::endl;

	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();
	//cout<<"(forwarding.cc-packetFromDirection) 收到兴趣包的位置" << "x: "<<nrheader.getX() << " " <<"y: "<< nrheader.getY() <<endl;
	//getchar();
	const std::string &currentType = m_sensor->getType();
	if (currentType == "RSU")
	{
		if (remoteId >= numsofvehicles)
		{
			std::pair<bool, double> result = m_sensor->RSUGetDistanceWithRSU(remoteId, currentroute);
			return result;
		}
		else
		{
			std::pair<bool, double> result = m_sensor->RSUGetDistanceWithVehicle(m_node->GetId(), x, y);
			return result;
		}
	}
	else
	{
		if (remoteId >= numsofvehicles)
		{
			std::pair<bool, double> result = m_sensor->VehicleGetDistanceWithRSU(x, y, forwardId);
			return result;
		}
		else
		{
			std::pair<bool, double> result = m_sensor->VehicleGetDistanceWithVehicle(x, y);
			return result;
		}
	}
}

bool NavigationRouteHeuristic::isDuplicatedInterest(
	uint32_t id, uint32_t nonce)
{
	NS_LOG_FUNCTION(this);
	if (!m_sendingInterestEvent.count(id))
		return false;
	else
		return m_sendingInterestEvent[id].count(nonce);
}

void NavigationRouteHeuristic::ExpireInterestPacketTimer(uint32_t nodeId, uint32_t seq)
{
	NS_LOG_FUNCTION(this << "ExpireInterestPacketTimer id" << nodeId << "sequence" << seq);
	//1. Find the waiting timer
	if (!isDuplicatedInterest(nodeId, seq))
	{
		return;
	}

	EventId &eventid = m_sendingInterestEvent[nodeId][seq];

	//2. cancel the timer if it is still running
	eventid.Cancel();
}

void NavigationRouteHeuristic::CachingInterestPacket(uint32_t nonce, Ptr<Interest> interest)
{
	bool result = m_cs->AddInterest(nonce, interest);
	if (result)
	{
		if (interest->GetScope() == DELETE_MESSAGE)
			cout << "(forwarding.cc-CachingInterestPacket) At Time " << Simulator::Now().GetSeconds() << " 节点 " << m_node->GetId() << " 已缓存删除包 " << nonce << endl;
		else
		{
			cout << "(forwarding.cc-CachingInterestPacket) At Time " << Simulator::Now().GetSeconds() << " 节点 " << m_node->GetId() << " 已缓存兴趣包 " << nonce << endl;
			//cout<<"(forwarding.cc-CachingInterestPacket) 兴趣包 "<<nonce<<" 的转发路线为 "<<interest->GetRoutes()<<endl;
		}
	}
	else
	{
		cout << "(forwarding.cc-CachingInterestPacket) 该兴趣包未能成功缓存" << endl;
	}
}

/*
 * 2017.12.29
 * added by sy
 */
void NavigationRouteHeuristic::CachingDataPacket(uint32_t signature, Ptr<const Data> data /*,std::unordered_set<std::string> lastroutes*/)
{
	bool result = m_cs->AddData(signature, data);
	if (result)
	{
		//cout << "(forwarding.cc-CachingDataPacket) At Time " << Simulator::Now().GetSeconds() << " 节点 " << m_node->GetId() << " 已缓存数据包" << signature << endl;
	}
}

void NavigationRouteHeuristic::CachingDataSourcePacket(uint32_t signature, Ptr<Data> data)
{
	bool result = m_cs->AddDataSource(signature, data);
	if (result)
	{
		//cout<<"(forwarding.cc-CachingDataSourcePacket) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<m_node->GetId()<<" 已缓存数据包" <<signature<<endl;
	}
}

void NavigationRouteHeuristic::BroadcastStopInterestMessage(Ptr<Interest> src)
{
	if (!m_running)
		return;
	//cout<<"(forwarding.cc-BroadcastStopInterestMessage) 节点 "<<m_node->GetId()<<" 广播停止转发兴趣包的消息 "<<src->GetNonce()<<endl;
	//cout<<"(forwarding.cc-BroadcastStopMessage) 兴趣包的名字为 "<<src->GetName().toUri() <<endl;
	NS_LOG_FUNCTION(this << " broadcast a stop message of " << src->GetName().toUri());
	//1. copy the interest packet
	Ptr<Interest> interest = Create<Interest>(*src);

	//2. set the nack type
	interest->SetNack(Interest::NACK_LOOP);

	//3.Remove the useless payload, save the bandwidth
	Ptr<const Packet> nrPayload = src->GetPayload();
	ndn::nrndn::nrHeader srcheader, dstheader;
	nrPayload->PeekHeader(srcheader);
	dstheader.setSourceId(srcheader.getSourceId());
	//2018.1.16
	dstheader.setForwardId(m_node->GetId());

	double x = m_sensor->getX();
	double y = m_sensor->getY();
	dstheader.setX(x);
	dstheader.setY(y);

	Ptr<Packet> newPayload = Create<Packet>();
	newPayload->AddHeader(dstheader);
	interest->SetPayload(newPayload);

	//4. send the payload
	Simulator::Schedule(MilliSeconds(m_uniformRandomVariable->GetInteger(0, 10)),
						&NavigationRouteHeuristic::SendInterestPacket, this, interest);
}

void NavigationRouteHeuristic::ForwardInterestPacket(Ptr<const Interest> src, std::vector<uint32_t> newPriorityList)
{
	if (!m_running)
		return;
	NS_LOG_FUNCTION(this);
	uint32_t sourceId = 0;
	uint32_t nonce = 0;

	//记录转发的兴趣包
	// 1. record the Interest Packet(only record the forwarded packet)
	m_interestNonceSeen.Put(src->GetNonce(), true);

	//准备兴趣包
	// 2. prepare the interest
	Ptr<const Packet> nrPayload = src->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	double x = m_sensor->getX();
	double y = m_sensor->getY();
	sourceId = nrheader.getSourceId();
	nonce = src->GetNonce();
	// source id do not change
	nrheader.setX(x);
	nrheader.setY(y);
	//2017.6.16
	nrheader.setForwardId(m_node->GetId());
	nrheader.setPriorityList(newPriorityList);
	Ptr<Packet> newPayload = Create<Packet>();
	newPayload->AddHeader(nrheader);

	Ptr<Interest> interest = Create<Interest>(*src);
	interest->SetPayload(newPayload);

	// 3. Send the interest Packet. Already wait, so no schedule
	SendInterestPacket(interest);

	// 4. record the forward times
	// 2017.12.23 added by sy
	// 若源节点与当前节点相同，则不需要记录转发次数
	uint32_t nodeId = m_node->GetId();
	if (nodeId != sourceId)
	{
		if (src->GetScope() == DELETE_MESSAGE)
		{
			ndn::nrndn::nrUtils::IncreaseDeleteForwardCounter(sourceId, nonce);
		}
		else
		{
			ndn::nrndn::nrUtils::IncreaseInterestForwardCounter(sourceId, nonce);
		}
	}
}

bool NavigationRouteHeuristic::PitCoverTheRestOfRoute(
	const vector<string> &remoteRoute)
{
	NS_LOG_FUNCTION(this);
	//获取本节点的导航路线
	const vector<string> &localRoute = m_sensor->getNavigationRoute();

	//获取当前所在的道路
	const string &localLane = m_sensor->getLane();
	vector<string>::const_iterator localRouteIt;
	vector<string>::const_iterator remoteRouteIt;
	localRouteIt = std::find(localRoute.begin(), localRoute.end(), localLane);
	remoteRouteIt = std::find(remoteRoute.begin(), remoteRoute.end(), localLane);

	//The route is in reverse order in Name, example:R1-R2-R3, the name is /R3/R2/R1
	while (localRouteIt != localRoute.end() && remoteRouteIt != remoteRoute.end())
	{
		if (*localRouteIt == *remoteRouteIt)
		{
			++localRouteIt;
			++remoteRouteIt;
		}
		else
			break;
	}
	return (remoteRouteIt == remoteRoute.end());
}

void NavigationRouteHeuristic::DoInitialize(void)
{
	super::DoInitialize();
}

void NavigationRouteHeuristic::DropPacket()
{
	NS_LOG_DEBUG("Drop Packet");
}

void NavigationRouteHeuristic::DropDataPacket(Ptr<Data> data)
{
	NS_LOG_DEBUG("Drop data Packet");
	/*
	 * @Date 2015-03-17 For statistics reason, count the disinterested data
	 * */
	//Choice 1:
	NotifyUpperLayer(data);

	//Choice 2:
	//DropPacket();
}

void NavigationRouteHeuristic::DropInterestePacket(Ptr<Interest> interest)
{
	NS_LOG_DEBUG("Drop interest Packet");
	DropPacket();
}

void NavigationRouteHeuristic::SendInterestPacket(Ptr<Interest> interest)
{
	if (!m_running)
		return;

	if (HELLO_MESSAGE != interest->GetScope() || m_HelloLogEnable)
		NS_LOG_FUNCTION(this);

	// if the node has multiple out Netdevice face, send the interest package to them all
	// makde sure this is a NetDeviceFace!!!!!!!!!!!
	vector<Ptr<Face>>::iterator fit;
	for (fit = m_outFaceList.begin(); fit != m_outFaceList.end(); ++fit)
	{
		(*fit)->SendInterest(interest);
		//if(HELLO_MESSAGE ==interest->GetScope())
		//cout<<"(forwarding.cc-SendInterestPacket) 心跳包"<<m_node->GetId()<<" "<<Simulator::Now().GetSeconds()<<endl;
		//else
		//	cout<<"(forwarding.cc-SendInterestPacket) 兴趣包"<<m_node->GetId()<<" "<<Simulator::Now().GetSeconds()<<endl;
	}
	ndn::nrndn::nrUtils::AggrateInterestPacketSize(interest);
}

void NavigationRouteHeuristic::NotifyNewAggregate()
{
	if (m_sensor == 0)
	{
		m_sensor = GetObject<ndn::nrndn::NodeSensor>();
	}

	if (m_nrpit == 0)
	{
		Ptr<Pit> pit = GetObject<Pit>();
		if (pit)
			m_nrpit = DynamicCast<pit::nrndn::NrPitImpl>(pit);
	}

	if (m_cs == 0)
	{
		Ptr<ContentStore> cs = GetObject<ContentStore>();
		if (cs)
		{
			m_cs = DynamicCast<ns3::ndn::cs::nrndn::NrCsImpl>(cs);
		}
	}

	if (m_node == 0)
	{
		m_node = GetObject<Node>();
	}

	super::NotifyNewAggregate();
}

void NavigationRouteHeuristic::HelloTimerExpire()
{
	if (!m_running)
		return;

	if (m_HelloLogEnable)
		NS_LOG_FUNCTION(this);
	SendHello();

	m_htimer.Cancel();
	Time base(HelloInterval - m_offset);
	m_offset = MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100));
	m_htimer.Schedule(base + m_offset);
}

void NavigationRouteHeuristic::FindBreaksLinkToNextHop(uint32_t BreakLinkNodeId)
{
	NS_LOG_FUNCTION(this);
}

void NavigationRouteHeuristic::SendHello()
{
	if (!m_running)
		return;

	if (m_HelloLogEnable)
		NS_LOG_FUNCTION(this);
	const double &x = m_sensor->getX();
	const double &y = m_sensor->getY();
	const string &LaneName = m_sensor->getLane();

	////getchar();
	//1.setup name
	Ptr<Name> name = ns3::Create<Name>('/' + LaneName);

	//2. setup payload
	Ptr<Packet> newPayload = Create<Packet>();
	ndn::nrndn::nrHeader nrheader;
	nrheader.setX(x);
	nrheader.setY(y);
	nrheader.setSourceId(m_node->GetId());

	newPayload->AddHeader(nrheader);

	//3. setup interest packet
	Ptr<Interest> interest = Create<Interest>(newPayload);
	interest->SetScope(HELLO_MESSAGE); // The flag indicate it is hello message
	interest->SetName(name);		   //interest name is lane;
	interest->SetRoutes(LaneName);	 //2017.12.25 added by sy 2018.4.28 readded by sy
	//4. send the hello message
	SendInterestPacket(interest);
}

void NavigationRouteHeuristic::ProcessHello(Ptr<Interest> interest)
{
	if (!m_running)
	{
		return;
	}

	if (m_HelloLogEnable)
		NS_LOG_DEBUG(this << interest << "\tReceived HELLO packet from " << interest->GetNonce());

	Ptr<const Packet> nrPayload = interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t sourceId = nrheader.getSourceId();
	uint32_t nodeId = m_node->GetId();

	//cout<<"(forwarding.cc-ProcessHello) 当前节点 "<<nodeId<<" 发送心跳包的节点 "<<sourceId<<" At time "<<Simulator::Now().GetSeconds()<<endl;

	string remoteroute = interest->GetName().get(0).toUri();
	m_nb.Update(sourceId, nrheader.getX(), nrheader.getY(), remoteroute, Time(AllowedHelloLoss * HelloInterval));

	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb = m_nb.getNb().begin();

	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();

	// 2017.12.21 获取当前前方邻居数目
	uint32_t nums_car_front = 0;
	// 2018.1.7 获取当前后方邻居数目
	uint32_t nums_car_behind = 0;
	for (; nb != m_nb.getNb().end(); ++nb)
	{
		//跳过自身节点
		if (nb->first == m_node->GetId())
			continue;

		if (nb->first >= numsofvehicles)
		{
			std::pair<bool, double> result = m_sensor->VehicleGetDistanceWithRSU(nb->second.m_x, nb->second.m_y, nb->first);

			//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			if (result.first && result.second > 0)
			{
				nums_car_front += 1;
			}
			else if (result.first && result.second < 0)
			{
				nums_car_behind += 1;
			}
			////getchar();
		}
		else
		{
			std::pair<bool, double> result = m_sensor->VehicleGetDistanceWithVehicle(nb->second.m_x, nb->second.m_y);
			//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			if (result.first && result.second > 0)
			{
				nums_car_front += 1;
			}
			else if (result.first && result.second < 0)
			{
				nums_car_behind += 1;
			}
		}
	}
	//cout<<endl<<"(forwarding.cc-ProcessHello) nums_car_behind "<<nums_car_behind<<endl;

	//前方道路有车辆
	if (nums_car_front > 0)
	{
		//cout<<"(forwarding.cc-ProcessHello) 前方道路有车辆"<<endl;
		//有兴趣包在缓存中
		if (m_cs->GetInterestSize() > 0)
		{
			//cout<<"(forwarding.cc-ProcessHello) 有兴趣包在缓存中"<<endl;
			const string &localLane = m_sensor->getLane();
			//cout<<"(forwarding.cc-ProcessHello) 车辆当前所在路段为 "<<localLane<<endl;
			//获得缓存的兴趣包
			map<uint32_t, Ptr<const Interest>> interestcollection = m_cs->GetInterest(localLane);
			//cout<<"(forwarding.cc-ProcessHello) 获得缓存的兴趣包"<<endl;
			if (!interestcollection.empty())
			{
				double interval = Simulator::Now().GetSeconds() - m_sendInterestTime;
				if (interval < 1)
				{
					cout << "(forwarding.cc-SendInterestInCache) 时间小于一秒，不发送 m_sendInterestTime " << m_sendInterestTime << " 节点 " << m_node->GetId() << endl;
					return;
				}

				m_sendInterestTime = Simulator::Now().GetSeconds();
				SendInterestInCache(interestcollection);
			}
		}
		else
		{
			//cout<<"(forwarding.cc-ProcessHello) 无兴趣包在缓存中"<<endl;
		}
		////getchar();
	}

	// 2018.1.7
	if (nums_car_behind > 0)
	{
		if (m_cs->GetDataSize() > 0)
		{
			map<uint32_t, Ptr<const Data>> datacollection = m_cs->GetData();
			if (!datacollection.empty())
			{
				double interval = Simulator::Now().GetSeconds() - m_sendDataTime;
				if (interval < 1)
				{
					//cout<<"(forwarding.cc-SendDataInCache) 时间小于一秒，不发送 m_sendDataTime "<<m_sendDataTime<<endl;
					return;
				}
				m_sendDataTime = Simulator::Now().GetSeconds();
				SendDataInCache(datacollection);
				//cout<<"(forwarding.cc-ProcessHello) 当前节点为 "<<m_node->GetId()<<" 从缓存中取出数据包"<<endl;
			}

			////getchar();
		}
	}
}

void NavigationRouteHeuristic::notifyUpperOnInterest(uint32_t id)
{
	NodeContainer c = NodeContainer::GetGlobal();
	NodeContainer::Iterator it;
	uint32_t idx = 0;
	for (it = c.Begin(); it != c.End(); ++it)
	{
		Ptr<Application> app = (*it)->GetApplication(ndn::nrndn::nrUtils::appIndex["ns3::ndn::nrndn::nrConsumer"]);
		Ptr<ns3::ndn::nrndn::nrConsumer> consumer = DynamicCast<ns3::ndn::nrndn::nrConsumer>(app);
		NS_ASSERT(consumer);
		if (!consumer->IsActive())
		{ //非活跃节点直接跳过，避免段错误
			idx++;
			continue;
		}
		if (idx == id)
		{
			cout << "(forwarding.cc-notifyUpperOnInterest) idx " << idx << endl;
			consumer->SendPacket();
			break;
		}
		idx++;
	}
}

// 2017.12.21 发送缓存的兴趣包
void NavigationRouteHeuristic::SendInterestInCache(std::map<uint32_t, Ptr<const Interest>> interestcollection)
{
	std::map<uint32_t, Ptr<const Interest>>::iterator it;
	for (it = interestcollection.begin(); it != interestcollection.end(); it++)
	{
		uint32_t nonce = it->first;
		uint32_t nodeId = m_node->GetId();

		//added by sy
		Ptr<const Interest> interest = it->second;
		ndn::nrndn::nrHeader nrheader;
		interest->GetPayload()->PeekHeader(nrheader);
		uint32_t sourceId = nrheader.getSourceId();

		cout << "(forwarding.cc-SendInterestInCache) 兴趣包的nonce " << nonce << " 当前节点 " << nodeId << " 源节点为 " << sourceId << " 当前时间 " << Simulator::Now().GetSeconds() << endl;

		//cout<<"(forwarding.cc-SendInterestInCache) 兴趣包源节点为 "<<sourceId<<" 兴趣包的实际转发路线为 "<<interest->GetRoutes()<<endl;
		std::vector<std::string> routes;
		SplitString(interest->GetRoutes(), routes, " ");
		cout << "(forwarding.cc-SendInterestInCache) 兴趣包转发优先级列表为 " << endl;

		std::vector<uint32_t> newPriorityList;
		if (m_sensor->getType() == "RSU")
			newPriorityList = RSUGetPriorityListOfInterest(routes[0]);
		else
			newPriorityList = VehicleGetPriorityListOfInterest();

		for (uint32_t i = 0; i < newPriorityList.size(); i++)
		{
			cout << newPriorityList[i] << " ";
		}
		cout << endl;
		double random = m_uniformRandomVariable->GetInteger(0, 100);
		Time sendInterval(MilliSeconds(random));
		m_sendingInterestEvent[sourceId][nonce] = Simulator::Schedule(sendInterval, &NavigationRouteHeuristic::ForwardInterestPacket, this, interest, newPriorityList);
	}
}

void NavigationRouteHeuristic::SendDataInCache(std::map<uint32_t, Ptr<const Data>> datacollection)
{
	std::map<uint32_t, Ptr<const Data>>::iterator it;
	for (it = datacollection.begin(); it != datacollection.end(); it++)
	{
		uint32_t signature = it->first;
		uint32_t nodeId = m_node->GetId();
		Ptr<const Data> data = it->second;

		//added by sy
		ndn::nrndn::nrHeader nrheader;
		data->GetPayload()->PeekHeader(nrheader);
		uint32_t sourceId = nrheader.getSourceId();

		std::vector<uint32_t> newPriorityList;

		if (m_sensor->getType() == "RSU")
		{
			Ptr<pit::Entry> Will = WillInterestedData(data);
			Ptr<pit::Entry> WillSecond = WillInterestedDataInSecondPit(data);
			if (!Will && !WillSecond)
			{
				NS_ASSERT_MSG(false, "RSU对该数据包不感兴趣");
			}
			else
			{
				cout << endl
					 << "(forwarding.cc-SendDataInCache) 数据包的signature " << signature << " 当前节点 " << nodeId << " 源节点为 " << sourceId << endl;

				std::unordered_set<std::string> allinteresRoutes = getAllInterestedRoutes(Will, WillSecond);

				NS_ASSERT_MSG(allinteresRoutes.size() != 0, "感兴趣的上一跳路段不该为0");

				//获取该数据包已转发过的上一跳路段
				std::set<std::string> forwardedroutes = RSUForwarded.getForwardedRoads(nodeId, signature);

				std::unordered_set<std::string> newinterestRoutes;
				if (forwardedroutes.empty())
				{
					newinterestRoutes = allinteresRoutes;
					cout << "(forwarding.cc-SendDataInCache) 数据包 " << signature << " 对应的上一跳路段全部未转发过" << endl;
				}
				else
				{
					//从上一跳路段中去除已转发过的路段
					for (std::unordered_set<std::string>::const_iterator itinterest = allinteresRoutes.begin(); itinterest != allinteresRoutes.end(); itinterest++)
					{
						std::set<std::string>::iterator itforward = forwardedroutes.find(*itinterest);
						if (itforward != forwardedroutes.end())
						{
							cout << "(forwarding.cc-SendDataInCache) 数据包 " << signature << " 已转发过的上一跳路段为 " << *itinterest << endl;
							continue;
						}
						newinterestRoutes.insert(*itinterest);
						cout << "(forwarding.cc-SendDataInCache) 数据包 " << signature << " 未转发过的上一跳路段为 " << *itinterest << endl;
					}
				}

				if (newinterestRoutes.empty())
				{
					//2018.2.8 修改后不该进入该函数
					cout << "数据包 " << signature << " 对应的上一跳路段全部转发过" << endl;
					continue;
				}

				std::pair<std::vector<uint32_t>, std::unordered_set<std::string>> collection = RSUGetPriorityListOfData(data->GetName(), newinterestRoutes);
				newPriorityList = collection.first;
				std::unordered_set<std::string> remainroutes = collection.second;

				if (newPriorityList.empty())
				{
					//cout<<"数据包 "<<signature<<" 对应的未转发过的上一跳路段为空"<<endl;
					//从m_datasource中得到的数据包，对应的转发优先级列表为空，需要存储到m_data中
					CachingDataPacket(signature, data);
					continue;
				}
			}
		}
		else
		{
			newPriorityList = VehicleGetPriorityListOfData();
		}

		double random = m_uniformRandomVariable->GetInteger(0, 100);
		Time sendInterval(MilliSeconds(random));
		m_sendingDataEvent[sourceId][signature] = Simulator::Schedule(sendInterval, &NavigationRouteHeuristic::ForwardDataPacket, this, data, newPriorityList);
	}
}

void NavigationRouteHeuristic::ProcessHelloRSU(Ptr<Interest> interest)
{
	if (!m_running)
	{
		return;
	}
	if (m_HelloLogEnable)
		NS_LOG_DEBUG(this << interest << "\tReceived HELLO packet from " << interest->GetNonce());

	Ptr<const Packet> nrPayload = interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t sourceId = nrheader.getSourceId();
	uint32_t nodeId = m_node->GetId();
	//const std::string& lane = m_sensor->getLane();
	//获取心跳包所在路段
	string remoteroute = interest->GetName().get(0).toUri();
	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();

	//cout<<"(forwarding.cc-ProcessHelloRSU) 当前RSU "<<nodeId<<" 发送心跳包的节点 "<<sourceId<<" At time "<<Simulator::Now().GetSeconds()<<endl;
	//cout<<"(forwarding.cc-ProcessHelloRSU) 心跳包当前所在路段为 "<<remoteroute<<endl;

	std::string junctionid = m_sensor->RSUGetJunctionId(nodeId);
	//cout<<"(forwarding.cc-ProcessHelloRSU) 当前RSU的交点ID为 "<<junctionid<<endl;

	//更新邻居列表
	m_nb.Update(sourceId, nrheader.getX(), nrheader.getY(), remoteroute, Time(AllowedHelloLoss * HelloInterval));

	if (sourceId < numsofvehicles)
	{
		//删除超车的PIT
		deleteOverTake(interest, sourceId, remoteroute);
	}

	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb = m_nb.getNb().begin();
	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator prenb = m_preNB.getNb().begin();

	// 2017.12.27 added by sy
	// routes_front代表有车辆/RSU的前方路段集合
	std::map<std::string, uint32_t> routes_front;
	std::map<std::string, uint32_t>::iterator itroutes_front;

	//2018.1.8 added by sy
	// routes_behind代表有车辆/RSU的后方路段集合
	std::map<std::string, uint32_t> routes_behind;
	std::map<std::string, uint32_t>::iterator itroutes_behind;

	for (; nb != m_nb.getNb().end(); ++nb)
	{
		if (nb->first >= numsofvehicles)
		{
			//获取RSU-RSU组成的道路集合
			set<string> roadCollection = m_sensor->RSUGetRoadWithRSU(nb->first);
			set<string>::iterator itroad = roadCollection.begin();
			for (; itroad != roadCollection.end(); itroad++)
			{
				std::pair<bool, double> result = m_sensor->RSUGetDistanceWithRSU(nb->first, *itroad);
				//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
				if (result.first && result.second > 0)
				{
					//cout<<"(forwarding.cc-ProcessHelloRSU) 路段 "<<*itroad<<" 前方有RSU"<<endl;
					itroutes_front = routes_front.find(*itroad);
					if (itroutes_front != routes_front.end())
					{
						(itroutes_front->second)++;
					}
					else
					{
						routes_front[*itroad] = 1;
					}
				}
				else if (result.first && result.second < 0)
				{
					//cout<<"(forwarding.cc-ProcessHelloRSU) 路段 "<<*itroad<<" 后方有RSU"<<endl;
					itroutes_behind = routes_behind.find(*itroad);
					if (itroutes_behind != routes_behind.end())
					{
						(itroutes_behind->second)++;
					}
					else
					{
						routes_behind[*itroad] = 1;
					}
				}
			}
		}
		else
		{
			std::pair<bool, double> result = m_sensor->RSUGetDistanceWithVehicle(m_node->GetId(), nb->second.m_x, nb->second.m_y);
			//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			if (result.first && result.second > 0)
			{
				//cout<<"(forwarding.cc-ProcessHelloRSU) 路段 "<<nb->second.m_lane<<" 前方有车辆"<<endl;
				itroutes_front = routes_front.find(nb->second.m_lane);
				if (itroutes_front != routes_front.end())
				{
					(itroutes_front->second)++;
				}
				else
				{
					routes_front[nb->second.m_lane] = 1;
				}
			}
			else if (result.first && result.second < 0)
			{
				//cout<<"(forwarding.cc-ProcessHelloRSU) 路段 "<<nb->second.m_lane<<" 后方有车辆"<<endl;
				itroutes_behind = routes_behind.find(nb->second.m_lane);
				if (itroutes_behind != routes_behind.end())
				{
					(itroutes_behind->second)++;
				}
				else
				{
					routes_behind[nb->second.m_lane] = 1;
				}
			}
		}
	}
	//cout<<endl;
	//getchar();
	int front_change_mode = 0;
	int behind_change_mode = 0;

	if (routes_front_pre.size() < routes_front.size())
	{
		//前方邻居增加
		front_change_mode = 2;
	}
	else if (routes_front_pre.size() > routes_front.size())
	{
		//前方邻居减少
		front_change_mode = 1;
	}
	else
	{
		bool nbchangeroad = false;
		bool nbchangecar = false;
		std::map<std::string, uint32_t>::iterator itroutes_front_pre = routes_front_pre.begin();
		for (; itroutes_front_pre != routes_front_pre.end(); ++itroutes_front_pre)
		{
			itroutes_front = routes_front.find(itroutes_front_pre->first);
			if (itroutes_front == routes_front.end())
			{
				nbchangeroad = true;
				break;
			}
			else
			{
				uint32_t numofNow = itroutes_front->second;
				uint32_t numofBefore = itroutes_front_pre->second;
				if (numofNow > numofBefore)
				{
					nbchangecar = true;
					break;
				}
			}
		}
		//邻居数目不变，但是有增加
		if (nbchangeroad || nbchangecar)
		{
			front_change_mode = 3;
		}
	}

	//cout<<"(forwarding.cc-ProcessHelloRSU) front_change_mode "<<front_change_mode<<endl;

	if (routes_behind_pre.size() < routes_behind.size())
	{
		behind_change_mode = 2;
	}
	else if (routes_behind_pre.size() > routes_behind.size())
	{
		behind_change_mode = 1;
	}
	else
	{
		bool nbchangeroad = false;
		bool nbchangecar = false;
		std::map<std::string, uint32_t>::iterator itroutes_behind_pre = routes_behind_pre.begin();
		for (; itroutes_behind_pre != routes_behind_pre.end(); ++itroutes_behind_pre)
		{
			itroutes_behind = routes_behind.find(itroutes_behind_pre->first);
			if (itroutes_behind == routes_behind.end())
			{
				nbchangeroad = true;
				break;
			}
			else
			{
				uint32_t numofNow = itroutes_behind->second;
				uint32_t numofBefore = itroutes_behind_pre->second;
				if (numofNow > numofBefore)
				{
					nbchangecar = true;
					break;
				}
			}
		}
		//邻居数目不变，但是有增加
		if (nbchangeroad || nbchangecar)
			behind_change_mode = 3;
	}

	if (front_change_mode > 1 && m_cs->GetInterestSize() > 0)
	{
		cout << "(forwarding.cc-ProcessHelloRSU) front_change_mode " << front_change_mode << endl;
		for (itroutes_front = routes_front.begin(); itroutes_front != routes_front.end(); itroutes_front++)
		{
			map<uint32_t, Ptr<const Interest>> interestcollection = m_cs->GetInterest(itroutes_front->first);
			if (!interestcollection.empty())
			{
				cout << "(forwarding.cc-ProcessHelloRSU) 缓存的兴趣包对应的路段 " << itroutes_front->first << " 有车辆。当前节点为 " << nodeId << endl;
				SendInterestInCache(interestcollection);
			}
			//getchar();
		}
	}
	else
	{
		//cout<<"(forwarding.cc-ProcessHelloRSU) routes_front.size "<<routes_front.size()<<endl;
		//cout<<"(forwarding.cc-ProcessHelloRSU) RSU前方无路段存在车辆或者兴趣包缓存为空"<<endl;
	}

	if (behind_change_mode > 1 && m_cs->GetDataSize() > 0)
	{
		cout << "(forwarding.cc-ProcessHelloRSU) behind_change_mode " << behind_change_mode << endl;
		cout << "(forwarding.cc-ProcessHelloRSU) 当前节点 " << nodeId << " 发送心跳包的节点 " << sourceId << " At time " << Simulator::Now().GetSeconds() << endl;

		//cout<<"(forwarding.cc-ProcessHelloRSU) 有车辆的路段为 "<<endl;
		std::unordered_set<std::string> routesCollection;
		for (std::map<std::string, uint32_t>::iterator it = routes_behind.begin(); it != routes_behind.end(); it++)
		{
			routesCollection.insert(it->first);
		}
		//cout << endl;
		//cout<<"(forwarding.cc-ProcessHelloRSU) routes_behind的大小为 "<<routes_behind.size()<<endl;

		//cout<<"(forwarding.cc-ProcessHelloRSU)PIT中对应的数据包为 "<<endl;

		//2018.12.27 用map毫无意义，只是为了输出上一跳路段
		std::unordered_map<std::string, std::unordered_set<std::string>> dataandroutes = m_nrpit->GetDataNameandLastRoute(routesCollection);

		/*for (std::unordered_map<std::string, std::unordered_set<std::string>>::iterator it = dataandroutes.begin(); it != dataandroutes.end(); it++)
		{
			//cout<<"数据包名称为 "<<it->first<<endl;
			//cout<<"对应的上一跳路段为 ";
			std::unordered_set<std::string> collection = it->second;
			for (std::unordered_set<std::string>::iterator itcollect = collection.begin(); itcollect != collection.end(); itcollect++)
			{
				//cout<<*itcollect<<" ";
			}
			//cout<<endl;
		}*/

		std::map<uint32_t, Ptr<const Data>> datacollection = m_cs->GetData(dataandroutes);
		if (!datacollection.empty())
		{
			SendDataInCache(datacollection);
		}
	}

	routes_front_pre = routes_front;
	routes_behind_pre = routes_behind;
	m_preNB = m_nb;
}

void NavigationRouteHeuristic::deleteOverTake(Ptr<Interest> interest, uint32_t sourceId, string remoteroute)
{
	pair<bool, double> msgdirection = packetFromDirection(interest);
	//cout<<"(forwarding.cc-ProcessHelloRSU) 车辆发送的心跳包的位置为 "<<msgdirection.first<<" "<<msgdirection.second<<endl;

	//发送心跳包的车辆位于当前节点后方
	//2018.12.14 该心跳包由车辆发送
	if (msgdirection.first && msgdirection.second < 0)
	{
		overtake.insert(sourceId);
	}
	else if (msgdirection.first && msgdirection.second > 0)
	{
		std::unordered_set<uint32_t>::iterator it = overtake.find(sourceId);
		//该车辆超车
		if (it != overtake.end())
		{
			//cout<<"(forwarding.cc-ProcessHelloRSU) 当前节点 "<<nodeId<<" 发送心跳包的节点 "<<sourceId<<" At time "<<Simulator::Now().GetSeconds()<<endl;
			//cout<<"(forwarding.cc-ProcessHelloRSU) 心跳包的位置为 "<<msgdirection.first<<" "<<msgdirection.second<<endl;
			std::pair<bool, uint32_t> resend = m_nrpit->DeleteFrontNode(remoteroute, sourceId);
			NodesToDeleteFromTable(sourceId);

			overtake.erase(it);

			//2018.3.17
			//该车辆已经超过RSU，记录下来
			alreadyPassed.insert(sourceId);

			if (resend.first == false)
				notifyUpperOnInterest(resend.second);
		}
	}
}

void NavigationRouteHeuristic::NodesToDeleteFromTable(uint32_t sourceId)
{
	std::unordered_map<uint32_t, std::string>::iterator it = nodeWithRoutes.find(sourceId);
	if (it != nodeWithRoutes.end())
	{
		cout << "(forwarding.cc-NodesToDeleteFromTable) 源节点 " << sourceId << " 在此处重新选择了兴趣路线" << it->second << endl;

		// routes代表车辆的实际转发路线
		vector<std::string> routes;
		SplitString(it->second, routes, " ");
		std::vector<uint32_t> newPriorityList = RSUGetPriorityListOfInterest(routes[0]);
		cout << "删除包的转发优先级列表为 ";
		for (uint32_t i = 0; i < newPriorityList.size(); ++i)
		{
			cout << newPriorityList[i] << " ";
		}
		cout << endl;

		//创建删除包
		//1. setup name
		const string &LaneName = routes[0];
		Ptr<Name> name = ns3::Create<Name>('/' + LaneName);

		//2. setup payload
		Ptr<Packet> newPayload = Create<Packet>();
		ndn::nrndn::nrHeader nrheader;

		const double &x = m_sensor->getX();
		const double &y = m_sensor->getY();

		nrheader.setX(x);
		nrheader.setY(y);
		nrheader.setSourceId(sourceId);
		nrheader.setForwardId(m_node->GetId());
		nrheader.setPriorityList(newPriorityList);
		newPayload->AddHeader(nrheader);

		//3. setup delete packet
		Ptr<Interest> deletepacket = Create<Interest>(newPayload);
		deletepacket->SetNonce(m_rand.GetValue());
		deletepacket->SetScope(DELETE_MESSAGE);
		deletepacket->SetRoutes(it->second);
		deletepacket->SetName(name);

		nodeWithRoutes.erase(it);

		if (newPriorityList.empty())
		{
			cout << "(forwarding.cc-NodesToDeleteFromTable) At Time " << Simulator::Now().GetSeconds() << " 节点 " << m_node->GetId() << "准备缓存删除包 " << deletepacket->GetNonce() << endl;
			CachingInterestPacket(deletepacket->GetNonce(), deletepacket);
			return;
		}

		m_interestNonceSeen.Put(deletepacket->GetNonce(), true);
		cout << "(forwarding.cc-NodesToDeleteFromTable) 记录删除包 nonce " << deletepacket->GetNonce() << " 源节点 " << sourceId << endl;

		// 3. Then forward the interest packet directly
		Simulator::Schedule(MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100)),
							&NavigationRouteHeuristic::SendInterestPacket, this, deletepacket);
	}
}

vector<string> NavigationRouteHeuristic::ExtractRouteFromName(const Name &name)
{
	// Name is in reverse order, so reverse it again
	// eg. if the navigation route is R1-R2-R3, the name is /R3/R2/R1
	vector<string> result;
	Name::const_reverse_iterator it;
	//Question:有非字符内容输出
	//cout<<"(forwarding.cc-ExtractRouteFromName) 得到该兴趣包的兴趣路线："<<endl;
	for (it = name.rbegin(); it != name.rend(); ++it)
	{
		result.push_back(it->toUri());
		//cout<<it->toUri()<<endl;
	}
	return result;
}

Ptr<Packet> NavigationRouteHeuristic::GetNrPayload(HeaderHelper::Type type, Ptr<const Packet> srcPayload, uint32_t forwardId, const Name &dataName /*= *((Name*)NULL) */)
{
	NS_LOG_INFO("Get nr payload, type:" << type);
	Ptr<Packet> nrPayload = Create<Packet>(*srcPayload);
	std::vector<uint32_t> priorityList;

	switch (type)
	{
	case HeaderHelper::INTEREST_NDNSIM:
	{
		priorityList = VehicleGetPriorityListOfInterest();
		break;
	}
	case HeaderHelper::CONTENT_OBJECT_NDNSIM:
	{
		priorityList = VehicleGetPriorityListOfData();
		break;
	}
	default:
	{
		NS_ASSERT_MSG(false, "unrecognize packet type");
		break;
	}
	}

	const double &x = m_sensor->getX();
	const double &y = m_sensor->getY();
	ndn::nrndn::nrHeader nrheader(m_node->GetId(), x, y, priorityList);
	nrheader.setForwardId(forwardId);
	nrheader.setLane(m_sensor->getLane());
	nrPayload->AddHeader(nrheader);
	return nrPayload;
}

void NavigationRouteHeuristic::ExpireDataPacketTimer(uint32_t nodeId, uint32_t signature)
{
	NS_LOG_FUNCTION(this << "ExpireDataPacketTimer id\t" << nodeId << "\tsignature:" << signature);
	//1. Find the waiting timer
	if (!isDuplicatedData(nodeId, signature))
	{
		return;
	}

	EventId &eventid = m_sendingDataEvent[nodeId][signature];

	//2. cancel the timer if it is still running
	eventid.Cancel();
}

Ptr<pit::Entry>
NavigationRouteHeuristic::WillInterestedData(Ptr<const Data> data)
{
	return m_pit->Find(data->GetName());
}

Ptr<pit::Entry>
NavigationRouteHeuristic::WillInterestedDataInSecondPit(Ptr<const Data> data)
{
	return m_nrpit->FindSecondPIT(data->GetName());
}

bool NavigationRouteHeuristic::IsInterestData(const Name &name)
{
	std::vector<std::string> result;
	//获取当前路段
	const std::string &currentLane = m_sensor->getLane();
	std::vector<std::string>::const_iterator it;
	std::vector<std::string>::const_iterator it2;
	//获取导航路线
	const std::vector<std::string> &route = m_sensor->getNavigationRoute();

	//当前路段在导航路线中的位置
	it = std::find(route.begin(), route.end(), currentLane);

	//name是否会出现在未来的导航路线中
	//如是，则证明该节点对该数据感兴趣；否则不感兴趣
	it2 = std::find(it, route.end(), name.get(0).toUri());

	return (it2 != route.end());
}

bool NavigationRouteHeuristic::isDuplicatedData(uint32_t id, uint32_t signature)
{
	NS_LOG_FUNCTION(this);
	if (!m_sendingDataEvent.count(id))
		return false;
	else
		return m_sendingDataEvent[id].count(signature);
}

void NavigationRouteHeuristic::BroadcastStopDataMessage(Ptr<Data> src)
{
	if (!m_running)
		return;
	//cout<<"(forwarding.cc-BroadcastStopDataMessage) 节点 "<<m_node->GetId()<<" 广播停止转发数据包的消息 "<<src->GetSignature()<<endl;
	NS_LOG_FUNCTION(this << " broadcast a stop message of " << src->GetName().toUri());

	Ptr<Packet> nrPayload = src->GetPayload()->Copy();
	ndn::nrndn::nrHeader srcheader, dstheader;
	nrPayload->PeekHeader(srcheader);
	std::vector<uint32_t> newPriorityList;
	dstheader.setSourceId(srcheader.getSourceId()); //Stop message contains an empty priority list

	dstheader.setForwardId(m_node->GetId());

	// 2018.1.19
	double x = m_sensor->getX();
	double y = m_sensor->getY();
	dstheader.setX(x);
	dstheader.setY(y);
	dstheader.setLane(m_sensor->getLane());

	Ptr<Packet> newPayload = Create<Packet>();
	newPayload->AddHeader(dstheader);

	Ptr<Data> data = Create<Data>(*src);
	data->SetPayload(newPayload);

	//4. send the payload
	Simulator::Schedule(MilliSeconds(m_uniformRandomVariable->GetInteger(0, 10)),
						&NavigationRouteHeuristic::SendDataPacket, this, data);
}

void NavigationRouteHeuristic::ForwardDataPacket(Ptr<const Data> src, std::vector<uint32_t> newPriorityList)
{
	if (!m_running)
		return;
	NS_LOG_FUNCTION(this);
	uint32_t sourceId = 0;
	uint32_t signature = 0;
	// 1. record the Data Packet(only record the forwarded packet)
	m_dataSignatureSeen.Put(src->GetSignature(), true);

	// 2. prepare the data
	Ptr<Packet> nrPayload = src->GetPayload()->Copy();
	//Ptr<Packet> newPayload = Create<Packet> ();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->RemoveHeader(nrheader);
	double x = m_sensor->getX();
	double y = m_sensor->getY();
	sourceId = nrheader.getSourceId();
	signature = src->GetSignature();
	//cout << "(forwarding.cc-ForwardDataPacket) 当前节点 " <<m_node->GetId() << " 源节点 "<< sourceId << " Signature " << signature << endl;

	// 	2.1 setup nrheader, source id do not change
	nrheader.setX(x);
	nrheader.setY(y);
	nrheader.setForwardId(m_node->GetId());
	nrheader.setLane(m_sensor->getLane());

	nrheader.setPriorityList(newPriorityList);
	nrPayload->AddHeader(nrheader);

	// 	2.2 setup FwHopCountTag
	//FwHopCountTag hopCountTag;
	//if (!IsClearhopCountTag)
	//	nrPayload->PeekPacketTag( hopCountTag);
	//	newPayload->AddPacketTag(hopCountTag);

	// 	2.3 copy the data packet, and install new payload to data
	Ptr<Data> data = Create<Data>(*src);
	data->SetPayload(nrPayload);

	// 3. Send the data Packet. Already wait, so no schedule
	SendDataPacket(data);

	// 4. record the forward times
	ndn::nrndn::nrUtils::IncreaseForwardCounter(sourceId, signature);
}

void NavigationRouteHeuristic::SendDataPacket(Ptr<Data> data)
{
	if (!m_running)
		return;
	vector<Ptr<Face>>::iterator fit;
	for (fit = m_outFaceList.begin(); fit != m_outFaceList.end(); ++fit)
	{
		(*fit)->SendData(data);
	}
	ndn::nrndn::nrUtils::AggrateDataPacketSize(data);
}

/*
 * 2017.12.28
 * added by sy
 * 车辆获取数据包转发优先级列表
 */
std::vector<uint32_t> NavigationRouteHeuristic::VehicleGetPriorityListOfData()
{
	std::vector<uint32_t> PriorityList;
	//节点中的车辆数目
	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();
	cout << "(forwarding.cc-VehicleGetPriorityListOfData) 当前节点为 " << m_node->GetId() << " 时间为 " << Simulator::Now().GetSeconds() << endl;
	std::multimap<double, uint32_t, std::greater<double>> sortlistRSU;
	std::multimap<double, uint32_t, std::greater<double>> sortlistVehicle;

	// step 1. 寻找位于导航路线后方的一跳邻居列表,m_nb为邻居列表
	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb;

	for (nb = m_nb.getNb().begin(); nb != m_nb.getNb().end(); ++nb)
	{
		//判断车辆与RSU的位置关系
		if (nb->first >= numsofvehicles)
		{
			std::pair<bool, double> result = m_sensor->VehicleGetDistanceWithRSU(nb->second.m_x, nb->second.m_y, nb->first);

			//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			if (result.first && result.second < 0)
			{
				sortlistRSU.insert(std::pair<double, uint32_t>(-result.second, nb->first));
			}
		}
		//判断车辆与其他车辆的位置关系
		else
		{
			//忽略自身节点
			if (m_node->GetId() == nb->first)
				continue;

			std::pair<bool, double> result = m_sensor->VehicleGetDistanceWithVehicle(nb->second.m_x, nb->second.m_y);
			//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			if (result.first && result.second < 0)
			{
				sortlistVehicle.insert(std::pair<double, uint32_t>(-result.second, nb->first));
			}
		}
	}

	uint32_t totalSize = sortlistRSU.size() + sortlistVehicle.size();
	if (totalSize != 0)
	{
		PriorityList.push_back(1);
		PriorityList.push_back(totalSize);
	}
	cout << "总共的车辆数目为 " << totalSize;

	cout << endl
		 << "加入RSU：";
	// step 2. Sort By Distance Descending
	std::multimap<double, uint32_t>::iterator it;
	// 加入RSU
	for (it = sortlistRSU.begin(); it != sortlistRSU.end(); ++it)
	{
		PriorityList.push_back(it->second);
		cout << " " << it->second;
	}
	cout << endl
		 << "加入普通车辆：";
	// 加入普通车辆
	for (it = sortlistVehicle.begin(); it != sortlistVehicle.end(); ++it)
	{
		PriorityList.push_back(it->second);
		cout << " " << it->second;
	}
	cout << endl;
	//getchar();
	return PriorityList;
}

std::pair<std::vector<uint32_t>, std::unordered_set<std::string>> NavigationRouteHeuristic::RSUGetPriorityListOfData(const Name &dataName, const std::unordered_set<std::string> &interestRoutes)
{
	std::vector<uint32_t> priorityList;
	std::pair<std::vector<uint32_t>, std::unordered_set<std::string>> collection;
	//无车辆的感兴趣路段
	std::unordered_set<std::string> remainroutes(interestRoutes);
	std::unordered_map<std::string, std::multimap<double, uint32_t, std::greater<double>>> sortvehicles;
	std::unordered_map<std::string, std::multimap<double, uint32_t, std::greater<double>>>::iterator itvehicles;

	//added by sy
	cout << "(forwarding.cc-RSUGetPriorityListOfData) At time:" << Simulator::Now().GetSeconds() << " 当前节点 " << m_node->GetId() << " Current dataName:" << dataName.toUri() << endl;
	//m_nrpit->showPit();

	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();
	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb;
	for (nb = m_nb.getNb().begin(); nb != m_nb.getNb().end(); ++nb)
	{
		//判断RSU与RSU的位置关系
		if (nb->first >= numsofvehicles)
		{
			//忽略自身节点
			if (nb->first == m_node->GetId())
				continue;

			//获取另一RSU所在的交点id
			std::string junction = m_sensor->RSUGetJunctionId(nb->first);
			std::unordered_set<std::string>::const_iterator it = interestRoutes.begin();
			//遍历路段集合，查看RSU是否为路段终点
			for (; it != interestRoutes.end(); it++)
			{
				//获得路段的起点和终点
				std::pair<std::string, std::string> junctions = m_sensor->GetLaneJunction(*it);
				//感兴趣路段的起点ID和另一RSU的交点ID相同
				if (junctions.first == junction)
				{
					cout << "(forwarding.cc-RSUGetPriorityListOfData) 上一跳路段为 " << *it << " RSU交点ID为 " << junction << endl;
					std::pair<bool, double> result = m_sensor->RSUGetDistanceWithRSU(nb->first, *it);
					cout << "(" << nb->first << " " << result.first << " " << result.second << ")" << endl;

					itvehicles = sortvehicles.find(*it);
					//sortvehicles中已经有该路段
					if (itvehicles != sortvehicles.end())
					{
						std::multimap<double, uint32_t, std::greater<double>> distance = itvehicles->second;
						distance.insert(std::pair<double, uint32_t>(-result.second, nb->first));
						sortvehicles[*it] = distance;
					}
					else
					{
						std::multimap<double, uint32_t, std::greater<double>> distance;
						distance.insert(std::pair<double, uint32_t>(-result.second, nb->first));
						sortvehicles[*it] = distance;
					}
				}
			}
		}
		//判断RSU与其他车辆的位置关系
		else
		{
			//获得邻居车辆当前所在路段
			std::string lane = nb->second.m_lane;
			NS_ASSERT_MSG(lane != "", "lane的长度为空");
			std::unordered_set<std::string>::const_iterator it = interestRoutes.find(lane);
			//车辆位于数据包下一行驶路段
			if (it != interestRoutes.end())
			{
				std::pair<bool, double> result = m_sensor->RSUGetDistanceWithVehicle(m_node->GetId(), nb->second.m_x, nb->second.m_y);
				//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<endl;
				if (result.first && result.second < 0)
				{
					itvehicles = sortvehicles.find(*it);
					//sortvehicles中已经有该路段
					if (itvehicles != sortvehicles.end())
					{
						std::multimap<double, uint32_t, std::greater<double>> distance = itvehicles->second;
						distance.insert(std::pair<double, uint32_t>(-result.second, nb->first));
						sortvehicles[*it] = distance;
					}
					//sortvehicles中无该路段
					else
					{
						std::multimap<double, uint32_t, std::greater<double>> distance;
						distance.insert(std::pair<double, uint32_t>(-result.second, nb->first));
						sortvehicles[*it] = distance;
					}
				}
			}
			else
			{
				//cout<<"(forwarding.cc-RSUGetPriorityListOfData) 车辆 "<<nb->first<<" 所在路段为 "<<nb->second.m_lane<<" 不在上一跳路段中"<<endl;
			}
		}
	}

	//先加入有车辆的路段数目
	uint32_t size = sortvehicles.size();
	if (size != 0)
	{
		priorityList.push_back(size);
	}
	//cout << "(RSUGetPriorityListOfData) 有车辆的路段数目为 " << size << endl;

	//加入每一个路段的车辆数目
	std::unordered_set<std::string>::iterator itroutes;
	for (itvehicles = sortvehicles.begin(); itvehicles != sortvehicles.end(); itvehicles++)
	{
		//从remainroutes中删除存在车辆的路段
		itroutes = remainroutes.find(itvehicles->first);
		if (itroutes != remainroutes.end())
		{
			remainroutes.erase(itroutes);
		}
		priorityList.push_back((itvehicles->second).size());
		//cout << "上一跳路段为 " << itvehicles->first << " 车辆数目为 " << (itvehicles->second).size() << endl;
	}

	for (itvehicles = sortvehicles.begin(); itvehicles != sortvehicles.end(); itvehicles++)
	{
		cout << "上一跳路段为 " << itvehicles->first << " ";
		//位于该条路段的车辆集合
		std::multimap<double, uint32_t, std::greater<double>> distance = itvehicles->second;
		std::multimap<double, uint32_t, std::greater<double>>::iterator itdis = distance.begin();
		for (; itdis != distance.end(); itdis++)
		{
			cout << "(" << itdis->second << " " << itdis->first << ") ";
			priorityList.push_back(itdis->second);
		}
		cout << endl;
	}

	cout << "(forwarding.cc-RSUGetPriorityListOfData) 数据包转发优先级列表为 " << endl;
	for (uint32_t i = 0; i < priorityList.size(); i++)
	{
		cout << priorityList[i] << " ";
	}
	cout << endl;

	cout << "(forwarding.cc-RSUGetPriorityListOfData) 无车辆的感兴趣路段为 " << endl;
	for (itroutes = remainroutes.begin(); itroutes != remainroutes.end(); itroutes++)
	{
		cout << *itroutes << " ";
	}
	cout << endl;

	return std::pair<std::vector<uint32_t>, std::unordered_set<std::string>>(priorityList, remainroutes);
}

std::unordered_set<uint32_t> NavigationRouteHeuristic::converVectorList(
	const std::vector<uint32_t> &list)
{
	std::unordered_set<uint32_t> result;
	std::vector<uint32_t>::const_iterator it;
	for (it = list.begin(); it != list.end(); ++it)
		result.insert(*it);
	return result;
}

void NavigationRouteHeuristic::ToContentStore(Ptr<Data> data)
{
	NS_LOG_DEBUG("To content store.(Just a trace)");
	return;
}

void NavigationRouteHeuristic::NotifyUpperLayer(Ptr<Data> data)
{
	if (!m_running)
		return;

	//cout<<"进入(forwarding.cc-NotifyUpperLayer)"<<endl;
	// 1. record the Data Packet received
	//2018.1.6 m_dataSignatureSeen用于记录转发过的数据包，而转发到上层的数据包并不一定会转发给其他节点
	//m_dataSignatureSeen.Put(data->GetSignature(),true);

	// 2. notify upper layer
	vector<Ptr<Face>>::iterator fit;
	for (fit = m_inFaceList.begin(); fit != m_inFaceList.end(); ++fit)
	{
		//App::OnData() will be executed,
		//including nrProducer::OnData.
		//But none of its business, just ignore
		(*fit)->SendData(data);
	}
}

} /* namespace nrndn */
} /* namespace fw */
} /* namespace ndn */
} /* namespace ns3 */
