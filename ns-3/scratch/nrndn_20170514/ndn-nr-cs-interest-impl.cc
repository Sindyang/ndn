/*
 * ndn-nr-cs-impl.cc
 *
 *  Created on: Dec 18, 2017
 *      Author: DJ
 */

#include "ndn-nr-cs-interest-impl.h"
#include "nrHeader.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("ndn.cs.NrCsInterestImpl");

#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"

namespace ns3
{
namespace ndn
{
namespace cs
{
namespace nrndn
{


NS_OBJECT_ENSURE_REGISTERED (NrCsInterestImpl);

TypeId
NrCsInterestImpl::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ndn::cs::nrndn::NrCsInterestImpl")
    .SetGroupName ("Ndn")
    .SetParent<ContentStore> ()
    .AddConstructor< NrCsInterestImpl > ()
    ;
  return tid;
}

NrCsInterestImpl::NrCsInterestImpl ()
{
	
}

NrCsInterestImpl::~NrCsInterestImpl ()
{
	
}


void
NrCsInterestImpl::NotifyNewAggregate ()
{
	if (m_forwardingStrategy == 0)
	{
		m_forwardingStrategy = GetObject<ForwardingStrategy>();
	}
  ContentStore::NotifyNewAggregate ();
}


bool NrCsInterestImpl::AddInterest(Ptr<const Interest> interest)
{
	std::cout<<"(NrCsInterestImpl-Add)"<<std::endl;
    Ptr<cs::EntryInterest> csEntryInterest = ns3::Create<cs::EntryInterest>(this,interest) ;
    m_csInterestContainer.push_back(csEntryInterest);
	PrintCache();
	return true;
}

bool NrCsInterestImpl::Add (Ptr<const Data> data)
{
	return false;
}

void
NrCsInterestImpl::DoDispose ()
{
	m_forwardingStrategy = 0;
	ContentStore::DoDispose ();
}
  
  
Ptr<cs::EntryInterest>
NrCsInterestImpl::Find(const uint32_t nonce,const uint32_t sourceId)
{
	std::vector<Ptr<cs::EntryInterest> >::iterator it;
	//NS_ASSERT_MSG(m_csInterestContainer.size()!=0,"Empty cs container. No initialization?");
	for(it=m_csInterestContainer.begin();it!=m_csInterestContainer.end();++it)
	{
		Ptr<const Interest> interest = (*it)->GetInterest();
		ndn::nrndn::nrHeader nrheader;
        interest->GetPayload()->PeekHeader(nrheader);
        uint32_t nodeId = nrheader.getSourceId();
		
		if(interest->GetNonce()==nonce && nodeId == sourceId)
			return *it;
	}
	return 0;
}
  

Ptr<Data>
NrCsInterestImpl::Lookup (Ptr<const Interest> interest)
{
   return 0;
}

void
NrCsInterestImpl::MarkErased (Ptr<cs::EntryInterest> item)
{
	NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::MarkErased (Ptr<EntryInterest> item) should not be invoked");
	return;
}
  
  
void
NrCsInterestImpl::Print (std::ostream& os) const
{
}

void
NrCsInterestImpl::PrintCache () const
{
	std::vector<Ptr<cs::EntryInterest> >::const_iterator it; 
	for(it=m_csInterestContainer.begin();it!=m_csInterestContainer.end();++it)
	{
		std::cout<<(*it)->GetInterest()->GetNonce()<<" "<<(*it)->GetName().toUri()<<std::endl;
	}
	std::cout<<std::endl;
}


uint32_t
NrCsInterestImpl::GetSize () const
{
	return m_csInterestContainer.size ();
}

Ptr<cs::Entry>
NrCsInterestImpl::Begin ()
{
	return 0;
}
  
Ptr<cs::EntryInterest>
NrCsInterestImpl::BeginEntryInterest ()
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::Begin () should not be invoked");
	if(m_csInterestContainer.begin() == m_csInterestContainer.end())
		return EndEntryInterest();
	else
		return *(m_csInterestContainer.begin());
}

Ptr<cs::Entry>
NrCsInterestImpl::End ()
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::End () should not be invoked");
	return 0;
}

Ptr<cs::EntryInterest>
NrCsInterestImpl::EndEntryInterest ()
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::End () should not be invoked");
	return 0;
}
  
Ptr<cs::Entry>
NrCsInterestImpl::Next (Ptr<cs::Entry>)
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::End () should not be invoked");
	return 0;
}

Ptr<cs::EntryInterest>
NrCsInterestImpl::NextEntryInterest (Ptr<cs::EntryInterest> from)
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::Next () should not be invoked");
	if (from == 0) return 0;

	std::vector<Ptr<cs::EntryInterest> >::iterator it;
	it = find(m_csInterestContainer.begin(),m_csInterestContainer.end(),from);
	if(it==m_csInterestContainer.end())
		return EndEntryInterest();
	else
	{
		++it;
		if(it==m_csInterestContainer.end())
			return EndEntryInterest();
		else
			return *it;
	}
}


std::vector<Ptr<const Interest> >
NrCsInterestImpl::GetInterest(std::string lane)
{
	std::vector<Ptr<const Interest> > InterestCollection;
	std::vector<Ptr<cs::EntryInterest> >::iterator it;
	for(it = m_csInterestContainer.begin();it != m_csInterestContainer.end();it++)
	{
		Ptr<const Interest> interest = (*it)->GetInterest();
		std::vector<std::string> routes = interest->GetRoutes();
		if(routes.front() == lane)
			InterestCollection.push_back(interest);
	}
	return InterestCollection;
}


void NrCsInterestImpl::DoInitialize(void)
{
	ContentStore::DoInitialize();
}

} /* namespace nrndn */
} /* namespace cs */
} /* namespace ndn */
} /* namespace ns3 */


