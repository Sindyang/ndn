/*
 * ndn-nr-cs-interest-impl.h
 *
 *  Created on: Dec 18, 2017
 *  Author: WSY
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

#include <map>


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
    
    NrCsInterestImpl();
    
    /**
     * \brief Destructor
     */
    virtual ~NrCsInterestImpl();
    
    
    /**
     * \brief 查找该nonce对应的兴趣包
     *
     */
    virtual Ptr<EntryInterest>
    Find (const uint32_t nonce);
    
    //abandon
    virtual Ptr<Data>
    Lookup (Ptr<const Interest> interest);
    
    //abandon
    virtual void
    MarkErased (Ptr<EntryInterest> EntryInterest);
    
    virtual void
    Print (std::ostream &os) const;
    
    /**
     * \brief 输出缓存中的全部兴趣包
     *
     */
    void PrintCache() const;
    
    /**
     * \brief 输出该nonce对应的兴趣包信息
     */
    void PrintEntryInterest(uint32_t nonce);
    
    /**
     * \brief 得到缓存空间的大小
     */
    virtual uint32_t
    GetSize () const;
    
    //abandon
    virtual Ptr<cs::Entry>
    Begin ();
    
    /**
     * \返回缓存空间的第一个表项
     */
    virtual Ptr<EntryInterest>
    BeginEntryInterest ();
    
    //abandon
    virtual Ptr<cs::Entry>
    End ();
    
     /**
     * \返回缓存空间的最后一个表项
     */
    virtual Ptr<EntryInterest>
    EndEntryInterest ();
    
    //abandon
    virtual Ptr<cs::Entry>
    Next(Ptr<cs::Entry>);
    
    /**
       * \brief Add a new Interest to the interest store.
       *
       * \param header Fully parsed Data
       * \param packet Fully formed Ndn packet to add to content store
       * (will be copied and stripped down of headers)
       * @returns true if an existing EntryInterest was updated, false otherwise
       */
    virtual bool
    AddInterest (uint32_t nonce, Ptr<const Interest> Interest);
    
    //abandon
    virtual bool
    Add (Ptr<const Data> data);
    
	//返回当前所在路段为lane的兴趣包
    std::vector<Ptr<const Interest> >
    GetInterest(std::string lane);
    
    
protected:
    // inherited from Object class
    virtual void NotifyNewAggregate (); ///< @brief Even when object is aggregated to another Object
    virtual void DoDispose (); ///< @brief Do cleanup
    virtual void DoInitialize(void);



private:
    Ptr<ForwardingStrategy>		             m_forwardingStrategy;
    std::map<uint32_t,Ptr<cs::EntryInterest> > m_csInterestContainer;
};


}/* namespace nrndn */
} /* namespace cs */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NDN_NR_CS_INTEREST_IMPL_H_ */
