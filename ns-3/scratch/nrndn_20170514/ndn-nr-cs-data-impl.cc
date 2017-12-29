/*
 * ndn-nr-cs-impl.cc
 *
 *  Created on: Dec 29, 2017
 *      Author: WSY
 */

#include "ndn-nr-cs-data-impl.h"
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


bool NrCsImpl::Add (uint32_t,Ptr<const Data> data)
{
	std::cout<<"(cs-data.cc-Add) 添加数据包 "<<data->GetName().toUri()<<std::endl;
	Ptr<cs::Entry> csEntry = Find(nonce);
	if(csEntry != 0)
	{
		std::cout<<"(cs-data.cc-Add) 该数据包已经被加入到缓存中"<<std::endl;
		return false;
	}
	uint32_t size = GetSize();
	std::cout<<"(NrCsImpl.cc-Add) 加入该数据包前的缓存大小为 "<<size<<std::endl;
    csEntry = ns3::Create<cs::Entry>(this,data) ;
    m_csContainer[nonce] = csEntry;
	
	size = GetSize();
	std::cout<<"(NrCsImpl.cc-Add) 加入该数据包后的缓存大小为 "<<size<<std::endl;
	return true;
}

void
NrCsImpl::DoDispose ()
{
	m_forwardingStrategy = 0;
	ContentStore::DoDispose ();
}
  
    
Ptr<Entry>
NrCsImpl::Find (const uint32_t nonce)
{
	std map<uint32_t,Ptr<cs::Entry>>::iterator it = m_csContainer.find(nonce);
	if(it != m_csContainer.end())
		return it->second;
	return 0;
}
  
Ptr<Data>
NrCsImpl::Lookup (Ptr<const Interest> interest)
{
   return 0;
}

void
NrCsImpl::MarkErased (Ptr<Entry> item)
{
	NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::MarkErased (Ptr<Entry> item) should not be invoked");
	return;
}
  
  
void
NrCsImpl::Print (std::ostream& os) const
{
	os<<"CS Content Data Names:  ";
	std::vector<Ptr<Entry> >::const_iterator it = m_csContainer.begin();
	for( ; it!=m_csContainer.end(); ++it)
	{
		os<<(*it)->GetName().toUri()<<" ";
	}
	os<<std::endl;
	return;
}

void
NrCsImpl::PrintEntry(uint32_t nonce) 
{
	Ptr<cs::Entry> csEntry = Find(nonce);
	Ptr<const Data> data = csEntry->GetData();
	Ptr<const Packet> nrPayload	= data->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	//获取发送数据包节点的ID
	uint32_t nodeId = nrheader.getSourceId();
	std::cout<<"(cs-data.cc-PrintEntry) 数据包的nonce为 "<<data->GetNonce()<<" 源节点为 "<<nodeId
	<<" 数据包的名字为 "<<csEntry->GetName().toUri()<<std::endl;
}

std::map<uint32_t,Ptr<const Data> >
NrCsImpl::GetData(const Name &prefix)
{
	uint32_t size = GetSize();
	std::cout<<"(NrCsImpl.cc-GetData) 删除数据包前的缓存大小为 "<<size<<std::endl;
	std::map<uint32_t,Ptr<const Data> > DataCollection;
	std::map<uint32_t,Ptr<cs::Entry> >::iterator it;
	for(it = m_csContainer.begin();it != m_csContainer.end();)
	{
		
		if(it->second->GetName() == prefix)
		{
			Ptr<const Data> data = it->second->GetData();
			DataCollection[data->GetNonce()] = data;
			m_csInterestContainer.erase(it++);
	  	}
		else
		{
			++it;
		}
			
	}
	size = GetSize();
	std::cout<<"(NrCsImpl.cc-GetData) 删除数据包后的缓存大小为 "<<size<<std::endl;
	return DataCollection;
}


uint32_t
NrCsImpl::GetSize () const
{
	return m_csContainer.size ();
}

  
void
NrCsImpl::PrintCache () const
{
	std::cout<<"(NrCsImpl.cc-PrintCache.cc)"<<std::endl;
	std::map<uint32_t,Ptr<cs::Entry> >::const_iterator it; 
	for(it=m_csContainer.begin();it!=m_csContainer.end();++it)
	{
		Ptr<const Data> data = it->second->GetData();
		Ptr<const Packet> nrPayload	= data->GetPayload();
		ndn::nrndn::nrHeader nrheader;
		nrPayload->PeekHeader(nrheader);
		//获取发送兴趣包节点的ID
		uint32_t nodeId = nrheader.getSourceId();
		std::cout<<"数据包的nonce为 "<<data->GetNonce()<<" 源节点为 "<<nodeId
		<<" 数据包的名字为 "<<it->second->GetName().toUri()<<std::endl;
	}
	std::cout<<std::endl;
}

Ptr<Entry>
NrCsImpl::Begin ()
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::Begin () should not be invoked");

	if(m_csContainer.begin() == m_csContainer.end())
		return End();
	else
		return *(m_csContainer.begin());
}


Ptr<Entry>
NrCsImpl::End ()
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::End () should not be invoked");
	return 0;
}

  
Ptr<Entry>
NrCsImpl::Next (Ptr<Entry> from)
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::Next () should not be invoked");
	if (from == 0) return 0;

	std::vector<Ptr<Entry> >::iterator it;
	it = find(m_csContainer.begin(),m_csContainer.end(),from);
	if(it==m_csContainer.end())
		return End();
	else
	{
		++it;
		if(it==m_csContainer.end())
			return End();
		else
			return *it;
	}
}

/*
 * 2017.12.29 added by sy
 * 获得缓存中数据包的名字
 */
std::unordered_set<std::string> 
NrCsImpl::GetDataName() const
{
	std::unordered_set<std::string> collection;
	std::vector<Ptr<Entry>>::iteratot it;
	for(it = m_csContainer.begin();it != m_csContainer.end();it++)
	{
		collection.insert((*it)->GetName());
	}
	return collection;
}

std

void NrCsImpl::DoInitialize(void)
{
	ContentStore::DoInitialize();
}

} /* namespace nrndn */
} /* namespace cs */
} /* namespace ndn */
} /* namespace ns3 */


