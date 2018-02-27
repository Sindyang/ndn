/*
 * ndn-nr-pit-impl.cc
 *
 *  Created on: Jan 20, 2015
 *      Author: chen
 */

#include "ndn-nr-pit-impl.h"
#include "ndn-pit-entry-nrimpl.h"
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


/* 2017.12.25 added by sy
 * junction为RSU所在交点的ID
 * forwardRoute:兴趣包的实际转发路线
 * interestRoute:兴趣包的兴趣路线（包含已走完的兴趣路线）
 * id为兴趣包源节点
 */
bool 
NrPitImpl::UpdateRSUPit(std::string junction,const std::string forwardRoute,const std::vector<std::string>& interestRoute, const uint32_t& id)
{
	std::size_t found = forwardRoute.find(" ");
	std::string currentroute = forwardRoute.substr(0,found);
	std::cout<<"(NrPitImpl.cc-UpdateRSUPit) 兴趣包当前所在路段为 "<<currentroute<<std::endl;
	std::vector<std::string>::const_iterator it = std::find(interestRoute.begin(),interestRoute.end(),currentroute);
	//该兴趣包来时的路段为兴趣路段
	if(it != interestRoute.end())
	{
		std::cout<<"(NrPitImpl.cc-UpdateRSUPit) 兴趣包来时的路段为兴趣路段"<<std::endl;
		bool result = UpdatePrimaryPit(interestRoute,id,currentroute);
		return result;
	}
	//该兴趣包来时的路段不是兴趣路段，RSU为借路RSU
	else
	{
		std::cout<<"(NrPitImpl.cc-UpdateRSUPit) 兴趣包来时的路段不是兴趣路段"<<std::endl;
		//更新副待处理兴趣列表 这个函数还没写
		// update secondary pit
		
		// 其实我觉得这里应该只有一条路
		bool result = true;
		std::pair<std::vector<std::string>,std::vector<std::string> > collection = getInterestRoutesReadytoPass(junction,forwardRoute,interestRoute);
		std::vector<std::string> futureInterestRoutes = collection.first;
		std::vector<std::string> unpassedRoutes = collection.second;
		
		std::cout<<"(NrPitImpl.cc-UpdateRSUPit)需要记录在副表中的兴趣路线为 "<<std::endl;
		for(uint32_t i = 0;i < futureInterestRoutes.size();i++)
		{
			std::cout<<futureInterestRoutes[i]<<" ";
		}
		std::cout<<std::endl;
		
		std::cout<<"(NrPitImpl.cc-UpdateRSUPit) 未来会通过该节点的兴趣路线"<<std::endl;
		for(uint32_t i = 0;i < unpassedRoutes.size();i++)
		{
			std::cout<<unpassedRoutes[i]<<" ";
		}
		std::cout<<std::endl;
		
		/*for(std::vector<std::string>::iterator it = unpassedRoutes.begin();it != unpassedRoutes.end();it++)
		{
			result = UpdatePrimaryPit(interestRoute,id,*it);
			result &= result;
		}*/
		
		
		getchar();
		return result;
	}
}

/*
 * 2017.12.25 added by sy
 * 得到未行驶且会通过当前RSU的兴趣路线
 */
std::pair<std::vector<std::string>,std::vector<std::string> >
NrPitImpl::getInterestRoutesReadytoPass(const std::string junction,const std::string forwardRoute,const std::vector<std::string>& interestRoute)
{
	std::vector<std::string> forwardRoutes;
	std::vector<std::string> futureInterestRoutes;
	std::vector<std::string> unpassedRoutes;
	
	std::vector<std::string>::iterator itforward;
	std::vector<std::string>::const_iterator itinterest;
	SplitString(forwardRoute,forwardRoutes," "); 
	//从实际转发路线中寻找第一条未行驶的兴趣路段
	for(itforward = forwardRoutes.begin();itforward != forwardRoutes.end();itforward++)
	{
		std::string lane = *itforward;
		itinterest = std::find(interestRoute.begin(),interestRoute.end(),lane);
		if(itinterest != interestRoute.end())
		{
			std::cout<<"(NrPitImpl.cc-getInterestRoutesReadytoPass) 从实际转发路线中找到了第一条未行驶的兴趣路段 "<<*itinterest<<std::endl;
			break;
		}
	}
	//加入未行驶的兴趣路段
	std::cout<<"(NrPitImpl.cc-getInterestRoutesReadytoPass) 未行驶的兴趣路线为 ";
	for(;itforward != forwardRoutes.end() && itinterest != interestRoute.end();itforward++,itinterest++)
	{
		if(*itforward == *itinterest)
		{
			futureInterestRoutes.push_back(*itforward);
			std::cout<<*itforward<<" ";
		}
		else
		{
			//TEST IT
			NS_ASSERT_MSG(false,"不知道为啥会进入这部分");
		}
	}
	std::cout<<std::endl;
	
	for(uint32_t i = 0;i < futureInterestRoutes.size();i++)
	{
		std::pair<std::string,std::string> junctions = m_sensor->GetLaneJunction(futureInterestRoutes[i]);
		
		//const map<string,vanetmobility::sumomobility::Edge>& edges = m_sumodata->getRoadmap().getEdges();
		//std::map<std::string,vanetmobility::sumomobility::Edge>::const_iterator eit;
		//eit = edges.find(futureInterestRoutes[i]);
		//if(eit != edges.end())
		//{
		if(junction == junctions.second)
		{
			unpassedRoutes.push_back(futureInterestRoutes[i]);
		}
		//}
	}
	
	
	//判断未行驶的兴趣路段的终点是否为junction
	/*for(std::vector<std::string>::iterator itunpassed = unpassedRoutes.begin();itunpassed != unpassedRoutes.end();)
	{
		const std::map<std::string,vanetmobility::sumomobility::Edge>& edges = m_sumodata->getRoadmap().getEdges();
		std::map<std::string,vanetmobility::sumomobility::Edge>::const_iterator eit;
		const std::string& lane = *itunpassed;
		std::cout<<lane<<std::endl;
		eit = edges.find(lane);
		
		if(eit != edges.end())
		{
			if(junction ==  eit->second.to)
			{
				std::cout<<"(NrPitImpl.cc-getInterestRoutesReadytoPass) 路段 "<<*itunpassed<<" 的终点为 "<<junction<<std::endl;
				itunpassed++;
			}
			else
			{
			itunpassed = unpassedRoutes.erase(itunpassed);
			}
		}
	}*/
	return std::pair<std::vector<std::string>,std::vector<std::string>>(futureInterestRoutes,unpassedRoutes);
	//return unpassedRoutes;
}

/*
 * 2017.12 25 added by sy
 * 分割字符串
 */
void
NrPitImpl::SplitString(const std::string& s,std::vector<std::string>& v,const std::string& c)
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


/*
 * 2017.12.24 added by sy
 * 更新主待处理兴趣列表
 * interestRoute:兴趣包的兴趣路线
 * id:兴趣包的源节点
 * currentRoute:兴趣包的源节点所对应的车辆在未来会经过的路段
 */
bool 
NrPitImpl::UpdatePrimaryPit(const std::vector<std::string>& interestRoute, const uint32_t& id,const std::string currentRoute)
{
	std::ostringstream os;
	std::vector<Ptr<Entry>>::iterator pit;
	//std::cout<<"(ndn-nr-pit-impl.cc-UpdatePrimaryPit) 车辆当前经过的路段为 "<<currentRoute<<std::endl;
	
	//判断当前路段是否出现在兴趣包的兴趣路线中
	std::vector<std::string>::const_iterator it = std::find(interestRoute.begin(),interestRoute.end(),currentRoute);
	
	//找不到
	if(it == interestRoute.end())
	{
		//std::cout<<"(ndn-nr-pit-impl.cc-UpdatePrimaryPit) 该兴趣包不该在主PIT中更新"<<std::endl;
		return false;
	}
	
	//将剩余的路线及节点加入PIT中
	for(;it != interestRoute.end();++it)
	{
		//std::cout<<"(ndn-nr-pit-impl.cc-UpdatePrimaryPit) 兴趣包的兴趣路段为 "<<*it<<std::endl;
		//寻找PIT中是否有该路段
		for(pit = m_pitContainer.begin();pit != m_pitContainer.end();++pit)
		{
			const name::Component &pitName = (*pit)->GetInterest()->GetName().get(0);
			//PIT中已经有该路段
			if(pitName.toUri() == *it)
			{
				//std::cout<<"(ndn-nr-pit-impl.cc-UpdatePrimaryPit) PIT中有该路段"<<std::endl;
				Ptr<EntryNrImpl> pitEntry = DynamicCast<EntryNrImpl>(*pit);
				pitEntry->AddIncomingNeighbors(currentRoute,id);
				os<<(*pit)->GetInterest()->GetName().toUri()<<" add Neighbor "<<id<<' ';
				break;
			}
		}
		//interestRoute不在PIT中
		if(pit == m_pitContainer.end())
		{
			//std::cout<<"(ndn-nr-pit-impl.cc-UpdatePrimaryPit) interestRoute "<<*it<<"不在PIT中"<<std::endl;
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
			pitEntry->AddIncomingNeighbors(currentRoute,id);
			os<<entry->GetInterest()->GetName().toUri()<<" add Neighbor "<<id<<' ';
		   // std::cout<<"(ndn-nr-pit-impl.cc-UpdatePrimaryPit) 兴趣的名字: "<<uriConvertToString(entry->GetInterest()->GetName().toUri())<<" "<<"add Neighbor "<<id<<std::endl;
			//getchar();
		}
		std::cout<<std::endl;
	}
	std::cout<<"(ndn-nr-pit-impl.cc-UpdatePrimaryPit)添加后 NodeId "<<id<<" 来时的路段为 "<<currentRoute<<std::endl;
	showPit();
	//getchar();
	NS_LOG_DEBUG("update PrimaryPit:"<<os.str());
	return true;
}

void 
NrPitImpl::showPit()
{
	std::cout<<"showPit"<<std::endl;
	for(uint32_t i = 0;i<m_pitContainer.size();++i)
	{
		Ptr<EntryNrImpl> pitEntry_siu = DynamicCast<EntryNrImpl>(m_pitContainer[i]);
		pitEntry_siu->listPitEntry();
	}
	std::cout<<std::endl;
}


/*
 * 2017.12.24
 * added by sy
 * lane为车辆当前所在路段
 */
std::pair<bool,uint32_t> NrPitImpl::DeleteFrontNode(const std::string lane,const uint32_t& id)
{
	//showPit();
	//std::cout<<"(ndn-nr-pit-impl.cc-DeleteFrontNode)"<<std::endl;
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
		bool flag = false;
		// 2018.1.28 
		for(pit = m_pitContainer.begin();pit != m_pitContainer.end();)
		{
			Ptr<EntryNrImpl> pitEntry = DynamicCast<EntryNrImpl>(*pit);
			pitEntry->CleanPITNeighbors(flag,id);
			//若PIT的表项为空，可以删除该表项
			const std::unordered_set<std::string>& interestroutes = pitEntry->getIncomingnbs();
			//std::cout<<"(pit-impl.cc-DeleteFrontNode) 上一跳路段数目 "<<interestroutes.size()<<std::endl;
			if(interestroutes.empty())
			{
				const name::Component &pitName=pitEntry->GetInterest()->GetName().get(0);
				std::string pitname = pitName.toUri();
				//std::cout<<"(ndn-nr-pit-impl.cc-DeleteFrontNode) PIT中 "<<pitname<<" 为空"<<std::endl;
				pit = m_pitContainer.erase(pit);
			}
			else
			{
				pit++;
			}
			//std::cout<<std::endl;
		}
		if(flag == false)
		{
			std::cout<<"(ndn-nr-pit-impl.cc-DeleteFrontNode) 节点 "<<id<<"不在PIT中"<<std::endl;
			return std::pair<bool,uint32_t>(false,id);
		}
	}
	else
	{
		std::cout<<"(ndn-nr-pit-impl.cc-DeleteFrontNode) "<<lane<<" 不在PIT中"<<std::endl;
		return std::pair<bool,uint32_t>(false,id);
	}
	showPit();
	//getchar();
	return std::pair<bool,uint32_t>(true,id);
}

std::unordered_map<std::string,std::unordered_set<std::string> >
NrPitImpl::GetDataNameandLastRoute(std::unordered_set<std::string> routes_behind)
{
	std::vector<Ptr<Entry> >::iterator pit;
	std::unordered_map<std::string,std::unordered_set<std::string> > dataandroutes;
	for(pit = m_pitContainer.begin();pit != m_pitContainer.end();pit++)
	{
		Ptr<EntryNrImpl> pitEntry = DynamicCast<EntryNrImpl>(*pit);
		std::string dataname = pitEntry->GetDataName();
		std::unordered_set<std::string>::iterator itroutes;
		std::unordered_set<std::string> collection;
		//std::cout<<"routes_behind的大小为 "<<routes_behind.size()<<std::endl;
		for(itroutes = routes_behind.begin();itroutes != routes_behind.end();itroutes++)
		{
			if(pitEntry->IsRouteInEntry(*itroutes))
			{
				collection.insert(*itroutes);
				//std::cout<<"(ndn-nr-pit-impl.cc-GetDataNameandLastRoute) 数据名为 "<<dataname<<" 上一跳路段为 "<<*itroutes<<std::endl;
			}
		}
		//std::cout<<std::endl;
		if(collection.size() >0)
			dataandroutes.insert(std::pair<std::string,std::unordered_set<std::string>>(dataname,collection));
	}
	return dataandroutes;
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
	/*const std::vector<std::string>& route =	m_sensor->getNavigationRoute();
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
	//std::cout<<std::endl;*/
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


