/*
 * ndn-nr-pit-impl.h
 *
 *  Created on: Jan 20, 2015
 *      Author: chen
 */

#ifndef NDN_NR_PIT_IMPL_H_
#define NDN_NR_PIT_IMPL_H_

#include "ns3/ndn-pit.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndn-forwarding-strategy.h"
#include "ns3/ndn-name.h"
#include "ns3/SumoMobility.h"

#include "NodeSensor.h"

#include <vector>
#include <set>
#include <queue>
#include <algorithm>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>



namespace ns3
{
namespace ndn
{
namespace pit
{
namespace nrndn
{

/**
 * @ingroup ndn-pit
 * @brief Class implementing Pending Interests Table,
 * 		  with navigation route customize
 */
class NrPitImpl	: public Pit				
{
public:
    /**
     * \brief Interface ID
     *
     * \return interface ID
     */
    static TypeId GetTypeId ();
    
    /**
     * \brief PIT constructor
     */
    NrPitImpl();
    
    /**
     * \brief Destructor
     */
    virtual ~NrPitImpl();
    
    // inherited from Pit
    
    //abandon
    virtual Ptr<Entry>
    Lookup (const Data &header);
    
    //abandon
    virtual Ptr<Entry>
    Lookup (const Interest &header);
    
    //This prefix is different from the format of interest's name
    virtual Ptr<Entry>
    Find (const Name &prefix);
    
    //abandon
    virtual Ptr<Entry>
    Create (Ptr<const Interest> header);
    
    //replace NrPitImpl::Create
    bool
    InitializeNrPitEntry ();
    
    //abandon
    virtual void
    MarkErased (Ptr<Entry> entry);
    
    virtual void
    Print (std::ostream &os) const;
    
    virtual uint32_t
    GetSize () const;
    
    virtual Ptr<Entry>
    Begin ();
    
    virtual Ptr<Entry>
    End ();
    
    virtual Ptr<Entry>
    Next (Ptr<Entry>);
    
    //获取车辆当前所在的路段
    //added by siukwan
    std::string getCurrentLane();
    /**
     * This function update the pit using Interest packet
     * not simply add the name into the pit
     * multi entry of pit will be operated
     */
    //changed by sy
    // bool UpdateCarPit(const std::vector<std::string>& route,const uint32_t& id);
    
    //added by sy
    //bool UpdateRSUPit(const std::vector<std::string>& route,const uint32_t& id);
    
    bool UpdateRSUPit(std::string junction,const std::string forwardRoute,const std::vector<std::string>& interestRoute, const uint32_t& id);
    
    std::pair<std::vector<std::string>,std::vector<std::string> > getInterestRoutesReadytoPass(const std::string junction,const std::string forwardRoute,const std::vector<std::string>& interestRoute);
    
    void SplitString(const std::string& s,std::vector<std::string>& v,const std::string& c);
	
	bool UpdatePrimaryPit(const std::vector<std::string>& interestRoute, const uint32_t& id,const std::string currentRoute);
	
    //added by sy
    std::pair<bool,uint32_t> DeleteFrontNode(const std::string lane,const uint32_t& id);
	
	// 2017.1.8 
	std::unordered_map<std::string,std::unordered_set<std::string> > GetDataNameandLastRoute(std::unordered_set<std::string> routes);
    
    void showPit();
    void laneChange(std::string oldLane, std::string newLane);
	
    
    //小锟添加，2015-8-23
    std::string uriConvertToString(std::string str);
protected:
    // inherited from Object class
    virtual void NotifyNewAggregate (); ///< @brief Even when object is aggregated to another Object
    virtual void DoDispose (); ///< @brief Do cleanup
    virtual void DoInitialize(void);
    
    
    
private:
    Time m_cleanInterval;
    Ptr<ForwardingStrategy>		                   m_forwardingStrategy;
    std::vector<Ptr<Entry> >		               m_pitContainer;
	std::vector<Ptr<Entry> >                       m_secondPitContainer;
    Ptr<ndn::nrndn::NodeSensor>	                   m_sensor;
    Ptr<vanetmobility::sumomobility::SumoMobility> m_sumodata;
	Ptr<ndn::nrndn::NodeSensor> 				   m_sensor;
    friend class EntryNrImpl;
    
    
    Ptr<Fib> m_fib; ///< \brief Link to FIB table(Useless, Just for compatibility)
};


}/* namespace nrndn */
} /* namespace pit */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NDN_NR_PIT_IMPL_H_ */
