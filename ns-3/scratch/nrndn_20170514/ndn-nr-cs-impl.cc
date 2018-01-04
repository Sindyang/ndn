/*
 * ndn-nr-cs-impl.cc
 *
 *  Created on: Jan 4, 2018
 *      Author: WSY
 */
#include "ndn-nr-cs-impl.h"
#include "nrHeader.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("ndn.cs.NrCsImpl");

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


NS_OBJECT_ENSURE_REGISTERED (NrCsImpl);

TypeId
NrCsImpl::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ndn::cs::nrndn::NrCsImpl")
    .SetGroupName ("Ndn")
    .SetParent<ContentStore> ()
    .AddConstructor< NrCsImpl > ()
    ;

  return tid;
}


NrCsImpl::NrCsImpl ()
{
}


NrCsImpl::~NrCsImpl ()
{
}


void
NrCsImpl::NotifyNewAggregate ()
{
	if (m_forwardingStrategy == 0)
	{
		m_forwardingStrategy = GetObject<ForwardingStrategy>();
	}
  ContentStore::NotifyNewAggregate ();
}


void
NrCsImpl::DoDispose ()
{
	m_forwardingStrategy = 0;
	ContentStore::DoDispose ();
}


//abandon
Ptr<Data>
NrCsImpl::Lookup (Ptr<const Interest> interest)
{
   return 0;
}


//abandon
void
NrCsImpl::MarkErased (Ptr<Entry> item)
{
	NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::MarkErased (Ptr<Entry> item) should not be invoked");
	return;
}
  
  
//abandon
void
NrCsImpl::Print (std::ostream& os) const
{
	os<<"CS Content Data Names:  ";
	return;
}


//abandon
Ptr<Entry>
NrCsImpl::Begin ()
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::Begin () should not be invoked");
	return 0;
}


//abandon
Ptr<Entry>
NrCsImpl::End ()
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::End () should not be invoked");
	return 0;
}


//abandon
Ptr<Entry>
NrCsImpl::Next (Ptr<Entry> from)
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::Next () should not be invoked");
	if (from == 0) 
		return 0;
	return 0;
}
 
 
//abandon
uint32_t
NrCsImpl::GetSize () const
{
	return 0;
}
	
	
//abandon
bool NrCsImpl::Add(Ptr<const Data> data)
{
	return false;
}


/*数据包部分*/
bool NrCsImpl::AddData(uint32_t signature,Ptr<const Data> data)
{
	std::cout<<"(cs-impl.cc-AddData) 添加数据包 "<<data->GetName().get(0).toUri()<<std::endl;
	Ptr<cs::Entry> csEntry = FindData(signature);
	if(csEntry != 0)
	{
		std::cout<<"(cs-impl.cc-AddData) 该数据包已经在缓存中"<<std::endl;
		return false;
	}
	uint32_t size = GetDataSize();
	std::cout<<"(cs-impl.cc-AddData) 加入该数据包前的缓存大小为 "<<size<<std::endl;
    csEntry = ns3::Create<cs::Entry>(this,data) ;
    m_data[signature] = csEntry;
	
	size = GetDataSize();
	std::cout<<"(cs-impl.cc-AddData) 加入该数据包后的缓存大小为 "<<size<<std::endl;
	return true;
}
    

std::map<uint32_t,Ptr<const Data> >
NrCsImpl::GetData(const Name &prefix)
{
	uint32_t size = GetSize();
	std::cout<<"(cs-impl.cc-GetData) 删除数据包前的缓存大小为 "<<size<<std::endl;
	std::map<uint32_t,Ptr<const Data> > DataCollection;
	std::map<uint32_t,Ptr<cs::Entry> >::iterator it;
	for(it = m_data.begin();it != m_data.end();)
	{
		
		if(it->second->GetName() == prefix)
		{
			Ptr<const Data> data = it->second->GetData();
			DataCollection[data->GetSignature()] = data;
			m_data.erase(it++);
	  	}
		else
		{
			++it;
		}
	}
	size = GetDataSize();
	std::cout<<"(cs-impl.cc-GetData) 删除数据包后的缓存大小为 "<<size<<std::endl;
	return DataCollection;
}


uint32_t
NrCsImpl::GetDataSize () const
{
	return m_data.size ();
}


/*
 * 2017.12.29 added by sy
 * 获得缓存中数据包的名字
 */
std::unordered_set<std::string> 
NrCsImpl::GetDataName() const
{
	std::unordered_set<std::string> collection;
	std::map<uint32_t,Ptr<cs::Entry> >::const_iterator it;
	for(it = m_data.begin();it != m_data.end();it++)
	{
		
		collection.insert(it->second->GetName().toUri());
	}
	return collection;
}


Ptr<Entry>
NrCsImpl::FindData (const uint32_t signature)
{
	std::map<uint32_t,Ptr<cs::Entry>>::iterator it = m_data.find(signature);
	if(it != m_data.end())
		return it->second;
	return 0;
}


void
NrCsImpl::PrintDataEntry(uint32_t signature) 
{
	Ptr<cs::Entry> csEntry = FindData(signature);
	Ptr<const Data> data = csEntry->GetData();
	Ptr<const Packet> nrPayload	= data->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	//获取发送数据包节点的ID
	uint32_t nodeId = nrheader.getSourceId();
	std::cout<<"(cs-impl.cc-PrintDataEntry) 数据包的signature为 "<<data->GetSignature()<<" 源节点为 "<<nodeId
	<<" 数据包的名字为 "<<csEntry->GetName().toUri()<<std::endl;
}

void
NrCsImpl::PrintDataCache () const
{
	std::cout<<"(cs-impl.cc-PrintDataCache)"<<std::endl;
	std::map<uint32_t,Ptr<cs::Entry> >::const_iterator it; 
	for(it=m_data.begin();it!=m_data.end();++it)
	{
		Ptr<const Data> data = it->second->GetData();
		Ptr<const Packet> nrPayload	= data->GetPayload();
		ndn::nrndn::nrHeader nrheader;
		nrPayload->PeekHeader(nrheader);
		//获取发送兴趣包节点的ID
		uint32_t nodeId = nrheader.getSourceId();
		std::cout<<"数据包的signature为 "<<data->GetSignature()<<" 源节点为 "<<nodeId
		<<" 数据包的名字为 "<<it->second->GetName().toUri()<<std::endl;
	}
	std::cout<<std::endl;
}
/*数据包部分*/

/*兴趣包部分*/
bool NrCsImpl::AddInterest(uint32_t nonce,Ptr<const Interest> interest)
{
	Ptr<cs::EntryInterest> csEntryInterest = FindInterest(nonce);
	if(csEntryInterest != 0)
	{
		std::cout<<"(cs-impl.cc-AddInterest) 该兴趣包已经被加入到缓存中"<<std::endl;
		PrintInterestEntry(nonce);
		return false;
	}
	//uint32_t size = GetInterestSize();
	//std::cout<<"(cs-impl.cc-AddInterest) 加入该兴趣包前的缓存大小为 "<<size<<std::endl;
    csEntryInterest = ns3::Create<cs::EntryInterest>(this,interest);
	
	std::string routes = interest->GetRoutes();
	std::cout<<"(cs-interest.cc-AddInterest) routes "<<routes<<std::endl;
	//getchar();
	
    m_interest[nonce] = csEntryInterest;
	//size = GetInterestSize();
	//std::cout<<"(NrCsImpl.cc-AddInterest) 加入该兴趣包后的缓存大小为 "<<size<<std::endl;
	return true;
}

std::map<uint32_t,Ptr<const Interest> >
NrCsImpl::GetInterest(std::string lane)
{
	//uint32_t size = GetInterestSize();
	//std::cout<<"(cs-impl.cc-GetInterest) 该路段有车辆 "<<lane<<std::endl;
	//std::cout<<"(cs-impl.cc-GetInterest) 删除兴趣包前的缓存大小为 "<<size<<std::endl;
	//PrintInterestCache();
	std::map<uint32_t,Ptr<const Interest> > InterestCollection;
	std::map<uint32_t,Ptr<cs::EntryInterest> >::iterator it;
	for(it = m_interest.begin();it != m_interest.end();)
	{
		Ptr<const Interest> interest = it->second->GetInterest();
		std::string routes = interest->GetRoutes(); 
		//std::cout<<"(cs-interest.cc-GetInterest) 兴趣包实际导航路线 "<<routes<<std::endl;
		std::string currentroute = routes.substr(0,lane.length());
		//std::cout<<"(cs-impl.cc-GetInterest) 兴趣包下一行驶路段 "<<currentroute<<std::endl;
		//getchar();
		if(currentroute == lane)
		{
			//PrintEntryInterest(interest->GetNonce());
			InterestCollection[interest->GetNonce()] = interest;
			m_interest.erase(it++);
	  	}
		else
		{
			++it;
		}
	}
	//size = GetInterestSize();
	//std::cout<<"(cs-impl.cc-GetInterest) 删除兴趣包后的缓存大小为 "<<size<<std::endl;
	//getchar();
	return InterestCollection;
}

uint32_t
NrCsImpl::GetInterestSize () const
{
	return m_interest.size ();
}

Ptr<cs::EntryInterest>
NrCsImpl::FindInterest(const uint32_t nonce)
{
	std::map<uint32_t,Ptr<cs::EntryInterest> >::iterator it = m_interest.find(nonce);
	//NS_ASSERT_MSG(m_csInterestContainer.size()!=0,"Empty cs container. No initialization?");
	if(it != m_interest.end())
	{
		return it->second;
	}
	return 0;
}

void
NrCsImpl::PrintInterestCache () const
{
	std::cout<<"(cs-impl.cc-PrintInterestCache)"<<std::endl;
	std::map<uint32_t,Ptr<cs::EntryInterest> >::const_iterator it; 
	for(it=m_interest.begin();it!=m_interest.end();++it)
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
NrCsImpl::PrintInterestEntry(uint32_t nonce) 
{
	Ptr<cs::EntryInterest> csEntryInterest = FindInterest(nonce);
	Ptr<const Interest> interest = csEntryInterest->GetInterest();
	Ptr<const Packet> nrPayload	= interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	//获取发送兴趣包节点的ID
	uint32_t nodeId = nrheader.getSourceId();
	std::cout<<"(cs-impl.cc-PrintInterestEntry) 兴趣包的nonce为 "<<interest->GetNonce()<<" 源节点为 "<<nodeId
	<<" 兴趣包的名字为 "<<csEntryInterest->GetName().toUri()<<std::endl;
}

/*兴趣包部分*/

void NrCsImpl::DoInitialize(void)
{
	ContentStore::DoInitialize();
}

} /* namespace nrndn */
} /* namespace cs */
} /* namespace ndn */
} /* namespace ns3 */


