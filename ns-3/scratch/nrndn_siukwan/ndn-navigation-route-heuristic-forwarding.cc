/*
 * ndn-navigation-route-heuristic-forwarding.cc
 *
 *  Created on: Jan 1, 2016
 *      Author: siukwan
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
	m_TTLMax(1),
	NoFwStop(false),
	m_resendInterestTime(-1)
{
	m_firstSendInterest=true;
	m_nbChange_mode=0;
	m_htimer.SetFunction (&NavigationRouteHeuristic::HelloTimerExpire, this);
	m_nb.SetCallback (MakeCallback (&NavigationRouteHeuristic::FindBreaksLinkToNextHop, this));

}

NavigationRouteHeuristic::~NavigationRouteHeuristic ()
{

}

void NavigationRouteHeuristic::Start()
{
	NS_LOG_FUNCTION (this);
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
		cout<<"(forwarding.cc-AddFace) Node "<<m_node->GetId()<<" add NOT application face "<<face->GetId()<<endl;
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
	}
	NS_LOG_DEBUG(str.str());

	return PriorityList;
}


void  NavigationRouteHeuristic::OnInterest_application(Ptr<Interest> interest)
{
	//else cout<<"(forwarding.cc)"<<m_node->GetId()<<"邻居发生变化，发送兴趣包"<<endl;
	//consumer产生兴趣包，在路由层进行转发
	NS_LOG_DEBUG("Get interest packet from APPLICATION");
	// This is the source interest from the upper node application (eg, nrConsumer) of itself
	// 1.Set the payload
	interest->SetPayload(GetNrPayload(HeaderHelper::INTEREST_NDNSIM,interest->GetPayload(),999999999));

	Ptr<const Packet> nrPayload	= interest->GetPayload();
	uint32_t nodeId;
	//uint32_t seq;
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader( nrheader);
/*
	//获取发送兴趣包节点的ID
	nodeId = nrheader.getSourceId();
	uint32_t myNodeId=m_node->GetId();

	//提取兴趣树，并且还原
	string receive_tree_str = nrheader.getTree();
	Ptr<pit::nrndn::NrInterestTreeImpl> receive_tree = ns3::Create<pit::nrndn::NrInterestTreeImpl> ();
	receive_tree->root=receive_tree->deserialize_noId(receive_tree_str);
	receive_tree->NodeId=nodeId;
	string tmp_curLane=receive_tree->prefix+m_sensor->getLane();
	receive_tree->root =receive_tree->levelOrderDelete(tmp_curLane);
	vector<vector<string>> receiveRoutes(0);
	vector<string> tmpRoutes(0);
	receive_tree->tree2Routes(receiveRoutes,tmpRoutes,receive_tree->root);
	bool changeFlag=false;
	for(uint32_t i=0;i<receiveRoutes.size();++i)
	{
		bool flag1=false;
		flag1=(m_nrtree->MergeInterest(receive_tree->NodeId,receiveRoutes[i],m_sensor->getLane(),flag1));
		if(flag1)changeFlag=true;
	}
	*/
	//cout<<"forwarding.cc"<<myNodeId<<"发送应用层的兴趣包"<<nodeId<<endl;
	// 2. record the Interest Packet
	m_interestNonceSeen.Put(interest->GetNonce(),true);
	m_myInterest[interest->GetNonce()]=Simulator::Now().GetSeconds();
	// 3. Then forward the interest packet directly
	Simulator::Schedule(MilliSeconds(m_uniformRandomVariable->GetInteger(0,100)),
						&NavigationRouteHeuristic::SendInterestPacket,this,interest);	
}
void  NavigationRouteHeuristic::OnInterest_ackProcess(Ptr<Interest> interest)
{
			cout<<"收到ACK包"<<endl;
			Ptr<const Packet> nrPayload	= interest->GetPayload();
			uint32_t nodeId;
			uint32_t seq;
			ndn::nrndn::nrHeader nrheader;
			cout<<"PeekHeader"<<endl;
			nrPayload->PeekHeader( nrheader);
			//获取发送兴趣包节点的ID
			nodeId=nrheader.getSourceId();
			//获取兴趣的随机编码
			seq=interest->GetNonce();
			//如果重复
			cout<<"如果重复"<<endl;
			if(isDuplicatedInterest(nodeId,seq))
			{
				NS_LOG_DEBUG("Get ack packet from front or other direction and it is old packet");
				cout<<"forwarding.cc收到同样的ACK包,不再发送!"<<endl;
				getchar();
				ExpireInterestPacketTimer(nodeId,seq);
			}
			cout<<"如果重复,完成"<<endl;
}
void NavigationRouteHeuristic::OnInterest(Ptr<Face> face,
		Ptr<Interest> interest)
{
	//NS_LOG_UNCOND("Here is NavigationRouteHeuristic dealing with OnInterest");
	//NS_LOG_FUNCTION (this);
	if(!m_running) return;
	//cout << "来自应用层" <<endl;
	if(Face::APPLICATION==face->GetFlags())
	{
		OnInterest_application( interest);
		return;
	}

	//cout << "心跳包" <<endl;
	//如果它是个心跳包
	if(HELLO_MESSAGE==interest->GetScope())
	{//处理心跳
		ProcessHello(interest);
		return;
	}

	//cout << "处理ack包" <<endl;
	//处理ack包
	if(FORWARD_ACK == interest->GetScope())
	{
		OnInterest_ackProcess(interest);
		return;
	}

	//cout << "GetPayload" <<endl;
	//Payload是什么，payload是假负载
	Ptr<const Packet> nrPayload	= interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader( nrheader);
	//获取发送兴趣包节点的ID
	uint32_t nodeId = nrheader.getSourceId();
	//cout <<"forwarding.cc " << nodeId<< " GetPayload" <<endl;
	//获取兴趣的随机编码
	uint32_t seq = interest->GetNonce();
	//获取当前节点的id
	uint32_t myNodeId=m_node->GetId();
	//获取兴趣包的转发节点id
	uint32_t forwardId = nrheader.getForwardId();

	if(nodeId == myNodeId)
	{
		cout<<"forwarding.cc收到自己的兴趣包!!!!!!!!!!!!!!!!"<<myNodeId<<endl;
		getchar();
	}
	if(forwardId != 999999999)
	{
		//printf("my:%d  src:%d   fwd:%d",myNodeId,nodeId,forwardId);
		//cout<<endl;
	}

	//cout << "m_interestNonceSeen" << endl;

	//如果兴趣包已经被发送了，不再处理兴趣包，使用LRUcache结构
	//If the interest packet has already been sent, do not proceed the packet
	if(m_interestNonceSeen.Get(interest->GetNonce()))
	{
		if(m_myInterest.find(interest->GetNonce())!=m_myInterest.end() && nodeId==m_node->GetId())
		{
			if(Simulator::Now().GetSeconds()-m_myInterest[interest->GetNonce()]<10)
			{//10秒之内W
					cout<<"(forwarding.cc)"<<m_node->GetId()<<"10s内收到自己("<<nodeId<<")发的兴趣包"<<nrheader.getForwardId()<<"："<<interest->GetNonce()<<"   "<<m_myInterest[interest->GetNonce()]<<endl;
					//getchar();
			}
		}

		NS_LOG_DEBUG("The interest packet has already been sent, do not proceed the packet of "<<interest->GetNonce());
		return;
	}


	//cout << "获取优先列表" << endl;
	//获取优先列表
	const std::vector<uint32_t>& pri=nrheader.getPriorityList();

	//Deal with the stop message first
	//避免回环
	if(Interest::NACK_LOOP==interest->GetNack())
	{
		ExpireInterestPacketTimer(nodeId,seq);
		return;
	}
	//cout<<"forwarding.cc"<<myNodeId<<"packetFromDirection"<<nodeId<<endl;

	//If it is not a stop message, prepare to forward:
	pair<bool, double> msgdirection = packetFromDirection(interest);
	//cout<<"forwarding.cc"<<myNodeId<<"packetFromDirection OK"<<nodeId<<endl;
	if(!msgdirection.first || // from other direction
			msgdirection.second > 0)// or from front
	{
		NS_LOG_DEBUG("Get interest packet from front or other direction");
		if(!isDuplicatedInterest(nodeId,seq))// Is new packet
		{
			NS_LOG_DEBUG("Get interest packet from front or other direction and it is new packet");
			DropInterestePacket(interest);
		}
		else // Is old packet
		{
			NS_LOG_DEBUG("Get interest packet from front or other direction and it is old packet");
			ExpireInterestPacketTimer(nodeId,seq);
		}
		return ;
	}
	//cout<<"forwarding.cc"<<myNodeId<<"兴趣包来自后方"<<nodeId<<endl;
	
	//兴趣包来自后方
	// it is from nodes behind

	//如果重复
	if(isDuplicatedInterest(nodeId,seq))
	{
		NS_LOG_DEBUG("Get interest packet from front or other direction and it is old packet");
		cout<<"forwarding.cc后方数据包重复,不再发送!"<<endl;
		//getchar();
		ExpireInterestPacketTimer(nodeId,seq);
		return;
	}
	//cout<<"forwarding.cc"<<myNodeId<<"如果重复"<<nodeId<<endl;

	NS_LOG_DEBUG("Get interest packet from nodes behind");
	const vector<string> remoteRoute= ExtractRouteFromName(interest->GetName());

	//cout<<"forwarding.cc"<<myNodeId<<"准备转发兴趣包"<<nodeId<<endl;
	//getchar();

	//提取兴趣树，并且还原
	//cout << myNodeId << "提取兴趣树，并且还原" << endl;
	string receive_tree_str = nrheader.getTree();
	Ptr<pit::nrndn::NrInterestTreeImpl> receive_tree = ns3::Create<pit::nrndn::NrInterestTreeImpl> ();
	//cout<<"(forwarding.cc)"<<m_node->GetId()<<"接收得到来自节点："<<nodeId<<"解序列化:"<<nrheader.getTree()<<endl;
	receive_tree->root = receive_tree->deserialize_noId(receive_tree_str);
	receive_tree->NodeId = nodeId;
	//cout<<"\n(forwarding.cc)\n"<<m_node->GetId()<<"接收得到来自节点"<<nodeId<<"的兴趣树"<<endl;
	//receive_tree->levelOrder();
	string tmp_curLane = receive_tree->prefix+m_sensor->getLane();
	//cout<<"(forwarding.cc)所在路段为："<<tmp_curLane<<"\ndelete后的兴趣树："<<endl;

	//找到当前路段，把当前路段作为根结点，其余的删除
	//cout << myNodeId << "找到当前路段，把当前路段作为根结点，其余的删除" << endl;
	receive_tree->root = receive_tree->levelOrderDelete(tmp_curLane);
	//cout << myNodeId << "找到当前路段，把当前路段作为根结点，其余的删除，完成" << endl;
	//receive_tree->levelOrder();
	//getchar();
	vector<vector<string>> receiveRoutes(0);
	vector<string> tmpRoutes(0);
	receive_tree->tree2Routes(receiveRoutes, tmpRoutes, receive_tree->root);
	//cout << myNodeId << "tree2Routes OK" << endl;


	//cout<<"\n(forwarding.cc)\n"<<m_node->GetId()<<"接收得到来自节点"<<nodeId<<"的兴趣树"<<endl;
	//receive_tree->levelOrder();
	bool changeFlag = false;
	for(uint32_t i = 0; i < receiveRoutes.size(); ++i)
	{
		bool flag1 = false;
		//cout<<"(forwarding.cc)合并的节点："<<receiveNode[i]<<endl;
		receive_tree->NodeId = nodeId;
		flag1=(m_nrtree->MergeInterest(nodeId,receiveRoutes[i],m_sensor->getLane(),flag1));
		if(flag1)changeFlag=true;
	}
	//cout << myNodeId << "changeFlag OK" << endl;

	// Update the PIT here
	//更新PIT表
	//m_nrpit->UpdatePitByInterestTree(receive_tree, nodeId);
	
	//以当前的兴趣树，更新PIT

	//cout << myNodeId << "以当前的兴趣树，更新PIT" << endl;
	m_nrpit->UpdatePitByInterestTree2(m_nrtree);
	
	//当前所在路段？使用pit的currentlane会存在问题，pit有时候在十字路口，没有把过去的路段删除，直接使用sensor的getlane
	//string currentLane=m_nrpit->getCurrentLane();
	string currentLane = m_sensor->getLane();
	//cout << myNodeId << "获取当前道路" << currentLane << endl;

	//兴趣树没有发生变化
	if(!changeFlag)
	{
		/*
		ofstream ofile;
		ofile.open("../packetfiles/int"+int2Str(seq),ios::app);
		ofile<<Simulator::Now().GetSeconds()<<" "<<nodeId<<" 兴趣树没发生变化"<<endl;
		ofile.close();
		*/
		NS_LOG_DEBUG("InterestTree no changed");

		//cout << "forwarding.cc原来的随机码" << interest->GetNonce();
		interest->SetPayload(GetNrPayload(HeaderHelper::INTEREST_NDNSIM,interest->GetPayload(),m_node->GetId()));

		//cout << "forwarding.cc SetPayload后的随机码" << interest->GetNonce();
		//getchar();
		Ptr<const Packet> nrPayload_tmp	= interest->GetPayload();
		ndn::nrndn::nrHeader nrheader_tmp;
		nrPayload_tmp->PeekHeader( nrheader_tmp);
		if(m_node->GetId() == nrheader_tmp.getSourceId() && nrheader.getForwardId() != 999999999)
		{
			forwardNeighbors[nodeId]=true;
			cout<<"forwarding.cc!changeFlag"<<m_node->GetId()<<"收到自己的ID！！！！！！！"<<nodeId<<"  "<<myNodeId<<endl;
		}
		//cout<<"forwarding.cc!changeFlag"<<m_node->GetId()<<"兴趣树没有发生变化,发送ack"<<endl;

		//发送ack包
		//Start a timer and wait
		vector<uint32_t>::const_iterator idit;
		idit = find(pri.begin(), pri.end(), m_node->GetId());
		bool idIsInPriorityList = (idit != pri.end());
		double index;
		//在优先级列表中,则设置响应的等待时间
		if(idIsInPriorityList)
			index = distance(pri.begin(), idit);
		else//不在优先级列表中时,也要设置等待时间
			index = pri.size()+1;
		
		/*
		double random = m_uniformRandomVariable->GetInteger(0, 20);
		Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
		m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval,
					&NavigationRouteHeuristic::SendAckPacket, this);
		*/
		//SendAckPacket();
		DropInterestePacket(interest);
		//cout << "forwarding.cc" << myNodeId << "DropInterestePacket" << endl;
		return ;
	}

	cout << myNodeId << "changeFlag" << endl;
	//evaluate whether receiver's id is in sender's priority list
	bool idIsInPriorityList=true;
	vector<uint32_t>::const_iterator idit;
	idit = find(pri.begin(), pri.end(), m_node->GetId());
	idIsInPriorityList = (idit != pri.end());
	
	cout<<"forwarding.cc优先级列表为:";
	for(size_t ii=0;ii<pri.size();++ii)
		cout<<pri[ii]<< " ";
	//cout<<endl;
	//cout<<"Forwarding.cc 优先级列表判断为"<<idIsInPriorityList<<endl;
	//evaluate end
	idIsInPriorityList=true;
		
	if (idIsInPriorityList)
	{
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
			//cout << "forwarding.cc原来的随机码" << interest->GetNonce();
			interest->SetPayload(GetNrPayload(HeaderHelper::INTEREST_NDNSIM,interest->GetPayload(),m_node->GetId()));
			//cout << "forwarding.cc SetPayload后的随机码" << interest->GetNonce();
			//getchar();
			Ptr<const Packet> nrPayload_tmp	= interest->GetPayload();
			ndn::nrndn::nrHeader nrheader_tmp;
			nrPayload_tmp->PeekHeader( nrheader_tmp);

			cout<<"forwarding.cc我的ID"<<m_node->GetId()<<"  转发前的ID"<<nrheader.getForwardId()<<"  原始ID为"<<
					nrheader_tmp.getSourceId()<<"   转发后的ID"<<nrheader_tmp.getForwardId()<<endl;

			if(m_node->GetId() == nrheader_tmp.getSourceId() && nrheader.getForwardId()!=999999999)
			{
				cout<<"forwarding.cc"<<m_node->GetId()<<"收到自己的ID！！！！！！！"<<endl;
				//getchar();
			}

			//如果重复
			if(isDuplicatedInterest(nodeId, seq))
			{
				cout << "forwarding.cc重复" << endl;
				ExpireInterestPacketTimer(nodeId, seq);
				return;
			}

			if(nrheader_tmp.getSourceId() != nrheader_tmp.getForwardId())
				getchar();
			//Start a timer and wait
			double index = distance(pri.begin(), idit);
			double random = m_uniformRandomVariable->GetInteger(0, 20);
			Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
			m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval,
					&NavigationRouteHeuristic::ForwardInterestPacket, this,
					interest);
	
			cout<<"forwarding.cc"<<myNodeId<<"转发成功"<<nodeId<<endl;
			/*ofstream ofile;
			ofile.open("../packetfiles/i"+int2Str(interest->GetNonce()),ios::app);
			ofile<<Simulator::Now().GetSeconds()<<" "<<nodeId<<" 在优先级列表中，转发成功"<<endl;
			ofile.close();
			*/
		}
	}
	//不在优先列表中，则不转发
	else
	{
		cout<<"forwarding.cc"<<myNodeId<<"不在优先列表中!!"<<nodeId<<endl;
		NS_LOG_DEBUG("Node id is not in PriorityList");
		DropInterestePacket(interest);
	}
}


void NavigationRouteHeuristic::OnData(Ptr<Face> face, Ptr<Data> data)
{
	NS_LOG_FUNCTION (this);
	Ptr<const Packet> nrPayload	= data->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t nodeId=nrheader.getSourceId();
	uint32_t forwardId = nrheader.getForwardId();
	uint32_t signature=data->GetSignature();
	std::vector<uint32_t> newPriorityList;
	bool IsClearhopCountTag=true;
	const std::vector<uint32_t>& pri=nrheader.getPriorityList();
	
	
	//cout<<"forwarding.cc 节点id："<<m_node->GetId()<<m_running<<": "<<Simulator::Now().GetSeconds()<<" 收到数据包ID:"<<nodeId<<endl;
	//cout<<"forwarding.cc 当前节点："<<m_node->GetId()<<" 原始节点："<<nodeId<<"  转发节点："<<forwardId<<endl;
	//getchar();
	if(!m_running) return;
	if(Face::APPLICATION && face->GetFlags())
	{
		NS_LOG_DEBUG("Get data packet from APPLICATION");
		
		// This is the source data from the upper node application (eg, nrProducer) of itself
		// 1.Set the payload
		Ptr<Packet> payload = GetNrPayload(HeaderHelper::CONTENT_OBJECT_NDNSIM,data->GetPayload(),999999999,data->GetName());
		std::vector<uint32_t> priorityList = GetPriorityListOfDataSource(data->GetName());
		if(priorityList.empty())//There is no interested nodes behind
			cout<<"优先级列表为空"<<endl;
		cout<<data->GetName()<<" 收到来自应用层的数据包："<<payload->GetSize()<<endl;
		//getchar();
		if(!payload->GetSize())
			return;
		data->SetPayload(payload);

		ndn::nrndn::nrHeader nrheader;
		payload->PeekHeader(nrheader);
		uint32_t nodeId=nrheader.getSourceId();
		uint32_t signature=data->GetSignature();
		
		//ofstream ofile;
		//ofile.open("../packetfiles/dat"+int2Str(signature),ios::app);
		//ofile<<Simulator::Now().GetSeconds()<<" "<<nodeId<<" 原始发包"<<endl;
		//ofile.close();
		
		// 2. record the Data Packet(only record the forwarded packet)
		m_dataSignatureSeen.Put(data->GetSignature(),true);
		// 3. Then forward the data packet directly
		Simulator::Schedule(
				MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100)),
				&NavigationRouteHeuristic::SendDataPacket, this, data);

		cout<<"应用层的数据包事件设置成功"<<endl;
		//getchar();
		// 4. Although it is from itself, include into the receive record
		NotifyUpperLayer(data);

		return;
	}

	//If the data packet has already been sent, do not proceed the packet
	/*if(m_dataSignatureSeen.Get(data->GetSignature()))
	{
		NS_LOG_DEBUG("The Data packet has already been sent, do not proceed the packet of "<<data->GetSignature());
		return;
	}*/

	


	//ofstream ofile;
	//ofile.open("../packetfiles/dat"+int2Str(signature),ios::app);
	//ofile<<Simulator::Now().GetSeconds()<<" "<<nodeId<<endl;
	//ofile.close();

	/*
	if(isDuplicatedData(nodeId,signature))
	{
		//cout<<"OnData重复包"<<endl;
	}
	*/
	//已经被发送了，不用再转发
	if(m_dataSignatureSeen.Get(data->GetSignature()))
	{
		//cout<<"OnData重复包2......................."<<endl;
		NS_LOG_DEBUG("The Data packet has already been sent, do not proceed the packet of "<<data->GetSignature());
		return;
	}

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
	
	//找出当前节点是否在优先级列表中
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
		//cout<<"forward.cc 数据包在的道路header："<<nrheader.getLane()<<" 当前节点所在道路："<<m_sensor->getLane()<<endl;
		if(isDuplicatedData(nodeId,signature) )//&& nrheader.getLane() == m_sensor->getLane())
		{
			//cout<<"重复丢弃"<<endl;
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
			//cout<<"进行转发"<<endl;
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
						DropDataPacket(data);
						return;
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
						cout << m_node->GetId() << " TTL:" <<  hopCountTag.Get() 
							<< " nodeId:" << nodeId << " sig:" << signature << " fwdID:" << forwardId << endl;
						cout << "isTTLReachMax:" <<  hopCountTag.Get() << endl;
						getchar();
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

			//Ptr<Packet> payload22 = GetNrPayload(HeaderHelper::CONTENT_OBJECT_NDNSIM,data->GetPayload(),m_node->GetId(),data->GetName(), false);
			//data->SetPayload(payload22);
			
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
	//cout << nrheader.getX() << " " << nrheader.getY() <<endl;
	const vector<string> route	= ExtractRouteFromName(interest->GetName());
	//cout << m_running << " route.size:" << route.size() <<endl;

	pair<bool, double> result =
			m_sensor->getDistanceWith(nrheader.getX(),nrheader.getY(),route);
	//cout << "result" << route.size() <<endl;

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
	NS_LOG_FUNCTION (this<< "ExpireInterestPacketTimer id"<<nodeId<<"sequence"<<seq);
	//1. Find the waiting timer
	EventId& eventid = m_sendingInterestEvent[nodeId][seq];
	
	//2. cancel the timer if it is still running
	eventid.Cancel();
}

void NavigationRouteHeuristic::BroadcastStopMessage(Ptr<Interest> src)
{
	if(!m_running) return;
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
	uint32_t sourceId=0;
	uint32_t nonce=0;

	//记录转发的兴趣包
	// 1. record the Interest Packet(only record the forwarded packet)
	m_interestNonceSeen.Put(src->GetNonce(),true);

	//准备兴趣包
	// 2. prepare the interest
	Ptr<const Packet> nrPayload=src->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader( nrheader);
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
	nrheader.setPriorityList(priorityList);

	//设置信息,设置兴趣树
	nrheader.setTree(m_nrtree->serialize_noId());



	Ptr<Packet> newPayload	= Create<Packet> ();
	newPayload->AddHeader(nrheader);

	Ptr<Interest> interest = Create<Interest> (*src);
	interest->SetPayload(newPayload);

	//直接转发
	// 3. Send the interest Packet. Already wait, so no schedule
	SendInterestPacket(interest);

	//记录转发次数
	// 4. record the forward times
	ndn::nrndn::nrUtils::IncreaseInterestForwardCounter(sourceId,nonce);
}
//用于判断本节点是否包含remoteRoute中的路段
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
	//std::cout<<"(NavigationRouteHeuristic):初始化"<<m_node->GetId()<<std::endl;
	super::DoInitialize();
	//std::cout<<"(NavigationRouteHeuristic):初始化完成"<<std::endl;

	m_nrtree->NodeId=m_node->GetId();
	//获取所有的导航路线
	const std::vector<std::string>& route =	m_sensor->getNavigationRoute();
	bool tmpflag=false;
	m_nrtree->insertInterest(m_nrtree->NodeId,0,route,m_nrtree->root,tmpflag);
	//m_nrtree->levelOrder();
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
	NS_LOG_DEBUG ("Drop interest Packet");
	DropPacket();
}

void NavigationRouteHeuristic::SendInterestPacket(Ptr<Interest> interest)
{
	if(!m_running) return;


	if(HELLO_MESSAGE!=interest->GetScope()||m_HelloLogEnable)
		NS_LOG_FUNCTION (this);

	//    if the node has multiple out Netdevice face, send the interest package to them all
	//    makde sure this is a NetDeviceFace!!!!!!!!!!!1
	vector<Ptr<Face> >::iterator fit;
	for(fit=m_outFaceList.begin();fit!=m_outFaceList.end();++fit)
	{
		(*fit)->SendInterest(interest);
		ndn::nrndn::nrUtils::AggrateInterestPacketSize(interest);
		//if(HELLO_MESSAGE ==interest->GetScope())
			//cout<<"(forwarding.cc-SendInterestPacket)"<<m_node->GetId()<<" "<<Simulator::Now().GetSeconds()<<endl;
	}
}


void NavigationRouteHeuristic::NotifyNewAggregate()
{

	if (m_sensor == 0)
	{
		m_sensor = GetObject<ndn::nrndn::NodeSensor> ();
	}

	//getchar();
	if (m_nrpit == 0)
	{
		//std::cout<<"新建PIT表"<<std::endl;
		Ptr<Pit> pit=GetObject<Pit>();
		if(pit)
		{
			m_nrpit = DynamicCast<pit::nrndn::NrPitImpl>(pit);
		}
		//std::cout<<"建立完毕"<<std::endl;
	}

	if(m_node==0)
	{
		m_node=GetObject<Node>();
	}


	//m_nrtree = GetObject<pit::nrndn::NrInterestTreeImpl> ();
	//m_nrtree = ns3::Create<pit::nrndn::NrInterestTreeImpl> ();
	m_nrtree = m_nrpit->m_nrtree;
	super::NotifyNewAggregate ();
	//std::cout<<"(n..forwarding):"<<m_node->GetId()<<std::endl;
}


void
NavigationRouteHeuristic::HelloTimerExpire ()
{
	if(!m_running) return;

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

//4月11日添加,发送ack包
void NavigationRouteHeuristic::SendAckPacket()
{
	if(!m_running) return;
	//1.setup name
	Ptr<Name> name = ns3::Create<Name>();
	//2. setup payload
	Ptr<Packet> newPayload	= Create<Packet> ();
	ndn::nrndn::nrHeader nrheader;
	nrheader.setX(0);
	nrheader.setY(0);
	nrheader.setSourceId(m_node->GetId());
	//ACK包中不需要发送兴趣树，所以把兴趣树清空
	nrheader.setTree("");
	newPayload->AddHeader(nrheader);

	//3. setup interest packet
	Ptr<Interest> ackPacket	= Create<Interest> (newPayload);
	ackPacket->SetScope(FORWARD_ACK);	// 标志为ACK包
	ackPacket->SetName(name); //ack name is ack;

	//4. send the ackPacket message
	SendInterestPacket(ackPacket);
}



void
NavigationRouteHeuristic::SendHello()
{
	if(!m_running) return;
	//cout<<"(forwarding)发送hello包"<<m_node->GetId()<<endl;
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
	//心跳包中不需要发送兴趣树，所以把兴趣树清空
	nrheader.setTree("");
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
	if(!m_running) return;

	if(m_HelloLogEnable)
		NS_LOG_DEBUG (this << interest << "\tReceived HELLO packet from "<<interest->GetNonce());

	Ptr<const Packet> nrPayload	= interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	//update neighbor list
	//根据心跳包来更新邻居列表
	m_nb.Update(nrheader.getSourceId(),nrheader.getX(),nrheader.getY(),Time (AllowedHelloLoss * HelloInterval));

	m_nbChange_mode=0;
	//进行邻居变化的检测
	if(m_preNB.getNb().size()<m_nb.getNb().size())//数量不等，邻居发生变化
	{//发送兴趣包
		//cout<<"邻居增加，重发"<<endl;
		//getchar();
		m_nbChange_mode=2;//邻居增加
	}
	else if(m_preNB.getNb().size()>m_nb.getNb().size())//数量不等，邻居发生变化
	{
		/*cout<<"邻居减少，重发"<<endl;
		getchar();*/
		m_nbChange_mode=1;//邻居减少
	}
	else
	{
		bool nbChange=false;//邻居表变化
		std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator prenb=m_preNB.getNb().begin();
		std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb=m_nb.getNb().begin();
		for(;nb!=m_nb.getNb().end() && prenb!=m_preNB.getNb().end()  ;  ++prenb  , ++nb)//O(n) complexity
		{
			if(m_nb.getNb().find(prenb->first)==m_nb.getNb().end())
			{//寻找上次的邻居，看看能不能找到，找不到证明变化了
				nbChange=true;
				break;
			}

		}
		if(nbChange)
		{//邻居变化，发送兴趣包

			m_nbChange_mode=3;//邻居变化
			/*
			cout<<"邻居变化，重发"<<endl;
			prenb=m_preNB.getNb().begin();
			nb=m_nb.getNb().begin();
			cout<<"原来的邻居："<<endl;
			for(; prenb!=m_preNB.getNb().end()  ;  ++prenb)//O(n) complexity
			{
				cout<<prenb->first<<" ";
			}
			cout<<"\n现在的邻居："<<endl;
			for(;nb!=m_nb.getNb().end();  ++nb)//O(n) complexity
			{
				cout<<nb->first<<" ";
			}

			getchar();
			*/
		}
	}





	bool lostForwardNeighbor = false;
	//检测转发邻居map中的邻居是否还在
	for(auto ite = forwardNeighbors.begin(); ite != forwardNeighbors.end(); )
	{
		auto preIte = ite;
		ite++;
		if(m_nb.getNb().find(ite->first) == m_nb.getNb().end())
		{
			lostForwardNeighbor=true;
			forwardNeighbors.erase(preIte);
		}
	}

	if(lostForwardNeighbor)
	{
		//cout<<"forwarding.cc: id"<<m_node->GetId() << " time:" << Simulator::Now().GetSeconds() <<" 负责转发的邻居丢失了,需要重发兴趣包"<< m_nbChange_mode << endl;
		//getchar();
	}



	//判断兴趣包的方向
		pair<bool, double> msgdirection = packetFromDirection(interest);

		//hello信息来自前方，且邻居变化，即前面邻居变化
		if(msgdirection.first && msgdirection.second > 0 && m_nbChange_mode>1) //m_nbChange_mode>1  lostForwardNeighbor
		{//
			m_nbChange_mode=0;
			//printf("%d收到hello信息来自前方，且邻居发生变化%d\n",m_node->GetId(),m_nbChange_mode);
			notifyUpperOnInterest(m_node->GetId());
			
		}


	m_preNB=m_nb;//更新把上一次的邻居表
}
//利用face通知上层应用调用OnInterest
void NavigationRouteHeuristic::notifyUpperOnInterest(uint32_t type)
{//把type存放到interest中，以此来区分不同的notify类型
	//增加一个时间限制，超过1s才进行转发
	double interval = Simulator::Now().GetSeconds() - m_resendInterestTime;
	m_resendInterestTime =  Simulator::Now().GetSeconds();
	if( interval >= 1)
	{
		//cout << "id"<<m_node->GetId() << "允许发送兴趣包 间隔：" <<interval << " time："<<Simulator::Now().GetSeconds() << endl;
	}
	else
	{

		//cout <<"id"<<m_node->GetId()<< "禁止发送兴趣包 间隔：" <<interval << " time："<<Simulator::Now().GetSeconds() <<endl;
		return;
	}
	vector<Ptr<Face> >::iterator fit;
	Ptr<Interest> interest = Create<Interest> ();
	interest->SetNonce(type);
	int count=0;
	for (fit = m_inFaceList.begin(); fit != m_inFaceList.end(); ++fit)
	{//只有一个Face？有两个，一个是consumer，一个是producer
		//App::OnInterest() will be executed,
		//including nrProducer::OnInterest.
		count++;
		(*fit)->SendInterest(interest);
	}
	if(count>2)
	{
		cout<<"(forwarding.cc)notifyUpperOnInterest中的Face数量大于2："<<count<<endl;
		getchar();
	}
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
	for(it=name.rbegin();it!=name.rend();++it)
		result.push_back(it->toUri());
	return result;
}

Ptr<Packet> NavigationRouteHeuristic::GetNrPayload(HeaderHelper::Type type, Ptr<const Packet> srcPayload,uint32_t forwardId, const Name& dataName/* = *((Name*)NULL)*/)
{
	NS_LOG_INFO("Get nr payload, type:"<<type);
	Ptr<Packet> nrPayload = Create<Packet>(*srcPayload);
	std::vector<uint32_t> priorityList;
	string m_nrtree_str="";
	switch (type)
	{
	case HeaderHelper::INTEREST_NDNSIM:
		{
			priorityList = GetPriorityList();
			//兴趣包才设置兴趣树的序列化，加入到header
			m_nrtree_str = m_nrtree->serialize_noId();
			break;
		}
	case HeaderHelper::CONTENT_OBJECT_NDNSIM:
		{
			priorityList = GetPriorityListOfDataSource(dataName);
			if(priorityList.empty())//There is no interested nodes behind
			{
				cout<<"(forwarding.cc)"<<m_node->GetId()<<"优先级列表为空"<<endl;
				//getchar();
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

	//if(type == HeaderHelper::INTEREST_NDNSIM || original)
	//{
		const double& x = m_sensor->getX();
		const double& y = m_sensor->getY();
		ndn::nrndn::nrHeader nrheader(m_node->GetId(), x, y, priorityList);
		//设置信息,设置兴趣树
		nrheader.setTree(m_nrtree_str);
		nrheader.setForwardId(forwardId);
		nrPayload->AddHeader(nrheader);
	//}
	/*
	if( HeaderHelper::CONTENT_OBJECT_NDNSIM == type)
	{
		
		ndn::nrndn::nrHeader nrheader;
		srcPayload->PeekHeader(nrheader);
		nrheader.setForwardId(forwardId);
		nrPayload->AddHeader(nrheader);
		cout<<"(forwarding.cc)"<<m_node->GetId()<<"GetNrPayload，源ID:"<<nrheader.getSourceId()<<endl;
		getchar();
	}
	
	*/
	return nrPayload;
}

std::vector<uint32_t> NavigationRouteHeuristic::GetPriorityListOfDataSource(const Name& dataName)
{
	NS_LOG_INFO("GetPriorityListOfDataSource");
	std::vector<uint32_t> priorityList;
	std::multimap<double,uint32_t,std::greater<double> > sortInterest;
	std::multimap<double,uint32_t,std::greater<double> > sortNotInterest;
	//NS_ASSERT_MSG(false,"NavigationRouteHeuristic::GetPriorityListOfDataFw");
	Ptr<pit::nrndn::EntryNrImpl> entry = DynamicCast<pit::nrndn::EntryNrImpl>(m_nrpit->Find(dataName));
	if(entry == 0)
	{
		cout<<" entry not find, NodeID:"<<m_node->GetId()<<" At time:"<<Simulator::Now().GetSeconds()
			<<" Current dataName:"<<dataName.toUri();
		return priorityList;
	}
	//NS_ASSERT_MSG(entry!=0," entry not find, NodeID:"<<m_node->GetId()<<" At time:"<<Simulator::Now().GetSeconds()
	//		<<" Current dataName:"<<dataName.toUri());
	const std::unordered_set<uint32_t>& interestNodes = entry->getIncomingnbs();
	const vector<string>& route  = m_sensor->getNavigationRoute();
	
	cout<<"GetPriorityListOfDataSource："<<interestNodes.size()<<endl;
	//getchar();
	if(!interestNodes.empty())// There is interested nodes behind
	{
		cout<<"数据包所在的位置有兴趣节点"<<endl;
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
	}
	return priorityList;
}

void NavigationRouteHeuristic::ExpireDataPacketTimer(uint32_t nodeId,uint32_t signature)
{
	//NS_ASSERT_MSG(false,"NavigationRouteHeuristic::ExpireDataPacketTimer");
	NS_LOG_FUNCTION (this<< "ExpireDataPacketTimer id\t"<<nodeId<<"\tsignature:"<<signature);
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

//转发数据包
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
	//cout << "forward.cc转发数据包" <<m_node->GetId() << " "<< sourceId << " " << signature << endl;
	//getchar();
	// 	2.1 setup nrheader, source id do not change
	nrheader.setX(x);
	nrheader.setY(y);
	nrheader.setForwardId(m_node->GetId());
	nrheader.setLane(m_sensor->getLane());
	//cout<<"forwarding.cc转发数据包，当前道路为"<<nrheader.getLane()<<endl;
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


