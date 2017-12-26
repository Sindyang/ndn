/*
 * ndn-pit-entry-nrimpl.cc
 *
 *  Created on: Jan 21, 2015
 *      Author: chenyishun
 */


#include "ndn-pit-entry-nrimpl.h"
#include "ns3/ndn-interest.h"
#include "ns3/core-module.h"
#include "ns3/ndn-forwarding-strategy.h"

#include "ns3/log.h"
NS_LOG_COMPONENT_DEFINE ("ndn.pit.nrndn.EntryNrImpl");

namespace ns3 {
namespace ndn {

class Pit;

namespace pit {
namespace nrndn{
EntryNrImpl::EntryNrImpl(Pit &container, Ptr<const Interest> header,Ptr<fib::Entry> fibEntry,Time cleanInterval)
	:Entry(container,header,fibEntry),
	 m_infaceTimeout(cleanInterval)
{
	NS_ASSERT_MSG(header->GetName().size()<2,"In EntryNrImpl, "
			"each name of interest should be only one component, "
			"for example: /routeSegment, do not use more than one slash, "
			"such as/route1/route2/...");
	m_interest_name=header->GetName().toUri();
}

EntryNrImpl::~EntryNrImpl ()
{
  
}

//添加邻居信息
/*std::unordered_set< uint32_t >::iterator
EntryNrImpl::AddIncomingNeighbors(uint32_t id)
{
	//AddNeighborTimeoutEvent(id);
	std::unordered_set< uint32_t >::iterator incomingnb = m_incomingnbs.find(id);

	if(incomingnb==m_incomingnbs.end())
	{   //Not found
		std::pair<std::unordered_set< uint32_t >::iterator,bool> ret =
				m_incomingnbs.insert (id);
		return ret.first;
	}
	else
	{
		return incomingnb;
	}
}*/

/* 2017.12.24 added by sy 
 * 添加邻居信息
 * lane为兴趣包来时的路段,id为兴趣包的源节点
 */
std::unordered_map<std::string,std::unordered_set<uint32_t> >::iterator
EntryNrImpl::AddIncomingNeighbors(std::string lane,uint32_t id)
{
	std::cout<<"(ndn-pit-entry-nrimpl.cc-AddIncomingNeighbors) 兴趣包来时的路段为 "<<lane<<" 兴趣包源节点为 "<<id<<std::endl;
	std::unordered_map<std::string,std::unordered_set<uint32_t> > ::iterator incominglane = m_incomingnbs.find(lane);
	//未找到该路段
	if(incominglane == m_incomingnbs.end())
	{
		std::cout<<"(ndn-pit-entry-nrimpl.cc-AddIncomingNeighbors) 未在表项中找到该路段"<<std::endl;
		std::unordered_set<uint32_t> neighbors;
		//添加邻居信息
		neighbors.insert(id);
		
		std::pair<std::unordered_map<std::string,std::unordered_set<uint32_t> >,bool> ret;
		ret = m_incomingnbs.insert(std::pair<std::string,std::unordered_set<uint32_t>>(lane,neighbors));
		
		std::cout<<"(ndn-pit-entry-nrimpl.cc-AddIncomingNeighbors) 已添加该路段"<<std::endl;
		return ret.first;
	}
	else
	{
		std::cout<<"(ndn-pit-entry-nrimpl.cc-AddIncomingNeighbors) 在表项中找到该路段"<<std::endl;
		std::unordered_set<uint32_t> neighbors = incominglane->second;
		std::unordered_set<uint32_t>::iterator itcomingnb = neighbors.find(id);
		if(itcomingnb == neighbors.end())
		{
			std::cout<<"(ndn-pit-entry-nrimpl.cc-AddIncomingNeighbors) 未找到源节点"<<std::endl;
			incominglane->second.insert(id);
			std::cout<<"(ndn-pit-entry-nrimpl.cc-AddIncomingNeighbors) 已添加该节点"<<std::endl;
			return incominglane;
		}
		else
		{
			std::cout<<"(ndn-pit-entry-nrimpl.cc-AddIncomingNeighbors) 已找到源节点"<<std::endl;
			return incominglane;
		}
	}
	return incominglane;
}

//删除PIT中指定id的邻居，和CleanExpiredIncomingNeighbors一样
/*void EntryNrImpl::CleanPITNeighbors(uint32_t id)
{
	NS_LOG_DEBUG("At PIT Entry:"<<GetInterest()->GetName().toUri()<<" To delete neighbor:"<<id);
	std::unordered_set< uint32_t >::iterator incomingnb  = m_incomingnbs.find(id);
	if (incomingnb != m_incomingnbs.end())
	{
		m_incomingnbs.erase(incomingnb);
		std::cout<<std::endl<<"(ndn-pit-entry-nrimpl.cc-CleanPITNeighbors)删除邻居 "<<id<<".At time "<<Simulator::Now().GetSeconds()<<std::endl;
	}
	else
	{
		std::cout<<std::endl<<"(ndn-pit-entry-nrimpl.cc-CleanPITNeighbors) 邻居 "<<id<<" 并不在PIT该表项中"<<std::endl;
	}
}*/

// 2017.12.24 added by sy
void EntryNrImpl::CleanPITNeighbors(uint32_t id)
{
	NS_LOG_DEBUG("At PIT Entry:"<<GetInterest()->GetName().toUri()<<" To delete neighbor:"<<id);
	std::unordered_map<std::string,std::unordered_set<uint32_t> >::iterator it;
	for(it = m_incomingnbs.begin();it != m_incomingnbs.end();it++)
	{
		std::unordered_set<uint32_t> neighbors = it->second;
		std::unordered_set<uint32_t>::iterator incomingnb = neighbors.find(id);
		if(incomingnb != neighbors.end())
		{
			neighbors.erase(incomingnb);
			std::cout<<"(ndn-pit-entry-nrimpl.cc-CleanPITNeighbors)删除节点 "<<id<<" 节点上一跳所在路段为 "<<it->first
			<<" .At time "<<Simulator::Now().GetSeconds()<<std::endl;
		}
		else
		{
			std::cout<<"(ndn-pit-entry-nrimpl.cc-CleanPITNeighbors) 节点 "<<id<<" 并不在该表项对应的路段 "<<it->first<<" 中"<<std::endl;
		}	
	}
}

//删除所有节点
void EntryNrImpl::CleanAllNodes()
{
	m_incomingnbs.clear();
}

//cout表项内容
/*void EntryNrImpl::listPitEntry()
{
	std::cout<<"(pit-entry.cc-listPitEntry) interest_name："<<m_interest_name<<": ";
	for(std::unordered_set< uint32_t >::iterator ite = m_incomingnbs.begin();ite != m_incomingnbs.end();ite++)
	{
		std::cout<<*ite<<" ";
	}
	std::cout<<std::endl;
}*/

void EntryNrImpl::listPitEntry()
{
	std::cout<<"(ndn-pit-entry-nrimpl.cc-listPitEntry) interest_name："<<m_interest_name<<std::endl;
	for(std::unordered_map<std::string,std::unordered_set<uint32_t> >::iterator ite = m_incomingnbs.begin();ite != m_incomingnbs.end();ite++)
	{
		std::cout<<"上一跳路段为 "<<ite->first<<" 对应的节点为 ";
		std::unordered_set<uint32_t> neighbors = ite->second;
		std::unordered_set<uint32_t>::iterator it = neighbors.begin();
		for(;it != neighbors.end();it++)
		{
			std::cout<<*it<<" ";
		}
		std::cout<<std::endl;
	}
}

/*void EntryNrImpl::listPitEntry1(uint32_t node)
{
	std::cout<<"(pit-entry.cc-listPitEntry1) interest_name："<<m_interest_name<<": ";
	uint32_t temp;
	for(std::unordered_set< uint32_t >::iterator ite = m_incomingnbs.begin();ite != m_incomingnbs.end();ite++)
	{
		std::cout<<*ite<<" ";
		temp = *ite;
	}
	if(m_incomingnbs.size() == 1)
	{
		if(node == temp)
		{
			std::cout<<"当前节点在PIT列表中"<<std::endl;
		}
		else
		{
			std::cout<<"当前节点不在PIT列表中"<<std::endl;
		}
	}
	std::cout<<std::endl;
}*/

void EntryNrImpl::AddNeighborTimeoutEvent(uint32_t id)
{
	/*if(m_nbTimeoutEvent.find(id)!=m_nbTimeoutEvent.end())
	{
		m_nbTimeoutEvent[id].Cancel();
		Simulator::Remove (m_nbTimeoutEvent[id]); // slower, but better for memory
	}
	//Schedule a new cleaning event
	//std::cout<<"(ndn-pit-entry-nrimpl.cc)m_infaceTimeout "<<m_infaceTimeout<<std::endl;
	m_nbTimeoutEvent[id]
	                   = Simulator::Schedule(m_infaceTimeout,
	                		   &EntryNrImpl::CleanExpiredIncomingNeighbors,this,id);*/
}

//删除超时的邻居
void EntryNrImpl::CleanExpiredIncomingNeighbors(uint32_t id)
{
	/*std::unordered_set< uint32_t >::iterator it;

	NS_LOG_DEBUG("At PIT Entry:"<<GetInterest()->GetName().toUri()<<" To delete neighbor:"<<id);

	std::unordered_set< uint32_t >::iterator incomingnb  = m_incomingnbs.find(id);
	
	if (incomingnb != m_incomingnbs.end())
	{
		m_incomingnbs.erase(incomingnb);
		std::cout<<"删除邻居 "<<id<<std::endl;
	}	
	listPitEntry();*/
}

void EntryNrImpl::RemoveAllTimeoutEvent()
{
	//std::cout<<"(forwarding.cc-RemoveAllTimeoutEvent)"<<std::endl;
	/*std::unordered_map< uint32_t,EventId>::iterator it;
	for(it=m_nbTimeoutEvent.begin();it!=m_nbTimeoutEvent.end();++it)
	{
		it->second.Cancel();
		Simulator::Remove (it->second); // slower, but better for memory
	}*/
}

} // namespace nrndn
} // namespace pit
} // namespace ndn
} // namespace ns3


