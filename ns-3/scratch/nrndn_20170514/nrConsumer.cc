/*
 * nrConsumer.cc
 *
 *  Created on: Jan 4, 2015
 *      Author: chen
 */

#include "ns3/core-module.h"
#include "ns3/log.h"

#include "ns3/ndn-interest.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/ndn-app.h"
#include "ns3/ndn-face.h"

#include "ndn-navigation-route-heuristic-forwarding.h"
#include "nrConsumer.h"
#include "NodeSensor.h"
#include "nrHeader.h"
#include "nrUtils.h"

NS_LOG_COMPONENT_DEFINE ("ndn.nrndn.nrConsumer");

namespace ns3
{
namespace ndn
{
namespace nrndn
{
NS_OBJECT_ENSURE_REGISTERED (nrConsumer);


TypeId nrConsumer::GetTypeId()
{
	static TypeId tid = TypeId ("ns3::ndn::nrndn::nrConsumer")
		    .SetGroupName ("Nrndn")
		    .SetParent<ConsumerCbr> ()
		    .AddConstructor<nrConsumer> ()
//		    .AddAttribute("sensor", "The vehicle sensor used by the nrConsumer.",
//		    	   	    		PointerValue (),
//		    	   	    		MakePointerAccessor (&nrConsumer::m_sensor),
//		    	   	    		MakePointerChecker<ns3::ndn::nrndn::NodeSensor> ())
		    .AddAttribute ("PayloadSize", "Virtual payload size for traffic Content packets",
		    		            UintegerValue (1024),
		    	                MakeUintegerAccessor (&nrConsumer::m_virtualPayloadSize),
		    		            MakeUintegerChecker<uint32_t> ())
		    ;
		  return tid;
}

nrConsumer::nrConsumer():
		m_virtualPayloadSize(1024)
{
	// TODO Auto-generated constructor stub
}

nrConsumer::~nrConsumer()
{
	// TODO Auto-generated destructor stub
}

void nrConsumer::StartApplication()
{
	NS_LOG_FUNCTION_NOARGS ();
	//std::cout<<"(nrConsumer.cc-StartApplication) "<<GetNode()->GetId()<<std::endl;
	m_forwardingStrategy->Start();
	// retransmission timer expiration is not necessary here, just cancel the event
	//m_retxEvent.Cancel ();
	super::StartApplication();
}

void nrConsumer::StopApplication()
{
	NS_LOG_FUNCTION_NOARGS ();
	m_forwardingStrategy->Stop();
	super::StopApplication();
}

bool nrConsumer::IsActive()
{
	return m_active;
}

//计划下一个包
void nrConsumer::ScheduleNextPacket()
{
	//std::cout<<"进入(nrConsumer.cc-ScheduleNextPacket) "<<endl;
	//1. refresh the Interest
	 std::vector<std::string> interest=GetCurrentInterest();
	 std::string prefix="";
	 std::vector<std::string>::reverse_iterator it;
	 for(it=interest.rbegin();it!=interest.rend();++it)
	 {
		 prefix+=*it;
	 }

	 //2. set the Interest (reverse of  the residual navigation route)
	//std::cout<<"(nrConsumer.cc-ScheduleNextPacket) 兴趣 "<<prefix<<std::endl;
	if(prefix=="")
	{   //兴趣为空，直接返回
		std::cout<<"(nrConsumer.cc-ScheduleNextPacket) "<<"ID:"<<GetNode()->GetId()<<" 兴趣为空"<<std::endl;
		doConsumerCbrScheduleNextPacket();//added by siukwan
		return;
	}
	this->Consumer::SetAttribute("Prefix", StringValue(prefix));
	NS_LOG_INFO ("Node "<<GetNode()->GetId()<<" now is interestd on "<<prefix.data());
	//std::cout<<"(nrConsumer.cc-ScheduleNextPacket) NodeId "<<GetNode()->GetId()<<" now is interested on "<<prefix.data()<<std::endl;
	//3. Schedule next packet
	doConsumerCbrScheduleNextPacket();
}

std::vector<std::string> nrConsumer::GetCurrentInterest()
{
	//std::cout<<"进入(nrConsumer.cc-GetCurrentInterest)"<<endl;
	std::string prefix = "/";
	std::string str;
	std::vector<std::string> result;
	Ptr<NodeSensor> sensor = this->GetNode()->GetObject<NodeSensor>();
	const std::string& currentLane = sensor->getLane();
	std::vector<std::string>::const_iterator it;
	const std::vector<std::string>& route = sensor->getNavigationRoute();

	//遍历，寻找和当前道路相同的道路，把剩余的道路加入兴趣list中
	for(it=route.begin();it!=route.end();++it)
	{
		//std::cout<<"(nrConsumer.cc-GetCurrentInterest) NodeId "<<this->GetNode()->GetId()<<" "<<"route "<<*it <<"\t"<<"currentLane "<<currentLane.data() <<std::endl;
		if(*it==currentLane)//一直遍历寻找到当前道路，然后把后面的压紧容器返回
			break;
	}
	//int routeSum=0;//2015.9.25  小锟添加，只对未来的10条道路有兴趣。结论：没什么作用，兴趣包数量大，主要是由于转发次数过多造成
	for(;it!=route.end()/*&&routeSum<10*/;++it)
	{
		str=prefix+(*it);
		result.push_back(str);
	//	++routeSum;
	}
	//cout<<"(nrConsumer.cc-GetCurrentInterest) CurrentInterest :"<<" ";
	for(it = result.begin();it != result.end();it++)
	{
		//std::cout<<*it<<" ";
	}
	//std::cout<<std::endl;
	return result;
}

//changed by siukwan
void nrConsumer::doConsumerCbrScheduleNextPacket()
{
	//std::cout<<"进入(nrConsumer.cc-doConsumerCbrScheduleNextPacket) "<<std::endl;
	if (m_firstTime)
	{
		m_sendEvent = Simulator::Schedule (Seconds (0.0),&nrConsumer::SendPacket, this);
		m_firstTime = false;
	}
	//else if (!m_sendEvent.IsRunning ())
	  //  m_sendEvent = Simulator::Schedule ((m_random == 0) ?Seconds(1.0 / m_frequency):Seconds(m_random->GetValue ()),&nrConsumer::SendPacket, this);
}


//changed by siukwan
void nrConsumer::SendPacket()
{	
	if(!m_active)
	{
		return;
	}		 
	
	//added by sy
	Ptr<NodeSensor> sensor = this->GetNode()->GetObject<NodeSensor>();
	const std::string& currentType = sensor->getType();
	//RSU的Type为”RSU" RSU不发送兴趣包
	if(currentType == "RSU")
	{
		//cout<<"(nrConsumer.cc-SendPacket) 该节点为RSU "<<GetNode()->GetId()<<"不该产生并发送兴趣包"<<endl<<endl;
		return;
	}
	
	//std::cout<<"进入(nrConsumer.cc-SendPacket) "<<GetNode()->GetId()<<endl;
	
	NS_LOG_FUNCTION_NOARGS ();

	//返回编译器允许的uint32_t型数的最大值
	uint32_t seq=std::numeric_limits<uint32_t>::max (); //invalid

	//在ndn-consumer-cbr.cc中,m_seqMax被初始化为std::numeric_limits<uint32_t>::max ();
	if (m_seqMax != std::numeric_limits<uint32_t>::max())
	{
		if (m_seq >= m_seqMax)
		{
			return; // we are totally done
		}
	}

	//2017.12.13 Question:m_seq到底是啥
	seq = m_seq++;
	
	//2017.12.13 Question：m_interestName的定义
	Ptr<Name> nameWithSequence = Create<Name> (m_interestName);
	nameWithSequence->appendSeqNum (seq);

	//2017.12.13 added by sy 
	//这里需要得到兴趣路线
	std::vector<std::string> routes = ExtractActualRouteFromName(*nameWithSequence);
	std::string temp = "";
	for(int i = 0;i < (signed)routes.size();i++)
	{
		 temp+=routes[i];
		 temp+=" ";
		 
	}
	//std::cout<<"(nrConsumer.cc-SendPacket) temp "<<temp<<std::endl;
	
	std::string forwardingroutes = "";
	//const std::string& currentLane = sensor->getLane();
	std::vector<std::string>::const_iterator it;
	const std::vector<std::string>& route = sensor->getNavigationRoute();
	for(int i = 0;i < (signed)route.size();i++)
	{
		 forwardingroutes+=route[i];
		 forwardingroutes+=" ";
		 
	}
	//std::cout<<"(nrConsumer.cc-SendPacket) forwardingroutes "<<forwardingroutes<<std::endl;
	//getchar();
	
	Ptr<Interest> interest = Create<Interest> ();
	interest->SetNonce(m_rand.GetValue ());
	interest->SetName(nameWithSequence);
	interest->SetInterestLifetime(m_interestLifeTime);
	//2017.12.13 加入实际转发路线
	interest->SetRoutes(forwardingroutes);

	// NS_LOG_INFO ("Requesting Interest: \n" << *interest);
	NS_LOG_INFO ("> Interest for " <<nameWithSequence->toUri()<<" seq "<< seq);

	//WillSendOutInterest (seq);

	FwHopCountTag hopCountTag;
	interest->GetPayload ()->AddPacketTag (hopCountTag);

	m_transmittedInterests (interest, this, m_face);
	m_face->ReceiveInterest (interest);
	std::cout<<"(nrConsumer.cc-SendPacket) "<<GetNode()->GetId()<<std::endl<<std::endl;
	getchar();
	//ScheduleNextPacket ();
}

std::vector<std::string> nrConsumer::ExtractActualRouteFromName(const Name& name)
{
	// Name is in reverse order, so reverse it again
	// eg. if the navigation route is R1-R2-R3, the name is /R3/R2/R1
	std::vector<std::string> result;
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

void nrConsumer::OnData(Ptr<const Data> data)
{
	NS_LOG_FUNCTION (this);
	Ptr<Packet> nrPayload = data->GetPayload()->Copy();
	const Name& name = data->GetName();
	nrHeader nrheader;
	nrPayload->RemoveHeader(nrheader);
	uint32_t nodeId=nrheader.getSourceId();
	uint32_t signature=data->GetSignature();
	uint32_t packetPayloadSize = nrPayload->GetSize();

	NS_LOG_DEBUG("At time "<<Simulator::Now().GetSeconds()<<":"<<m_node->GetId()<<"\treceived data "<<name.toUri()<<" from "<<nodeId<<"\tSignature "<<signature<<"\t forwarded by("<<nrheader.getX()<<","<<nrheader.getY()<<")");
	NS_LOG_DEBUG("payload Size:"<<packetPayloadSize);
	std::cout<<"(nrConsumer.cc-OnData)"<<"At time "<<Simulator::Now().GetSeconds()<<" 当前节点 "<<m_node->GetId()<<" 收到数据包 "<<name.toUri()<<" 源节点 "<<nodeId<<" Signature "<<signature<<" forwarded by("<<nrheader.getX()<<","<<nrheader.getY()<<")\n";
	//getchar();
	NS_ASSERT_MSG(packetPayloadSize == m_virtualPayloadSize,"packetPayloadSize is not equal to "<<m_virtualPayloadSize);

	//延迟为数据包从发出到Consumer收到的时间
	double delay = Simulator::Now().GetSeconds() - data->GetTimestamp().GetSeconds();
	nrUtils::InsertTransmissionDelayItem(nodeId,signature,delay);
	if(IsInterestData(data->GetName()))
	{
		nrUtils::IncreaseInterestedNodeCounter(nodeId,signature);
		std::cout<<"(nrConsumer.cc-OnData) 当前节点对该数据包感兴趣"<<std::endl;
	}
	else
	{
		nrUtils::IncreaseDisinterestedNodeCounter(nodeId,signature);
		std::cout<<"(nrConsumer.cc-OnData) 当前节点对该数据包不感兴趣"<<std::endl;
	}
	std::cout<<std::endl;
	//NS_LOG_UNCOND("At time "<<Simulator::Now().GetSeconds()<<":"<<m_node->GetId()<<"\treceived data "<<name.toUri()<<" from "<<nodeId<<"\tSignature "<<signature);
}

void nrConsumer::NotifyNewAggregate()
{
  //std::cout<<"进入(nrConsumer.cc-NotifyNewAggregate) "<<endl;
  super::NotifyNewAggregate ();
}

void nrConsumer::DoInitialize(void)
{
	
	if (m_forwardingStrategy == 0)
	{
		//m_forwardingStrategy = GetObject<fw::nrndn::NavigationRouteHeuristic>();
		Ptr<ForwardingStrategy> forwardingStrategy=m_node->GetObject<ForwardingStrategy>();
		NS_ASSERT_MSG(forwardingStrategy,"nrConsumer::DoInitialize cannot find ns3::ndn::fw::ForwardingStrategy");
		m_forwardingStrategy = DynamicCast<fw::nrndn::NavigationRouteHeuristic>(forwardingStrategy);
	}
	if (m_sensor == 0)
	{
		m_sensor =  m_node->GetObject<ndn::nrndn::NodeSensor>();
		//std::cout<<"(nrConsumer.cc-DoInitialize) "<<GetNode()->GetId()<<endl;
		NS_ASSERT_MSG(m_sensor,"nrConsumer::DoInitialize cannot find ns3::ndn::nrndn::NodeSensor");
	}
	super::DoInitialize();
}

void nrConsumer::OnTimeout(uint32_t sequenceNumber)
{
	return;
}

//changed by sy
void nrConsumer::OnInterest(Ptr<const Interest> interest)
{
	//std::cout<<"(nrConsumer.cc-OnInterest) nrConsumer should not be supposed to receive Interest Packet!!"<<std::endl;
	//cout<<"(nrConsumer.cc-OnInterest)consumer收到兴趣包，触发发送兴趣包"<<endl;
	SendPacket();
	//getchar();
}

bool nrConsumer::IsInterestData(const Name& name)
{
	std::vector<std::string> result;
	Ptr<NodeSensor> sensor = this->GetNode()->GetObject<NodeSensor>();
	//获取当前路段
	const std::string& currentLane = sensor->getLane();
	std::vector<std::string>::const_iterator it;
	std::vector<std::string>::const_iterator it2;
	//获取导航路线
	const std::vector<std::string>& route = sensor->getNavigationRoute();

	//当前路段在导航路线中的位置
	it =std::find(route.begin(),route.end(),currentLane);

	//name是否会出现在未来的导航路线中
	//如是，则证明该节点对该数据感兴趣；否则不感兴趣
	it2=std::find(it,route.end(),name.get(0).toUri());

	return (it2!=route.end());
}

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */
