/*
 * ndn-nr-cs-impl.cc
 *
 *  Created on: Jan 2, 2016
 *      Author: DJ
 */

#include "ndn-nr-cs-data-impl.h"
//#include "ndn-fib-entry-nrimpl.h"
//#include "NodeSensor.h"


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
    //.AddAttribute ("CleanInterval", "cleaning interval of the timeout incoming faces of FIB entry",
   		//	                    TimeValue (Seconds (10)),
   			//                    MakeTimeAccessor (&NrCsImpl::m_cleanInterval),
   			  //                  MakeTimeChecker ())
    ;

  return tid;
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
  


  
//need to modify by DJ Dec 25,2015. Fib update according to source packet.
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
  
//modify by DJ Dec 25,2015. Fib update according to source packet.
/*Ptr<Entry>
NrCsImpl::Create (Ptr<const Data> header)
 {

	NS_LOG_DEBUG (header->GetName ());
	NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::Create (Ptr<const Data> header) "
			"should not be invoked, use "
			"NrCsImpl::CreateNrCsEntry instead");
	return 0;
}*/

//need to modify by DJ Dec 25,2015. Fib update according to source packet.
/*bool
NrCsImpl::InitializeNrFibEntry()
{
	NS_LOG_FUNCTION (this);
	const std::vector<std::string>& route =	m_sensor->getNavigationRoute();
	std::vector<std::string>::const_iterator rit;
	for(rit=route.begin();rit!=route.end();++rit)
	{
		Ptr<Name> name = ns3::Create<Name>('/'+*rit);
		Ptr<Data> data=ns3::Create<Data> ();
		//Ptr<Interest> interest=ns3::Create<Interest> ();
		//interest->SetInterestLifetime();
		data->SetName				(name);
		//by DJ on Dec 27,2015:setLifeTime
		//data->SetInterestLifetime	(Time::Max());//never expire

		//Create a fake FIB entry(if not ,L3Protocol::RemoveFace will have problem when using fibEntry->GetFibEntry)
		Ptr<fib::Entry> fibEntry=ns3::Create<fib::Entry>(Ptr<Fib>(0),Ptr<Name>(0));

		Ptr<Entry> entry = ns3::Create<EntryNrImpl>(*this,data,fibEntry,m_cleanInterval) ;
		m_csContainer.push_back(entry);
		NS_LOG_DEBUG("Initialize fib:Push_back"<<name->toUri());
	}
	return true;
}*/
  
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
NrCsImpl::GetSizeEntry () const
{
	return m_csContainer.size ();
}
  
Ptr<Entry>
NrCsImpl::BeginEntry ()
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::Begin () should not be invoked");

	if(m_csContainer.begin() == m_csContainer.end())
		return EndEntry();
	else
		return *(m_csContainer.begin());
}

Ptr<Entry>
NrCsImpl::EndEntry ()
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::End () should not be invoked");
	return 0;
}
  
Ptr<Entry>
NrCsImpl::NextEntry (Ptr<Entry> from)
{
	//NS_ASSERT_MSG(false,"In NrCsImpl,NrCsImpl::Next () should not be invoked");
	if (from == 0) return 0;

	std::vector<Ptr<Entry> >::iterator it;
	it = find(m_csContainer.begin(),m_csContainer.end(),from);
	if(it==m_csContainer.end())
		return EndEntry();
	else
	{
		++it;
		if(it==m_csContainer.end())
			return EndEntry();
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


