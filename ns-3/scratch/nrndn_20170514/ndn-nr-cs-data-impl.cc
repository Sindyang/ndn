/*
 * ndn-nr-cs-impl.cc
 *
 *  Created on: Dec 19, 2017
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


bool NrCsImpl::Add (Ptr<const Data> data)
{
	//std::cout<<"add CS Entry  name:"<<data->GetName().toUri()<<std::endl;
	if(Find(data->GetName()))
	{
		//this->Print(std::cout);
		return true;
	}
    Ptr<cs::Entry> csEntry = ns3::Create<cs::Entry>(this,data) ;
    m_csContainer.push_back(csEntry);

    //this->Print(std::cout);
	return true;
}

void
NrCsImpl::DoDispose ()
{
	m_forwardingStrategy = 0;
	//m_fib = 0;
  
	ContentStore::DoDispose ();
}
  
  
Ptr<Entry>
NrCsImpl::Find (const Name &prefix)
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::Find (const Name &prefix) should not be invoked");
	 NS_LOG_INFO ("Finding prefix"<<prefix.toUri());
	 std::vector<Ptr<Entry> >::iterator it;
	 //NS_ASSERT_MSG(m_csContainer.size()!=0,"Empty cs container. No initialization?");
	 for(it=m_csContainer.begin();it!=m_csContainer.end();++it)
	 {
		 if((*it)->GetName()==prefix)
			 return *it;
	 }
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

uint32_t
NrCsImpl::GetSize () const
{
	return m_csContainer.size ();
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


void NrCsImpl::DoInitialize(void)
{
	ContentStore::DoInitialize();
}

} /* namespace nrndn */
} /* namespace cs */
} /* namespace ndn */
} /* namespace ns3 */


