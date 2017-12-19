/*
 * ndn-nr-cs-interest-impl.h
 *
 *  Created on: Dec 18, 2017
 *      Author: WSY
 */

#ifndef NDN_NR_CS_INTEREST_IMPL_H_
#define NDN_NR_CS_INTEREST_IMPL_H_

#include "ns3/ndn-content-store.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndn-forwarding-strategy.h"
#include "ns3/ndn-name.h"

#include "NodeSensor.h"

#include <vector>


namespace ns3
{
namespace ndn
{
namespace cs
{
namespace nrndn
{

/**
 * @ingroup ndn-cs
 * @brief Class implementing Content Store,
 * 		  with navigation route customize
 */
class NrCsInterestImpl	: public ContentStore
{
public:
  /**
   * \brief Interface ID
   *
   * \return interface ID
   */
  static TypeId GetTypeId ();

  /**
   * \brief FIB constructor
   */
  NrCsInterestImpl();

  /**
   * \brief Destructor
   */
  virtual ~NrCsInterestImpl();


  //This prefix is different from the format of interest's name
  virtual Ptr<EntryInterest>
  Find (const uint32_t nonce,const uint32_t sourceId);

  //abandon
 /* virtual Ptr<EntryInterest>
  Create (Ptr<const Interest> header);
*/

  //question by DJ on Jan 2,2016:cs need to be modified?
  //replace NrFibImpl::Create
  //bool
  //InitializeNrFibEntryInterest ();

  virtual Ptr<Data>
  Lookup (Ptr<const Interest> interest);

  //abandon
  virtual void
  MarkErased (Ptr<EntryInterest> EntryInterest);

  virtual void
  Print (std::ostream &os) const;
  
  void PrintCache()const;
  
  virtual uint32_t
  GetSize () const;
  
  virtual Ptr<cs::Entry>
  Begin ();

  virtual Ptr<EntryInterest>
  BeginEntryInterest ();

  virtual Ptr<cs::Entry>
  End ();
	
  virtual Ptr<EntryInterest>
  EndEntryInterest ();
  
  virtual Ptr<cs::Entry>
  Next(Ptr<cs::Entry>);

  virtual Ptr<EntryInterest>
  NextEntryInterest (Ptr<EntryInterest>);

  /**
     * \brief Add a new content to the content store.
     *
     * \param header Fully parsed Data
     * \param packet Fully formed Ndn packet to add to content store
     * (will be copied and stripped down of headers)
     * @returns true if an existing EntryInterest was updated, false otherwise
     */
  virtual bool
  Add (Ptr<const Interest> Interest);
  
  std::vector<Ptr<const Interest> >
  GetInterest(std::string lane);


protected:
  // inherited from Object class
  virtual void NotifyNewAggregate (); ///< @brief Even when object is aggregated to another Object
  virtual void DoDispose (); ///< @brief Do cleanup
  virtual void DoInitialize(void);



private:
  Ptr<ForwardingStrategy>		            m_forwardingStrategy;
  std::vector<Ptr<cs::EntryInterest> > 		m_csInterestContainer;
};


}/* namespace nrndn */
} /* namespace cs */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NDN_INTEREST_STORE_H */
