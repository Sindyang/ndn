/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011,2012 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *         Ilya Moiseenko <iliamo@cs.ucla.edu>
 *         
 */

#include "ndn-interest-store.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"

NS_LOG_COMPONENT_DEFINE ("ndn.cs.ContentStoreInterest");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ContentStoreInterestContentStoreInterest);

TypeId
ContentStoreInterest::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ContentStoreInterest")
    .SetGroupName ("Ndn")
    .SetParent<Object> ()
	
    .AddTraceSource ("CacheHits", "Trace called every time there is a cache hit",
                     MakeTraceSourceAccessor (&ContentStoreInterest::m_cacheHitsTrace))

    .AddTraceSource ("CacheMisses", "Trace called every time there is a cache miss",
                     MakeTraceSourceAccessor (&ContentStoreInterest::m_cacheMissesTrace))
    ;
  return tid;
}


ContentStoreInterest::~ContentStoreInterest () 
{
}


namespace cs {

//////////////////////////////////////////////////////////////////////

EntryInterest::EntryInterest (Ptr<ContentStoreInterest> cs, Ptr<const Interest> interest)
  : m_cs (cs)
  , m_interest (interest)
{
}

const Name&
EntryInterest::GetName () const
{
  return m_interest->GetName ();
}

Ptr<const Interest>
EntryInterest::GetInterest () const
{
  return m_interest;
}

Ptr<ContentStoreInterest>
EntryInterest::GetContentStoreInterest ()
{
  return m_cs;
}

} // namespace cs
} // namespace ndn
} // namespace ns3
