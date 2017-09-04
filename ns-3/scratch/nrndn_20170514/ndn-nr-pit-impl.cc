/*
 * ndn-nr-pit-impl.cc
 *
 *  Created on: Jan 20, 2015
 *      Author: chen
 */

#include "ndn-nr-pit-impl.h"
#include "ndn-pit-entry-nrimpl.h"
//#include "NodeSensor.h"


#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("ndn.pit.NrPitImpl");

#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"

namespace ns3
{
namespace ndn
{
namespace pit
{
namespace nrndn
{


NS_OBJECT_ENSURE_REGISTERED (NrPitImpl);

TypeId
NrPitImpl::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ndn::pit::nrndn::NrPitImpl")
    .SetGroupName ("Ndn")
    .SetParent<Pit> ()
    .AddConstructor< NrPitImpl > ()
    .AddAttribute ("CleanInterval", "cleaning interval of the timeout incoming faces of PIT entry",
   			                    TimeValue (Seconds (10)),
   			                    MakeTimeAccessor (&NrPitImpl::m_cleanInterval),
   			                    MakeTimeChecker ())
    ;

  return tid;
}

NrPitImpl::NrPitImpl ():
		m_cleanInterval(Seconds(10.0))
{
	//std::cout<<"(ndn-nr-pit-impl.cc-NrPitImpl)m_cleanInterval "<<m_cleanInterval<<std::endl;
}

NrPitImpl::~NrPitImpl ()
{
}


void
NrPitImpl::NotifyNewAggregate ()
{
	//std::cout<<"(ndn-nr-pit-impl.cc-NotifyNewAggregate)m_cleanInterval "<<m_cleanInterval<<std::endl;
	if (m_fib == 0)
	{
		m_fib = GetObject<Fib>();
	}
	if (m_forwardingStrategy == 0)
	{
		m_forwardingStrategy = GetObject<ForwardingStrategy>();
	}

	if (m_sensor == 0)
	{
		m_sensor = GetObject<ndn::nrndn::NodeSensor>();
		// Setup Lane change action
		if (m_sensor != NULL)
		{
			m_sensor->TraceConnectWithoutContext("LaneChange",
					MakeCallback(&NrPitImpl::laneChange, this));
			//PIT需要m_sensor，所以在m_sensor初始化后，马上初始化PIT表
			//std::cout<<"(ndn-nr-pit-impl.cc-NotifyNewAggregate)初始化PIT"<<std::endl;
			InitializeNrPitEntry();
		}
	}

  Pit::NotifyNewAggregate ();
}

//获取车辆当前所在的路段
std::string NrPitImpl::getCurrentLane()
{
	std::vector<Ptr<Entry> >::iterator pit=m_pitContainer.begin();
	Ptr<Entry> entry = *pit;
	//获取兴趣
	Name::const_iterator head=entry->GetInterest()->GetName().begin();
	//当前路段为：uriConvertToString(head->toUri())
	return uriConvertToString(head->toUri());
}

//更新普通车辆的PIT表
bool 
NrPitImpl::UpdateCarPit(const std::vector<std::string>& route,const uint32_t& id)
{
	std::ostringstream os;
	std::vector<Ptr<Entry> >::iterator pit=m_pitContainer.begin();
	Ptr<Entry> entry = *pit;
	//head->toUri()为车辆当前所在的路段
	Name::const_iterator head=entry->GetInterest()->GetName().begin();
	//Can name::Component use "=="?
	std::vector<std::string>::const_iterator it=
			std::find(route.begin(),route.end(),head->toUri());
			
	//当前路段为：uriConvertToString(head->toUri())
	//std::cout<<"(ndn-nr-pit-impl.cc-UpdateCarPit)节点当前所在路段 "<<uriConvertToString(head->toUri())<<std::endl;
	
	//判断当前路段是否出现在收到的兴趣包的兴趣路线中
	//找不到
	if(it==route.end())
		return false;
	//找到，把后面的添加进去
	for(;pit!=m_pitContainer.end()&&it!=route.end();++pit,++it)
	{
		const name::Component &pitName=(*pit)->GetInterest()->GetName().get(0);
		//std::cout<<"(ndn-nr-pit-impl.cc-UpdateCarPit) pitName: "<<uriConvertToString(pitName.toUri())<<std::endl;
		if(pitName.toUri() == *it)
		{
			Ptr<EntryNrImpl> pitEntry = DynamicCast<EntryNrImpl>(*pit);
			pitEntry->AddIncomingNeighbors(id);
			os<<(*pit)->GetInterest()->GetName().toUri()<<" add Neighbor "<<id<<' ';
			//std::cout<<"(ndn-nr-pit-impl.cc-UpdateCarPit) 兴趣的名字: "<<uriConvertToString((*pit)->GetInterest()->GetName().toUri())<<" "<<"add Neighbor "<<id<<std::endl;
			//getchar();
		}
		else
			break;
	}
	//std::cout<<"(ndn-nr-pit-impl.cc-UpdatePit)添加后 NodeId "<<id<<std::endl;
	showPit();
	//getchar();
	//NS_LOG_UNCOND("update pit:"<<os.str());
	NS_LOG_DEBUG("update Carpit:"<<os.str());
	return true;
}


//added by sy
bool 
NrPitImpl::UpdateRSUPit(const std::vector<std::string>& route, const uint32_t& id)
{
	std::ostringstream os;
	std::vector<Ptr<Entry>>::iterator pit;
	//获取RSU当前所在路线
	const std::string& CurrentRoute = m_sensor->getLane();
	//std::cout<<"(ndn-nr-pit-impl.cc-UpdateRSUPit) RSU所在路段为 "<<CurrentRoute<<std::endl;
	
	//判断当前路段是否出现在收到的兴趣包的兴趣路线中
	std::vector<std::string>::const_iterator it = std::find(route.begin(),route.end(),CurrentRoute);
	
	//找不到
	if(it == route.end())
	{
		std::cout<<"(ndn-nr-pit-impl.cc-UpdateRSUPit) RSU不该收到该兴趣包"<<std::endl;
		return false;
	}
	
	//将剩余的路线及节点加入PIT中
	for(;it != route.end();++it)
	{
		//std::cout<<"(ndn-nr-pit-impl.cc-UpdateRSUPit) 兴趣包的兴趣路段为 "<<*it<<std::endl;
		//寻找PIT中是否有该路段
		for(pit = m_pitContainer.begin();pit != m_pitContainer.end();++pit)
		{
			const name::Component &pitName = (*pit)->GetInterest()->GetName().get(0);
			//PIT中已经有该路段
			if(pitName.toUri() == *it)
			{
				//std::cout<<"(ndn-nr-pit-impl.cc-UpdateRSUPit) PIT中有该路段"<<std::endl;
				Ptr<EntryNrImpl> pitEntry = DynamicCast<EntryNrImpl>(*pit);
				pitEntry->AddIncomingNeighbors(id);
				os<<(*pit)->GetInterest()->GetName().toUri()<<" add Neighbor "<<id<<' ';
				break;
			}
		}
		//route不在PIT中
		if(pit == m_pitContainer.end())
		{
			//std::cout<<"(ndn-nr-pit-impl.cc-UpdateRSUPit) route "<<*it<<"不在PIT中"<<std::endl;
			//创建一个新的表项
			Ptr<Name> name = ns3::Create<Name>('/'+*it);
			Ptr<Interest> interest = ns3::Create<Interest>();
			interest->SetName(name);
			interest->SetInterestLifetime(Time::Max());//never expire
			
			//Create a fake FIB entry(if not ,L3Protocol::RemoveFace will have problem when using pitEntry->GetFibEntry)
		    Ptr<fib::Entry> fibEntry=ns3::Create<fib::Entry>(Ptr<Fib>(0),Ptr<Name>(0));
			
			Ptr<Entry> entry = ns3::Create<EntryNrImpl>(*this,interest,fibEntry,Seconds(10.0)) ;
		    m_pitContainer.push_back(entry);
			Ptr<EntryNrImpl> pitEntry = DynamicCast<EntryNrImpl>(entry);
			pitEntry->AddIncomingNeighbors(id);
			os<<entry->GetInterest()->GetName().toUri()<<" add Neighbor "<<id<<' ';
		    //std::cout<<"(ndn-nr-pit-impl.cc-UpdateRSUPit) 兴趣的名字: "<<uriConvertToString(entry->GetInterest()->GetName().toUri())<<" "<<"add Neighbor "<<id<<std::endl;
			//getchar();
		}
	}
	//std::cout<<"(ndn-nr-pit-impl.cc-UpdateRSUPit)添加后 NodeId "<<id<<std::endl;
	showPit();
	//getchar();
	NS_LOG_DEBUG("update RSUpit:"<<os.str());
	return true;
}

void 
NrPitImpl::showPit()
{
	for(uint32_t i = 0;i<m_pitContainer.size();++i)
	{
		Ptr<EntryNrImpl> pitEntry_siu = DynamicCast<EntryNrImpl>(m_pitContainer[i]);
		pitEntry_siu->listPitEntry();
	}
	std::cout<<std::endl;
}

//added by sy
void 
NrPitImpl::DeleteFrontNode(const std::string lane,const uint32_t& id)
{
	std::vector<Ptr<Entry> >::iterator pit;
	//找到lane在PIT表项中的位置
	for(pit = m_pitContainer.begin();pit != m_pitContainer.end();pit++)
	{
		const name::Component &pitName = (*pit)->GetInterest()->GetName().get(0);
		if(pitName.toUri() == lane)
		{
			break;
		}
	}
	
	if(pit != m_pitContainer.end())
	{
		std::cout<<"(ndn-nr-pit-impl.cc-DeleteFrontNode) 已找到 "<<lane<<" 在PIT表项中的位置"<<std::endl;
		std::cout<<"(ndn-nr-pit-impl.cc-DeleteFrontNode) 准备删除节点 "<<id<<"。At time "<<Simulator::Now().GetSeconds()<<std::endl;
		for(;pit != m_pitContainer.end();pit++)
		{
			Ptr<EntryNrImpl> pitEntry = DynamicCast<EntryNrImpl>(*pit);
			pitEntry->CleanPITNeighbors(id);
			//若PIT的表项为空，可以删除该表项
			//只有RSU的PIT才有为空的可能性，因为普通车辆的PIT表项中含有自身节点
			const std::unordered_set<uint32_t>& interestNodes = pitEntry->getIncomingnbs();
			if(interestNodes.empty())
			{
				const name::Component &pitName=pitEntry->GetInterest()->GetName().get(0);
				std::string pitname = pitName.toUri();
				std::cout<<"(ndn-nr-pit-impl.cc-DeleteFrontNode) PIT中 "<<pitname<<" 为空"<<std::endl;
				pit = m_pitContainer.erase(pit);
			}
		}
	}
	else
	{
		std::cout<<"(ndn-nr-pit-impl.cc-DeleteFrontNode) "<<lane<<" 不在PIT中"<<std::endl;
	}
	showPit();
}

void
NrPitImpl::DoDispose ()
{
	m_forwardingStrategy = 0;
	//m_fib = 0;
	
	Pit::DoDispose ();
 }
  

Ptr<Entry>
NrPitImpl::Lookup (const Data &header)
{
	NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Lookup (const Data &header) should not be invoked");
	return 0;
}
  
Ptr<Entry>
NrPitImpl::Lookup (const Interest &header)
{
	NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Lookup (const Interest &header) should not be invoked");
	return 0;
 }
  

Ptr<Entry>
NrPitImpl::Find (const Name &prefix)
{
	//NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Find (const Name &prefix) should not be invoked");
	 NS_LOG_INFO ("Finding prefix"<<prefix.toUri());
	 std::vector<Ptr<Entry> >::iterator it;
	  if(m_pitContainer.size() == 0)
	 	return 0;
	 //NS_ASSERT_MSG(m_pitContainer.size()!=0,"Empty pit container. No initialization?");
	 for(it=m_pitContainer.begin();it!=m_pitContainer.end();++it)
	 {
		  //如果找到兴趣，就返回入口
		 if((*it)->GetPrefix()==prefix)
			 return *it;
	 }
	return 0;
}
  

Ptr<Entry>
NrPitImpl::Create (Ptr<const Interest> header)
{

	NS_LOG_DEBUG (header->GetName ());
	NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Create (Ptr<const Interest> header) "
			"should not be invoked, use "
			"NrPitImpl::CreateNrPitEntry instead");
	return 0;
}

//PIT表初始化
bool
NrPitImpl::InitializeNrPitEntry()
{
	NS_LOG_FUNCTION (this);
	//获取所有的导航路线
	const std::vector<std::string>& route =	m_sensor->getNavigationRoute();
	//added by sy
	uint32_t id = m_sensor->getNodeId();
	std::vector<std::string>::const_iterator rit;
	//每个路段配置一个兴趣
	for(rit=route.begin();rit!=route.end();++rit)
	{
		Ptr<Name> name = ns3::Create<Name>('/'+*rit);
		Ptr<Interest> interest=ns3::Create<Interest> ();
		interest->SetName(name);
		interest->SetInterestLifetime(Time::Max());//never expire

		//Create a fake FIB entry(if not ,L3Protocol::RemoveFace will have problem when using pitEntry->GetFibEntry)
		Ptr<fib::Entry> fibEntry=ns3::Create<fib::Entry>(Ptr<Fib>(0),Ptr<Name>(0));

		//sy:m_cleanInterval在构造函数中已经被赋值，但这里仍然是0，并不知道为什么
		//Ptr<Entry> entry = ns3::Create<EntryNrImpl>(*this,interest,fibEntry,m_cleanInterval) ;
		Ptr<Entry> entry = ns3::Create<EntryNrImpl>(*this,interest,fibEntry,Seconds(10.0)) ;
		m_pitContainer.push_back(entry);
		//added by sy
		Ptr<EntryNrImpl> pitEntry = DynamicCast<EntryNrImpl>(entry);
		//将节点自身添加到PIT表中
		pitEntry->AddIncomingNeighbors(id);
		NS_LOG_DEBUG("Initialize pit:Push_back"<<name->toUri());
		//std::cout<<"(ndn-nr-pit-impl.cc-InitializeNrPitEntry) name: "<<uriConvertToString(name->toUri())<<std::endl;
	}
	//std::cout<<std::endl;
	return true;
}
  

void
NrPitImpl::MarkErased (Ptr<Entry> item)
{
	NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::MarkErased (Ptr<Entry> item) should not be invoked");
	return;
}
  
  
void
NrPitImpl::Print (std::ostream& os) const
{

}

uint32_t
NrPitImpl::GetSize () const
{
	return m_pitContainer.size ();
}
  
Ptr<Entry>
NrPitImpl::Begin ()
{
	//NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Begin () should not be invoked");

	if(m_pitContainer.begin() == m_pitContainer.end())
		return End();
	else
		return *(m_pitContainer.begin());
}

Ptr<Entry>
NrPitImpl::End ()
{
	//NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::End () should not be invoked");
	return 0;
}
  
Ptr<Entry>
NrPitImpl::Next (Ptr<Entry> from)
{
	//NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Next () should not be invoked");
	if (from == 0)
	{
		return 0;
	}

	std::vector<Ptr<Entry> >::iterator it;
	it = find(m_pitContainer.begin(),m_pitContainer.end(),from);
	if(it==m_pitContainer.end())
		return End();
	else
	{
		++it;
		if(it==m_pitContainer.end())
			return End();
		else
			return *it;
	}
}

//小锟添加，2015-8-23
std::string NrPitImpl::uriConvertToString(std::string str)
{
	//因为获取兴趣时使用toUri，避免出现类似[]的符号，进行编码转换
	std::string ret="";
	for(uint32_t i=0;i<str.size();i++)
	{
		if(i+2<str.size())
		{
			if(str[i]=='%'&&str[i+1]=='5'&&str[i+2]=='B')
			{
				ret+="[";
				i=i+2;
			}
			else if(str[i]=='%'&&str[i+1]=='5'&&str[i+2]=='D')
			{
				ret+="]";
				i=i+2;
			}
			else if(str[i]=='%'&&str[i+1]=='2'&&str[i+2]=='3')
			{
				ret+="#";
				i=i+2;
			}
			else
				ret+=str[i];
		}
		else
			ret+=str[i];
	}
	return ret;
}

void NrPitImpl::laneChange(std::string oldLane, std::string newLane)
{
	if (oldLane.empty()
			|| (ndn::nrndn::NodeSensor::emptyLane == oldLane
					&& ndn::nrndn::NodeSensor::emptyLane != newLane))
		return;
	NS_LOG_INFO ("Deleting old lane pit entry of "<<oldLane);

	std::vector<Ptr<Entry> >::iterator it;
	it =m_pitContainer.begin();
	if(it == m_pitContainer.end())
	{   //pit表为空
		return;
	}

	bool IsOldLaneAtPitBegin =(uriConvertToString((*it)->GetInterest()->GetName().get(0).toUri())==(oldLane));

	if(!IsOldLaneAtPitBegin)
	{
		std::cout<<"(ndn-nr-pit-impl.cc-laneChange)"<<"旧路段不在头部:"<<"oldLane:"<<(oldLane)<<" newLane:"<<uriConvertToString((*it)->GetInterest()->GetName().get(0).toUri())<<std::endl;

		//遍历整个Pit
		std::vector<Ptr<Entry> >::iterator itTraversal;
		itTraversal =m_pitContainer.begin();
		bool findOldLane=false;
		std::cout<<"(ndn-nr-pit-impl.cc-laneChange)寻找oldLane中...\n";
		//遍历整个PIT表，寻找oldLane是否在表中
		for(;itTraversal!=m_pitContainer.end();itTraversal++)
		{   
			if( uriConvertToString((*itTraversal)->GetInterest()->GetName().get(0).toUri()) == (oldLane) )
			{   //如果找到则直接跳出
				findOldLane=true;
				break;
			}
		}
		if(findOldLane)
		{
			it = m_pitContainer.begin();
			int a = 0;
			//删除旧路段之前的路段
			while(uriConvertToString((*it)->GetInterest()->GetName().get(0).toUri())!=(oldLane)&&it!=m_pitContainer.end())
			{
				std::cout<<"(ndn-nr-pit-impl.cc-laneChange)"<<a<<"遍历删除中："<<uriConvertToString( (*it)->GetInterest()->GetName().get(0).toUri())<<" OLd:"<<(oldLane)<<std::endl;
				a++;
				DynamicCast<EntryNrImpl>(*it)->RemoveAllTimeoutEvent();
				m_pitContainer.erase(it);
				it =m_pitContainer.begin();
			}
			//删除旧路段
			if(it<=m_pitContainer.end())
			{
				std::cout<<"(ndn-nr-pit-impl.cc-laneChange)"<<"最后遍历删除中："<<uriConvertToString( (*it)->GetInterest()->GetName().get(0).toUri())<<" OLd:"<<(oldLane)<<std::endl;
				//1. Befor erase it, cancel all the counting Timer fore the neighbor to expire
				DynamicCast<EntryNrImpl>(*it)->RemoveAllTimeoutEvent();
				//2. erase it
				m_pitContainer.erase(it);
				std::cout<<"(ndn-nr-pit-impl.cc-laneChange)"<<"删除完毕\n";
			}
			else
				std::cout<<"(ndn-nr-pit-impl.cc-laneChange)"<<"删除完毕：迭代器为空\n";
		}
		else
		{
			std::cout<<"(ndn-nr-pit-impl.cc-laneChange)"<<"没找到旧路段...\n";
		}
	}
	else
	{   
        //旧路段在pit头部才进行删除
		//报错？
		//NS_ASSERT_MSG(IsOldLaneAtPitBegin,"The old lane should at the beginning of the pitContainer. Please Check~");
		//1. Befor erase it, cancel all the counting Timer fore the neighbor to expire
		DynamicCast<EntryNrImpl>(*it)->RemoveAllTimeoutEvent();

		//2. erase it
		m_pitContainer.erase(it);
		//std::cout<<"(ndn-nr-pit-impl.cc-laneChange)"<<"erase OK!"<<std::endl;
		return;
	}
}

void NrPitImpl::DoInitialize(void)
{
	Pit::DoInitialize();
}

} /* namespace nrndn */
} /* namespace pit */
} /* namespace ndn */
} /* namespace ns3 */


