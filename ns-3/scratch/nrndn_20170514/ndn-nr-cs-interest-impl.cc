/*
 * ndn-nr-cs-impl.cc
 *
 *  Created on: Dec 18, 2017
 *      Author: DJ
 */

#include "ndn-nr-cs-interest-impl.h"
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
    .SetParent<ContentStoreInterest> ()
    .AddConstructor< NrCsInterestImpl > ()
    ;
  return tid;
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
  ContentStoreInterest::NotifyNewAggregate ();
}


bool NrCsInterestImpl::Add(Ptr<const Interest> interest)
{
    Ptr<cs::EntryInterest> csEntryInterest = ns3::Create<cs::EntryInterest>(this,interest) ;
    m_csInterestContainer.push_back(csEntryInterest);
	return true;
}


void
NrCsInterestImpl::DoDispose ()
{
	m_forwardingStrategy = 0;
	ContentStoreInterest::DoDispose ();
}
  
  
Ptr<EntryInterest>
NrCsInterestImpl::Find(const uint32_t nonce,const uint32_t sourceId)
{
	std::vector<Ptr<EntryInterest> >::iterator it;
	//NS_ASSERT_MSG(m_csInterestContainer.size()!=0,"Empty cs container. No initialization?");
	for(it=m_csInterestContainer.begin();it!=m_csInterestContainer.end();++it)
	{
		Ptr<Interest> interest = (*it)->GetInterest();
		ndn::nrndn::nrHeader nrheader;
        interest->GetPayload()->PeekHeader(nrheader);
        uint32_t nodeId = nrheader.getSourceId();
		
		if((*it)->GetNonce()==nonce && nodeId == sourceId)
			return *it;
	}
	return 0;
}
  

//Ptr<interest>
//NrCsInterestImpl::Lookup (Ptr<const Interest> interest)
//{
  // return 0;
//}

void
NrCsInterestImpl::MarkErased (Ptr<EntryInterest> item)
{
	NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::MarkErased (Ptr<EntryInterest> item) should not be invoked");
	return;
}
  
  
void
NrCsInterestImpl::Print (std::ostream& os) const
{
	std::vector<Ptr<EntryInterest>>::const_iterator it; 
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
  
Ptr<EntryInterest>
NrCsInterestImpl::Begin ()
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::Begin () should not be invoked");
	if(m_csInterestContainer.begin() == m_csInterestContainer.end())
		return End();
	else
		return *(m_csInterestContainer.begin());
}

Ptr<EntryInterest>
NrCsInterestImpl::End ()
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::End () should not be invoked");
	return 0;
}
  
Ptr<EntryInterest>
NrCsInterestImpl::Next (Ptr<EntryInterest> from)
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::Next () should not be invoked");
	if (from == 0) return 0;

	std::vector<Ptr<EntryInterest> >::iterator it;
	it = find(m_csInterestContainer.begin(),m_csInterestContainer.end(),from);
	if(it==m_csInterestContainer.end())
		return End();
	else
	{
		++it;
		if(it==m_csInterestContainer.end())
			return End();
		else
			return *it;
	}
}


vector<Ptr<Interest>>
NrCsInterestImpl::GetInterest(std::string lane)
{
	vector<Ptr<Interest> InterestCollection;
	std::vector<Ptr<EntryInterest>>::iterator it;
	for(it = m_csInterestContainer.begin();it != m_csInterestContainer.end();it++)
	{
		Ptr<Interest> interest = (*it)->GetInterest();
		vector<str::string> routes = interest->GetRoutes();
		if(routes.front() == lane)
			InterestCollection.push_back(interest);
	}
	return InterestCollection;
}


void NrCsInterestImpl::DoInitialize(void)
{
	ContentStoreInterest::DoInitialize();
}

} /* namespace nrndn */
} /* namespace cs */
} /* namespace ndn */
} /* namespace ns3 */


