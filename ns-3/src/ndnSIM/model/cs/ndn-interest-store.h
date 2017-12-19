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
 */

#ifndef NDN_INTEREST_STORE_H
#define	NDN_INTEREST_STORE_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"

#include <boost/tuple/tuple.hpp>

namespace ns3 {

class Packet;

namespace ndn {

class Data;
class Interest;
class Name;
class ContentStoreInterest;

/**
 * @ingroup ndn
 * @defgroup ndn-cs Content Store
 */

/**
 * @ingroup ndn-cs
 * @brief Namespace for ContentStore operations
 */
namespace cs {

/**
 * @ingroup ndn-cs
 * @brief NDN content store EntryInterest
 */
class EntryInterest : public SimpleRefCount<EntryInterest>
{
public:
  /**
   * \brief Construct content store EntryInterest
   *
   * \param header Parsed Interest header
   * \param packet Original Ndn packet
   *
   * The constructor will make a copy of the supplied packet and calls
   * RemoveHeader and RemoveTail on the copy.
   */
  EntryInterest (Ptr<ContentStoreInterest> cs, Ptr<const Interest> interest);

  /**
   * \brief Get prefix of the stored EntryInterest
   * \returns prefix of the stored EntryInterest
   */
  const Name&
  GetName () const;

  /**
   * \brief Get Data of the stored EntryInterest
   * \returns Data of the stored EntryInterest
   */
  Ptr<const Interest>
  GetInterest () const;

  /**
   * @brief Get pointer to access store, to which this EntryInterest is added
   */
  Ptr<ContentStoreInterest>
  GetContentStoreInterest ();

private:
  Ptr<ContentStoreInterest> m_cs; ///< \brief content store to which EntryInterest is added
  Ptr<const Interest> m_interest; ///< \brief non-modifiable Data
};

} // namespace cs


/**
 * @ingroup ndn-cs
 * \brief Base class for NDN content store
 *
 * Particular implementations should implement Lookup, Add, and Print methods
 */
class ContentStoreInterest : public Object
{
public:
  /**
   * \brief Interface ID
   *
   * \return interface ID
   */
  static
  TypeId GetTypeId ();

  /**
   * @brief Virtual destructor
   */
  virtual
  ~ContentStoreInterest ();

  /**
   * \brief Find corresponding CS EntryInterest for the given interest
   *
   * \param interest Interest for which matching content store EntryInterest
   * will be searched
   *
   * If an EntryInterest is found, it is promoted to the top of most recent
   * used entries index, \see m_contentStore
   */
  //virtual Ptr<Data>
  //Lookup (Ptr<const Interest> interest) = 0;

  /**
   * \brief Add a new content to the content store.
   *
   * \param header Fully parsed Data
   * \param packet Fully formed Ndn packet to add to content store
   * (will be copied and stripped down of headers)
   * @returns true if an existing EntryInterest was updated, false otherwise
   */
  virtual bool
  Add (Ptr<const Interest> interest) = 0;

  // /*
  //  * \brief Add a new content to the content store.
  //  *
  //  * \param header Interest header for which an EntryInterest should be removed
  //  * @returns true if an existing EntryInterest was removed, false otherwise
  //  */
  // virtual bool
  // Remove (Ptr<Interest> header) = 0;

  /**
   * \brief Print out content store entries
   */
  virtual void
  Print (std::ostream &os) const = 0;


  /**
   * @brief Get number of entries in content store
   */
  virtual uint32_t
  GetSize () const = 0;

  /**
   * @brief Return first element of content store (no order guaranteed)
   */
  virtual Ptr<cs::EntryInterest>
  Begin () = 0;

  /**
   * @brief Return item next after last (no order guaranteed)
   */
  virtual Ptr<cs::EntryInterest>
  End () = 0;

  /**
   * @brief Advance the iterator
   */
  virtual Ptr<cs::EntryInterest>
  Next (Ptr<cs::EntryInterest>) = 0;

  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Static call to cheat python bindings
   */
  static inline Ptr<ContentStoreInterest>
  GetContentStoreInterest (Ptr<Object> node);

protected:
  TracedCallback<Ptr<const Interest>,
                 Ptr<const Data> > m_cacheHitsTrace; ///< @brief trace of cache hits

  TracedCallback<Ptr<const Interest> > m_cacheMissesTrace; ///< @brief trace of cache misses
};

inline std::ostream&
operator<< (std::ostream &os, const ContentStoreInterest &cs)
{
  cs.Print (os);
  return os;
}

inline Ptr<ContentStoreInterest>
ContentStoreInterest::GetContentStoreInterest (Ptr<Object> node)
{
  return node->GetObject<ContentStoreInterest> ();
}


} // namespace ndn
} // namespace ns3

#endif // NDN_CONTENT_STORE_H
