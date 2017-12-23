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
}


//删除PIT中指定id的邻居，和CleanExpiredIncomingNeighbors一样
void EntryNrImpl::CleanPITNeighbors(uint32_t id)
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
}

//删除所有节点
void EntryNrImpl::CleanAllNodes()
{
	m_incomingnbs.clear();
}

//cout表项内容
void EntryNrImpl::listPitEntry()
{
	std::cout<<"(pit-entry.cc-listPitEntry) interest_name："<<m_interest_name<<": ";
	for(std::unordered_set< uint32_t >::iterator ite = m_incomingnbs.begin();ite != m_incomingnbs.end();ite++)
	{
		std::cout<<*ite<<" ";
	}
	std::cout<<std::endl;
}

void EntryNrImpl::listPitEntry1(uint32_t node)
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
}


} // namespace nrndn
} // namespace pit
} // namespace ndn
} // namespace ns3


