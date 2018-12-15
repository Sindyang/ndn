/*
 * SumoNodeSensor.h
 *
 *  Created on: Jan 4, 2015
 *      Author: chen
 */

#ifndef SUMONODESENSOR_H_
#define SUMONODESENSOR_H_

#include "NodeSensor.h"
#include "ns3/SumoMobility.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <set>

namespace ns3
{
namespace ndn
{
namespace nrndn
{

class SumoNodeSensor: public NodeSensor
{
public:
	static TypeId GetTypeId ();
	SumoNodeSensor();
	virtual ~SumoNodeSensor();

	virtual double getX();
	virtual double getY();
			double getPos();
	virtual double getTime();
	virtual double getAngle();
	virtual double getSlope();
	virtual double getSpeed();
	virtual const std::string& getType() ;
	virtual const std::string& getLane();
	//added by sy
	virtual const uint32_t getNumsofVehicles();
	//added by sy
	virtual const std::uint32_t getNodeId();
	virtual const std::vector<std::string>& getNavigationRoute();
	//added by sy
	virtual const std::string RSUGetJunctionId(uint32_t id);

	std::string uriConvertToString(std::string str);
	//std::vector <double> split(const std::string & s, const std::string & d);
	virtual std::pair<bool, double> getDistanceWith(const double& x,const double& y,const std::vector<std::string>& route);
	
	//2017.12.16 added by sy
	virtual std::pair<bool, double> VehicleGetDistanceWithVehicle(const double& x,const double& y);
	
	//2017.12.16 added by sy
	virtual std::pair<bool, double> VehicleGetDistanceWithRSU(const double& x,const double& y,const uint32_t& RSUID);
	
	//2017.12.25 added by sy
	virtual std::pair<bool,double> RSUGetDistanceWithVehicle(const uint32_t RSUID,const double& x,const double& y);
	
	//2017.12.25 added by sy
	virtual std::pair<bool,double> RSUGetDistanceWithRSU(const uint32_t remoteid,std::string lane);
	
	//2017.12.27 added by sy
	virtual std::pair<std::string,std::string> GetLaneJunction(const std::string lane);
	
	//2018.12.15 获得RSU之间的道路
	set<string> RSUGetRoadWithRSU(const uint32_t remoteid);
	
	//virtual bool IsCoverThePath(const double& x,const double& y,const std::vector<std::string>& route);
	//bool IsCorrectPosition(bool x_increase,bool y_increase,int x_begin, int y_begin, int x_end,int y_end, int x,int y);
private:
	/*
	 * @brief convert the coordinate (x,y) to the lane and offset pair
	 * \return (lane,offset)
	 * */
	std::pair<std::string,double> convertCoordinateToLanePos(const double& x,const double& y);


private:
	Ptr<vanetmobility::sumomobility::SumoMobility> m_sumodata;
	std::vector<std::string> m_navigationRoute;
	std::string m_sumoLane;


};

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* SUMONODESENSOR_H_ */
