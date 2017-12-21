/*
 * ndn-nr-cs-impl.cc
 *
 *  Created on: Dec 18, 2017
 *      Author: WSY
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


bool NrCsInterestImpl::AddInterest(uint32_t nonce,Ptr<const Interest> interest)
{
	Ptr<cs::EntryInterest> csEntryInterest = Find(nonce);
	if(csEntryInterest != 0)
	{
		std::cout<<"()"<<std::endl;
		PrintEntryInterest(nonce);
		return false;
	}
	uint32_t size = GetSize();
	std::cout<<"(NrCsInterestImpl.cc-AddInterest) 加入该兴趣包前的缓存大小为 "<<size<<std::endl;
    csEntryInterest = ns3::Create<cs::EntryInterest>(this,interest) ;
    m_csInterestContainer[nonce] = csEntryInterest;
	std::cout<<"(NrCsInterestImpl.cc-AddInterest) 兴趣包已经添加到了缓存中"<<std::endl;
	size = GetSize();
	std::cout<<"(NrCsInterestImpl.cc-AddInterest) 加入该兴趣包后的缓存大小为 "<<size<<std::endl;
	PrintEntryInterest(nonce);
	
	return true;
}

//abandon
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
NrCsInterestImpl::Find(const uint32_t nonce)
{
	std::map<uint32_t,Ptr<cs::EntryInterest> >::iterator it;
	//NS_ASSERT_MSG(m_csInterestContainer.size()!=0,"Empty cs container. No initialization?");
	for(it=m_csInterestContainer.begin();it!=m_csInterestContainer.end();++it)
	{
		if(it->first == nonce)
		{
			return it->second;
		}
	}
	return 0;
}
  
//abandon
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
	std::cout<<"(NrCsInterestImpl.cc-PrintCache.cc)"<<std::endl;
	std::map<uint32_t,Ptr<cs::EntryInterest> >::const_iterator it; 
	for(it=m_csInterestContainer.begin();it!=m_csInterestContainer.end();++it)
	{
		Ptr<const Interest> interest = it->second->GetInterest();
		Ptr<const Packet> nrPayload	= interest->GetPayload();
		ndn::nrndn::nrHeader nrheader;
		nrPayload->PeekHeader(nrheader);
		//获取发送兴趣包节点的ID
		uint32_t nodeId = nrheader.getSourceId();
		std::cout<<"兴趣包的nonce为 "<<interest->GetNonce()<<" 源节点为 "<<nodeId
		<<" 兴趣包的名字为 "<<it->second->GetName().toUri()<<std::endl;
	}
	std::cout<<std::endl;
}

void
NrCsInterestImpl::PrintEntryInterest(uint32_t nonce) 
{
	Ptr<cs::EntryInterest> csEntryInterest = Find(nonce);
	Ptr<const Interest> interest = csEntryInterest->GetInterest();
	Ptr<const Packet> nrPayload	= interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	//获取发送兴趣包节点的ID
	uint32_t nodeId = nrheader.getSourceId();
	std::cout<<"(cs-interest.cc-PrintEntryInterest) 兴趣包的nonce为 "<<interest->GetNonce()<<" 源节点为 "<<nodeId
	<<" 兴趣包的名字为 "<<csEntryInterest->GetName().toUri()<<std::endl;
}

uint32_t
NrCsInterestImpl::GetSize () const
{
	return m_csInterestContainer.size ();
}

//abandon
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
		return m_csInterestContainer.begin()->second;
}

//abandon
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
  
//abandon
Ptr<cs::Entry>
NrCsInterestImpl::Next (Ptr<cs::Entry>)
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::End () should not be invoked");
	return 0;
}


std::map<uint32_t,Ptr<Interest> >
NrCsInterestImpl::GetInterest(std::string lane)
{
	std::map<uint32_t,Ptr<Interest> > InterestCollection;
	std::map<uint32_t,Ptr<cs::EntryInterest> >::iterator it;
	for(it = m_csInterestContainer.begin();it != m_csInterestContainer.end();)
	{
		Ptr<Interest> interest = const_cast<Ptr<Interest>>it->second->GetInterest();
		std::vector<std::string> routes = interest->GetRoutes();
		if(routes.front() == lane)
		{
			PrintEntryInterest(interest->GetNonce());
			InterestCollection[interest->GetNonce()] = interest;
			m_csInterestContainer.erase(it++);
		}
		else
		{
			it++;
		}
			
	}
	uint32_t size = GetSize();
	std::cout<<"(NrCsInterestImpl.cc-GetInterest) 删除兴趣包后的缓存大小为 "<<size<<std::endl;
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


