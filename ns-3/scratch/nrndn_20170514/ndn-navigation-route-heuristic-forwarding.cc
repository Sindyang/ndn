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

#include <algorithm>    // std::find

NS_LOG_COMPONENT_DEFINE ("ndn.fw.NavigationRouteHeuristic");

namespace ns3
{
namespace ndn
{
namespace fw
{
namespace nrndn
{

NS_OBJECT_ENSURE_REGISTERED (NavigationRouteHeuristic);

TypeId NavigationRouteHeuristic::GetTypeId(void)
{
	  static TypeId tid = TypeId ("ns3::ndn::fw::nrndn::NavigationRouteHeuristic")
	    .SetGroupName ("Ndn")
	    .SetParent<GreenYellowRed> ()
	    .AddConstructor<NavigationRouteHeuristic>()
	    .AddAttribute ("HelloInterval", "HELLO messages emission interval.",
	            TimeValue (Seconds (1)),
	            MakeTimeAccessor (&NavigationRouteHeuristic::HelloInterval),
	            MakeTimeChecker ())
	     .AddAttribute ("AllowedHelloLoss", "Number of hello messages which may be loss for valid link.",
	            UintegerValue (2),
	            MakeUintegerAccessor (&NavigationRouteHeuristic::AllowedHelloLoss),
	            MakeUintegerChecker<uint32_t> ())

	   	 .AddAttribute ("gap", "the time gap between interested nodes and disinterested nodes for sending a data packet.",
	   	        UintegerValue (20),
	   	        MakeUintegerAccessor (&NavigationRouteHeuristic::m_gap),
	   	        MakeUintegerChecker<uint32_t> ())
//		.AddAttribute ("CacheSize", "The size of the cache which records the packet sent, use LRU scheme",
//				UintegerValue (6000),
//				MakeUintegerAccessor (&NavigationRouteHeuristic::SetCacheSize,
//									  &NavigationRouteHeuristic::GetCacheSize),
//				MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("UniformRv", "Access to the underlying UniformRandomVariable",
        		 StringValue ("ns3::UniformRandomVariable"),
        		 MakePointerAccessor (&NavigationRouteHeuristic::m_uniformRandomVariable),
        		 MakePointerChecker<UniformRandomVariable> ())
        .AddAttribute ("HelloLogEnable", "the switch which can turn on the log on Functions about hello messages",
        		 BooleanValue (true),
        		 MakeBooleanAccessor (&NavigationRouteHeuristic::m_HelloLogEnable),
        		 MakeBooleanChecker())
        .AddAttribute ("NoFwStop", "When the PIT covers the nodes behind, no broadcast stop message",
        		 BooleanValue (false),
        		 MakeBooleanAccessor (&NavigationRouteHeuristic::NoFwStop),
        		 MakeBooleanChecker())
		.AddAttribute ("TTLMax", "This value indicate that when a data is received by disinterested node, the max hop count it should be forwarded",
		         UintegerValue (3),
		         MakeUintegerAccessor (&NavigationRouteHeuristic::m_TTLMax),
		         MakeUintegerChecker<uint32_t> ())
	    ;
	  return tid;
}

NavigationRouteHeuristic::NavigationRouteHeuristic():
	HelloInterval (Seconds (1)),
	AllowedHelloLoss (2),
	m_htimer (Timer::CANCEL_ON_DESTROY),
	m_timeSlot(Seconds (0.05)),
	m_CacheSize(5000),// Cache size can not change. Because if you change the size, the m_interestNonceSeen and m_dataNonceSeen also need to change. It is really unnecessary
	m_interestNonceSeen(m_CacheSize),
	m_dataSignatureSeen(m_CacheSize),
	m_nb (HelloInterval),
	m_preNB (HelloInterval),
	m_running(false),
	m_runningCounter(0),
	m_HelloLogEnable(true),
	m_gap(20),
	m_TTLMax(3),
	NoFwStop(false),
	m_sendInterestTime(0),
	m_sendDataTime(0)
{
	m_htimer.SetFunction (&NavigationRouteHeuristic::HelloTimerExpire, this);
	m_nb.SetCallback (MakeCallback (&NavigationRouteHeuristic::FindBreaksLinkToNextHop, this));
}

NavigationRouteHeuristic::~NavigationRouteHeuristic ()
{

}

void NavigationRouteHeuristic::Start()
{
	NS_LOG_FUNCTION (this);
	//cout<<"进入(forwarding.cc-Start)"<<endl;
	if(!m_runningCounter)
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
	NS_LOG_FUNCTION (this);
	//cout<<"(forwarding.cc-Stop)"<<endl;
	if(m_runningCounter)
		m_runningCounter--;
	else
		return;

	if(m_runningCounter)
		return;
	m_running = false;
	m_htimer.Cancel();
	m_nb.CancelTimer();
}

void NavigationRouteHeuristic::WillSatisfyPendingInterest(
		Ptr<Face> inFace, Ptr<pit::Entry> pitEntry)
{
	 NS_LOG_FUNCTION (this);
	 NS_LOG_UNCOND(this <<" is in unused function");
}

bool NavigationRouteHeuristic::DoPropagateInterest(
		Ptr<Face> inFace, Ptr<const Interest> interest,
		Ptr<pit::Entry> pitEntry)
{
	  NS_LOG_FUNCTION (this);
	  NS_LOG_UNCOND(this <<" is in unused function");
	  NS_ASSERT_MSG (m_pit != 0, "PIT should be aggregated with forwarding strategy");

	  return true;
}

void NavigationRouteHeuristic::WillEraseTimedOutPendingInterest(
		Ptr<pit::Entry> pitEntry)
{
	 NS_LOG_FUNCTION (this);
	 NS_LOG_UNCOND(this <<" is in unused function");
}

void NavigationRouteHeuristic::AddFace(Ptr<Face> face) {
	//every time face is added to NDN stack?
	NS_LOG_FUNCTION(this);
	if(Face::APPLICATION==face->GetFlags())
	{
		NS_LOG_DEBUG("Node "<<m_node->GetId()<<" add application face "<<face->GetId());
		cout<<"(forwarding.cc-AddFace) Node "<<m_node->GetId()<<" add application face "<<face->GetId()<<endl;
		//getchar();
		m_inFaceList.push_back(face);
		
	}
	else
	{
		NS_LOG_DEBUG("Node "<<m_node->GetId()<<" add NOT application face "<<face->GetId());
		cout<<"(forwarding.cc-AddFace) Node "<<m_node->GetId()<<" add NOT application face "<<face->GetId()<<endl;
		//getchar();
		m_outFaceList.push_back(face);
	}
}

void NavigationRouteHeuristic::RemoveFace(Ptr<Face> face)
{
	NS_LOG_FUNCTION(this);
	if(Face::APPLICATION==face->GetFlags())
	{
		NS_LOG_DEBUG("Node "<<m_node->GetId()<<" remove application face "<<face->GetId());
		m_inFaceList.erase(find(m_inFaceList.begin(),m_inFaceList.end(),face));
	}
	else
	{
		NS_LOG_DEBUG("Node "<<m_node->GetId()<<" remove NOT application face "<<face->GetId());
		m_outFaceList.erase(find(m_outFaceList.begin(),m_outFaceList.end(),face));
	}
}
void NavigationRouteHeuristic::DidReceiveValidNack(
		Ptr<Face> incomingFace, uint32_t nackCode, Ptr<const Interest> nack,
		Ptr<pit::Entry> pitEntry)
{
	 NS_LOG_FUNCTION (this);
	 NS_LOG_UNCOND(this <<" is in unused function");
}

//车辆获取转发优先级列表
std::vector<uint32_t> NavigationRouteHeuristic::VehicleGetPriorityListOfInterest()
{
	std::vector<uint32_t> PriorityList;
	//节点中的车辆数目
	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();
	//cout<<"(forwarding.cc-VehicleGetPriorityListOfInterest) numsofvehicles "<<numsofvehicles<<endl;
	cout<<"(forwarding.cc-VehicleGetPriorityListOfInterest) 当前节点为 "<<m_node->GetId()<<" 时间为 "<<Simulator::Now().GetSeconds()<<endl;
	
	std::multimap<double,uint32_t,std::greater<double> > sortlistRSU;
	std::multimap<double,uint32_t,std::greater<double> > sortlistVehicle;

	// step 1. 寻找位于导航路线前方的一跳邻居列表,m_nb为邻居列表
	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb;
	
	for(nb = m_nb.getNb().begin();nb != m_nb.getNb().end();++nb)
	{
		//判断车辆与RSU的位置关系
		if(nb->first >= numsofvehicles)
		{
			std::pair<bool,double> result = m_sensor->VehicleGetDistanceWithRSU(nb->second.m_x,nb->second.m_y,nb->first);
			//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			if(result.first && result.second > 0)
			{
				sortlistRSU.insert(std::pair<double,uint32_t>(result.second,nb->first));
			}
			//getchar();
		}
		//判断车辆与其他车辆的位置关系
		else
		{
			std::pair<bool, double> result = m_sensor->VehicleGetDistanceWithVehicle(nb->second.m_x,nb->second.m_y);
			//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			//若result.second >= 0,会将自身加入转发优先级列表中
			if(result.first && result.second > 0)
			{
				sortlistVehicle.insert(std::pair<double,uint32_t>(result.second,nb->first));
			}
		}
	}
	cout<<endl<<"加入RSU：";
	// step 2. Sort By Distance Descending
	std::multimap<double,uint32_t>::iterator it;
	// 加入RSU
	for(it=sortlistRSU.begin();it!=sortlistRSU.end();++it)
	{
		PriorityList.push_back(it->second);
		cout<<" "<<it->second;
	}
	cout<<endl<<"加入普通车辆：";
	// 加入普通车辆
	for(it=sortlistVehicle.begin();it!=sortlistVehicle.end();++it)
	{
		PriorityList.push_back(it->second);
		cout<<" "<<it->second;
	}	
	cout<<endl;
	//getchar();
	return PriorityList;
} 

/*
 * 2017.12.27
 * lane为兴趣包下一行驶路段
 */
std::vector<uint32_t> NavigationRouteHeuristic::RSUGetPriorityListOfInterest(const std::string lane)
{
	std::vector<uint32_t> PriorityList;
	//节点中的车辆数目
	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();
	//cout<<"(forwarding.cc-RSUGetPriorityListOfInterest) 当前节点为 "<<m_node->GetId()<<" 时间为 "<<Simulator::Now().GetSeconds()<<endl;
	std::pair<std::string,std::string> junctions = m_sensor->GetLaneJunction(lane);
	//cout<<"(forwarding.cc-RSUGetPriorityListOfInterest) 道路的起点为 "<<junctions.first<<" 终点为 "<<junctions.second<<endl;
	std::multimap<double,uint32_t,std::greater<double> > sortlistVehicle;
	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb;
	
	for(nb = m_nb.getNb().begin();nb != m_nb.getNb().end();++nb)
	{
		//判断RSU与RSU的位置关系
		if(nb->first >= numsofvehicles)
		{
			//获取另一RSU所在的交点id
			std::string junction = m_sensor->RSUGetJunctionId(nb->first);
			cout<<endl<<"(forwarding.cc-RSUGetPriorityListOfInterest) RSU所在的交点为 "<<junction<<endl;
			if(junction == junctions.second)
			{
				PriorityList.push_back(nb->first);
				cout<<"(forwarding.cc-RSUGetPriorityListOfInterest) 加入RSU "<<nb->first<<endl;
			}
		}
		//判断RSU与其他车辆的位置关系
		else
		{
			//车辆位于兴趣包下一行驶路段
			if(lane == nb->second.m_lane)
			{
				std::pair<bool,double> result = m_sensor->RSUGetDistanceWithVehicle(m_node->GetId(),nb->second.m_x,nb->second.m_y);
				//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
				if(result.first && result.second > 0)
				{
					sortlistVehicle.insert(std::pair<double,uint32_t>(result.second,nb->first));
				}
			}
		}
	}
	cout<<endl<<"加入普通车辆：";
	for(std::multimap<double,uint32_t>::iterator it=sortlistVehicle.begin();it!=sortlistVehicle.end();++it)
	{
		PriorityList.push_back(it->second);
		cout<<" "<<it->second;
	}	
	cout<<endl;
	return PriorityList;
}

void NavigationRouteHeuristic::OnInterest(Ptr<Face> face,
		Ptr<Interest> interest)
{
	//NS_LOG_UNCOND("Here is NavigationRouteHeuristic dealing with OnInterest");
	//NS_LOG_FUNCTION (this);
	if(!m_running) return;
	
	//2017.12.12 added by sy
	//分情况处理普通车辆和RSU
	const std::string& currentType = m_sensor->getType();
	if(currentType == "RSU")
	{
		OnInterest_RSU(face,interest);
	}
	else if(currentType == "DEFAULT_VEHTYPE")
	{
		OnInterest_Car(face,interest);
	}
	else
	{
		NS_ASSERT_MSG(false,"车辆类型出错");
	}	
}

void NavigationRouteHeuristic::OnInterest_Car(Ptr<Face> face,Ptr<Interest> interest)
{
	//NS_LOG_UNCOND("Here is NavigationRouteHeuristic dealing with OnInterest");
	//NS_LOG_FUNCTION (this);
	if(!m_running) return;
	//getchar();
	//cout<<endl<<"进入(forwarding.cc-OnInterest_Car)"<<endl;
	
	if(Face::APPLICATION==face->GetFlags())
	{
		//consumer产生兴趣包，在路由层进行转发
		//cout << "(forwarding.cc-OnInterest_Car)该兴趣包来自应用层。当前节点为 "<<m_node->GetId() <<endl;
		NS_LOG_DEBUG("Get interest packet from APPLICATION");
		// This is the source interest from the upper node application (eg, nrConsumer) of itself
		// 1.Set the payload
		interest->SetPayload(GetNrPayload(HeaderHelper::INTEREST_NDNSIM,interest->GetPayload(),999999999));
		
		//更新兴趣包的实际转发路线
		std::string routes = interest->GetRoutes();
		//cout<<"(forwarding.cc-OnInterest_Car) routes "<<routes<<endl;
		const string& localLane = m_sensor->getLane();
		//cout<<"(forwarding.cc-OnInterest_Car) localLane "<<localLane<<endl;
		std::size_t found = routes.find(localLane);
		std::string newroutes = routes.substr(found);
		//cout<<"(forwarding.cc-OnInterest_Car) newroutes "<<newroutes<<endl;
		interest->SetRoutes(newroutes);
		//getchar();
        
        //added by sy
        ndn::nrndn::nrHeader nrheader;
        interest->GetPayload()->PeekHeader(nrheader);
        uint32_t nodeId = nrheader.getSourceId();
		
		// 2017.12.23 added by sy
		const std::vector<uint32_t>& pri=nrheader.getPriorityList();
		// 转发优先级列表为空，需要缓存兴趣包
		if(pri.empty())
		{
			cout<<"(forwarding.cc-OnInterest_Car) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<m_node->GetId()<<"准备缓存自身的兴趣包 "<<interest->GetNonce()<<endl;
			//Simulator::Schedule(MilliSeconds(m_uniformRandomVariable->GetInteger(0,100)),&NavigationRouteHeuristic::CachingInterestPacket,this,interest->GetNonce(),interest);
			CachingInterestPacket(interest->GetNonce(),interest);
			//if(m_node->GetId() == 97)
				//getchar();
			return;
		}
		
		//2017.12.13 输出兴趣包实际转发路线
		//std::string routes = interest->GetRoutes();
		//std::cout<<"(forwarding.cc-OnInterest) routes "<<routes<<std::endl;
		//getchar();
	 
		
		//cout<<"(forwarding.cc-OnInterest) 兴趣包序列号为 "<<interest->GetNonce()<<endl;
		//getchar();

		// 2. record the Interest Packet
		m_interestNonceSeen.Put(interest->GetNonce(),true);
		//cout<<"(forwarding.cc-OnInterest) 记录兴趣包 nonce "<<interest->GetNonce()<<" from NodeId "<<nodeId<<endl;

		// 3. Then forward the interest packet directly
		Simulator::Schedule(MilliSeconds(m_uniformRandomVariable->GetInteger(0,100)),
				&NavigationRouteHeuristic::SendInterestPacket,this,interest);
		
		
	   // cout<<"(forwarding.cc-OnInterest_Car)来自应用层的兴趣包处理完毕。源节点 "<<nodeId<<endl;
		//getchar();
		return;
	}
	
	if(HELLO_MESSAGE==interest->GetScope())
	{		
		ProcessHello(interest);
		return;
	}
	
	Ptr<const Packet> nrPayload	= interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	//获取发送兴趣包节点的ID
	uint32_t nodeId = nrheader.getSourceId();
	//获取兴趣的随机编码
	uint32_t seq = interest->GetNonce();
	//获取当前节点Id
	uint32_t myNodeId = m_node->GetId();
	//获取兴趣包的转发节点id
	uint32_t forwardId = nrheader.getForwardId();
	
	cout<<endl<<"(forwarding.cc-OnInterest_Car)At Time "<<Simulator::Now().GetSeconds()<<" 当前车辆Id为 "<<myNodeId<<",源节点 "<<nodeId<<",转发节点 "<<forwardId<<" seq "<<seq<<endl;
	

	//If the interest packet has already been sent, do not proceed the packet
	pair<bool, double> msgdirection = packetFromDirection(interest);
	cout<<"(forwarding.cc-OnInterest_Car) msgdirection first "<<msgdirection.first<<" second "<<msgdirection.second<<endl;
	
	if(m_interestNonceSeen.Get(interest->GetNonce()))
	{
		//2018.1.16 从缓存中删除兴趣包
		if(msgdirection.first && msgdirection.second > 0)
		{
			m_cs->DeleteInterest(interest->GetNonce());
			//getchar();
		}
		cout<<"(forwarding.cc-OnInterest_Car) 源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",该兴趣包已经被发送, nonce为 "<<interest->GetNonce()<<endl;
		NS_LOG_DEBUG("The interest packet has already been sent, do not proceed the packet of "<<interest->GetNonce());
		// 2017.12.27 
		//return;
	}
	
	//获取优先列表
	//cout << "(forwarding.cc-OnInterest) 兴趣包的转发优先级列表为: ";
	const std::vector<uint32_t>& pri=nrheader.getPriorityList();
    for(auto it = pri.begin();it != pri.end();it++)
	{
		cout<<*it<<" ";
	}
	cout<<endl;
	//getchar();

	//Deal with the stop message first
	//避免回环
	if(Interest::NACK_LOOP==interest->GetNack())
	{
		//cout<<"(forwarding.cc-OnInterest_Car) 该兴趣包为NACK_LOOP。源节点 "<<nodeId<<endl;
		ExpireInterestPacketTimer(nodeId,seq);
		//getchar();
		return;
	}
	
	
	//If it is not a stop message, prepare to forward
	if(!msgdirection.first || // from other direction
			msgdirection.second >= 0)// or from front
	{
		NS_LOG_DEBUG("Get interest packet from front or other direction");
		if(!isDuplicatedInterest(nodeId,seq))// Is new packet
		{
			//sy:对于从前方收到的兴趣包，若是第一次收到的，直接丢弃即可
			NS_LOG_DEBUG("Get interest packet from front or other direction and it is new packet");
			//cout<<"(forwarding.cc-OnInterest_Car) 该兴趣包从前方或其他路线得到，且该兴趣包是新的。源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",转发节点 "<<forwardId<<endl<<endl;
			//getchar();
			DropInterestePacket(interest);
		}
		else // Is old packet
		{
			//wsy:对于从前方收到的兴趣包，若之前已经收到过，则没有必要再转发该兴趣包
			NS_LOG_DEBUG("Get interest packet from front or other direction and it is old packet");
		//	cout<<"(forwarding.cc-OnInterest_Car) 该兴趣包从前方或其他路线得到，且该兴趣包是旧的。源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",转发节点 "<<forwardId<<endl<<endl;
			//getchar();
			ExpireInterestPacketTimer(nodeId,seq);
		}
	}
	else// it is from nodes behind
	{
		NS_LOG_DEBUG("Get interest packet from nodes behind");
		cout<<"(forwarding.cc-OnInterest_Car) 该兴趣包从后方得到。源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",转发节点 "<<forwardId<<endl;

		//evaluate whether receiver's id is in sender's priority list
		bool idIsInPriorityList;
		vector<uint32_t>::const_iterator idit;
		idit = find(pri.begin(), pri.end(), m_node->GetId());
		idIsInPriorityList = (idit != pri.end());
		//evaluate end

		if (idIsInPriorityList)
		{
			cout<<"(forwarding.cc-OnInterest_Car) Node id is in PriorityList"<<endl;
			NS_LOG_DEBUG("Node id is in PriorityList");

			//bool IsPitCoverTheRestOfRoute=PitCoverTheRestOfRoute(remoteRoute);

			//NS_LOG_DEBUG("IsPitCoverTheRestOfRoute?"<<IsPitCoverTheRestOfRoute);
			//if(NoFwStop)
				//IsPitCoverTheRestOfRoute = false;

			//if (IsPitCoverTheRestOfRoute)
			//{
				//BroadcastStopMessage(interest);
				//return;
			//}
			//else
			//{
				//Start a timer and wait
			double index = distance(pri.begin(), idit);
			double random = m_uniformRandomVariable->GetInteger(0, 20);
			Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
			//构造转发优先级列表，并判断前方邻居是否为空
			std::vector<uint32_t> newPriorityList = VehicleGetPriorityListOfInterest();
			if(newPriorityList.empty())
			{
				cout<<"(forwarding.cc-OnInterest_Car) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<myNodeId<<"准备缓存兴趣包 "<<seq<<endl;
				//getchar();
				//Simulator::Schedule(sendInterval,&NavigationRouteHeuristic::CachingInterestPacket,this,seq,interest);
				CachingInterestPacket(seq,interest);
				//BroadcastStopMessage(interest);
				m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval,&NavigationRouteHeuristic::BroadcastStopInterestMessage,this,interest);
				//if(nodeId == 97)
					//getchar();
			}
			else
			{
				m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval,&NavigationRouteHeuristic::ForwardInterestPacket,this,interest,newPriorityList);
			}
			//}
		}
		else
		{
			cout<<"(forwarding.cc-OnInterest_Car) Node id is not in PriorityList"<<endl;
			NS_LOG_DEBUG("Node id is not in PriorityList");
			DropInterestePacket(interest);
			//getchar();
		}
		//getchar();
		//cout<<endl;
	}
}

void NavigationRouteHeuristic::OnInterest_RSU(Ptr<Face> face,Ptr<Interest> interest)
{
	//cout<<endl<<"进入(forwarding.cc-OnInterest_RSU)"<<endl;
	
	// RSU不会收到自己发送的兴趣包
	if(Face::APPLICATION==face->GetFlags())
	{
		NS_ASSERT_MSG(false,"RSU收到了自己发送的兴趣包");
		return;
	}
	
	//收到心跳包
	if(HELLO_MESSAGE==interest->GetScope())
	{		
		ProcessHelloRSU(interest);
		return;
	}
	
	//收到删除包
	//删除包可以与兴趣包有相同的结构
	
	
	Ptr<const Packet> nrPayload	= interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	//获取发送兴趣包节点的ID
	uint32_t nodeId = nrheader.getSourceId();
	//获取兴趣的序列号
	uint32_t seq = interest->GetNonce();
	//获取当前节点Id
	uint32_t myNodeId = m_node->GetId();
	//获取兴趣包的转发节点id
	uint32_t forwardId = nrheader.getForwardId();
	//获取兴趣包的实际转发路线
	std::string forwardRoute = interest->GetRoutes();
	
	cout<<endl<<"(forwarding.cc-OnInterest_RSU)At Time "<<Simulator::Now().GetSeconds()<<" 当前RSUId为 "<<myNodeId<<",源节点 "<<nodeId<<",转发节点 "<<forwardId<<" seq "<<seq<<endl;
	cout<<"兴趣包实际转发路线为 "<<forwardRoute<<endl;
	
	//if(nodeId == 24)
		//getchar();
	//If it is not a stop message, prepare to forward:
	pair<bool, double> msgdirection = packetFromDirection(interest);
	cout<<"(forwarding.cc-OnInterest_RSU) msgdirection first "<<msgdirection.first<<" second "<<msgdirection.second<<endl;
	
	//If the interest packet has already been sent, do not proceed the packet
	if(m_interestNonceSeen.Get(interest->GetNonce()))
	{
		//2018.1.16 从缓存中删除兴趣包
		if(msgdirection.first && msgdirection.second)
		{
			m_cs->DeleteInterest(interest->GetNonce());
			//cout<<"(forwarding.cc-OnInterest_RSU) 从缓存中删除兴趣包 "<<interest->GetNonce()<<endl;
			//getchar();
		}
		
		cout<<"(forwarding.cc-OnInterest_RSU) 源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",该兴趣包已经被发送, nonce为 "<<interest->GetNonce()<<endl;
		NS_LOG_DEBUG("The interest packet has already been sent, do not proceed the packet of "<<interest->GetNonce());
		// 2017.12.27
		//return;
	}
	
	//获取优先列表
	cout << "(forwarding.cc-OnInterest_RSU) 兴趣包的转发优先级列表为: ";
	const std::vector<uint32_t>& pri=nrheader.getPriorityList();
    for(auto it = pri.begin();it != pri.end();it++)
	{
		cout<<*it<<" ";
	}
	cout<<endl;

	//Deal with the stop message first
	//避免回环
	//2017.12.23 按理来说，若RSU收到的兴趣包为NACK_LOOP，RSU应该为该兴趣包所在路段的起点处 TEST IT
	if(Interest::NACK_LOOP==interest->GetNack())
	{
		cout<<"(forwarding.cc-OnInterest_RSU) 该兴趣包为NACK_LOOP。源节点 "<<nodeId<<endl;
		ExpireInterestPacketTimer(nodeId,seq);
		//if(nodeId == 24)
		   // getchar();
		return;
	}

	
	if(!msgdirection.first || // from other direction
			msgdirection.second >= 0)// or from front
	{
		NS_LOG_DEBUG("Get interest packet from front or other direction");
		if(!isDuplicatedInterest(nodeId,seq))// Is new packet
		{
			NS_LOG_DEBUG("Get interest packet from front or other direction and it is new packet");
			//cout<<"(forwarding.cc-OnInterest_RSU) 该兴趣包从前方或其他路线得到，且该兴趣包是新的。源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",转发节点 "<<forwardId<<endl<<endl;
			//getchar();
			DropInterestePacket(interest);
		}
		else // Is old packet
		{
			//按理来说，不应该进入该函数；因为RSU具有处理兴趣包的最高优先级
			NS_LOG_DEBUG("Get interest packet from front or other direction and it is old packet");
			//cout<<"(forwarding.cc-OnInterest_RSU) 该兴趣包从前方或其他路线得到，且该兴趣包是旧的。源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",转发节点 "<<forwardId<<endl<<endl;
			//getchar();
			ExpireInterestPacketTimer(nodeId,seq);
		}
	}
	else// it is from nodes behind
	{
		NS_LOG_DEBUG("Get interest packet from nodes behind");
		cout<<"(forwarding.cc-OnInterest_RSU) 该兴趣包从后方得到。源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",转发节点 "<<forwardId<<endl;
		//if(nodeId == 24)
		    //getchar();
		
		vector<string> interestRoute= ExtractRouteFromName(interest->GetName());
		
		// routes代表车辆的实际转发路线
		vector<std::string> routes;
		SplitString(forwardRoute,routes," ");
		
		std::string junction = m_sensor->RSUGetJunctionId(myNodeId);
		// Update the PIT here
		m_nrpit->UpdateRSUPit(junction,forwardRoute,interestRoute,nodeId);
		// Update finish
		
		
		//这里需要获取当前及之后的兴趣路线
		vector<string> futureinterest = GetLocalandFutureInterest(routes,interestRoute);
		
		/*cout<<"(forwarding.cc-OnInterest_RSU) "<<endl;
		for(uint32_t i = 0;i < futureinterest.size();i++)
		{
			cout<<futureinterest[i]<<" ";
		}
		cout<<endl;*/
		//getchar();

		//查看缓存中是否有对应的数据包
		std::map<uint32_t,Ptr<const Data> > datacollection = m_cs->GetData(futureinterest);
		//getchar();
		
		if(!datacollection.empty())
		{
			SendDataInCache(datacollection);
			cout<<"(forwarding.cc-OnInterest_RSU) 从缓存中取出数据包"<<endl;
		}
		
		
		//evaluate whether receiver's id is in sender's priority list
		bool idIsInPriorityList;
		vector<uint32_t>::const_iterator idit;
		idit = find(pri.begin(), pri.end(), m_node->GetId());
		idIsInPriorityList = (idit != pri.end());

		//evaluate end

		if (idIsInPriorityList)
		{
			//查看下一路段是否为兴趣路段
			std::vector<std::string> routes;
			SplitString(forwardRoute,routes," ");
			if(routes.size() <= 1)
			{
				std::cout<<"(forwarding.cc-OnInterest_RSU) 该兴趣包已经行驶完了所有的兴趣路线 "<<seq<<std::endl;
				BroadcastStopInterestMessage(interest);
				//getchar();
				return;
			}
			std::string nextroute = routes[1];
			std::vector<std::string>::const_iterator it = std::find(interestRoute.begin(),interestRoute.end(),nextroute);
		
			//下一路段为兴趣路段
			if(it != interestRoute.end())
			{
				double index = distance(pri.begin(), idit);
				double random = m_uniformRandomVariable->GetInteger(0, 20);
				Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
				//构造转发优先级列表，并判断前方邻居是否为空
				std::vector<uint32_t> newPriorityList = RSUGetPriorityListOfInterest(nextroute);
				if(newPriorityList.empty())
				{
					forwardRoute = forwardRoute.substr(nextroute.size()+1);
					//cout<<"兴趣包实际转发路线为 "<<forwardRoute<<endl;
					//更新兴趣包的实际转发路线
					interest->SetRoutes(forwardRoute);
					cout<<"(forwarding.cc-OnInterest_RSU) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<myNodeId<<"准备缓存兴趣包 "<<seq<<endl;
					//Simulator::Schedule(sendInterval,&NavigationRouteHeuristic::CachingInterestPacket,this,seq,interest);
					CachingInterestPacket(seq,interest);
					//BroadcastStopMessage(interest);
					m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval,&NavigationRouteHeuristic::BroadcastStopInterestMessage,this,interest);
					//重新选择路线发送兴趣包
					//if(nodeId == 24)
					    //getchar();
				}
				else
				{
					forwardRoute = forwardRoute.substr(nextroute.size()+1);
					//cout<<"兴趣包实际转发路线为 "<<forwardRoute<<endl;
					//更新兴趣包的实际转发路线
					interest->SetRoutes(forwardRoute);
					m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval,&NavigationRouteHeuristic::ForwardInterestPacket,this,interest,newPriorityList);
					//getchar();
				}
			}
			else
			{
				NS_ASSERT_MSG(false,"兴趣包的下一路段不为兴趣路段");
			}
			
			/*cout<<"(forwarding.cc-OnInterest) Node id is in PriorityList"<<endl;
			NS_LOG_DEBUG("Node id is in PriorityList");

			//判断主待处理兴趣列表是否有增加新的表项
			bool IsPitCoverTheRestOfRoute=PitCoverTheRestOfRoute(remoteRoute);

			//NS_LOG_DEBUG("IsPitCoverTheRestOfRoute?"<<IsPitCoverTheRestOfRoute);
			//if(NoFwStop)
				//IsPitCoverTheRestOfRoute = false;

			if (IsPitCoverTheRestOfRoute)
			{
				BroadcastStopMessage(interest);
				return;
			}
			else
			{
				//查看下一路段是否为兴趣路段
				if(yes)
				{
					//查看下一路段前方邻居是否为空
					if(yes)
					{
						//缓存该兴趣包的备份
						//重新选择路线
						//查看新路段是否有车辆
						if()
						{
							double index = distance(pri.begin(), idit);
							double random = m_uniformRandomVariable->GetInteger(0, 20);
							Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
							m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval,
								&NavigationRouteHeuristic::ForwardInterestPacket, this,
								interest);
						cout<<"(forwarding.cc-OnInterest)ForwardInterestPacket"<<endl;
						}
						else
						{
							//广播停止转发的消息
							//这里需不需要等，我也不知道
						}
						
					}
					else
					{
						double index = distance(pri.begin(), idit);
						double random = m_uniformRandomVariable->GetInteger(0, 20);
						Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
						m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval,
							&NavigationRouteHeuristic::ForwardInterestPacket, this,
							interest);
						cout<<"(forwarding.cc-OnInterest)ForwardInterestPacket"<<endl;
					}
				}
				else
				{
					//RSU自身是否处于某一兴趣路段的起点处
					//实际转发路线是否包含了兴趣路线
					//借路路线是否和兴趣路线处于同一条道路上
					//若上述条件都满足，RSU生成一个新的兴趣包
					//查看下一路段前方邻居是否为空
					if(yes)
					{
						//缓存该兴趣包的备份
						//广播停止转发的消息
					}
					else
					{
						double index = distance(pri.begin(), idit);
						double random = m_uniformRandomVariable->GetInteger(0, 20);
						Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
						m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval,
							&NavigationRouteHeuristic::ForwardInterestPacket, this,
							interest);
						cout<<"(forwarding.cc-OnInterest)ForwardInterestPacket"<<endl;
					}
					
				}
			}
			//判断前方邻居是否为空
			if()
			{
				double index = distance(pri.begin(),idit);
				double random = m_uniformRandomVariable->GetInteger(0, 20);
				Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
				//此处的函数为等待一定时间后，广播停止转发该兴趣包的消息
			}
			else
			{
				double index = distance(pri.begin(), idit);
				double random = m_uniformRandomVariable->GetInteger(0, 20);
				Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
				m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval,
						&NavigationRouteHeuristic::ForwardInterestPacket, this,
						interest);
				cout<<"(forwarding.cc-OnInterest)ForwardInterestPacket"<<endl;
			}*/
		}
		else
		{
			//不在转发优先级列表中
			cout<<"(forwarding.cc-OnInterest) Node id is not in PriorityList"<<endl;
			NS_LOG_DEBUG("Node id is not in PriorityList");
			DropInterestePacket(interest);
		}
		//getchar();
		//cout<<endl;
	}
}

/*
 * 2017.12 25 added by sy
 * 分割字符串
 */
void
NavigationRouteHeuristic::SplitString(const std::string& s,std::vector<std::string>& v,const std::string& c)
{
	std::size_t pos1,pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while(std::string::npos != pos2)
	{
		v.push_back(s.substr(pos1,pos2-pos1));
		pos1 = pos2+c.size();
		pos2 = s.find(c,pos1);
	}
	if(pos1 != s.length())
		v.push_back(s.substr(pos1));
}


vector<string> 
NavigationRouteHeuristic::GetLocalandFutureInterest(vector<string> forwardroute,vector<string> interestroute)
{
	string currentroute = forwardroute[0];
	vector<string>::iterator it = std::find(interestroute.begin(),interestroute.end(),currentroute);
	
	vector<string> futureinterest;
	if(it != interestroute.end())
	{
		//cout<<"(forwarding.cc-GetLocalandFutureInterest) 兴趣包来时的路段为兴趣路段"<<endl;
		for(;it != interestroute.end();it++)
		{
			futureinterest.push_back(*it);
		}
	}
	else
	{
		NS_ASSERT_MSG(false,"还未测试这部分代码");
		cout<<"(forwarding.cc-GetLocalandFutureInterest) 兴趣包来时的路段不是兴趣路段"<<endl;
		for(uint32_t i = 0;i < forwardroute.size();i++)
		{
			string route = forwardroute[i];
			it = find(interestroute.begin(),interestroute.end(),route);
			if(it != interestroute.end())
			{
				cout<<"(forwarding.cc-GetLocalandFutureInterest) 在转发路线中找到了兴趣路段"<<endl;
				break;
			}
		}
		for(;it != interestroute.end();it++)
		{
			futureinterest.push_back(*it);
		}
	}
	return futureinterest;
}


void NavigationRouteHeuristic::OnData(Ptr<Face> face, Ptr<Data> data)
{
	NS_LOG_FUNCTION (this);
	if(!m_running) 
		return;
	
	//2017.12.29 added by sy
	//分情况处理普通车辆和RSU
	const std::string& currentType = m_sensor->getType();
	if(currentType == "RSU")
	{
		OnData_RSU(face,data);
	}
	else if(currentType == "DEFAULT_VEHTYPE")
	{
		OnData_Car(face,data);
	}
	else
	{
		NS_ASSERT_MSG(false,"车辆类型出错");
	}	
}

void NavigationRouteHeuristic::OnData_Car(Ptr<Face> face,Ptr<Data> data)
{
	NS_LOG_FUNCTION (this);
	//收到了来自应用层的数据包
	if(Face::APPLICATION & face->GetFlags())
	{
		// This is the source data from the upper node application (eg, nrProducer) of itself
		NS_LOG_DEBUG("Get data packet from APPLICATION");
		
		// 1.Set the payload
		Ptr<Packet> payload = GetNrPayload(HeaderHelper::CONTENT_OBJECT_NDNSIM,data->GetPayload(),999999999,data->GetName());
		
		data->SetPayload(payload);
		
		ndn::nrndn::nrHeader nrheader;
		data->GetPayload()->PeekHeader(nrheader);
		const std::vector<uint32_t>& pri = nrheader.getPriorityList();
		
		//若转发优先级列表为空，缓存数据包
		if(pri.empty())
		{
			cout<<"(forwarding.cc-OnData_Car) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<m_node->GetId()<<"准备缓存自身的数据包"<<endl;
			//getchar();
			//std::unordered_set<std::string> lastroutes;
			
			// 2018.1.11 added by sy
			NotifyUpperLayer(data);
			
			CachingDataPacket(data->GetSignature(),data);
			
			Simulator::Schedule(
				MilliSeconds(m_uniformRandomVariable->GetInteger(0,100)),
				&NavigationRouteHeuristic::SendDataPacket, this, data);
			
			return;
		}

		// 2. record the Data Packet(only record the forwarded packet)
		m_dataSignatureSeen.Put(data->GetSignature(),true);

		// 3. Then forward the data packet directly
		Simulator::Schedule(
				MilliSeconds(m_uniformRandomVariable->GetInteger(0,100)),
				&NavigationRouteHeuristic::SendDataPacket, this, data);
						
		// 4. Although it is from itself, include into the receive record
		NotifyUpperLayer(data);

		uint32_t myNodeId = m_node->GetId();
		//cout<<"(forwarding.cc-OnData_Car) 应用层的数据包事件设置成功，源节点 "<<myNodeId<<endl<<endl;
		//getchar();
		return;
	}
	
	Ptr<const Packet> nrPayload	= data->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	//获取数据包源节点
	uint32_t nodeId=nrheader.getSourceId();
	//获取数据包的随机编码
	uint32_t signature=data->GetSignature();
	//获取当前节点Id
	uint32_t myNodeId = m_node->GetId();
	//获取数据包的转发节点id
	uint32_t forwardId = nrheader.getForwardId();
	
	cout<<endl<<"(forwarding.cc-OnData_Car) 源节点 "<<nodeId<<" 转发节点 "<<forwardId<<" 当前节点 "<<myNodeId<<" Signature "<<data->GetSignature()<<endl;
	
	const std::vector<uint32_t>& pri=nrheader.getPriorityList();
	cout<<"(forwarding.cc-OnData_Car) 数据包的转发优先级列表为 ";
	for(uint32_t i = 0;i < pri.size();i++)
	{
		cout<<pri[i]<<" ";
	}
	cout<<endl;
	
	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();
	pair<bool, double> msgdirection;
	
	//获取车辆上一跳节点
	uint32_t remoteId = (forwardId == 999999999)?nodeId:forwardId;
	
	if(remoteId >= numsofvehicles)
	{
		msgdirection = m_sensor->VehicleGetDistanceWithRSU(nrheader.getX(), nrheader.getY(),remoteId);
	}
	else
	{
		msgdirection = m_sensor->VehicleGetDistanceWithVehicle(nrheader.getX(),nrheader.getY());
	}
	
	cout<<"(forwarding.cc-OnData_Car) 数据包的方向为 "<<msgdirection.first<<" "<<msgdirection.second<<endl;
	
	//该数据包已经被转发过
	// 车辆应该不会重复转发数据包，但是RSU有可能重复转发数据包 
	if(m_dataSignatureSeen.Get(data->GetSignature()))
	{
		cout<<"该数据包已经被发送"<<endl;
		//2018.1.24 从缓存中删除数据包
		if(msgdirection.first && msgdirection.second < 0)
		{
			m_cs->DeleteData(data->GetSignature());
			//getchar();
		}
	
		//getchar();
		NS_LOG_DEBUG("The Data packet has already been sent, do not proceed the packet of "<<data->GetSignature());
		// 2018.1.18
		return;
	}
	
	//Deal with the stop message first. Stop message contains an empty priority list
	if(pri.empty())
	{
		// 2018.1.6 如果收到了停止转发的消息，不论是否感兴趣，都停止转发
		//if(!IsInterestData(data->GetName()))// if it is interested about the data, ignore the stop message)
		// 2018.1.11
		if(!isDuplicatedData(nodeId,signature) && IsInterestData(data->GetName()))
		{
			// 1.Buffer the data in ContentStore
			ToContentStore(data);
			// 2. Notify upper layer
			NotifyUpperLayer(data);
			cout<<"(forwarding.cc-OnData_Car) 车辆对该数据包感兴趣"<<endl;
		}
		ExpireDataPacketTimer(nodeId,signature);
		cout<<"该数据包停止转发 "<<"signature "<<data->GetSignature()<<endl;
		//getchar();
		return;
	}
	
	//If it is not a stop message, prepare to forward:
	
	//getchar();
	
	if(!msgdirection.first || msgdirection.second <= 0)// 数据包位于其他路段或当前路段后方
	{
		//第一次收到该数据包
		if(!isDuplicatedData(nodeId,signature))
		{
			if(IsInterestData(data->GetName()))
			{
				// 1.Buffer the data in ContentStore
				ToContentStore(data);
				// 2. Notify upper layer
				NotifyUpperLayer(data);
				cout<<"(forwarding.cc-OnData_Car) 车辆第一次从后方或其他路段收到数据包且对该数据包感兴趣"<<endl;
				//getchar();
				return;
			}
			else
			{
				//cout<<"(forwarding.cc-OnData_Car) 车辆第一次从后方或其他路段收到数据包且当前节点对该数据包不感兴趣"<<endl;
				DropDataPacket(data);
				//getchar();
				return;
			}
		}
		else // duplicated data
		{
			if(!msgdirection.first)
			{
				//cout<<"(forwarding.cc-OnData_Car) 该数据包从其他路段得到，且为重复数据包，不作处理"<<endl;
				//getchar();
				return;
			}
			//cout<<"(forwarding.cc-OnData_Car) 该数据包从后方得到且为重复数据包"<<endl<<endl;
			ExpireDataPacketTimer(nodeId,signature);
			//getchar();
			return;
		}
	}
	//数据包来源于当前路段前方
	else
	{
		if(isDuplicatedData(nodeId,signature))
		{
			cout<<"(forwarding.cc-OnData_Car) 该数据包从前方或其他路段得到，重复，丢弃"<<endl;
			ExpireDataPacketTimer(nodeId,signature);
			//getchar();
			return;
			/*if(priorityListIt==pri.end())
			{
				ExpireDataPacketTimer(nodeId,signature);
				return;
			}
			//在优先级列表中
			else
			{
				DropDataPacket(data);
				return;
			}*/
		}
		//Ptr<pit::Entry> Will = WillInterestedData(data);
		if(!IsInterestData(data->GetName()))
		{
			DropDataPacket(data);
			cout<<"(forwarding.cc-OnData_Car) 车辆对该数据包不感兴趣"<<endl;
		}
		else
		{
			// 1.Buffer the data in ContentStore
			ToContentStore(data);
			// 2. Notify upper layer
			NotifyUpperLayer(data);
			cout<<"(forwarding.cc-OnData_Car) 车辆对该数据包感兴趣"<<endl;
		}
		//getchar();
		
		bool idIsInPriorityList;
		std::vector<uint32_t>::const_iterator priorityListIt;
  	    //找出当前节点是否在优先级列表中
	    priorityListIt = find(pri.begin(),pri.end(),m_node->GetId());
		idIsInPriorityList = (priorityListIt != pri.end());
		
		if(idIsInPriorityList)
		{
			cout<<"(forwarding.cc-OnData_Car) 车辆在数据包转发优先级列表中"<<endl;
			double index = distance(pri.begin(),priorityListIt);
			double random = m_uniformRandomVariable->GetInteger(0, 20);
			Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
			std::vector<uint32_t> newPriorityList = VehicleGetPriorityListOfData();
			if(newPriorityList.empty())
			{
				cout<<"(forwarding.cc-OnData_Car) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<myNodeId<<"准备缓存数据包 "<<signature<<endl;
				std::unordered_set<std::string> lastroutes;
				//Simulator::Schedule(sendInterval,&NavigationRouteHeuristic::CachingDataPacket,this,signature,data/*,lastroutes*/);
				CachingDataPacket(signature,data);
				//BroadcastStopMessage(data);
				m_sendingDataEvent[nodeId][signature]=
					Simulator::Schedule(sendInterval,
					&NavigationRouteHeuristic::BroadcastStopDataMessage, this, data);
			}
			else
			{
				m_sendingDataEvent[nodeId][signature]=
					Simulator::Schedule(sendInterval,
					&NavigationRouteHeuristic::ForwardDataPacket, this, data,
					newPriorityList);
				cout<<"(forwarding.cc-OnData_Car) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<myNodeId<<"准备发送数据包 "<<signature<<endl;
			}
			//getchar();
		}
		else
		{
			cout<<"(forwarding.cc-OnData_Car) Node id is not in PriorityList"<<endl;
			NS_LOG_DEBUG("Node id is not in PriorityList");
			//getchar();
		}
	}
}

void NavigationRouteHeuristic::OnData_RSU(Ptr<Face> face,Ptr<Data> data)
{
	if(Face::APPLICATION == face->GetFlags())
	{
		NS_ASSERT_MSG(false,"RSU收到了自己发送的数据包");
		return;
	}
	
	Ptr<const Packet> nrPayload	= data->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	//获取数据包源节点
	uint32_t nodeId=nrheader.getSourceId();
	//获取数据包的随机编码
	uint32_t signature=data->GetSignature();
	//获取当前节点Id
	uint32_t myNodeId = m_node->GetId();
	//获取数据包的转发节点id
	uint32_t forwardId = nrheader.getForwardId();
	std::string forwardLane = nrheader.getLane();
	
	
	cout<<endl<<"(forwarding.cc-OnData_RSU) 源节点 "<<nodeId<<" 转发节点 "<<forwardId<<" 当前节点 "<<myNodeId<<" Signature "<<data->GetSignature()<<endl;
	
	const std::vector<uint32_t>& pri=nrheader.getPriorityList();
	
	//If the data packet has already been sent, do not proceed the packet
	//该数据包已经被转发过
	// 车辆应该不会重复转发数据包，但是RSU有可能重复转发数据包 
	/*if(m_dataSignatureSeen.Get(data->GetSignature()))
	{
		cout<<"该数据包已经被发送"<<endl;
		//getchar();
		NS_LOG_DEBUG("The Data packet has already been sent, do not proceed the packet of "<<data->GetSignature());
		//2018.1.2 RSU有可能重复转发数据包
		//return;
	}*/
	
	//Deal with the stop message first. Stop message contains an empty priority list
	if(pri.empty())
	{
		//if(!IsInterestData(data->GetName()))// if it is interested about the data, ignore the stop message)
		ExpireDataPacketTimer(nodeId,signature);
		cout<<"该数据包停止转发 "<<"signature "<<data->GetSignature()<<endl<<endl;
		//getchar();
		return;
	}
	
	m_nrpit->showPit();
	
	//If it is not a stop message, prepare to forward:
	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();
	pair<bool, double> msgdirection;
	
	//获取车辆上一跳节点
	uint32_t remoteId = (forwardId == 999999999)?nodeId:forwardId;
	
	//在仿真地图中不会进入这部分函数
	if(remoteId >= numsofvehicles)
	{
		//忽略自身节点
		if(remoteId == m_node->GetId())
			return;
		
		NS_ASSERT_MSG(false,"仿真地图中不会进入该函数");
		
		//msgdirection = m_sensor->VehicleGetDistanceWithRSU(nrheader.getX(), nrheader.getY(),forwardId);
		Ptr<pit::Entry> Will = WillInterestedData(data);
		if(!Will)
		{
			//或者改为广播停止转发数据包
			BroadcastStopDataMessage(data);
			return;
		}
		else
		{
			//这部分不一定需要
			// 1.Buffer the data in ContentStore
			ToContentStore(data);
			// 2. Notify upper layer
			NotifyUpperLayer(data);
		}
			
		Ptr<pit::nrndn::EntryNrImpl> entry = DynamicCast<pit::nrndn::EntryNrImpl>(Will);
		const std::unordered_set<std::string>& interestRoutes =entry->getIncomingnbs();
		NS_ASSERT_MSG(interestRoutes.size()!=0,"感兴趣的上一跳路段不该为0");
		
		cout<<"(forwarding.cc-OnData_RSU) 感兴趣的上一跳路段数目为 "<<interestRoutes.size()<<endl;
		
		
		bool idIsInPriorityList;
		std::vector<uint32_t>::const_iterator priorityListIt;
		//找出当前节点是否在优先级列表中
		priorityListIt = find(pri.begin(),pri.end(),m_node->GetId());
		idIsInPriorityList = (priorityListIt != pri.end());
		
		if(idIsInPriorityList)
		{
			cout<<"(forwarding.cc-OnData_RSU) 车辆在数据包转发优先级列表中"<<endl;
			double index = distance(pri.begin(),priorityListIt);
			double random = m_uniformRandomVariable->GetInteger(0, 20);
			Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
			std::pair<std::vector<uint32_t>,std::unordered_set<std::string>> collection = RSUGetPriorityListOfData(data->GetName(),interestRoutes);
			//getchar();
			std::vector<uint32_t> newPriorityList = collection.first;
			std::unordered_set<std::string> remainroutes = collection.second;
			
			//获取该数据包已转发过的上一跳路段
			//std::unordered_set<std::string> forwardedroutes;
			//std::map< uint32_t,std::unordered_set<std::string> >::iterator itrsu = m_RSUforwardedData.find(signature);
			//if(itrsu != m_RSUforwardedData.end())
				//forwardedroutes = itrsu->second;
			
			//for(std::unordered_set<std::string>::const_iterator itinterest = interestRoutes.begin();itinterest != interestRoutes.end();itinterest++)
			//{
				//std::unordered_set<std::string>::iterator itremain = remainroutes.find(*itinterest);
				//if(itremain != remainroutes.end())
					//continue;
				//forwardedroutes.insert(*itinterest);
				//cout<<"(forwarding.cc-OnData_RSU) 准备转发的上一跳路段为 "<<*itinterest<<endl;
			//}
			//if(!forwardedroutes.empty())
				//m_RSUforwardedData[signature] = forwardedroutes;
			
			getchar();
			if(newPriorityList.empty())
			{
				cout<<"(forwarding.cc-OnData_RSU) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<myNodeId<<"准备缓存数据包 "<<signature<<endl;
				//Simulator::Schedule(sendInterval,&NavigationRouteHeuristic::CachingDataPacket,this,signature,data/*,remainroutes*/);
				CachingDataPacket(signature,data);
				//BroadcastStopMessage(data);
				m_sendingDataEvent[nodeId][signature]=
				Simulator::Schedule(sendInterval,
				&NavigationRouteHeuristic::BroadcastStopDataMessage, this, data);
				//getchar();
			}
			else
			{
				m_sendingDataEvent[nodeId][signature]=
				Simulator::Schedule(sendInterval,
				&NavigationRouteHeuristic::ForwardDataPacket, this, data,
				newPriorityList);
				cout<<"(forwarding.cc-OnData_RSU) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<myNodeId<<"准备发送数据包 "<<signature<<endl;
				//getchar();
			}
		}
		else
		{
			//丢掉该数据包
			cout<<"(forwarding.cc-OnData_RSU) Node id is not in PriorityList"<<endl;
			NS_LOG_DEBUG("Node id is not in PriorityList");
			//getchar();
		}
	}
	else
	{
		msgdirection = m_sensor->RSUGetDistanceWithVehicle(m_node->GetId(),nrheader.getX(),nrheader.getY());
		cout<<"(forwarding.cc-OnData_RSU) 数据包的方向为 "<<msgdirection.first<<" "<<msgdirection.second<<endl;
		
		if(m_dataSignatureSeen.Get(data->GetSignature()))
		{
			if(msgdirection.first && msgdirection.second < 0)
			{
				//获取该数据包已转发过的上一跳路段
				std::unordered_set<std::string> forwardedroutes;
				std::map< uint32_t,std::unordered_set<std::string> >::iterator itrsu = m_RSUforwardedData.find(signature);
				if(itrsu != m_RSUforwardedData.end())
					forwardedroutes = itrsu->second;
				
				forwardedroutes.insert(forwardLane);
				m_RSUforwardedData[signature] = forwardedroutes;
			}
			cout<<"数据包 "<<signature<<" 已经发送，上一跳路段为 "<<forwardLane<<endl;
			//getchar();
			NS_LOG_DEBUG("The Data packet has already been sent, do not proceed the packet of "<<data->GetSignature());
			//2018.1.2 RSU有可能重复转发数据包
			//return;
		}
		
		if(!msgdirection.first || msgdirection.second <= 0)// 数据包位于其他路段或当前路段后方
		{
			//第一次收到该数据包
			if(!isDuplicatedData(nodeId,signature))
			{
				//这部分不一定需要 
				Ptr<pit::Entry> Will = WillInterestedData(data);
				if(Will)
				{
					// 1.Buffer the data in ContentStore
					//ToContentStore(data);
					// 2. Notify upper layer
					//NotifyUpperLayer(data);
					
					Ptr<pit::nrndn::EntryNrImpl> entry = DynamicCast<pit::nrndn::EntryNrImpl>(Will);
					const std::unordered_set<std::string>& interestRoutes =entry->getIncomingnbs();
					// 2018.1.6 added by sy
					CachingDataPacket(data->GetSignature(),data/*,interestRoutes*/);
					//BroadcastStopMessage(data);
					cout<<"该数据包第一次从后方或其他路段收到数据包且对该数据包感兴趣"<<endl;
					//cout<<"缓存该数据包"<<endl;
					//getchar();
					return;
				}
				else
				{
					cout<<"该数据包第一次从后方或其他路段收到数据包且当前节点对该数据包不感兴趣"<<endl;
					//DropDataPacket(data);
					//getchar();
					return;
				}
			}
			else // duplicated data
			{
				cout<<"(forwarding.cc-OnData_RSU) 该数据包从后方得到且为重复数据包"<<endl<<endl;
				ExpireDataPacketTimer(nodeId,signature);
				//getchar();
				return;
			}
		}
		//数据包来源于当前路段前方
		else
		{
			if(isDuplicatedData(nodeId,signature))
			{
				cout<<"(forwarding.cc-OnData_RSU) 该数据包从前方或其他路段得到，重复,仍然转发"<<endl;
				//getchar();
				//return;
			}
			
			//缓存数据包
			CachingDataPacket(signature,data);
			
			
			Ptr<pit::Entry> Will = WillInterestedData(data);
			if(!Will)
			{
				//或者改为广播停止转发数据包
				BroadcastStopDataMessage(data);
				cout<<"PIT列表中没有该数据包对应的表项"<<endl;
			//	getchar();
				return;
			}
			else
			{
				//这部分不一定需要
				// 1.Buffer the data in ContentStore
				//ToContentStore(data);
				// 2. Notify upper layer
				//NotifyUpperLayer(data);
			}
			
			
			Ptr<pit::nrndn::EntryNrImpl> entry = DynamicCast<pit::nrndn::EntryNrImpl>(Will);
			const std::unordered_set<std::string>& interestRoutes =entry->getIncomingnbs();
			NS_ASSERT_MSG(interestRoutes.size()!=0,"感兴趣的上一跳路段不该为0");
			
			cout<<"(forwarding.cc-OnData_RSU) 感兴趣的上一跳路段数目为 "<<interestRoutes.size()<<endl;
		
		
			bool idIsInPriorityList;
			std::vector<uint32_t>::const_iterator priorityListIt;
			//找出当前节点是否在优先级列表中
			priorityListIt = find(pri.begin(),pri.end(),m_node->GetId());
			idIsInPriorityList = (priorityListIt != pri.end());
			
			if(idIsInPriorityList)
			{
				cout<<"(forwarding.cc-OnData_RSU) 车辆在数据包转发优先级列表中"<<endl;
				double index = distance(pri.begin(),priorityListIt);
				double random = m_uniformRandomVariable->GetInteger(0, 20);
				Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
				std::pair<std::vector<uint32_t>,std::unordered_set<std::string>> collection = RSUGetPriorityListOfData(data->GetName(),interestRoutes);
				//getchar();
				std::vector<uint32_t> newPriorityList = collection.first;
				std::unordered_set<std::string> remainroutes = collection.second;
				
				//获取该数据包已转发过的上一跳路段
				//std::unordered_set<std::string> forwardedroutes;
				//m_RSUforwardedData[signature] = forwardedroutes;
				getchar();
			
				// 2018.1.15 
				if(newPriorityList.empty())
				{
					//BroadcastStopMessage(data);
					m_sendingDataEvent[nodeId][signature]=
					Simulator::Schedule(sendInterval,
					&NavigationRouteHeuristic::BroadcastStopDataMessage, this, data);
					cout<<"(forwarding.cc-OnData_RSU) 广播停止转发数据包的消息"<<endl;
				}
				else
				{
					m_sendingDataEvent[nodeId][signature]=
					Simulator::Schedule(sendInterval,
					&NavigationRouteHeuristic::ForwardDataPacket, this, data,
					newPriorityList);
					cout<<"(forwarding.cc-OnData_RSU) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<myNodeId<<"准备发送数据包 "<<signature<<endl;
					//getchar();
				}
			}
			else
			{
				cout<<"(forwarding.cc-OnData_RSU) Node id is not in PriorityList"<<endl;
				NS_LOG_DEBUG("Node id is not in PriorityList");
				//getchar();
			}
		}
	}
}

// 2017.12.25 changed by sy
pair<bool, double>
NavigationRouteHeuristic::packetFromDirection(Ptr<Interest> interest)
{
	NS_LOG_FUNCTION (this);
	Ptr<const Packet> nrPayload	= interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	//获取兴趣包的转发节点id
	uint32_t sourceId = nrheader.getSourceId();
	uint32_t forwardId = nrheader.getForwardId();
	//cout<<"sourceId "<<sourceId<<" forwardId "<<forwardId<<endl;
	uint32_t remoteId = (forwardId == 999999999) ? sourceId:forwardId;
	const double x = nrheader.getX();
	const double y = nrheader.getY();
	
	std::string routes = interest->GetRoutes(); 
	//std::cout<<"(forwarding.cc-packetFromDirection) 兴趣包实际转发路线 "<<routes<<std::endl;
	std::size_t found = routes.find(" ");
	std::string currentroute = routes.substr(0,found);
	//std::cout<<"(forwarding.cc-packetFromDirection) 兴趣包当前所在路段 "<<currentroute<<std::endl;
	
	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();
	//cout<<"(forwarding.cc-packetFromDirection) 收到兴趣包的位置" << "x: "<<nrheader.getX() << " " <<"y: "<< nrheader.getY() <<endl;
	//getchar();
	const std::string& currentType = m_sensor->getType();
	//当前节点为RSU
	if(currentType == "RSU")
	{
		if(remoteId >= numsofvehicles)
		{
			std::pair<bool,double> result = m_sensor->RSUGetDistanceWithRSU(remoteId,currentroute);
			//getchar();
			return result;
		}
		else
		{
			std::pair<bool, double> result = m_sensor->RSUGetDistanceWithVehicle(m_node->GetId(),x,y);
			//getchar();
			return result;
		}
	}
	//当前节点为普通车辆
	else
	{
		if(remoteId >= numsofvehicles)
		{
			std::pair<bool,double> result = m_sensor->VehicleGetDistanceWithRSU(x,y,forwardId);
			return result;
		}
		//判断车辆与其他车辆的位置关系
		else
		{
			std::pair<bool, double> result = m_sensor->VehicleGetDistanceWithVehicle(x,y);
			return result;
		}
	}
}

bool NavigationRouteHeuristic::isDuplicatedInterest(
		uint32_t id, uint32_t nonce)
{
	NS_LOG_FUNCTION (this);
	if(!m_sendingInterestEvent.count(id))
		return false;
	else
		return m_sendingInterestEvent[id].count(nonce);
}

void NavigationRouteHeuristic::ExpireInterestPacketTimer(uint32_t nodeId,uint32_t seq)
{
	//cout<<"(forwarding.cc-ExpireInterestPacketTimer) nodeId"<<nodeId<<"sequence "<<seq<<endl;
	NS_LOG_FUNCTION (this<< "ExpireInterestPacketTimer id"<<nodeId<<"sequence"<<seq);
	//1. Find the waiting timer
	if(!isDuplicatedInterest(nodeId,seq))
	{
		//cout<<"(forwarding.cc-ExpireInterestPacketTimer) 第一次收到该兴趣包"<<endl;
		return;
	}
	
	EventId& eventid = m_sendingInterestEvent[nodeId][seq];
	
	//2. cancel the timer if it is still running
	eventid.Cancel();
}

void NavigationRouteHeuristic::CachingInterestPacket(uint32_t nonce, Ptr<Interest> interest)
{
	//获取兴趣的随机编码
	//cout<<"(forwarding.cc-CachingInterestPacket) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<m_node->GetId()<<" 缓存兴趣包 "<<nonce<<endl;
	//std::string routes = interest->GetRoutes();
	//std::cout<<"(forwarding.cc-CachingInterestPacket) routes "<<routes<<std::endl;
	//getchar();
	
	bool result = m_cs->AddInterest(nonce,interest);
	if(result)
	{
		cout<<"(forwarding.cc-CachingInterestPacket) At Time "<<Simulator::Now().GetSeconds()<<"节点 "<<m_node->GetId()<<" 已缓存兴趣包 "<<nonce<<endl;
	}
	else
	{
		//cout<<"(forwarding.cc-CachingInterestPacket) 该兴趣包未能成功缓存"<<endl;
	}
	//getchar();
}

/*
 * 2017.12.29
 * added by sy
 */
void NavigationRouteHeuristic::CachingDataPacket(uint32_t signature,Ptr<Data> data/*,std::unordered_set<std::string> lastroutes*/)
{
	//cout<<"(forwarding.cc-CachingDataPacket)"<<endl;
	//bool result = m_cs->AddData1(signature,data,lastroutes);
	bool result = m_cs->AddData(signature,data);
	if(result)
	{
		cout<<"(forwarding.cc-CachingDataPacket) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<m_node->GetId()<<" 已缓存数据包" <<signature<<endl;
	}
	else
	{
		//cout<<"(forwarding.cc-CachingDataPacket) 该数据包未能成功缓存"<<endl;
	}
	//getchar();
}

void NavigationRouteHeuristic::BroadcastStopInterestMessage(Ptr<Interest> src)
{
	if(!m_running) return;
	cout<<"(forwarding.cc-BroadcastStopInterestMessage) 节点 "<<m_node->GetId()<<" 广播停止转发兴趣包的消息 "<<src->GetNonce()<<endl;
	//cout<<"(forwarding.cc-BroadcastStopMessage) 兴趣包的名字为 "<<src->GetName().toUri() <<endl;
	NS_LOG_FUNCTION (this<<" broadcast a stop message of "<<src->GetName().toUri());
	//1. copy the interest packet
	Ptr<Interest> interest = Create<Interest> (*src);

	//2. set the nack type
	interest->SetNack(Interest::NACK_LOOP);

	//3.Remove the useless payload, save the bandwidth
	Ptr<const Packet> nrPayload=src->GetPayload();
	ndn::nrndn::nrHeader srcheader,dstheader;
	nrPayload->PeekHeader(srcheader);
	dstheader.setSourceId(srcheader.getSourceId());
	//2018.1.16
	dstheader.setForwardId(m_node->GetId());
	
	double x= m_sensor->getX();
	double y= m_sensor->getY();
	dstheader.setX(x);
	dstheader.setY(y);
	
	Ptr<Packet> newPayload	= Create<Packet> ();
	newPayload->AddHeader(dstheader);
	interest->SetPayload(newPayload);

	//4. send the payload
	Simulator::Schedule(MilliSeconds(m_uniformRandomVariable->GetInteger(0,10)),
					&NavigationRouteHeuristic::SendInterestPacket,this,interest);
}

void NavigationRouteHeuristic::ForwardInterestPacket(Ptr<const Interest> src,std::vector<uint32_t> newPriorityList)
{
	if(!m_running) return;
	NS_LOG_FUNCTION (this);
	//cout<<"进入(forwarding.cc-ForwardInterestPacket)"<<endl;
	uint32_t sourceId=0;
	uint32_t nonce=0;

	//记录转发的兴趣包
	// 1. record the Interest Packet(only record the forwarded packet)
	m_interestNonceSeen.Put(src->GetNonce(),true);

	//准备兴趣包
	// 2. prepare the interest
	Ptr<const Packet> nrPayload=src->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	double x= m_sensor->getX();
	double y= m_sensor->getY();
	//const vector<string> route	=
		//	ExtractRouteFromName(src->GetName());
	//const std::vector<uint32_t> priorityList=GetPriorityList(route);
	//const std::vector<uint32_t> priorityList=VehicleGetPriorityListOfInterest(route);
	sourceId=nrheader.getSourceId();
	nonce   =src->GetNonce();
	// source id do not change
	nrheader.setX(x);
	nrheader.setY(y);
	//2017.6.16
	nrheader.setForwardId(m_node->GetId());
	nrheader.setPriorityList(newPriorityList);
	Ptr<Packet> newPayload = Create<Packet> ();
	newPayload->AddHeader(nrheader);

	Ptr<Interest> interest = Create<Interest> (*src);
	interest->SetPayload(newPayload);
	
	// 3. Send the interest Packet. Already wait, so no schedule
	SendInterestPacket(interest);
	
	//m_cs->AddForwardInterest(nonce,interest);
	m_sendInterestTime = Simulator::Now().GetSeconds();	
	
	// 4. record the forward times
	// 2017.12.23 added by sy
	// 若源节点与当前节点相同，则不需要记录转发次数
	uint32_t nodeId = m_node->GetId();
	if(nodeId != sourceId)
	{
		ndn::nrndn::nrUtils::IncreaseInterestForwardCounter(sourceId,nonce);
	}
	
    cout<<"(forwarding.cc-ForwardInterestPacket) 源节点 "<<sourceId<<" 当前节点 "<<m_node->GetId()<<" nonce "<<nonce<<" m_sendInterestTime "<<m_sendInterestTime<<endl;
	//if(sourceId == 97)
		//getchar();
	//getchar();
}

bool NavigationRouteHeuristic::PitCoverTheRestOfRoute(
		const vector<string>& remoteRoute)
{
	NS_LOG_FUNCTION (this);
	//获取本节点的导航路线
	const vector<string>& localRoute =m_sensor->getNavigationRoute();
	
	//获取当前所在的道路
	const string& localLane=m_sensor->getLane();
	vector<string>::const_iterator localRouteIt;
	vector<string>::const_iterator remoteRouteIt;
	localRouteIt  = std::find( localRoute.begin(), localRoute.end(), localLane);
	remoteRouteIt = std::find(remoteRoute.begin(), remoteRoute.end(),localLane);

	//The route is in reverse order in Name, example:R1-R2-R3, the name is /R3/R2/R1
	while (localRouteIt != localRoute.end()
			&& remoteRouteIt != remoteRoute.end())
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
	//cout<<"(NavigationRouteHeuristic.cc-DoInitialize):初始化"<<m_node->GetId()<<std::endl;
	//getchar();
	super::DoInitialize();
	//cout<<"(NavigationRouteHeuristic.cc-DoInitialize):初始化完成"<<std::endl;
	//getchar();
}

void NavigationRouteHeuristic::DropPacket()
{
	NS_LOG_DEBUG ("Drop Packet");
}

void NavigationRouteHeuristic::DropDataPacket(Ptr<Data> data)
{
	NS_LOG_DEBUG ("Drop data Packet");
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
	//cout<<"(forward.cc-DropInterestePacket)"<<endl;
	NS_LOG_DEBUG ("Drop interest Packet");
	DropPacket();
}

void NavigationRouteHeuristic::SendInterestPacket(Ptr<Interest> interest)
{
	if(!m_running) 
		return;
	
	if(HELLO_MESSAGE!=interest->GetScope()||m_HelloLogEnable)
		NS_LOG_FUNCTION (this);
	
	//added by sy
  /*  ndn::nrndn::nrHeader nrheader;
    interest->GetPayload()->PeekHeader(nrheader);
    uint32_t nodeId = nrheader.getSourceId();
	if(nodeId == 22)
	{
		cout<<"(forwarding.cc-sendInterestPacket) 收到了源节点为22的兴趣包"<<endl;
		if(HELLO_MESSAGE == interest->GetScope())
			cout<<"该消息包为心跳包"<<endl;
		if(Interest::NACK_LOOP == interest->GetNack())
			cout<<"该消息包为NACK包"<<endl;
		//getchar();
	}*/
	
		

	//    if the node has multiple out Netdevice face, send the interest package to them all
	//    makde sure this is a NetDeviceFace!!!!!!!!!!!
	vector<Ptr<Face> >::iterator fit;
	for(fit=m_outFaceList.begin();fit!=m_outFaceList.end();++fit)
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
		//cout<<"(forwarding.cc-NotifyNewAggregate)新建NodeSensor"<<endl;
	    m_sensor = GetObject<ndn::nrndn::NodeSensor> ();
    }

    if (m_nrpit == 0)
    {
		//cout<<"(forwarding.cc-NotifyNewAggregate)新建PIT表"<<endl;
	    Ptr<Pit> pit=GetObject<Pit>();
	    if(pit)
			m_nrpit = DynamicCast<pit::nrndn::NrPitImpl>(pit);
	    //cout<<"(forwarding.cc-NotifyNewAggregate)建立完毕"<<endl;
    }
  
  
    if (m_cs == 0)
    {
		//cout<<"(forwarding.cc-NotifyNewAggregate)新建CS-Data"<<endl;
   	    Ptr<ContentStore> cs=GetObject<ContentStore>();
   	    if(cs)
		{
			m_cs = DynamicCast<ns3::ndn::cs::nrndn::NrCsImpl>(cs);
			//cout<<"(forwarding.cc-NotifyNewAggregate)建立完毕"<<endl;
		}
    }
	
  
    if(m_node==0)
    {
	    // cout<<"(forwarding.cc-NotifyNewAggregate)新建Node"<<endl;
	    m_node=GetObject<Node>();
    }

    super::NotifyNewAggregate ();
}

void
NavigationRouteHeuristic::HelloTimerExpire ()
{
	if(!m_running) return;
	//getchar();
	//cout<<endl<<"进入(forwarding.cc-HelloTimerExpire)"<<endl;

	if (m_HelloLogEnable)
		NS_LOG_FUNCTION(this);
	SendHello();

	m_htimer.Cancel();
	Time base(HelloInterval - m_offset);
	m_offset = MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100));
	m_htimer.Schedule(base + m_offset);
}

void
NavigationRouteHeuristic::FindBreaksLinkToNextHop(uint32_t BreakLinkNodeId)
{
	 NS_LOG_FUNCTION (this);
}
/*
void NavigationRouteHeuristic::SetCacheSize(uint32_t cacheSize)
{
	NS_LOG_FUNCTION(this);
	NS_LOG_INFO("Change cache size from "<<m_CacheSize<<" to "<<cacheSize);
	m_CacheSize = cacheSize;
	m_interestNonceSeen=ndn::nrndn::cache::LRUCache<uint32_t,bool>(m_CacheSize);
	m_dataNonceSeen=ndn::nrndn::cache::LRUCache<uint32_t,bool>(m_CacheSize);
}
*/
void
NavigationRouteHeuristic::SendHello()
{
	//cout<<"进入(forwarding.cc-SendHello)"<<endl;
	if(!m_running) return;

	if (m_HelloLogEnable)
		NS_LOG_FUNCTION(this);
	const double& x		= m_sensor->getX();
	const double& y		= m_sensor->getY();
	const string& LaneName=m_sensor->getLane();
	
	//cout<<"(forwarding.cc-SendHello) 普通车辆的数目为 "<<num<<endl;
	//getchar();
	//1.setup name
	Ptr<Name> name = ns3::Create<Name>('/'+LaneName);

	//2. setup payload
	Ptr<Packet> newPayload	= Create<Packet> ();
	ndn::nrndn::nrHeader nrheader;
	nrheader.setX(x);
	nrheader.setY(y);
	nrheader.setSourceId(m_node->GetId());
	
	newPayload->AddHeader(nrheader);

	//3. setup interest packet
	Ptr<Interest> interest	= Create<Interest> (newPayload);
	interest->SetScope(HELLO_MESSAGE);	// The flag indicate it is hello message
	interest->SetName(name); //interest name is lane;
	//interest->SetRoutes(LaneName);//2017.12.25 added by sy
	//4. send the hello message
	SendInterestPacket(interest);
}

void NavigationRouteHeuristic::ProcessHello(Ptr<Interest> interest)
{
	if(!m_running){
		return;
	}
	
	if(m_HelloLogEnable)
		NS_LOG_DEBUG (this << interest << "\tReceived HELLO packet from "<<interest->GetNonce());
	
	Ptr<const Packet> nrPayload	= interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t sourceId = nrheader.getSourceId();
	uint32_t nodeId = m_node->GetId();
	
	//cout<<"(forwarding.cc-ProcessHello) 当前节点 "<<nodeId<<" 发送心跳包的节点 "<<sourceId<<" At time "<<Simulator::Now().GetSeconds()<<endl;
	
	//更新邻居列表
	
	string remoteroute = interest->GetName().get(0).toUri();
	m_nb.Update(sourceId,nrheader.getX(),nrheader.getY(),remoteroute,Time (AllowedHelloLoss * HelloInterval));
	

	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb = m_nb.getNb().begin();
	
	
	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();
	
	// 2017.12.21 获取当前前方邻居数目
	uint32_t nums_car_front = 0;
	// 2018.1.7 获取当前后方邻居数目
	uint32_t nums_car_behind = 0;
	for(;nb != m_nb.getNb().end();++nb)
	{
		//跳过自身节点
		if(nb->first == m_node->GetId())
			continue;
		
		if(nb->first >= numsofvehicles)
		{
			std::pair<bool,double> result = m_sensor->VehicleGetDistanceWithRSU(nb->second.m_x,nb->second.m_y,nb->first);
			
			//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			if(result.first && result.second > 0)
			{
				nums_car_front += 1;
			}
			else if(result.first && result.second < 0)
			{
				nums_car_behind += 1;
			}
			//getchar();
		}
		else
		{
			std::pair<bool, double> result = m_sensor->VehicleGetDistanceWithVehicle(nb->second.m_x,nb->second.m_y);
			//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			if(result.first && result.second > 0)
			{
				nums_car_front += 1;
			}
			else if(result.first && result.second < 0)
			{
				nums_car_behind += 1;
			}
		}
	}
	//cout<<endl<<"(forwarding.cc-ProcessHello) nums_car_behind "<<nums_car_behind<<endl;
	
	
	//前方道路有车辆
	if(nums_car_front > 0)
	{
		//cout<<"(forwarding.cc-ProcessHello) 前方道路有车辆"<<endl;
		//有兴趣包在缓存中
		if(m_cs->GetInterestSize() > 0)
		{
			//cout<<"(forwarding.cc-ProcessHello) 有兴趣包在缓存中"<<endl;
			const string& localLane = m_sensor->getLane();
			//cout<<"(forwarding.cc-ProcessHello) 车辆当前所在路段为 "<<localLane<<endl;
			//获得缓存的兴趣包
			map<uint32_t,Ptr<const Interest> > interestcollection = m_cs->GetInterest(localLane);
			//cout<<"(forwarding.cc-ProcessHello) 获得缓存的兴趣包"<<endl;
		    if(!interestcollection.empty())
			{
				SendInterestInCache(interestcollection);
			}
		}
		else
		{
			//cout<<"(forwarding.cc-ProcessHello) 无兴趣包在缓存中"<<endl;
		}
		//getchar();
		/*if(m_cs->GetForwardInterestSize() > 0)
		{
			cout<<"(forwarding.cc-ProcessHello) 有未转发成功的兴趣包在缓存中"<<endl;
			const string& localLane = m_sensor->getLane();
			map<uint32_t,Ptr<const Interest> > forwardinterestcollection = m_cs->GetForwardInterest(localLane);
			if(!forwardinterestcollection.empty())
			{
				SendForwardInterestInCache(forwardinterestcollection);
			}
		}*/
	}
	
	// 2018.1.7
	if(nums_car_behind > 0)
	{
		if(m_cs->GetDataSize() > 0)
		{
			cout<<"(forwarding.cc-ProcessHello) 有数据包在缓存中"<<endl;
			std::unordered_map<std::string,std::unordered_set<std::string> > dataname_route;
			map<uint32_t,Ptr<const Data> > datacollection = m_cs->GetData();
			if(!datacollection.empty())
			{
				SendDataInCache(datacollection);
				cout<<"(forwarding.cc-ProcessHello) 从缓存中取出数据包"<<endl;
			}
			
			//getchar();
		}
	}
}

void NavigationRouteHeuristic::notifyUpperOnInterest(uint32_t id)
{
	NodeContainer c =NodeContainer::GetGlobal();
	NodeContainer::Iterator it;
	uint32_t idx = 0;
	for(it=c.Begin();it!=c.End();++it)
	{
		Ptr<Application> app=(*it)->GetApplication(ndn::nrndn::nrUtils::appIndex["ns3::ndn::nrndn::nrConsumer"]);
		Ptr<ns3::ndn::nrndn::nrConsumer> consumer = DynamicCast<ns3::ndn::nrndn::nrConsumer>(app);
		//cout << "(nrUtils.cc-GetNodeSizeAndInterestNodeSize) producer " << endl;
		NS_ASSERT(consumer);
		if(!consumer->IsActive())
		{   //非活跃节点直接跳过，避免段错误
	        idx++;
			continue;
		}
		if(idx == id)
		{
			cout<<"(forwarding.cc-notifyUpperOnInterest) idx "<<idx<<endl;
			consumer->SendPacket();
			getchar();
			break;
		}
		idx++;
	}
}

// 2017.12.21 发送缓存的兴趣包
void NavigationRouteHeuristic::SendInterestInCache(std::map<uint32_t,Ptr<const Interest> > interestcollection)
{
	//cout<<"进入(forwarding.cc-SendInterestInCache)"<<endl;
	double interval = Simulator::Now().GetSeconds() - m_sendInterestTime;
	if(interval < 1)
	{
		//cout<<"(forwarding.cc-SendInterestInCache) 时间小于一秒，不转发 m_sendInterestTime "<<m_sendInterestTime<<endl;
		return;
	}
	m_sendInterestTime = Simulator::Now().GetSeconds();
	std::map<uint32_t,Ptr<const Interest> >::iterator it;
	for(it = interestcollection.begin();it != interestcollection.end();it++)
	{
		uint32_t nonce = it->first;
		uint32_t nodeId = m_node->GetId();
		
		//added by sy
		Ptr<const Interest> interest = it->second;
        ndn::nrndn::nrHeader nrheader;
        interest->GetPayload()->PeekHeader(nrheader);
        uint32_t sourceId = nrheader.getSourceId();
		
		cout<<"(forwarding.cc-SendInterestInCache) 兴趣包的nonce "<<nonce<<" 当前节点 "<<nodeId<<" 源节点为 "<<sourceId<<" 当前时间 "<<Simulator::Now().GetSeconds()<<endl;
		
		//cout<<"(forwarding.cc-SendInterestInCache) 兴趣包源节点为 "<<sourceId<<" 兴趣包的实际转发路线为 "<<interest->GetRoutes()<<endl;
		std::vector<std::string> routes;
		SplitString(interest->GetRoutes(),routes," ");
		//cout<<"(forwarding.cc-SendInterestInCache) 兴趣包转发优先级列表为 "<<endl;
		
		std::vector<uint32_t> newPriorityList;
		if(m_sensor->getType() == "RSU")
			newPriorityList = RSUGetPriorityListOfInterest(routes[0]);
		else
		    newPriorityList = VehicleGetPriorityListOfInterest();
		
		for(uint32_t i = 0;i < newPriorityList.size();i++)
		{
			cout<<newPriorityList[i]<<" ";
		}
		cout<<endl;
		double random = m_uniformRandomVariable->GetInteger(0,100);
		Time sendInterval(MilliSeconds(random));
		m_sendingInterestEvent[sourceId][nonce] = Simulator::Schedule(sendInterval,&NavigationRouteHeuristic::ForwardInterestPacket,this,interest,newPriorityList);
		//if(sourceId == 97)
			//getchar();
		//getchar();
	}
}

/*void NavigationRouteHeuristic::SendForwardInterestInCache(std::map<uint32_t,Ptr<const Interest> > forwardinterestcollection)
{
	cout<<"进入(forwarding.cc-SendForwardInterestInCache)"<<endl;
	//增加一个时间限制，超过1s才进行转发
	double interval = Simulator::Now().GetSeconds() - m_sendInterestTime;
	if(interval < 1)
	{
		cout<<"(forwarding.cc-SendForwardInterestInCache) 时间小于一秒，不转发 m_sendInterestTime "<<m_sendInterestTime<<endl;
		return;
	}
	
	std::map<uint32_t,Ptr<const Interest> >::iterator it;
	for(it = forwardinterestcollection.begin();it != forwardinterestcollection.end();it++)
	{
		uint32_t nonce = it->first;
		uint32_t nodeId = m_node->GetId();
		
		//added by sy
		Ptr<const Interest> interest = it->second;
        ndn::nrndn::nrHeader nrheader;
        interest->GetPayload()->PeekHeader(nrheader);
        uint32_t sourceId = nrheader.getSourceId();
		
		cout<<"(forwarding.cc-SendForwardInterestInCache) 兴趣包的nonce "<<nonce<<" 当前节点 "<<nodeId<<" 源节点为 "<<sourceId<<" 当前时间 "<<Simulator::Now().GetSeconds()<<endl;
		
		//cout<<"(forwarding.cc-SendInterestInCache) 兴趣包源节点为 "<<sourceId<<" 兴趣包的实际转发路线为 "<<interest->GetRoutes()<<endl;
		std::vector<std::string> routes;
		SplitString(interest->GetRoutes(),routes," ");
		//cout<<"(forwarding.cc-SendInterestInCache) 兴趣包转发优先级列表为 "<<endl;
		
		std::vector<uint32_t> newPriorityList;
		if(m_sensor->getType() == "RSU")
			newPriorityList = RSUGetPriorityListOfInterest(routes[0]);
		else
		    newPriorityList = VehicleGetPriorityListOfInterest();
		
		for(uint32_t i = 0;i < newPriorityList.size();i++)
		{
			cout<<newPriorityList[i]<<" ";
		}
		cout<<endl;
		double random = m_uniformRandomVariable->GetInteger(0,100);
		Time sendInterval(MilliSeconds(random));
		m_sendingInterestEvent[nodeId][nonce] = Simulator::Schedule(sendInterval,&NavigationRouteHeuristic::ForwardInterestPacket,this,interest,newPriorityList);
		//if(sourceId == 97)
			//getchar();
		getchar();
	}
}*/

void NavigationRouteHeuristic::SendDataInCache(std::map<uint32_t,Ptr<const Data> > datacollection)
{
	double interval = Simulator::Now().GetSeconds() - m_sendDataTime;
	if(interval < 1)
	{
		//cout<<"(forwarding.cc-SendDataInCache) 时间小于一秒，不转发 m_sendDataTime "<<m_sendDataTime<<endl;
		return;
	}
	m_sendDataTime = Simulator::Now().GetSeconds();
	
	std::map<uint32_t,Ptr<const Data>>::iterator it;
	for(it = datacollection.begin();it != datacollection.end();it++)
	{
		uint32_t signature = it->first;
		uint32_t nodeId = m_node->GetId();
		Ptr<const Data> data = it->second;
		
		//added by sy
        ndn::nrndn::nrHeader nrheader;
        data->GetPayload()->PeekHeader(nrheader);
        uint32_t sourceId = nrheader.getSourceId();
		
		std::vector<uint32_t> newPriorityList;
		
		if(m_sensor->getType() == "RSU")
		{
			Ptr<pit::Entry> Will = WillInterestedData(data);
			if(!Will)
			{
				NS_ASSERT_MSG(false,"RSU对该数据包不感兴趣");
			}
			else
			{
				Ptr<pit::nrndn::EntryNrImpl> entry = DynamicCast<pit::nrndn::EntryNrImpl>(Will);
				const std::unordered_set<std::string>& interestRoutes =entry->getIncomingnbs();
				NS_ASSERT_MSG(interestRoutes.size()!=0,"感兴趣的上一跳路段不该为0");
				
				//获取该数据包已转发过的上一跳路段
				std::unordered_set<std::string> forwardedroutes;
				std::map< uint32_t,std::unordered_set<std::string> >::iterator itrsu = m_RSUforwardedData.find(signature);
				if(itrsu != m_RSUforwardedData.end())
					forwardedroutes = itrsu->second;
				
				std::unordered_set<std::string> newinterestRoutes;
				if(forwardedroutes.empty())
				{
					newinterestRoutes = interestRoutes;
					cout<<"(forwarding.cc-SendDataInCache) 数据包 "<<signature<<" 对应的上一跳路段全部未转发过"<<endl;
				}
				else
				{
					//从上一跳路段中去除已转发过的路段
					for(std::unordered_set<std::string>::const_iterator itinterest = interestRoutes.begin();itinterest != interestRoutes.end();itinterest++)
					{
						std::unordered_set<std::string>::iterator itforward = forwardedroutes.find(*itinterest);
						if(itforward != forwardedroutes.end())
						{
							cout<<"(forwarding.cc-SendDataInCache) 数据包 "<<signature<<" 已转发过的上一跳路段为 "<<*itinterest<<endl;
							continue;
						}
						newinterestRoutes.insert(*itinterest);
						cout<<"(forwarding.cc-SendDataInCache) 数据包 "<<signature<<" 未转发过的上一跳路段为 "<<*itinterest<<endl;
					}
				}
				
				if(newinterestRoutes.empty())
				{
					cout<<"数据包 "<<signature<<" 对应的上一跳路段全部转发过"<<endl;
					continue;
				}
					
				
				std::pair<std::vector<uint32_t>,std::unordered_set<std::string>> collection = RSUGetPriorityListOfData(data->GetName(),newinterestRoutes);
				newPriorityList = collection.first;
				std::unordered_set<std::string> remainroutes = collection.second;
				
				if(newPriorityList.empty())
				{
					cout<<"该数据包对应的未转发过的上一跳路段为空"<<endl;
					continue;
				}
			
				//加入准备转发的上一跳路段
				//for(std::unordered_set<std::string>::iterator itinterest = newinterestRoutes.begin();itinterest != newinterestRoutes.end();itinterest++)
				//{
					//std::unordered_set<std::string>::iterator itremain = remainroutes.find(*itinterest);
					//if(itremain != remainroutes.end())
						//continue;
					//forwardedroutes.insert(*itinterest);
					//cout<<"准备转发的上一跳路段为 "<<*itinterest<<endl;
				//}
				//if(!forwardedroutes.empty())
					//m_RSUforwardedData[signature] = forwardedroutes;
			}
		}
		else
		{
			newPriorityList = VehicleGetPriorityListOfData();
		}
			
		cout<<"(forwarding.cc-SendDataInCache) 数据包的signature "<<signature<<" 当前节点 "<<nodeId<<" 源节点为 "<<sourceId<<endl;	
		cout<<"(forwarding.cc-SendDataInCache) 数据包转发优先级列表为 "<<endl;	
		for(uint32_t i = 0;i < newPriorityList.size();i++)
		{
			cout<<newPriorityList[i]<<" ";
		}
		cout<<endl;
		
		getchar();
		
		double random = m_uniformRandomVariable->GetInteger(0,100);
		Time sendInterval(MilliSeconds(random));
		m_sendingDataEvent[sourceId][signature] = Simulator::Schedule(sendInterval,&NavigationRouteHeuristic::ForwardDataPacket,this,data,newPriorityList);
	}
}


void NavigationRouteHeuristic::ProcessHelloRSU(Ptr<Interest> interest)
{
	if(!m_running){
		return;
	}
	if(m_HelloLogEnable)
		NS_LOG_DEBUG (this << interest << "\tReceived HELLO packet from "<<interest->GetNonce());
	
	Ptr<const Packet> nrPayload	= interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t sourceId = nrheader.getSourceId();
	uint32_t nodeId = m_node->GetId();
	//const std::string& lane = m_sensor->getLane();
	//获取心跳包所在路段
	string remoteroute = interest->GetName().get(0).toUri();
	
	//cout<<"(forwarding.cc-ProcessHelloRSU) 当前节点 "<<nodeId<<" 发送心跳包的节点 "<<sourceId<<" At time "<<Simulator::Now().GetSeconds()<<endl;
	//cout<<"(forwarding.cc-ProcessHelloRSU) 心跳包当前所在路段为 "<<remoteroute<<endl;
	
	std::string junctionid = m_sensor->RSUGetJunctionId(nodeId);
	//cout<<"(forwarding.cc-ProcessHelloRSU) 交点ID为 "<<junctionid<<endl;
	
	//更新邻居列表
	m_nb.Update(sourceId,nrheader.getX(),nrheader.getY(),remoteroute,Time (AllowedHelloLoss * HelloInterval));
	
	pair<bool, double> msgdirection = packetFromDirection(interest);
	//cout<<"(forwarding.cc-ProcessHelloRSU) 心跳包的位置为 "<<msgdirection.first<<" "<<msgdirection.second<<endl;
	
	//发送心跳包的节点位于当前节点后方
	if(msgdirection.first && msgdirection.second < 0)
	{
		overtake.insert(sourceId);
	}
	else if(msgdirection.first && msgdirection.second > 0)
	{
		std::unordered_set<uint32_t>::iterator it = overtake.find(sourceId);
		//该车辆超车
		if(it != overtake.end())
		{
			cout<<"(forwarding.cc-ProcessHelloRSU) 当前节点 "<<nodeId<<" 发送心跳包的节点 "<<sourceId<<" At time "<<Simulator::Now().GetSeconds()<<endl;
			cout<<"(forwarding.cc-ProcessHelloRSU) 心跳包的位置为 "<<msgdirection.first<<" "<<msgdirection.second<<endl;
			std::pair<bool,uint32_t> resend = m_nrpit->DeleteFrontNode(remoteroute,sourceId);
			overtake.erase(it);
			
			cout<<"(forwarding.cc-ProcessHelloRSU) 车辆 "<<sourceId<<" 从PIT中删除该表项"<<endl;
			//getchar();
			if(resend.first == false)
				notifyUpperOnInterest(resend.second);
		}
		else
		{
			//cout<<"(forwarding.cc-ProcessHelloRSU) 车辆 "<<sourceId<<"一直位于前方"<<endl;
		}
	}
	
	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();
	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb = m_nb.getNb().begin();
	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator prenb = m_preNB.getNb().begin();
	
	// 2017.12.27 added by sy
	// routes_front代表有车辆的前方路段集合
	std::unordered_set<std::string> routes_front;
	std::unordered_set<std::string>::iterator itroutes_front;
	
	//2018.1.8 added by sy
	// routes_behind代表有车辆的后方路段集合
	std::unordered_set<std::string> routes_behind;
	std::unordered_set<std::string>::iterator itroutes_behind;
	
	for(;nb != m_nb.getNb().end();++nb)
	{
		if(nb->first >= numsofvehicles)
		{
			std::pair<bool,double> result = m_sensor->RSUGetDistanceWithRSU(nb->first,nb->second.m_lane);
			//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			if(result.first && result.second > 0)
			{
				//cout<<"(forwarding.cc-ProcessHelloRSU) 路段 "<<nb->second.m_lane<<"前方有车辆"<<endl;
				routes_front.insert(itroutes_front,nb->second.m_lane);
			}
			else if(result.first && result.second <0)
			{
				//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
				routes_behind.insert(itroutes_behind,nb->second.m_lane);
			}
		//getchar();
		}
		else
		{
			std::pair<bool, double> result = m_sensor->RSUGetDistanceWithVehicle(m_node->GetId(),nb->second.m_x,nb->second.m_y);
			//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			if(result.first && result.second > 0)
			{
				//cout<<"(forwarding.cc-ProcessHelloRSU) 路段 "<<nb->second.m_lane<<"前方有车辆"<<endl;
				routes_front.insert(itroutes_front,nb->second.m_lane);
			}
			else if(result.first && result.second <0)
			{
				//cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
				routes_behind.insert(itroutes_behind,nb->second.m_lane);
			}
		}
	}
	//cout<<endl;
	//getchar();
	
	if(routes_front.size() > 0 && m_cs->GetInterestSize() > 0)
	{
		for(itroutes_front = routes_front.begin();itroutes_front != routes_front.end();itroutes_front++)
		{
			//cout<<"(forwarding.cc-ProcessHelloRSU) 路段 "<<*itroutes_front<<"有车辆"<<endl;
			map<uint32_t,Ptr<const Interest> > interestcollection = m_cs->GetInterest(*itroutes_front);
			if(!interestcollection.empty())
			{
				cout<<"(forwarding.cc-ProcessHelloRSU) 获得缓存的兴趣包"<<endl;
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
	
	/*if(routes_front.size() > 0 && m_cs->GetForwardInterestSize() > 0)
	{
		cout<<"(forwarding.cc-ProcessHelloRSU) 有未转发成功的兴趣包在缓存中"<<endl;
		for(itroutes_front = routes_front.begin();itroutes_front != routes_front.end();itroutes_front++)
		{
			//cout<<"(forwarding.cc-ProcessHelloRSU) 路段 "<<*itroutes_front<<"有车辆"<<endl;
			map<uint32_t,Ptr<const Interest> > forwardinterestcollection = m_cs->GetForwardInterest(*itroutes_front);
			if(!forwardinterestcollection.empty())
			{
				cout<<"(forwarding.cc-ProcessHelloRSU) 获得缓存的兴趣包"<<endl;
				SendForwardInterestInCache(forwardinterestcollection);
			}
			//getchar();
		}
	}*/
	if(routes_behind.size() > 0 && m_cs->GetDataSize() > 0)
	{
		cout<<"(forwarding.cc-ProcessHelloRSU) 当前节点 "<<nodeId<<" 发送心跳包的节点 "<<sourceId<<" At time "<<Simulator::Now().GetSeconds()<<endl;
		//cout<<"(forwarding.cc-ProcessHelloRSU) 心跳包的位置为 "<<msgdirection.first<<" "<<msgdirection.second<<endl;
		
		cout<<"(forwarding.cc-ProcessHelloRSU) 有车辆的路段为 "<<endl;
		for(std::unordered_set<std::string>::iterator it = routes_behind.begin();it != routes_behind.end();it++)
		{
			cout<<*it<<" ";
		}
		cout<<endl;
		//cout<<"(forwarding.cc-ProcessHelloRSU) routes_behind的大小为 "<<routes_behind.size()<<endl;
	
		//cout<<"(forwarding.cc-ProcessHelloRSU)数据包对应的上一跳路段为 "<<endl;
		
		std::unordered_map<std::string,std::unordered_set<std::string> > dataandroutes = m_nrpit->GetDataNameandLastRoute(routes_behind);
		
		/*for(std::unordered_map<std::string,std::unordered_set<std::string>>::iterator it = dataandroutes.begin();it != dataandroutes.end();it++)
		{
			//cout<<"数据包名称为 "<<it->first<<endl;
			//cout<<"对应的上一跳节点为 ";
			std::unordered_set<std::string> collection = it->second;
			for(std::unordered_set<std::string>::iterator itcollect = collection.begin();itcollect != collection.end();itcollect++)
			{
				cout<<*itcollect<<" ";
			}
			cout<<endl;
		}*/
		
		std::map<uint32_t,Ptr<const Data> > datacollection = m_cs->GetData(dataandroutes);
		if(!datacollection.empty())
		{
			SendDataInCache(datacollection);
			cout<<"(forwarding.cc-ProcessHelloRSU) 获得缓存的数据包"<<endl;
		}
		//getchar();
	}
	m_preNB = m_nb;
	
}

vector<string> NavigationRouteHeuristic::ExtractRouteFromName(const Name& name)
{
	// Name is in reverse order, so reverse it again
	// eg. if the navigation route is R1-R2-R3, the name is /R3/R2/R1
	vector<string> result;
	Name::const_reverse_iterator it;
	//Question:有非字符内容输出
	//cout<<"(forwarding.cc-ExtractRouteFromName) 得到该兴趣包的兴趣路线："<<endl;
	for(it=name.rbegin();it!=name.rend();++it)
	{
		result.push_back(it->toUri());
		//cout<<it->toUri()<<endl;
	}
	return result;
}

Ptr<Packet> NavigationRouteHeuristic::GetNrPayload(HeaderHelper::Type type, Ptr<const Packet> srcPayload, uint32_t forwardId, const Name& dataName /*= *((Name*)NULL) */)
{
	NS_LOG_INFO("Get nr payload, type:"<<type);
	Ptr<Packet> nrPayload = Create<Packet>(*srcPayload);
	std::vector<uint32_t> priorityList;
	
	switch (type)
	{
		case HeaderHelper::INTEREST_NDNSIM:
		{
			priorityList = VehicleGetPriorityListOfInterest();
			//cout<<"(forwarding.cc-GetNrPayload)Node "<<m_node->GetId()<<"的兴趣包转发优先级列表大小为 "<<priorityList.size()<<endl;
			//getchar();
			break;
		}
		case HeaderHelper::CONTENT_OBJECT_NDNSIM:
		{
			priorityList = VehicleGetPriorityListOfData();
			//There is no interested nodes behind
			// 2017.12.29 changed by sy
			/*if(priorityList.empty())
			{
				//added by sy
				cout<<"(forwarding.cc-GetNrPayload) NodeId: "<<m_node->GetId()<<" 的数据包转发优先级列表为空"<<endl;
				return Create<Packet>();
			}*/			
			break;
		}
		default:
		{
			NS_ASSERT_MSG(false, "unrecognize packet type");
			break;
		}
	}

	const double& x = m_sensor->getX();
	const double& y = m_sensor->getY();
	ndn::nrndn::nrHeader nrheader(m_node->GetId(), x, y, priorityList);
	nrheader.setForwardId(forwardId);
	nrheader.setLane(m_sensor->getLane());
	nrPayload->AddHeader(nrheader);
	return nrPayload;
}



void NavigationRouteHeuristic::ExpireDataPacketTimer(uint32_t nodeId,uint32_t signature)
{
	//NS_ASSERT_MSG(false,"NavigationRouteHeuristic::ExpireDataPacketTimer");
	NS_LOG_FUNCTION (this<< "ExpireDataPacketTimer id\t"<<nodeId<<"\tsignature:"<<signature);
	//cout<<"(forwarding.cc-ExpireDataPacketTimer) NodeId: "<<nodeId<<" signature: "<<signature<<endl;
	//1. Find the waiting timer
	if(!isDuplicatedData(nodeId,signature))
	{
		//cout<<"(forwarding.cc-ExpireDataPacketTimer) 第一次收到该数据包"<<endl;
		return;
	}
	
	EventId& eventid = m_sendingDataEvent[nodeId][signature];

	//2. cancel the timer if it is still running
	eventid.Cancel();
}

Ptr<pit::Entry>
NavigationRouteHeuristic::WillInterestedData(Ptr<const Data> data)
{
	//NS_ASSERT_MSG(false,"NavigationRouteHeuristic::isInterestedData");
	return m_pit->Find(data->GetName());
}

bool NavigationRouteHeuristic::IsInterestData(const Name& name)
{
	std::vector<std::string> result;
	//获取当前路段
	const std::string& currentLane = m_sensor->getLane();
	std::vector<std::string>::const_iterator it;
	std::vector<std::string>::const_iterator it2;
	//获取导航路线
	const std::vector<std::string>& route = m_sensor->getNavigationRoute();

	//当前路段在导航路线中的位置
	it =std::find(route.begin(),route.end(),currentLane);

	//name是否会出现在未来的导航路线中
	//如是，则证明该节点对该数据感兴趣；否则不感兴趣
	it2=std::find(it,route.end(),name.get(0).toUri());

	return (it2!=route.end());
}

bool NavigationRouteHeuristic::isDuplicatedData(uint32_t id, uint32_t signature)
{
	NS_LOG_FUNCTION (this);
	//NS_ASSERT_MSG(false,"NavigationRouteHeuristic::isDuplicatedData");
	if(!m_sendingDataEvent.count(id))
		return false;
	else
		return m_sendingDataEvent[id].count(signature);
}

void NavigationRouteHeuristic::BroadcastStopDataMessage(Ptr<Data> src)
{
	if(!m_running) return;
	//cout<<"进入(forwarding.cc-BroadcastStopMessage)"<<endl;
	//NS_ASSERT_MSG(false,"NavigationRouteHeuristic::BroadcastStopMessage(Ptr<Data> src)");

	cout<<"(forwarding.cc-BroadcastStopDataMessage) 节点 "<<m_node->GetId()<<" 广播停止转发数据包的消息 "<<src->GetSignature()<<endl;
	NS_LOG_FUNCTION (this<<" broadcast a stop message of "<<src->GetName().toUri());
	//1. copy the data packet
	/*Ptr<Data> data = Create<Data> (*src);

	//2.Remove the useless payload, save the bandwidth
	Ptr<const Packet> nrPayload=src->GetPayload();
	ndn::nrndn::nrHeader srcheader,dstheader;
	nrPayload->PeekHeader(srcheader);
	
	dstheader.setSourceId(srcheader.getSourceId());//Stop message contains an empty priority list
	Ptr<Packet> newPayload = Create<Packet> ();
	newPayload->AddHeader(dstheader);
	data->SetPayload(newPayload);*/
	
	Ptr<Packet> nrPayload = src->GetPayload()->Copy();
	ndn::nrndn::nrHeader srcheader,dstheader;
	nrPayload->PeekHeader(srcheader);
	std::vector<uint32_t> newPriorityList;
	dstheader.setSourceId(srcheader.getSourceId());//Stop message contains an empty priority list
	
	dstheader.setForwardId(m_node->GetId());
	
	// 2018.1.19
	double x= m_sensor->getX();
	double y= m_sensor->getY();
	dstheader.setX(x);
	dstheader.setY(y);
	dstheader.setLane(m_sensor->getLane());
	
	Ptr<Packet> newPayload = Create<Packet> ();
	newPayload->AddHeader(dstheader);
	
	Ptr<Data> data = Create<Data> (*src);
	data->SetPayload(newPayload);

	//4. send the payload
	Simulator::Schedule(MilliSeconds(m_uniformRandomVariable->GetInteger(0,10)),
					&NavigationRouteHeuristic::SendDataPacket,this,data);
}

void NavigationRouteHeuristic::ForwardDataPacket(Ptr<const Data> src,std::vector<uint32_t> newPriorityList)
{
	if(!m_running) return;
	//NS_ASSERT_MSG(false,"NavigationRouteHeuristic::ForwardDataPacket");
	NS_LOG_FUNCTION (this);
	uint32_t sourceId=0;
	uint32_t signature=0;
	// 1. record the Data Packet(only record the forwarded packet)
	m_dataSignatureSeen.Put(src->GetSignature(),true);

	// 2. prepare the data
	Ptr<Packet> nrPayload=src->GetPayload()->Copy();
	//Ptr<Packet> newPayload = Create<Packet> ();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->RemoveHeader(nrheader);
	double x= m_sensor->getX();
	double y= m_sensor->getY();
	sourceId = nrheader.getSourceId();
	signature = src->GetSignature();
	cout << "(forwarding.cc-ForwardDataPacket) 当前节点 " <<m_node->GetId() << " 源节点 "<< sourceId << " Signature " << signature << endl;
	//getchar();
	
	// 	2.1 setup nrheader, source id do not change
	nrheader.setX(x);
	nrheader.setY(y);
	nrheader.setForwardId(m_node->GetId());
	nrheader.setLane(m_sensor->getLane());
	//cout<<"(forward.cc-ForwardDataPacket) 转发数据包，当前道路为"<<nrheader.getLane()<<endl;
	//getchar();
	
	nrheader.setPriorityList(newPriorityList);
	nrPayload->AddHeader(nrheader);

	// 	2.2 setup FwHopCountTag
	//FwHopCountTag hopCountTag;
	//if (!IsClearhopCountTag)
	//	nrPayload->PeekPacketTag( hopCountTag);
    //	newPayload->AddPacketTag(hopCountTag);

	// 	2.3 copy the data packet, and install new payload to data
	Ptr<Data> data = Create<Data> (*src);
	data->SetPayload(nrPayload);

	// 3. Send the data Packet. Already wait, so no schedule
	SendDataPacket(data);

	// 4. record the forward times
	ndn::nrndn::nrUtils::IncreaseForwardCounter(sourceId,signature);
}



void NavigationRouteHeuristic::SendDataPacket(Ptr<Data> data)
{
	if(!m_running) return;
	//cout<<"进入(forwarding.cc-sendDataPacket)"<<endl;
	//NS_ASSERT_MSG(false,"NavigationRouteHeuristic::SendDataPacket");
	vector<Ptr<Face> >::iterator fit;
	for (fit = m_outFaceList.begin(); fit != m_outFaceList.end(); ++fit)
	{
		(*fit)->SendData(data);
		//cout<<"(forwarding.cc-SendDataPacket) 数据包"<<m_node->GetId()<<" "<<Simulator::Now().GetSeconds()<<endl;
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
	cout<<"(forwarding.cc-VehicleGetPriorityListOfData) 当前节点为 "<<m_node->GetId()<<" 时间为 "<<Simulator::Now().GetSeconds()<<endl;
	std::multimap<double,uint32_t,std::greater<double> > sortlistRSU;
	std::multimap<double,uint32_t,std::greater<double> > sortlistVehicle;

	// step 1. 寻找位于导航路线后方的一跳邻居列表,m_nb为邻居列表
	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb;
	
	for(nb = m_nb.getNb().begin();nb != m_nb.getNb().end();++nb)
	{
		//判断车辆与RSU的位置关系
		if(nb->first >= numsofvehicles)
		{
			std::pair<bool,double> result = m_sensor->VehicleGetDistanceWithRSU(nb->second.m_x,nb->second.m_y,nb->first);
			
			cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			if(result.first && result.second < 0)
			{
				sortlistRSU.insert(std::pair<double,uint32_t>(-result.second,nb->first));
			}
			//getchar();
		}
		//判断车辆与其他车辆的位置关系
		else
		{
			//忽略自身节点
			if(m_node->GetId() == nb->first)
				continue;
			
			std::pair<bool, double> result = m_sensor->VehicleGetDistanceWithVehicle(nb->second.m_x,nb->second.m_y);
			cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			//若result.second <= 0,会将自身加入转发优先级列表中
			if(result.first && result.second < 0)
			{
				sortlistVehicle.insert(std::pair<double,uint32_t>(-result.second,nb->first));
			}
		}
	}
	cout<<endl<<"加入RSU：";
	// step 2. Sort By Distance Descending
	std::multimap<double,uint32_t>::iterator it;
	// 加入RSU
	for(it=sortlistRSU.begin();it!=sortlistRSU.end();++it)
	{
		PriorityList.push_back(it->second);
		cout<<" "<<it->second;
	}
	cout<<endl<<"加入普通车辆：";
	// 加入普通车辆
	for(it=sortlistVehicle.begin();it!=sortlistVehicle.end();++it)
	{
		PriorityList.push_back(it->second);
		cout<<" "<<it->second;
	}	
	cout<<endl;
	//getchar();
	return PriorityList;
}

/*
 * 2017.12.29
 * added by sy
 * interestRoutes代表对该数据感兴趣的路段集合
 * dataName为数据包的名字
 * RSU获取数据包转发优先级列表
 */
std::pair<std::vector<uint32_t>,std::unordered_set<std::string>> NavigationRouteHeuristic::RSUGetPriorityListOfData(const Name& dataName,const std::unordered_set<std::string>& interestRoutes)
{
	std::vector<uint32_t> priorityList;
	std::pair<std::vector<uint32_t>,std::unordered_set<std::string>> collection;
	//无车辆的感兴趣路段
	std::unordered_set<std::string> remainroutes(interestRoutes);
	std::unordered_map<std::string,std::multimap<double,uint32_t,std::greater<double> > > sortvehicles;
	std::unordered_map<std::string,std::multimap<double,uint32_t,std::greater<double> > > ::iterator itvehicles;
	
	//added by sy
	cout<<"(forwarding.cc-RSUGetPriorityListOfData) At time:"<<Simulator::Now().GetSeconds()<<" 当前节点 "<<m_node->GetId()<<" Current dataName:"<<dataName.toUri()<<endl;
	//m_nrpit->showPit();
	
	Ptr<pit::nrndn::EntryNrImpl> entry = DynamicCast<pit::nrndn::EntryNrImpl>(m_nrpit->Find(dataName));
	NS_ASSERT_MSG(entry != 0,"该数据包不在PIT中");
	if(entry == 0)
	{
		cout<<"该数据包并不在PIT中"<<endl;
		return collection;
	}
	
	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();
	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb;
	for(nb = m_nb.getNb().begin();nb != m_nb.getNb().end();++nb)
	{
		//判断RSU与RSU的位置关系
		if(nb->first >= numsofvehicles)
		{
			//忽略自身节点
			if(nb->first == m_node->GetId())
				continue;
			
			//获取另一RSU所在的交点id
			std::string junction = m_sensor->RSUGetJunctionId(nb->first);
			std::unordered_set<std::string>::const_iterator it = interestRoutes.begin();
			//遍历路段集合，查看RSU是否为路段终点
			for(;it != interestRoutes.end();it++)
			{
				//获得路段的起点和终点
				std::pair<std::string,std::string> junctions = m_sensor->GetLaneJunction(*it);
				//感兴趣路段的终点ID和另一RSU的交点ID相同
				if(junctions.second == junction)
				{
					cout<<"(forwarding.cc-RSUGetPriorityListOfData) 上一跳路段为 "<<*it<<" RSU交点ID为 "<<junction<<endl;
					std::pair<bool,double> result = m_sensor->RSUGetDistanceWithRSU(nb->first,nb->second.m_lane);
					cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<endl;
					
					itvehicles = sortvehicles.find(*it);
					//sortvehicles中已经有该路段
					if(itvehicles != sortvehicles.end())
					{
						std::multimap<double,uint32_t,std::greater<double> > distance = itvehicles->second;
						distance.insert(std::pair<double,uint32_t>(-result.second,nb->first));
						sortvehicles[*it] = distance; 
					}
					else
					{
						std::multimap<double,uint32_t,std::greater<double> > distance;
						distance.insert(std::pair<double,uint32_t>(-result.second,nb->first));
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
			NS_ASSERT_MSG(lane != "","lane的长度为空");
			std::unordered_set<std::string>::const_iterator it = interestRoutes.find(lane);
			//车辆位于数据包下一行驶路段
			if(it != interestRoutes.end())
			{
				std::pair<bool,double> result = m_sensor->RSUGetDistanceWithVehicle(m_node->GetId(),nb->second.m_x,nb->second.m_y);
				cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<endl;
				if(result.first && result.second < 0)
				{
					itvehicles = sortvehicles.find(*it);
					//sortvehicles中已经有该路段
					if(itvehicles != sortvehicles.end())
					{
						std::multimap<double,uint32_t,std::greater<double> > distance = itvehicles->second;
						distance.insert(std::pair<double,uint32_t>(-result.second,nb->first));
						sortvehicles[*it] = distance; 
					}
					//sortvehicles中无该路段
					else
					{
						std::multimap<double,uint32_t,std::greater<double> > distance;
						distance.insert(std::pair<double,uint32_t>(-result.second,nb->first));
						sortvehicles[*it] = distance; 
					}
				}
			}
			else
			{
				cout<<"(forwarding.cc-RSUGetPriorityListOfData) 车辆 "<<nb->first<<" 所在路段为 "<<nb->second.m_lane<<" 不在上一跳路段中"<<endl;
			}
		}
	}
	
	
	cout<<"(forwarding.cc-RSUGetPriorityListOfData) 输出各路段以及对应的车辆和距离"<<endl;
	std::unordered_set<std::string>::iterator itroutes;
	
	//加入每个路段距离最远的节点
	cout<<"(forwrading.cc-RSUGetPriorityListOfData) 每个路段距离最远的节点为"<<endl;
	std::multimap<double,uint32_t,std::greater<double> > farthest;
	for(itvehicles = sortvehicles.begin();itvehicles != sortvehicles.end();itvehicles++)
	{
		//从remainroutes中删除存在车辆的路段
		itroutes = remainroutes.find(itvehicles->first);
		if(itroutes != remainroutes.end())
		{
			remainroutes.erase(itroutes);
		}
		
		//位于该条路段的车辆集合
		std::multimap<double,uint32_t,std::greater<double> > distance = itvehicles->second;
		//位于该条路段距离最远的车辆
		std::multimap<double,uint32_t,std::greater<double> >::iterator itdis = distance.begin();
		cout<<"上一跳路段为 "<<itvehicles->first<<" 最远车辆为 ("<<itdis->second<<" "<<itdis->first<<")"<<endl;
		farthest.insert(std::pair<double,uint32_t>(itdis->first,itdis->second));
	}
	
	//将每个路段最远节点加入转发优先级列表中
	std::multimap<double,uint32_t,std::greater<double> >::iterator itfarthest = farthest.begin();
	for(;itfarthest != farthest.end();itfarthest++)
	{
		priorityList.push_back(itfarthest->second);
	}
	
	cout<<endl<<"(forwarding.cc-RSUGetPriorityListOfData) 加入每个路段剩余节点"<<endl;
	//加入剩余节点
	std::multimap<double,uint32_t,std::greater<double> >remainnodes;
	for(itvehicles = sortvehicles.begin();itvehicles != sortvehicles.end();itvehicles++)
	{
		//输出路段名
		cout<<itvehicles->first<<endl;
		std::multimap<double,uint32_t,std::greater<double> > distance = itvehicles->second;
		std::multimap<double,uint32_t,std::greater<double> >::iterator itdis = distance.begin();
		++itdis;
		for(;itdis != distance.end();itdis++)
		{
			cout<<"("<<itdis->first<<" "<<itdis->second<<")"<<" ";
			remainnodes.insert(std::pair<double,uint32_t>(itdis->first,itdis->second));
		}
		cout<<endl;
	}
	
	//将剩余节点加入转发优先级列表中
	std::multimap<double,uint32_t,std::greater<double> >::iterator itremain = remainnodes.begin();
	for(;itremain != remainnodes.end();itremain++)
	{
		priorityList.push_back(itremain->second);
	}
	
	cout<<"(forwarding.cc-RSUGetPriorityListOfData) 数据包转发优先级列表为 "<<endl;
	for(uint32_t i = 0;i < priorityList.size();i++)
	{
		cout<<priorityList[i]<<" ";
	}
	cout<<endl;
	return std::pair<std::vector<uint32_t>,std::unordered_set<std::string> > (priorityList,remainroutes);
}


std::unordered_set<uint32_t> NavigationRouteHeuristic::converVectorList(
		const std::vector<uint32_t>& list)
{
	std::unordered_set<uint32_t> result;
	std::vector<uint32_t>::const_iterator it;
	for(it=list.begin();it!=list.end();++it)
		result.insert(*it);
	return result;
}

void NavigationRouteHeuristic::ToContentStore(Ptr<Data> data)
{
	NS_LOG_DEBUG ("To content store.(Just a trace)");
	return;
}



void NavigationRouteHeuristic::NotifyUpperLayer(Ptr<Data> data)
{
	if(!m_running) return;
	
	//cout<<"进入(forwarding.cc-NotifyUpperLayer)"<<endl;
	// 1. record the Data Packet received
	//2018.1.6 m_dataSignatureSeen用于记录转发过的数据包，而转发到上层的数据包并不一定会转发给其他节点
	//m_dataSignatureSeen.Put(data->GetSignature(),true);

	// 2. notify upper layer
	vector<Ptr<Face> >::iterator fit;
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


