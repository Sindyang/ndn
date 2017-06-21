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

#include "ns3/core-module.h"
#include "ns3/ptr.h"
#include "ns3/ndn-interest.h"
#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/node.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"


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
	m_resendInterestTime(-1)
{
    m_firstSendInterest=true; //added by siukwan
	m_nbChange_mode=0; //added by siukwan
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
		m_inFaceList.push_back(face);
		
	}
	else
	{
		NS_LOG_DEBUG("Node "<<m_node->GetId()<<" add NOT application face "<<face->GetId());
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

//获取优先列表
std::vector<uint32_t> NavigationRouteHeuristic::GetPriorityList(
		const vector<string>& route /* = m_sensor->getNavigationRoute()*/)
{
	//NS_LOG_FUNCTION (this);
	std::vector<uint32_t> PriorityList;
	std::ostringstream str;
	str<<"PriorityList is";
	//cout<<"(forwarding.cc-GetPriorityList)PriorityList is ";

	// The default order of multimap is ascending order,
	// but I need a descending order
	std::multimap<double,uint32_t,std::greater<double> > sortlist;

	// step 1. Find 1hop Neighbors In Front Of Route,m_nb为邻居列表
	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb;
	
	for(nb = m_nb.getNb().begin() ; nb != m_nb.getNb().end();++nb)
	{
		std::pair<bool, double> result=
				m_sensor->getDistanceWith(nb->second.m_x,nb->second.m_y,route);
		// Be careful with the order, increasing or descending?
		if(result.first && result.second >= 0)
			sortlist.insert(std::pair<double,uint32_t>(result.second,nb->first));
	}
	// step 2. Sort By Distance Descending
	std::multimap<double,uint32_t>::iterator it;
	for(it=sortlist.begin();it!=sortlist.end();++it)
	{
		PriorityList.push_back(it->second);

		str<<'\t'<<it->second;
		//cout<<'\t'<<it->second;
	}
	NS_LOG_DEBUG(str.str());
	//cout<<endl<<"(forwarding.cc-GetPriorityList) 邻居数目为 "<<m_nb.getNb().size()<<endl;
	//getchar();
	return PriorityList;
}


void NavigationRouteHeuristic::OnInterest(Ptr<Face> face,
		Ptr<Interest> interest)
{
	//NS_LOG_UNCOND("Here is NavigationRouteHeuristic dealing with OnInterest");
	//NS_LOG_FUNCTION (this);
	if(!m_running) return;
	//getchar();
	//cout<<endl<<"进入(forwarding.cc-OnInterest)"<<endl;
	
	if(Face::APPLICATION==face->GetFlags())
	{
		//consumer产生兴趣包，在路由层进行转发
		//cout << "(forwarding.cc-OnInterest)该兴趣包来自应用层" <<endl;
		NS_LOG_DEBUG("Get interest packet from APPLICATION");
		// This is the source interest from the upper node application (eg, nrConsumer) of itself
		// 1.Set the payload
		interest->SetPayload(GetNrPayload(HeaderHelper::INTEREST_NDNSIM,interest->GetPayload(),999999999));
        
        //added by sy
        /*ndn::nrndn::nrHeader nrheader;
        interest->GetPayload()->PeekHeader(nrheader);
        uint32_t nodeId = nrheader.getSourceId();*/

		// 2. record the Interest Packet
		m_interestNonceSeen.Put(interest->GetNonce(),true);
		//cout<<"(forwarding.cc-OnInterest) 记录兴趣包 nonce "<<interest->GetNonce()<<" from NodeId "<<nodeId<<endl;

		// 3. Then forward the interest packet directly
		Simulator::Schedule(MilliSeconds(m_uniformRandomVariable->GetInteger(0,100)),
				&NavigationRouteHeuristic::SendInterestPacket,this,interest);
		//cout<<"(forwarding.cc-OnInterest) 来自应用层的兴趣包处理完毕"<<endl<<endl;
		return;
	}

	//cout<<"(forwarding.cc-OnInterest)当前车辆Id为 "<<m_node->GetId()<<endl;
	
	if(HELLO_MESSAGE==interest->GetScope())
	{		
		//cout << "(forwarding.cc-OnInterest) 心跳包" <<endl;
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
	
	//2017.6.16 
	if(nodeId == myNodeId)
	{
		forwardNode[nodeId] = forwardId;
		cout<<"(forwarding.cc-OnInterest)节点 "<<nodeId <<" 收到了自己发送的兴趣包,转发节点为："<<forwardId<<endl;
		//getchar();
	}
	

	//If the interest packet has already been sent, do not proceed the packet
	if(m_interestNonceSeen.Get(interest->GetNonce()))
	{
		//cout<<"(forwarding.cc-OnInterest)该兴趣包已经被发送, 不再传输该兴趣包，nonce为 "<<interest->GetNonce()<<endl;
		//getchar();
		NS_LOG_DEBUG("The interest packet has already been sent, do not proceed the packet of "<<interest->GetNonce());
		return;
	}
	
	//获取优先列表
	//cout << "(forwarding.cc-OnInterest) 获取优先级列表" << endl;
	const std::vector<uint32_t>& pri=nrheader.getPriorityList();
	//cout<<"(forwarding.cc-OnInterest) pri的大小为："<<pri.size()<<endl;

	//Deal with the stop message first
	//避免回环
	if(Interest::NACK_LOOP==interest->GetNack())
	{
		//cout<<"(forwarding.cc-OnInterest) 该兴趣包为NACK_LOOP。发送兴趣包的节点为： "<<nodeId<<endl;
		ExpireInterestPacketTimer(nodeId,seq);
		return;
	}

	//If it is not a stop message, prepare to forward:
	pair<bool, double> msgdirection = packetFromDirection(interest);
	if(!msgdirection.first || // from other direction
			msgdirection.second > 0)// or from front
	{
		//cout<<"(forwrding.cc-OnInterest) Get interest packet from front or other direction"<<endl;
		NS_LOG_DEBUG("Get interest packet from front or other direction");
		if(!isDuplicatedInterest(nodeId,seq))// Is new packet
		{
			NS_LOG_DEBUG("Get interest packet from front or other direction and it is new packet");
			//cout<<"(forwarding.cc-OnInterest) 该兴趣包从前方或其他路线得到，且该兴趣包是新的。发送兴趣包的节点为： "<<nodeId<<endl;
			DropInterestePacket(interest);
		}
		else // Is old packet
		{
			NS_LOG_DEBUG("Get interest packet from front or other direction and it is old packet");
			//cout<<"(forwarding.cc-OnInterest) 该兴趣包从前方或其他路线得到，且该兴趣包是旧的。发送兴趣包的节点为： "<<nodeId<<endl;
			ExpireInterestPacketTimer(nodeId,seq);
		}
	}
	else// it is from nodes behind
	{
		NS_LOG_DEBUG("Get interest packet from nodes behind");
		//cout<<"(forwarding.cc-OnInterest) 该兴趣包从后方得到，兴趣包的源节点为："<<nodeId<<",当前节点为: "<<myNodeId<<endl;
		const vector<string> remoteRoute=
							ExtractRouteFromName(interest->GetName());

		// Update the PIT here
		m_nrpit->UpdatePit(remoteRoute, nodeId);
		// Update finish

		//evaluate whether receiver's id is in sender's priority list
		bool idIsInPriorityList;
		vector<uint32_t>::const_iterator idit;
		idit = find(pri.begin(), pri.end(), m_node->GetId());
		idIsInPriorityList = (idit != pri.end());

		//evaluate end

		if (idIsInPriorityList)
		{
			//cout<<"(forwarding.cc-OnInterest) Node id is in PriorityList"<<endl;
			NS_LOG_DEBUG("Node id is in PriorityList");

			bool IsPitCoverTheRestOfRoute=PitCoverTheRestOfRoute(remoteRoute);

			NS_LOG_DEBUG("IsPitCoverTheRestOfRoute?"<<IsPitCoverTheRestOfRoute);
			if(NoFwStop)
				IsPitCoverTheRestOfRoute = false;

			if (IsPitCoverTheRestOfRoute)
			{
				BroadcastStopMessage(interest);
				return;
			}
			else
			{
				//Start a timer and wait
				double index = distance(pri.begin(), idit);
				double random = m_uniformRandomVariable->GetInteger(0, 20);
				Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
				m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval,
						&NavigationRouteHeuristic::ForwardInterestPacket, this,
						interest);
			}
		}
		else
		{
			//cout<<"(forwarding.cc-OnInterest) Node id is not in PriorityList"<<endl;
			NS_LOG_DEBUG("Node id is not in PriorityList");
			DropInterestePacket(interest);
		}

	}

}


void NavigationRouteHeuristic::OnData(Ptr<Face> face, Ptr<Data> data)
{
	NS_LOG_FUNCTION (this);
	if(!m_running) return;
	if(Face::APPLICATION & face->GetFlags())
	{
		NS_LOG_DEBUG("Get data packet from APPLICATION");
		// This is the source data from the upper node application (eg, nrProducer) of itself
		// 1.Set the payload
		//added by sy
		//节点Id
		//uint32_t id = m_sensor->getNode();		
        //cout<<"(forwarding.cc-OnData) 数据包来自应用层，产生该数据包的Node为 "<<id<<endl;	
		
		//cout<<"(forwarding.cc-OnData) data size before GetNrPayload is "<<data->GetPayload()->GetSize()<<endl;
		//getchar();
		Ptr<Packet> payload = GetNrPayload(HeaderHelper::CONTENT_OBJECT_NDNSIM,data->GetPayload(),999999999,data->GetName());
		//cout<<"(forwarding.cc-OnData) data size after GetNrPayload: "<<payload->GetSize()<<endl;
		//getchar();
		if(!payload->GetSize())
			return;
		data->SetPayload(payload);

		// 2. record the Data Packet(only record the forwarded packet)
		m_dataSignatureSeen.Put(data->GetSignature(),true);

		// 3. Then forward the data packet directly
		Simulator::Schedule(
				MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100)),
				&NavigationRouteHeuristic::SendDataPacket, this, data);

		//cout<<"(forwarding.cc-OnData) 应用层的数据包事件设置成功"<<endl;
		
		// 4. Although it is from itself, include into the receive record
		NotifyUpperLayer(data);

		return;
	}

	//If the data packet has already been sent, do not proceed the packet
	if(m_dataSignatureSeen.Get(data->GetSignature()))
	{
		//cout<<"(forwarding.cc-OnData)该数据包已经被发送，不再发送数据包 "<<data->GetSignature()<<endl;
		NS_LOG_DEBUG("The Data packet has already been sent, do not proceed the packet of "<<data->GetSignature());
		return;
	}

	Ptr<const Packet> nrPayload	= data->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t nodeId=nrheader.getSourceId();
	uint32_t signature=data->GetSignature();
	std::vector<uint32_t> newPriorityList;
	bool IsClearhopCountTag=true;
	const std::vector<uint32_t>& pri=nrheader.getPriorityList();

	//uint32_t id = m_sensor->getNode();		
	//cout<<"(forwarding.cc-OnData)Node "<<id<<" 收到其他节点发送的数据包"<<endl;
	//Deal with the stop message first. Stop message contains an empty priority list
	if(pri.empty())
	{
		if(!WillInterestedData(data))// if it is interested about the data, ignore the stop message)
			ExpireDataPacketTimer(nodeId,signature);
		return;
	}

	//If it is not a stop message, prepare to forward:
	pair<bool, double> msgdirection = m_sensor->getDistanceWith(
			nrheader.getX(), nrheader.getY(),
			m_sensor->getNavigationRoute());
	std::vector<uint32_t>::const_iterator priorityListIt;
	//找出发送数据包的节点是否在优先级列表中
	priorityListIt = find(pri.begin(),pri.end(),m_node->GetId());

	if(msgdirection.first
			&&msgdirection.second<0)// This data packet is on the navigation route of the local node, and it is from behind
	{
		if(!isDuplicatedData(nodeId,signature))
		{
			if(WillInterestedData(data))
			{
				// 1.Buffer the data in ContentStore
				ToContentStore(data);
				// 2. Notify upper layer
				NotifyUpperLayer(data);
				return;
			}
			else
			{
				DropDataPacket(data);
				return;
			}
		}
		else // duplicated data
		{
			ExpireDataPacketTimer(nodeId,signature);
			return;
		}
	}
	else// This data packet is 1) NOT on the navigation route of the local node
		//					or 2) and it is from location in front of it along the navigation route
	{
		if(isDuplicatedData(nodeId,signature))
		{
			//cout<<"(forwarding.cc-OnData)重复丢弃"<<endl;
			//getchar();
			//不在优先级列表中
			if(priorityListIt==pri.end())
			{
				ExpireDataPacketTimer(nodeId,signature);
				return;
			}
			//在优先级列表中
			else
			{
				DropDataPacket(data);
				return;
			}
		}
		else
		{
			//cout<<"(forwarding.cc-OnData) 进行转发"<<endl;
			//getchar();
			Ptr<pit::Entry> Will = WillInterestedData(data);
			if (!Will)
			{
				//不在优先级列表中
				if (priorityListIt == pri.end())
				{
					DropDataPacket(data);
					return;
				}
				else
				{
					bool isTTLReachMax;
					/*
					 * 		When a data is received by disinterested node,
					 * 	this node will not know which node behind him will
					 *	interest this data, so it forward the data blindly,
					 *	until it may reach a node which intereste at it.
					 *		However, the packet should not be forwarded blindly
					 *	for unlimit times. So a max TTL should be use to limit it.
					 *	If an interest node forward the data, the hop counter
					 *	will be reset to ZERO.
					 */
					FwHopCountTag hopCountTag;
					data->GetPayload()->PeekPacketTag(hopCountTag);
					isTTLReachMax = (hopCountTag.Get() > m_TTLMax);
					if (isTTLReachMax)
					{
						//cout << "(forwarding.cc-OnData) isTTLReachMax:" <<  hopCountTag.Get() << endl;
						//getchar();
						DropDataPacket(data);
						return;
					}
					else
					{
						/*
						 * Indicate whether to update TTL or not,
						 * Because TTL will be increased automatically
						 * by Face::SendData==>Face::Send.
						 * When sending an Interested data, the hop count
						 * tag should be clear to zero.
						 * Since now the packet is a disinterested data,
						 * and the IsClearhopCountTag=true by default,
						 * the IsClearhopCountTag should be changed to false,
						 * indicating that the hop count tag should NOT be
						 * clear to ZERO.
						 */
						IsClearhopCountTag=false;
					}
				}
				newPriorityList=GetPriorityListOfDataForwarderDisinterestd(pri);
			}
			else
			{
				// 1.Buffer the data in ContentStore
				ToContentStore(data);
				// 2. Notify upper layer
				NotifyUpperLayer(data);
				// 3. Is there any interested nodes behind?
				Ptr<pit::nrndn::EntryNrImpl> entry = DynamicCast<
						pit::nrndn::EntryNrImpl>(Will);
				const std::unordered_set<uint32_t>& interestNodes =
						entry->getIncomingnbs();
				if (interestNodes.empty())
				{
					BroadcastStopMessage(data);
					return;
				}
				newPriorityList = GetPriorityListOfDataForwarderInterestd(interestNodes,pri);
			}

			if(newPriorityList.empty())
				NS_LOG_DEBUG("priority list of data packet is empty. Is its neighbor list empty?");

			/*
			 * 	Schedule a data forwarding event and wait
			 *  This action can correct the priority list created by disinterested nodes.
			 *  Because the disinterested nodes sort the priority list simply by distance
			 *  for example, a priority list from a disinterested node is like this:
			 *  slot 1	|disinterested node 1|					time slot 2		|	interested node 2|
			 *  slot 2	|	interested node 2|					time slot 5		|	interested node 5|
			 *  slot 3	|disinterested node 3|					time slot 6		|	interested node 6|
			 *  slot 4	|disinterested node 4| ==> Time slot:	time slot 7		|	interested node 7|
			 *  slot 5	|	interested node 5|					time slot Gap+1	|disinterested node 1|
			 *  slot 6	|	interested node 6|					time slot Gap+3	|disinterested node 3|
			 *  slot 7	|	interested node 7|		 			time slot Gap+4	|disinterested node 4|
			 *
			 * */
			Time sendInterval;
			double random = m_uniformRandomVariable->GetInteger(0, 20);
			if(priorityListIt ==pri.end())
			{
				sendInterval = (MilliSeconds(random) + m_gap * m_timeSlot);
			}
			else
			{
				double index = distance(pri.begin(),priorityListIt);
				if(Will)
					sendInterval = (MilliSeconds(random) + index * m_timeSlot);
				else
					sendInterval = (MilliSeconds(random) + ( index + m_gap ) * m_timeSlot);
			}

			m_sendingDataEvent[nodeId][signature]=
					Simulator::Schedule(sendInterval,
					&NavigationRouteHeuristic::ForwardDataPacket, this, data,
					newPriorityList,IsClearhopCountTag);
			return;
		}
	}

}


pair<bool, double>
NavigationRouteHeuristic::packetFromDirection(Ptr<Interest> interest)
{
	NS_LOG_FUNCTION (this);
	Ptr<const Packet> nrPayload	= interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader( nrheader);
	//cout<<"(forwarding.cc-packetFromDirection) 收到兴趣包的位置" << "x: "<<nrheader.getX() << " " <<"y: "<< nrheader.getY() <<endl;
	//getchar();
	const vector<string> route	= ExtractRouteFromName(interest->GetName());
	//cout <<"(forwarding.cc-packetFromDirection)"<< m_running << " route.size:" << route.size() <<endl;
	//getchar();
	pair<bool, double> result =
			m_sensor->getDistanceWith(nrheader.getX(),nrheader.getY(),route);
	//added by sy
	//Question:determine real size
	//cout<<"(forwarding.cc-packetFromDirection) interest's size " <<interest->GetName().size()<<endl;
	//cout << "(forwarding.cc-packetFromDirection) route size " << route.size() <<endl;
	//getchar();
	return result;
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
	EventId& eventid = m_sendingInterestEvent[nodeId][seq];
	
	//2. cancel the timer if it is still running
	eventid.Cancel();
}

void NavigationRouteHeuristic::BroadcastStopMessage(Ptr<Interest> src)
{
	if(!m_running) return;
	//cout<<"进入(forwarding.cc-BroadcastStopMessage) 广播停止的消息为 "<<src->GetName().toUri() <<endl;
	NS_LOG_FUNCTION (this<<" broadcast a stop message of "<<src->GetName().toUri());
	//1. copy the interest packet
	Ptr<Interest> interest = Create<Interest> (*src);

	//2. set the nack type
	interest->SetNack(Interest::NACK_LOOP);

	//3.Remove the useless payload, save the bandwidth
	Ptr<const Packet> nrPayload=src->GetPayload();
	ndn::nrndn::nrHeader srcheader,dstheader;
	nrPayload->PeekHeader( srcheader);
	dstheader.setSourceId(srcheader.getSourceId());
	Ptr<Packet> newPayload	= Create<Packet> ();
	newPayload->AddHeader(dstheader);
	interest->SetPayload(newPayload);

	//4. send the payload
	Simulator::Schedule(MilliSeconds(m_uniformRandomVariable->GetInteger(0,10)),
					&NavigationRouteHeuristic::SendInterestPacket,this,interest);

}

void NavigationRouteHeuristic::ForwardInterestPacket(Ptr<Interest> src)
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
	const vector<string> route	=
			ExtractRouteFromName(src->GetName());
	const std::vector<uint32_t> priorityList=GetPriorityList(route);
	sourceId=nrheader.getSourceId();
	nonce   =src->GetNonce();
	// source id do not change
	nrheader.setX(x);
	nrheader.setY(y);
	//2017.6.16
	nrheader.setForwardId(m_node->GetId());
	nrheader.setPriorityList(priorityList);

	Ptr<Packet> newPayload	= Create<Packet> ();
	newPayload->AddHeader(nrheader);

	Ptr<Interest> interest = Create<Interest> (*src);
	interest->SetPayload(newPayload);

	//直接转发
	// 3. Send the interest Packet. Already wait, so no schedule
	SendInterestPacket(interest);
	//cout<<"(forwarding.cc-ForwardInterestPacket) 兴趣包的NodeId为"<<sourceId<<",nonce为"<<nonce<<endl;

	// 记录转发次数
	// 4. record the forward times
	ndn::nrndn::nrUtils::IncreaseInterestForwardCounter(sourceId,nonce);
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
	//Question:super
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
	if(!m_running) return;

	//added by sy
    /*ndn::nrndn::nrHeader nrheader;
    interest->GetPayload()->PeekHeader(nrheader);
    uint32_t nodeId = nrheader.getSourceId();
	uint32_t myNodeId = m_node->GetId();
	cout<<endl<<"(forwarding.cc-SendInterestPacket) 兴趣包的源节点为 "<<nodeId<<",转发该兴趣包的节点为 "<<myNodeId<<endl;*/

	if(HELLO_MESSAGE!=interest->GetScope()||m_HelloLogEnable)
		NS_LOG_FUNCTION (this);

	//    if the node has multiple out Netdevice face, send the interest package to them all
	//    makde sure this is a NetDeviceFace!!!!!!!!!!!1
	vector<Ptr<Face> >::iterator fit;
	for(fit=m_outFaceList.begin();fit!=m_outFaceList.end();++fit)
	{
		(*fit)->SendInterest(interest);
		ndn::nrndn::nrUtils::AggrateInterestPacketSize(interest);
	}
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
  
  if(m_node==0)
  {
	 // cout<<"(forwarding.cc-NotifyNewAggregate)新建Node"<<endl;
	  m_node=GetObject<Node>();
  }

  super::NotifyNewAggregate ();
}

//Question:这个函数的作用
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

	//4. send the hello message
	SendInterestPacket(interest);

}

void
NavigationRouteHeuristic::ProcessHello(Ptr<Interest> interest)
{
	if(!m_running) 
		return;

	if(m_HelloLogEnable)
		NS_LOG_DEBUG (this << interest << "\tReceived HELLO packet from "<<interest->GetNonce());

	Ptr<const Packet> nrPayload	= interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	//更新邻居列表
	m_nb.Update(nrheader.getSourceId(),nrheader.getX(),nrheader.getY(),Time (AllowedHelloLoss * HelloInterval));
	
	bool lostForwardNeighbor = true;
	std::unordered_map<uint32_t,Neighbors::Neighbor>::const_iterator nb;
	cout<<"(forwarding.cc-ProcessHello)节点 "<<m_node->GetId()<<"的当前邻居列表为: ";
	for(nb = m_nb.getNb().begin();nb != m_nb.getNb().end();nb++)
	{
		cout<<nb->first<<" ";
	}
	
	std::map<uint32_t,uint32_t>::iterator it;
	//判断该节点是否有转发节点
	it = forwardNode.find(m_node->GetId());
	if(it != forwardNode.end())
	{
		cout<<" 转发节点为: "<<it->second<<endl;
		//判断转发节点是否存在于邻居列表中
		for(nb = m_nb.getNb().begin();nb != m_nb.getNb().end();nb++)
		{
			if(nb->first == it->second)
			{
				lostForwardNeighbor = false;
				break;
			}
		}
		if(!lostForwardNeighbor)
		{
			cout<<"(forwarding.cc-ProcessHello) 转发节点仍在邻居列表中"<<endl;
			//getchar();
		}
		else
		{
			cout<<"(forwarding.cc-ProcessHello) 转发节点丢失"<<endl;
			notifyUpperOnInterest(m_node->GetId());
			getchar();
		}
	}
	cout<<endl;
}

void NavigationRouteHeuristic::notifyUpperOnInterest(uint32_t nodeId)
{
	//增加一个时间限制，超过1s才进行转发
	double interval = Simulator::Now().GetSeconds() - m_resendInterestTime;
	m_resendInterestTime =  Simulator::Now().GetSeconds();
	cout<<"(forwarding.cc-notifyUpperOnInterest) 时间间隔为 "<<interval<<endl;
	if( interval >= 1)
	{
		cout << "id"<<m_node->GetId() << "允许发送兴趣包 间隔：" <<interval << " time："<<Simulator::Now().GetSeconds() << endl;
	}
	else
	{
		cout <<"id"<<m_node->GetId()<< "禁止发送兴趣包 间隔：" <<interval << " time："<<Simulator::Now().GetSeconds() <<endl;
		return;
	}
	cout<<"(forwarding.cc-notifyUpperOnInterest)"<<endl;
	vector<Ptr<Face> >::iterator fit;
	Ptr<Interest> interest = Create<Interest> ();
	interest->SetNonce(nodeId);
	int count=0;
	for (fit = m_inFaceList.begin(); fit != m_inFaceList.end(); ++fit)
	{   //只有一个Face？有两个，一个是consumer，一个是producer
		//App::OnInterest() will be executed,
		//including nrProducer::OnInterest.
		count++;
		(*fit)->SendInterest(interest);
	}
	if(count>2)
	{
		cout<<"(forwarding.cc)notifyUpperOnInterest中的Face数量大于2："<<count<<endl;
		getchar();
	}*/
}

std::vector<uint32_t> NavigationRouteHeuristic::GetPriorityList()
{
	return GetPriorityList(m_sensor->getNavigationRoute());
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
			priorityList = GetPriorityList();
			//cout<<"(forwarding.cc-GetNrPayload)Node "<<m_node->GetId()<<"的兴趣包转发优先级列表大小为 "<<priorityList.size()<<endl;
			//getchar();
			break;
		}
		case HeaderHelper::CONTENT_OBJECT_NDNSIM:
		{
			priorityList = GetPriorityListOfDataSource(dataName);
			//There is no interested nodes behind
			if(priorityList.empty())
			{
				//added by sy
				cout<<"(forwarding.cc-GetNrPayload) NodeId: "<<m_node->GetId()<<" 的数据包转发优先级列表为空"<<endl;
				return Create<Packet>();
			}	
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
	nrPayload->AddHeader(nrheader);
	return nrPayload;
}

std::vector<uint32_t> NavigationRouteHeuristic::GetPriorityListOfDataSource(const Name& dataName)
{
	NS_LOG_INFO("GetPriorityListOfDataSource");
	//cout<<"进入(forwarding.cc-GetPriorityListOfDataSource)"<<endl;
	std::vector<uint32_t> priorityList;
	std::multimap<double,uint32_t,std::greater<double> > sortInterest;
	std::multimap<double,uint32_t,std::greater<double> > sortNotInterest;
	//NS_ASSERT_MSG(false,"NavigationRouteHeuristic::GetPriorityListOfDataFw");
	
	//added by sy
	//cout<<"(forwarding.cc-GetPriorityListOfDataSource) the pit of this node is: "<<endl;
	m_nrpit->showPit();
	//cout<<"(forwarding.cc-GetPriorityListOfDataSource)the name of this data is "<<dataName.toUri()<<endl;
	
	Ptr<pit::nrndn::EntryNrImpl> entry = DynamicCast<pit::nrndn::EntryNrImpl>(m_nrpit->Find(dataName));
	if(entry == 0)
	{
		//cout<<"(forwarding.cc-GetPriorityListOfDataSource) entry not find, NodeID:"<<m_node->GetId()<<" At time:"<<Simulator::Now().GetSeconds()
		//	<<" Current dataName:"<<dataName.toUri();
		//getchar();
		return priorityList;
	}
	//NS_ASSERT_MSG(entry!=0," entry not find, NodeID:"<<m_node->GetId()<<" At time:"<<Simulator::Now().GetSeconds()
	//		<<" Current dataName:"<<dataName.toUri());
	const std::unordered_set<uint32_t>& interestNodes = entry->getIncomingnbs();
	const vector<string>& route  = m_sensor->getNavigationRoute();
	//added by sy
	//cout<<"(forwarding.cc-GetPriorityListOfDataSource) the size of interestNodes: "<<interestNodes.size()<<endl;
	//getchar();
	
	if(!interestNodes.empty())// There is interested nodes behind
	{
		//cout<<"(forwarding.cc-GetPriorityListOfDataSource) 数据包所在的位置有兴趣节点"<<endl;
		//getchar();
		std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb;
		for(nb=m_nb.getNb().begin();nb!=m_nb.getNb().end();++nb)//O(n) complexity
		{
			std::pair<bool, double> result = m_sensor->getDistanceWith(nb->second.m_x,nb->second.m_y,route);
			if(interestNodes.count(nb->first))//O(1) complexity
			{
				if(!result.first)//from other direction, maybe from other lane behind
				{
					NS_LOG_DEBUG("At "<<Simulator::Now().GetSeconds()<<", "<<m_node->GetId()<<"'s neighbor "<<nb->first<<" do not on its route, but in its interest list.Check!!");
					sortInterest.insert(std::pair<double,uint32_t>(result.second,nb->first));
				}
				else{// from local route behind
					if(result.second < 0)
						sortInterest.insert(std::pair<double,uint32_t> ( -result.second , nb->first ) );
					else
					{
						// otherwise it is in front of route.No need to forward, simply do nothing
					}
				}
			}
			else
			{
				if(!result.first)//from other direction, maybe from other lane behind
					sortNotInterest.insert(std::pair<double,uint32_t>(result.second,nb->first));
				else
				{
					if(result.second<0)
						sortNotInterest.insert(std::pair<double,uint32_t> ( -result.second , nb->first ) );
				}
			}
		}

		std::multimap<double,uint32_t,std::greater<double> >::iterator it;
		//setp 1. push the interested nodes
		for(it = sortInterest.begin();it!=sortInterest.end();++it)
			priorityList.push_back(it->second);

		//step 2. push the not interested nodes
		for(it = sortNotInterest.begin();it!=sortNotInterest.end();++it)
			priorityList.push_back(it->second);
		
		//added by sy
		if(priorityList.empty())
		{
			//cout<<"(GetPriorityListOfDataSource) "<<m_node->GetId()<<" 转发优先级列表为空"<<endl;
			//getchar();
		}
	}
	return priorityList;
}

void NavigationRouteHeuristic::ExpireDataPacketTimer(uint32_t nodeId,uint32_t signature)
{
	//NS_ASSERT_MSG(false,"NavigationRouteHeuristic::ExpireDataPacketTimer");
	NS_LOG_FUNCTION (this<< "ExpireDataPacketTimer id\t"<<nodeId<<"\tsignature:"<<signature);
	//cout<<"(forwarding.cc-ExpireDataPacketTimer) NodeId: "<<nodeId<<" signature: "<<signature<<endl;
	//1. Find the waiting timer
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

bool NavigationRouteHeuristic::isDuplicatedData(uint32_t id, uint32_t signature)
{
	NS_LOG_FUNCTION (this);
	//NS_ASSERT_MSG(false,"NavigationRouteHeuristic::isDuplicatedData");
	if(!m_sendingDataEvent.count(id))
		return false;
	else
		return m_sendingDataEvent[id].count(signature);
}

void NavigationRouteHeuristic::BroadcastStopMessage(Ptr<Data> src)
{
	if(!m_running) return;
	//cout<<"进入(forwarding.cc-BroadcastStopMessage)"<<endl;
	//NS_ASSERT_MSG(false,"NavigationRouteHeuristic::BroadcastStopMessage(Ptr<Data> src)");

	NS_LOG_FUNCTION (this<<" broadcast a stop message of "<<src->GetName().toUri());
	//1. copy the interest packet
	Ptr<Data> data = Create<Data> (*src);

	//2.Remove the useless payload, save the bandwidth
	Ptr<const Packet> nrPayload=src->GetPayload();
	ndn::nrndn::nrHeader srcheader,dstheader;
	nrPayload->PeekHeader( srcheader);
	dstheader.setSourceId(srcheader.getSourceId());//Stop message contains an empty priority list
	Ptr<Packet> newPayload	= Create<Packet> ();
	newPayload->AddHeader(dstheader);
	data->SetPayload(newPayload);

	//4. send the payload
	Simulator::Schedule(MilliSeconds(m_uniformRandomVariable->GetInteger(0,10)),
					&NavigationRouteHeuristic::SendDataPacket,this,data);
}

void NavigationRouteHeuristic::ForwardDataPacket(Ptr<Data> src,std::vector<uint32_t> newPriorityList,bool IsClearhopCountTag)
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
	//Ptr<Packet> newPayload	= Create<Packet> ();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->RemoveHeader(nrheader);
	double x= m_sensor->getX();
	double y= m_sensor->getY();
	sourceId = nrheader.getSourceId();
	signature = src->GetSignature();
	//cout << "(forward.cc-ForwardDataPacket) 转发数据包" <<m_node->GetId() << " "<< sourceId << " " << signature << endl;
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

std::vector<uint32_t> NavigationRouteHeuristic::GetPriorityListOfDataForwarderInterestd(
		const std::unordered_set<uint32_t>& interestNodes,
		const std::vector<uint32_t>& recPri)
{
	//cout<<"进入(forwarding.cc-GetPriorityListOfDataForwarderInterestd)"<<endl;
	std::vector<uint32_t> priorityList;
	std::unordered_set<uint32_t> LookupPri=converVectorList(recPri);

	std::multimap<double,uint32_t,std::greater<double> > sortInterestFront;
	std::multimap<double,uint32_t,std::greater<double> > sortInterestBack;
	std::multimap<double,uint32_t,std::greater<double> > sortDisinterest;
	const vector<string>& route  = m_sensor->getNavigationRoute();

	NS_ASSERT (!interestNodes.empty());	// There must be interested nodes behind

	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb;
	for (nb = m_nb.getNb().begin(); nb != m_nb.getNb().end(); ++nb)	//O(n) complexity
	{
		std::pair<bool, double> result = m_sensor->getDistanceWith(
				nb->second.m_x, nb->second.m_y, route);
		if (interestNodes.count(nb->first))	//O(1) complexity
		{
			if (!result.first)//from other direction, maybe from other lane behind
			{
				sortInterestBack.insert(
						std::pair<double, uint32_t>(result.second, nb->first));
			}
			else
			{	if (result.second < 0)
				{	// from local route behind
					if(!LookupPri.count(nb->first))
						sortInterestBack.insert(
							std::pair<double, uint32_t>(-result.second,
									nb->first));
				}
				else
				{
					// otherwise it is in front of route, add to sortInterestFront
					sortInterestFront.insert(
							std::pair<double, uint32_t>(-result.second,
									nb->first));
				}
			}
		}
		else
		{
			if (!result.first)//from other direction, maybe from other lane behind
				sortDisinterest.insert(
						std::pair<double, uint32_t>(result.second, nb->first));
			else
			{
				if(result.second>0// nodes in front
						||!LookupPri.count(nb->first))//if it is not in front, need to not appear in priority list of last hop
				sortDisinterest.insert(
					std::pair<double, uint32_t>(-result.second, nb->first));
			}
		}
	}

	std::multimap<double, uint32_t, std::greater<double> >::iterator it;
	//setp 1. push the interested nodes from behind
	for (it = sortInterestBack.begin(); it != sortInterestBack.end(); ++it)
		priorityList.push_back(it->second);

	//step 2. push the disinterested nodes
	for (it = sortDisinterest.begin(); it != sortDisinterest.end(); ++it)
		priorityList.push_back(it->second);

	//step 3. push the interested nodes from front
	for (it = sortInterestFront.begin(); it != sortInterestFront.end(); ++it)
		priorityList.push_back(it->second);

	return priorityList;
}

std::vector<uint32_t> NavigationRouteHeuristic::GetPriorityListOfDataForwarderDisinterestd(const std::vector<uint32_t>& recPri)
{
	//cout<<"进入(forwarding.cc-GetPriorityListOfDataForwarderDisinterestd)"<<endl;
	std::vector<uint32_t> priorityList;
	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb;
	std::unordered_set<uint32_t> LookupPri=converVectorList(recPri);
	const vector<string>& route  = m_sensor->getNavigationRoute();
	std::multimap<double,uint32_t,std::greater<double> > sortDisinterest;

	for (nb = m_nb.getNb().begin(); nb != m_nb.getNb().end(); ++nb)	//O(n) complexity
	{
		std::pair<bool, double> result = m_sensor->getDistanceWith(
				nb->second.m_x, nb->second.m_y, route);

		if (!result.first)	//from other direction, maybe from other lane behind
			sortDisinterest.insert(
					std::pair<double, uint32_t>(result.second, nb->first));
		else
		{
			if (result.second > 0	// nodes in front
			|| !LookupPri.count(nb->first))	//if it is not in front, need to not appear in priority list of last hop
				sortDisinterest.insert(
						std::pair<double, uint32_t>(-result.second, nb->first));
		}
	}

	std::multimap<double, uint32_t, std::greater<double> >::iterator it;
	//step 1. push the unknown interest nodes
	for (it = sortDisinterest.begin(); it != sortDisinterest.end(); ++it)
		priorityList.push_back(it->second);

	return priorityList;
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
		ndn::nrndn::nrUtils::AggrateDataPacketSize(data);
	}
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
	m_dataSignatureSeen.Put(data->GetSignature(),true);

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


