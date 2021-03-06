/*
 * ndn-nr-cs-impl.cc
 *
 *  Created on: Jan 4, 2018
 *      Author: WSY
 */
#include "ndn-nr-cs-impl.h"
#include "nrHeader.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE("ndn.cs.NrCsImpl");

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

NS_OBJECT_ENSURE_REGISTERED(NrCsImpl);

TypeId
NrCsImpl::GetTypeId()
{
	static TypeId tid = TypeId("ns3::ndn::cs::nrndn::NrCsImpl")
							.SetGroupName("Ndn")
							.SetParent<ContentStore>()
							.AddConstructor<NrCsImpl>();

	return tid;
}

NrCsImpl::NrCsImpl()
{
}

NrCsImpl::~NrCsImpl()
{
}

void NrCsImpl::NotifyNewAggregate()
{
	if (m_forwardingStrategy == 0)
	{
		m_forwardingStrategy = GetObject<ForwardingStrategy>();
	}
	ContentStore::NotifyNewAggregate();
}

void NrCsImpl::DoDispose()
{
	m_forwardingStrategy = 0;
	ContentStore::DoDispose();
}

//abandon
Ptr<Data>
NrCsImpl::Lookup(Ptr<const Interest> interest)
{
	return 0;
}

//abandon
void NrCsImpl::MarkErased(Ptr<Entry> item)
{
	NS_ASSERT_MSG(false, "In NrCsImpl,NrCsImpl::MarkErased (Ptr<Entry> item) should not be invoked");
	return;
}

//abandon
void NrCsImpl::Print(std::ostream &os) const
{
	os << "CS Content Data Names:  ";
	return;
}

//abandon
Ptr<Entry>
NrCsImpl::Begin()
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::Begin () should not be invoked");
	return 0;
}

//abandon
Ptr<Entry>
NrCsImpl::End()
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::End () should not be invoked");
	return 0;
}

//abandon
Ptr<Entry>
NrCsImpl::Next(Ptr<Entry> from)
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::Next () should not be invoked");
	if (from == 0)
		return 0;
	return 0;
}

//abandon
uint32_t
NrCsImpl::GetSize() const
{
	return 0;
}

//abandon
bool NrCsImpl::Add(Ptr<const Data> data)
{
	return false;
}

bool NrCsImpl::AddData(uint32_t signature, Ptr<const Data> data)
{
	//std::cout<<"(cs-impl.cc-AddData) 添加数据包 "<<data->GetName().get(0).toUri()<<std::endl;
	Ptr<cs::Entry> csEntry = FindData(signature);
	if (csEntry != 0)
	{
		return false;
	}
	//数据包保留的时间 = 产生时间+有效时间-当前时间
	double interval = data->GetTimestamp().GetSeconds() + data->GetFreshness().GetSeconds() - Simulator::Now().GetSeconds();
	if (interval < 0)
	{
		return false;
	}

	csEntry = ns3::Create<cs::Entry>(this, data);
	m_data[signature] = csEntry;

	Simulator::Schedule(Seconds(interval), &NrCsImpl::CleanExpiredTimedoutData, this, signature);
	return true;
}

bool NrCsImpl::AddDataSource(uint32_t signature, Ptr<const Data> data)
{
	//std::cout<<"(cs-impl.cc-AddDataSource) 添加数据包 "<<data->GetName().get(0).toUri()<<std::endl;
	Ptr<cs::Entry> csEntry = FindDataSource(signature);
	if (csEntry != 0)
	{
		return false;
	}
	//数据包保留的时间 = 产生时间+有效时间-当前时间
	double interval = data->GetTimestamp().GetSeconds() + data->GetFreshness().GetSeconds() - Simulator::Now().GetSeconds();
	if (interval < 0)
	{
		return false;
	}

	csEntry = ns3::Create<cs::Entry>(this, data);
	m_datasource[signature] = csEntry;

	Simulator::Schedule(Seconds(interval), &NrCsImpl::CleanExpiredTimedoutDataSource, this, signature);
	return true;
}

void NrCsImpl::CleanExpiredTimedoutData(uint32_t signature)
{
	std::map<uint32_t, Ptr<cs::Entry>>::iterator it = m_data.find(signature);
	if (it != m_data.end())
	{
		m_data.erase(it);
	}
}

void NrCsImpl::CleanExpiredTimedoutDataSource(uint32_t signature)
{
	std::map<uint32_t, Ptr<cs::Entry>>::iterator it = m_datasource.find(signature);
	if (it != m_datasource.end())
	{
		m_datasource.erase(it);
	}
}

void NrCsImpl::DeleteData(const uint32_t signature)
{
	uint32_t size = GetDataSize();
	//std::cout<<"(cs-impl.cc-DeleteData) 删除数据包前的缓存大小为 "<<size<<std::endl;
	std::map<uint32_t, Ptr<cs::Entry>>::iterator it = m_data.find(signature);
	if (it != m_data.end())
	{
		m_data.erase(it);
		size = GetDataSize();
		std::cout << "(cs-impl.cc-DeleteData) 删除数据包后的缓存大小为 " << size << " 数据包序列号为 " << signature << std::endl;
	}
}

std::map<uint32_t, Ptr<const Data>>
NrCsImpl::GetData(std::unordered_set<std::string> interestDataName)
{
	std::map<uint32_t, Ptr<const Data>> DataCollection;
	std::map<uint32_t, Ptr<cs::Entry>>::iterator it;

	std::unordered_set<std::string>::iterator itdataroute = interestDataName.begin();
	for (; itdataroute != interestDataName.end(); itdataroute++)
	{
		for (it = m_data.begin(); it != m_data.end(); it++)
		{
			std::string dataname = it->second->GetName().toUri();
			//std::cout << "(cs-impl.cc-GetData) 缓存中的数据包名称为 " << dataname << std::endl;
			if (*itdataroute == dataname)
			{
				Ptr<const Data> src = it->second->GetData();
				Ptr<Data> data = Create<Data>(*src);
				//data->SetTimestamp(Simulator::Now());
				DataCollection[data->GetSignature()] = data;
				std::cout << "(cs-impl.cc-GetData) 从缓存中得到的数据包为 " << data->GetSignature() << std::endl;
			}
		}
	}
	return DataCollection;
}

std::map<uint32_t, Ptr<const Data>>
NrCsImpl::GetDataSource(std::vector<std::string> interest)
{
	std::map<uint32_t, Ptr<const Data>> DataCollection;
	std::vector<std::string>::iterator itinterest;
	std::map<uint32_t, Ptr<cs::Entry>>::iterator it;
	for (itinterest = interest.begin(); itinterest != interest.end(); itinterest++)
	{
		for (it = m_datasource.begin(); it != m_datasource.end(); it++)
		{
			std::string dataname = it->second->GetName().get(0).toUri();
			if (*itinterest == dataname)
			{
				Ptr<const Data> src = it->second->GetData();
				Ptr<Data> data = Create<Data>(*src);
				//data->SetTimestamp(Simulator::Now());
				DataCollection[data->GetSignature()] = data;
				std::cout << "(cs-impl.cc-GetDataSource) 从缓存中得到的数据包为 " << data->GetSignature() << std::endl;
			}
		}
	}
	return DataCollection;
}

std::map<uint32_t, Ptr<const Data>>
NrCsImpl::GetData()
{
	std::map<uint32_t, Ptr<const Data>> DataCollection;
	std::map<uint32_t, Ptr<cs::Entry>>::iterator it;

	for (it = m_data.begin(); it != m_data.end(); it++)
	{
		Ptr<const Data> src = it->second->GetData();
		//复制数据包
		Ptr<Data> data = Create<Data>(*src);
		DataCollection[data->GetSignature()] = data;
	}
	return DataCollection;
}

uint32_t
NrCsImpl::GetDataSize() const
{
	return m_data.size();
}

uint32_t
NrCsImpl::GetDataSourceSize() const
{
	return m_datasource.size();
}

Ptr<Entry>
NrCsImpl::FindData(const uint32_t signature)
{
	std::map<uint32_t, Ptr<cs::Entry>>::iterator it = m_data.find(signature);
	if (it != m_data.end())
		return it->second;
	return 0;
}

Ptr<Entry>
NrCsImpl::FindDataSource(const uint32_t signature)
{
	std::map<uint32_t, Ptr<cs::Entry>>::iterator it = m_datasource.find(signature);
	if (it != m_datasource.end())
		return it->second;
	return 0;
}

void NrCsImpl::PrintDataEntry(uint32_t signature)
{
	Ptr<cs::Entry> csEntry = FindData(signature);
	Ptr<const Data> data = csEntry->GetData();
	Ptr<const Packet> nrPayload = data->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	//获取发送数据包节点的ID
	uint32_t nodeId = nrheader.getSourceId();
	std::cout << "(cs-impl.cc-PrintDataEntry) 数据包的signature为 " << data->GetSignature() << " 源节点为 " << nodeId
			  << " 数据包的名字为 " << csEntry->GetName().toUri() << std::endl;
}

void NrCsImpl::PrintDataCache() const
{
	std::cout << "(cs-impl.cc-PrintDataCache)" << std::endl;
	std::map<uint32_t, Ptr<cs::Entry>>::const_iterator it;
	for (it = m_data.begin(); it != m_data.end(); ++it)
	{
		Ptr<const Data> data = it->second->GetData();
		Ptr<const Packet> nrPayload = data->GetPayload();
		ndn::nrndn::nrHeader nrheader;
		nrPayload->PeekHeader(nrheader);
		//获取发送兴趣包节点的ID
		uint32_t nodeId = nrheader.getSourceId();
		std::cout << "数据包的signature为 " << data->GetSignature() << " 源节点为 " << nodeId
				  << " 数据包的名字为 " << it->second->GetName().toUri() << std::endl;
	}
	std::cout << std::endl;
}
/*数据包部分*/

/*兴趣包部分*/
bool NrCsImpl::AddInterest(uint32_t nonce, Ptr<const Interest> interest)
{
	Ptr<cs::EntryInterest> csEntryInterest = FindInterest(nonce);
	if (csEntryInterest != 0)
	{
		return false;
	}
	csEntryInterest = ns3::Create<cs::EntryInterest>(this, interest);
	m_interest[nonce] = csEntryInterest;
	return true;
}

std::map<uint32_t, Ptr<const Interest>>
NrCsImpl::GetInterest(std::string lane)
{
	std::map<uint32_t, Ptr<const Interest>> InterestCollection;
	std::map<uint32_t, Ptr<cs::EntryInterest>>::iterator it;
	for (it = m_interest.begin(); it != m_interest.end(); it++)
	{
		Ptr<const Interest> interest = it->second->GetInterest();
		std::string routes = interest->GetRoutes();
		std::string currentroute = routes.substr(0, lane.length());
		if (currentroute == lane)
		{
			InterestCollection[interest->GetNonce()] = interest;
		}
	}
	return InterestCollection;
}

uint32_t
NrCsImpl::GetInterestSize() const
{
	return m_interest.size();
}

Ptr<cs::EntryInterest>
NrCsImpl::FindInterest(const uint32_t nonce)
{
	std::map<uint32_t, Ptr<cs::EntryInterest>>::iterator it = m_interest.find(nonce);
	if (it != m_interest.end())
	{
		return it->second;
	}
	return 0;
}

void NrCsImpl::DeleteInterest(const uint32_t nonce)
{
	uint32_t size = GetInterestSize();
	std::map<uint32_t, Ptr<cs::EntryInterest>>::iterator it = m_interest.find(nonce);
	if (it != m_interest.end())
	{
		m_interest.erase(it);
		size = GetInterestSize();
	}
}

void NrCsImpl::PrintInterestCache() const
{
	std::cout << "(cs-impl.cc-PrintInterestCache)" << std::endl;
	std::map<uint32_t, Ptr<cs::EntryInterest>>::const_iterator it;
	for (it = m_interest.begin(); it != m_interest.end(); ++it)
	{
		Ptr<const Interest> interest = it->second->GetInterest();
		Ptr<const Packet> nrPayload = interest->GetPayload();
		ndn::nrndn::nrHeader nrheader;
		nrPayload->PeekHeader(nrheader);
		//获取发送兴趣包节点的ID
		uint32_t nodeId = nrheader.getSourceId();
		std::cout << "兴趣包的nonce为 " << interest->GetNonce() << " 源节点为 " << nodeId
				  << " 兴趣包的转发路线为 " << it->second->GetInterest()->GetRoutes() << std::endl;
	}
	std::cout << std::endl;
}

void NrCsImpl::PrintInterestEntry(uint32_t nonce)
{
	Ptr<cs::EntryInterest> csEntryInterest = FindInterest(nonce);
	Ptr<const Interest> interest = csEntryInterest->GetInterest();
	Ptr<const Packet> nrPayload = interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	//获取发送兴趣包节点的ID
	uint32_t nodeId = nrheader.getSourceId();
	std::cout << "(cs-impl.cc-PrintInterestEntry) 兴趣包的nonce为 " << interest->GetNonce() << " 源节点为 " << nodeId
			  << " 兴趣包的名字为 " << csEntryInterest->GetName().toUri() << std::endl;
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
