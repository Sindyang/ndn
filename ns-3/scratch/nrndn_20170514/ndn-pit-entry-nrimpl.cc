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
std::unordered_set< uint32_t >::iterator
EntryNrImpl::AddIncomingNeighbors(uint32_t id)
{
	AddNeighborTimeoutEvent(id);
	std::unordered_set< uint32_t >::iterator incomingnb = m_incomingnbs.find(id);

	if(incomingnb==m_incomingnbs.end())
	{//Not found
		std::pair<std::unordered_set< uint32_t >::iterator,bool> ret =
				m_incomingnbs.insert (id);
		return ret.first;
	}
	else
	{
		return incomingnb;
	}
}

void EntryNrImpl::AddNeighborTimeoutEvent(uint32_t id)
{
	if(m_nbTimeoutEvent.find(id)!=m_nbTimeoutEvent.end())
	{
		m_nbTimeoutEvent[id].Cancel();
		Simulator::Remove (m_nbTimeoutEvent[id]); // slower, but better for memory
	}
	//Schedule a new cleaning event
	m_nbTimeoutEvent[id]
	                   = Simulator::Schedule(m_infaceTimeout,
	                		   &EntryNrImpl::CleanExpiredIncomingNeighbors,this,id);
}

void EntryNrImpl::CleanExpiredIncomingNeighbors(uint32_t id)
{
	std::unordered_set< uint32_t >::iterator it;

	//std::ostringstream os;
	//os<<"At PIT Entry:"<<GetInterest()->GetName().toUri()<<" To delete neighbor:"<<id;
	/*
	os<<"To delete neighbor:"<<id<<"\tList is\t";
	for(it=m_incomingnbs.begin();it!=m_incomingnbs.end();++it)
	{
		os<<*it<<"\t";
	}
	*/
	NS_LOG_DEBUG("At PIT Entry:"<<GetInterest()->GetName().toUri()<<" To delete neighbor:"<<id);

	std::unordered_set< uint32_t >::iterator incomingnb  = m_incomingnbs.find(id);

	if (incomingnb != m_incomingnbs.end())
	{
		m_incomingnbs.erase(incomingnb);
		std::cout<<"(ndn-pit-entry-nrimpl-CleanExpiredIncomingNeighbors)删除邻居 "<<id<<std::endl;
	}	
	listPitEntry();
}

//删除PIT中指定id的邻居，和CleanExpiredIncomingNeighbors一样
void EntryNrImpl::CleanPITNeighbors(uint32_t id)
{
	std::unordered_set< uint32_t >::iterator it;
	NS_LOG_DEBUG("At PIT Entry:"<<GetInterest()->GetName().toUri()<<" To delete neighbor:"<<id);
	std::unordered_set< uint32_t >::iterator incomingnb  = m_incomingnbs.find(id);
	if (incomingnb != m_incomingnbs.end())
	{
		m_incomingnbs.erase(incomingnb);
		std::cout<<"(ndn-pit-entry-nrimpl-CleanPITNeighbors)删除邻居 "<<id<<std::endl;
	}
		
}

void EntryNrImpl::CleanAllNodes()
{
	m_incomingnbs.clear();
}

//cout表项内容
void EntryNrImpl::listPitEntry()
{
	std::cout<<"(pit-entry.cc-listPitEntry) interest_name："<<m_interest_name<<":";
	//std::cout<<"(pit-entry.cc-listPitEntry)id及耗时:"<<std::endl;
	/*for(std::unordered_map< uint32_t,EventId>::iterator ite = m_nbTimeoutEvent.begin();ite!=m_nbTimeoutEvent.end();ite++)
	{
		std::cout<<ite->first<<"("<<ite->second<<") ";
	}*/
	//std::cout<<"(pit-entry.cc-listPitEntry)"<<"incomingnbs's NodeId:";
	for(std::unordered_set< uint32_t >::iterator ite = m_incomingnbs.begin();ite != m_incomingnbs.end();ite++)
	{
		std::cout<<*ite<<" ";
	}
	std::cout<<std::endl;
}

void EntryNrImpl::RemoveAllTimeoutEvent()
{
	std::unordered_map< uint32_t,EventId>::iterator it;
	for(it=m_nbTimeoutEvent.begin();it!=m_nbTimeoutEvent.end();++it)
	{
		it->second.Cancel();
		Simulator::Remove (it->second); // slower, but better for memory
	}
}

} // namespace nrndn
} // namespace pit
} // namespace ndn
} // namespace ns3


