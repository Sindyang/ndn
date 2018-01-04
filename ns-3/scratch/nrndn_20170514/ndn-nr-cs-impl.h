/*
 * ndn-nr-cs-impl.h
 *
 *  Created on: Jan. 4, 2017
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
class NrCsImpl: public ContentStore
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
    
    //abandon
    virtual Ptr<Data>
    Lookup (Ptr<const Interest> interest);
    
    //abandon
    virtual void
    MarkErased (Ptr<Entry> entry);
    
	//abandon
    virtual void
    Print (std::ostream &os) const;
    
	//abandon
    virtual Ptr<Entry>
    Begin ();
    
	//abandon
    virtual Ptr<Entry>
    End ();
    
	//abandon
    virtual Ptr<Entry>
    Next (Ptr<Entry>);
    
	//abandon
    virtual uint32_t
    GetSize () const;
	
    /**
     * \brief Add a new content to the content store.
     *
     * \param header Fully parsed Data
     * \param packet Fully formed Ndn packet to add to content store
     * (will be copied and stripped down of headers)
     * @returns true if an existing entry was updated, false otherwise
    **/
	//abandon
    virtual bool
    Add (Ptr<const Data> data);
	
	
	/*数据包部分*/
	
    // 添加数据包进入缓存中
    bool
    AddData (uint32_t signature,Ptr<const Data> data);
	
	// 得到相同名字的数据包
	std::map<uint32_t,Ptr<const Data> > 
	GetData(const Name &prefix);
    
	// 获取数据包缓存大小
	uint32_t
	GetDataSize () const;
	
	// 获取缓存中的数据包名字
    std::unordered_set<std::string> 
	GetDataName() const;
	
	// 根据序列号查找数据包
    Ptr<Entry>
    FindData (const uint32_t signature);
	
	// 输出缓存中的数据包
	void 
	PrintDataCache () const;
  
	// 根据序列号输出数据包
	void 
	PrintDataEntry(uint32_t signature);
  
	/*数据包部分*/
    
	
    /*兴趣包部分*/
	
	// 添加兴趣包进入缓存中
	bool
    AddInterest (uint32_t nonce, Ptr<const Interest> Interest);
	
	//返回当前所在路段为lane的兴趣包
    std::map<uint32_t,Ptr<const Interest> >
    GetInterest(std::string lane);
	
	uint32_t
    GetInterestSize () const;
	
	//根据序列号查找兴趣包
    Ptr<EntryInterest>
    FindInterest (const uint32_t nonce);
	
	// 输出缓存中的兴趣包
	void 
	PrintInterestCache() const;
	
	// 根据序列号输出兴趣包
	void 
	PrintInterestEntry(uint32_t nonce);
	
	/*兴趣包部分*/
  
    
protected:
    // inherited from Object class
    virtual void NotifyNewAggregate (); ///< @brief Even when object is aggregated to another Object
    virtual void DoDispose (); ///< @brief Do cleanup
    virtual void DoInitialize(void);
    
    
private:
    Ptr<ForwardingStrategy>		        m_forwardingStrategy;
    std::map<uint32_t,Ptr<cs::Entry> >	m_data;
	std::map<uint32_t,Ptr<cs::EntryInterest>> m_interest;
};


}/* namespace nrndn */
} /* namespace cs */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NDN_NR_CS_IMPL_H_ */
