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
bool NrCsImpl::AddData1(uint32_t signature,Ptr<const Data> data,std::unordered_set<std::string> lastroutes)
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
	
	//RSU需要加入未被满足的上一跳路段
	if(lastroutes.size() > 0)
	{
		m_lastroutes[signature] = lastroutes;
		std::cout<<"(cs-impl.cc-AddData) 对该数据包感兴趣的路段大小为 "<<lastroutes.size()<<std::endl;
	} 
	
	size = GetDataSize();
	std::cout<<"(cs-impl.cc-AddData) 加入该数据包后的缓存大小为 "<<size<<std::endl;
	return true;
}

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
NrCsImpl::GetData1(std::unordered_map<std::string,std::unordered_set<std::string> > dataname_route)
{
	uint32_t size = GetDataSize();
	std::cout<<"(cs-impl.cc-GetData) 删除数据包前的缓存大小为 "<<size<<std::endl;
	
	std::map<uint32_t,Ptr<const Data> > DataCollection;
	std::map<uint32_t,Ptr<cs::Entry> >::iterator it;
		
	if(dataname_route.empty())
	{
		std::cout<<"(cs-impl.cc-GetData) 普通车辆获取缓存中的数据包"<<std::endl;
		for(it = m_data.begin();it != m_data.end();it++)
		{
			Ptr<const Data> data = it->second->GetData();
			DataCollection[data->GetSignature()] = data;
		}
		m_data.clear();
	}
	else
	{
		std::cout<<"(cs-impl.cc-GetData) RSU获取缓存中的数据包"<<std::endl;
		std::unordered_map<std::string,std::unordered_set<std::string> >::iterator itdataroute = dataname_route.begin();
		//从缓存中取出对应的数据包
		for(;itdataroute != dataname_route.end();itdataroute++)
		{
			for(it = m_data.begin();it != m_data.end();it++)
			{
				std::string dataname = it->second->GetName().toUri();
				if(itdataroute->first == dataname)
				{
					std::cout<<"(cs-impl.cc-GetData) 从缓存中得到的数据包为 "<<dataname<<std::endl;
					Ptr<const Data> data = it->second->GetData();
					DataCollection[data->GetSignature()] = data;
				}
			}
		}
		
		//从缓存中删除已经满足的数据包
		for(it = m_data.begin();it != m_data.end();)
		{
			std::string dataname = it->second->GetName().toUri();
			uint32_t signature = it->second->GetData()->GetSignature();
			itdataroute = dataname_route.find(dataname);
			if(itdataroute != dataname_route.end())
			{
				std::cout<<"(cs-impl.cc-GetData) dataname_route中有数据包 "<<dataname<<std::endl;
				if(IsLastRoutesLeft(signature,itdataroute->second))
				{
					m_data.erase(it++);
					std::cout<<"(cs-impl.cc-GetData) 可以从缓存中删除该数据包"<<std::endl;
				}
			}
			else
			{
				++it;
			}
		}
	}
	size = GetDataSize();
	std::cout<<"(cs-impl.cc-GetData) 删除数据包后的缓存大小为 "<<size<<std::endl;
	
	return DataCollection;
}


std::map<uint32_t,Ptr<const Data> >
NrCsImpl::GetData(std::vector<std::string> interest)
{
	std::cout<<"(cs-impl.cc-GetData) RSU获取缓存中的数据包"<<std::endl;
	
	std::map<uint32_t,Ptr<const Data> > DataCollection;
	std::vector<std::string>::iterator itinterest;
	std::map<uint32_t,Ptr<cs::Entry> >::iterator it;
	for(itinterest = interest.begin();itinterest != interest.end();itinterest++)
	{
		std::cout<<"(cs-impl.cc-GetData) 想要得到的数据包为 "<<*itinterest<<std::endl;
		for(it = m_data.begin();it != m_data.end();it++)
		{
			std::string dataname = it->second->GetName().get(0).toUri();
			if(*itinterest == dataname)
			{
				std::cout<<"(cs-impl.cc-GetData) 缓存中有对应的数据包"<<std::endl;
				Ptr<const Data> data = it->second->GetData();
				DataCollection[data->GetSignature()] = data;
			}
		}
	}
	return DataCollection;
}

std::map<uint32_t,Ptr<const Data> >
NrCsImpl::GetData()
{
	std::cout<<"(cs-impl.cc-GetData) 普通车辆获取缓存中的数据包"<<std::endl;
	
	uint32_t size = GetDataSize();
	std::cout<<"(cs-impl.cc-GetData) 删除数据包前的缓存大小为 "<<size<<std::endl;
	
	std::map<uint32_t,Ptr<const Data> > DataCollection;
	std::map<uint32_t,Ptr<cs::Entry> >::iterator it;
	
	for(it = m_data.begin();it != m_data.end();it++)
	{
		Ptr<const Data> data = it->second->GetData();
		DataCollection[data->GetSignature()] = data;
	}
	m_data.clear();
	
	size = GetDataSize();
	std::cout<<"(cs-impl.cc-GetData) 删除数据包后的缓存大小为 "<<size<<std::endl;
	
	return DataCollection;
}

// routes为有车辆的上一跳路段
bool
NrCsImpl::IsLastRoutesLeft(uint32_t signature,std::unordered_set<std::string> routes)
{
	// lastroutes为未被满足的上一跳路段
	std::unordered_set<std::string> lastroutes = m_lastroutes[signature];
	std::unordered_set<std::string>::iterator itlastroutes;
	for(std::unordered_set<std::string>::iterator itroutes = routes.begin();itroutes != routes.end();itroutes++)
	{
		//从未满足的路段中删除存在车辆的路段
		itlastroutes = lastroutes.find(*itroutes);
		if(itlastroutes != lastroutes.end())
		{
			lastroutes.erase(itlastroutes);
			std::cout<<"(cs-impl.cc-IsLastRoutesLeft) 路段 "<<*itlastroutes<<" 有车辆"<<std::endl;
		}
			
	}
	if(lastroutes.empty())
	{
		std::map<uint32_t,std::unordered_set<std::string> >::iterator it = m_lastroutes.find(signature);
		m_lastroutes.erase(it);
		std::cout<<"(cs-impl.cc-IsLastRoutesLeft) 数据包 "<<signature<<" 的上一跳路段全部有车辆"<<std::endl;
		return true;
	}
	return false;
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
		//PrintInterestEntry(nonce);
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
	uint32_t size = GetInterestSize();
	//std::cout<<"(cs-impl.cc-GetInterest) 该路段有车辆 "<<lane<<std::endl;
	std::cout<<"(cs-impl.cc-GetInterest) 删除兴趣包前的缓存大小为 "<<size<<std::endl;
	//PrintInterestCache();
	std::map<uint32_t,Ptr<const Interest> > InterestCollection;
	std::map<uint32_t,Ptr<cs::EntryInterest> >::iterator it;
	for(it = m_interest.begin();it != m_interest.end();/*it++*/)
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
	size = GetInterestSize();
	std::cout<<"(cs-impl.cc-GetInterest) 删除兴趣包后的缓存大小为 "<<size<<std::endl;
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
NrCsImpl::DeleteInterest(const uint32_t nonce)
{
	uint32_t size = GetInterestSize();
	std::cout<<"(cs-impl.cc-DeleteInterest) 删除兴趣包前的缓存大小为 "<<size<<std::endl;
	std::map<uint32_t,Ptr<cs::EntryInterest> >::iterator it = m_interest.find(nonce);
	if(it != m_interest.end())
	{
		m_interest.erase(it);
		size = GetInterestSize();
		std::cout<<"(cs-impl.cc-DeleteInterest) 删除兴趣包后的缓存大小为 "<<size<<" 兴趣包序列号为 "<<nonce<<std::endl;
	}
	else
		std::cout<<"(cs-impl.cc-DeleteInterest) 该兴趣包不在缓存中"<<std::endl;
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

/*准备转发的兴趣包*/
bool NrCsImpl::AddForwardInterest(uint32_t nonce,Ptr<const Interest> interest)
{
	Ptr<cs::EntryInterest> csEntryInterest = FindForwardInterest(nonce);
	if(csEntryInterest != 0)
	{
		std::cout<<"(cs-impl.cc-AddForwardInterest) 该兴趣包已经被加入到缓存中"<<std::endl;
		//PrintInterestEntry(nonce);
		return false;
	}
	uint32_t size = GetForwardInterestSize();
	std::cout<<"(cs-impl.cc-AddForwardInterest) 加入该兴趣包前的缓存大小为 "<<size<<std::endl;
    csEntryInterest = ns3::Create<cs::EntryInterest>(this,interest);
	
    m_forwardinterest[nonce] = csEntryInterest;
	size = GetForwardInterestSize();
	std::cout<<"(NrCsImpl.cc-AddForwardInterest) 加入该兴趣包后的缓存大小为 "<<size<<std::endl;
	return true;
}

std::map<uint32_t,Ptr<const Interest> >
NrCsImpl::GetForwardInterest(std::string lane)
{ 
	//uint32_t size = GetForwardInterestSize();
	//std::cout<<"(cs-impl.cc-GetForwardInterest) 该路段有车辆 "<<lane<<std::endl;
	//std::cout<<"(cs-impl.cc-GetForwardInterest) 删除兴趣包前的缓存大小为 "<<size<<std::endl;
	//PrintInterestCache();
	std::map<uint32_t,Ptr<const Interest> > InterestCollection;
	std::map<uint32_t,Ptr<cs::EntryInterest> >::iterator it;
	for(it = m_forwardinterest.begin();it != m_forwardinterest.end();it++)
	{
		Ptr<const Interest> interest = it->second->GetInterest();
		std::string routes = interest->GetRoutes(); 
		std::cout<<"(cs-interest.cc-GetInterest) 兴趣包实际导航路线 "<<routes<<std::endl;
		std::string currentroute = routes.substr(0,lane.length());
		std::cout<<"(cs-impl.cc-GetInterest) 兴趣包下一行驶路段 "<<currentroute<<std::endl;
		//getchar();
		if(currentroute == lane)
		{
			//PrintEntryInterest(interest->GetNonce());
			InterestCollection[interest->GetNonce()] = interest;
			//m_forwardinterest.erase(it++);
	  	}
		//else
		//{
		//	++it;
		//}
	}
	//size = GetForwardInterestSize();
	//std::cout<<"(cs-impl.cc-GetForwardInterest) 删除兴趣包后的缓存大小为 "<<size<<std::endl;
	getchar();
	return InterestCollection;
}

uint32_t
NrCsImpl::GetForwardInterestSize () const
{
	return m_forwardinterest.size ();
}

Ptr<cs::EntryInterest>
NrCsImpl::FindForwardInterest(const uint32_t nonce)
{
	std::map<uint32_t,Ptr<cs::EntryInterest> >::iterator it = m_forwardinterest.find(nonce);
	//NS_ASSERT_MSG(m_csInterestContainer.size()!=0,"Empty cs container. No initialization?");
	if(it != m_forwardinterest.end())
	{
		return it->second;
	}
	return 0;
}

void
NrCsImpl::DeleteForwardInterest(const uint32_t nonce)
{
	uint32_t size = GetForwardInterestSize();
	std::cout<<"(cs-impl.cc-DeleteForwardInterest) 删除兴趣包前的缓存大小为 "<<size<<std::endl;
	std::map<uint32_t,Ptr<cs::EntryInterest> >::iterator it = m_forwardinterest.find(nonce);
	if(it != m_forwardinterest.end())
	{
		m_forwardinterest.erase(it);
		size = GetForwardInterestSize();
		std::cout<<"(cs-impl.cc-DeleteForwardInterest) 删除兴趣包后的缓存大小为 "<<size<<" 兴趣包序列号为 "<<nonce<<std::endl;
	}
	else
		std::cout<<"(cs-impl.cc-DeleteForwardInterest) 该兴趣包不在缓存中"<<std::endl;
}

void
NrCsImpl::PrintForwardInterestCache () const
{
	std::cout<<"(cs-impl.cc-PrintForwardInterestCache)"<<std::endl;
	std::map<uint32_t,Ptr<cs::EntryInterest> >::const_iterator it; 
	for(it=m_forwardinterest.begin();it!=m_forwardinterest.end();++it)
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
NrCsImpl::PrintForwardInterestEntry(uint32_t nonce) 
{
	Ptr<cs::EntryInterest> csEntryInterest = FindForwardInterest(nonce);
	Ptr<const Interest> interest = csEntryInterest->GetInterest();
	Ptr<const Packet> nrPayload	= interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	//获取发送兴趣包节点的ID
	uint32_t nodeId = nrheader.getSourceId();
	std::cout<<"(cs-impl.cc-PrintForwardInterestEntry) 兴趣包的nonce为 "<<interest->GetNonce()<<" 源节点为 "<<nodeId
	<<" 兴趣包的名字为 "<<csEntryInterest->GetName().toUri()<<std::endl;
}

/*准备转发的兴趣包*/

void NrCsImpl::DoInitialize(void)
{
	ContentStore::DoInitialize();
}

} /* namespace nrndn */
} /* namespace cs */
} /* namespace ndn */
} /* namespace ns3 */


