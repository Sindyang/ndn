/*
 * ndn-nr-cs-impl.h
 *
 *  Created on: Dec. 29, 2017
 *      Author: WSY
 */

#ifndef NDN_NR_CS_IMPL_H_
#define NDN_NR_CS_IMPL_H_

#include "ns3/ndn-content-store.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndn-forwarding-strategy.h"
#include "ns3/ndn-name.h"

#include "NodeSensor.h"

#include <map>
#include <unordered_set>



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
class NrCsImpl	: public ContentStore
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
  NrCsImpl();

  /**
   * \brief Destructor
   */
  virtual ~NrCsImpl();

  // 2017.12.29 added by sy
  virtual Ptr<Entry>
  Find (const uint32_t signature);

  virtual Ptr<Data>
  Lookup (Ptr<const Interest> interest);

  //abandon
  virtual void
  MarkErased (Ptr<Entry> entry);

  virtual void
  Print (std::ostream &os) const;

  virtual Ptr<Entry>
  Begin ();
  
  virtual Ptr<Entry>
  End ();

  virtual Ptr<Entry>
  Next (Ptr<Entry>);
  

  /**
     * \brief Add a new content to the content store.
     *
     * \param header Fully parsed Data
     * \param packet Fully formed Ndn packet to add to content store
     * (will be copied and stripped down of headers)
     * @returns true if an existing entry was updated, false otherwise
     */
  virtual bool
  Add (Ptr<const Data> data);
  
  //added by sy
  virtual bool
  AddData (uint32_t signature,Ptr<const Data> data);
  
  virtual uint32_t
  GetSize () const;
  
  // 2017.12.29 added by sy
  std::unordered_set<std::string> GetDataName() const;
  
  // 2017.12.29 added by sy
  void PrintCache () const;
  
  // 2017.12.29 added by sy
  void PrintEntry(uint32_t signature);
  
  // 2017.12.29 added by sy
  std::map<uint32_t,Ptr<const Data> > GetData(const Name &prefix);

protected:
  // inherited from Object class
  virtual void NotifyNewAggregate (); ///< @brief Even when object is aggregated to another Object
  virtual void DoDispose (); ///< @brief Do cleanup
  virtual void DoInitialize(void);


private:
  Ptr<ForwardingStrategy>		        m_forwardingStrategy;
  std::map<uint32_t,Ptr<cs::Entry> >	m_csContainer;
};


}/* namespace nrndn */
} /* namespace cs */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NDN_NR_CS_IMPL_H_ */
