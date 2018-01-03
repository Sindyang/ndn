/*
 * SumoNodeSensor.cc
 *
 *  Created on: Jan 4, 2015
 *      Author: chen
 */

#include "SumoNodeSensor.h"

#include "ns3/RouteElement.h"

namespace ns3
{
namespace ndn
{
namespace nrndn
{
using namespace std;
#define TransRange 300
NS_OBJECT_ENSURE_REGISTERED (SumoNodeSensor);

SumoNodeSensor::SumoNodeSensor()
{
	// TODO Auto-generated constructor stub

}

SumoNodeSensor::~SumoNodeSensor()
{
	// TODO Auto-generated destructor stub
}

TypeId SumoNodeSensor::GetTypeId()
{
	  static TypeId tid = TypeId ("ns3::ndn::nrndn::SumoNodeSensor")
	    .SetGroupName ("Nrndn")
	    .SetParent<NodeSensor> ()
	    .AddConstructor<SumoNodeSensor> ()
	    .AddAttribute("sumodata", "The trace data read by sumo files.",
	   	    		PointerValue (),
	   	    		MakePointerAccessor (&SumoNodeSensor::m_sumodata),
	   	    		MakePointerChecker<vanetmobility::sumomobility::SumoMobility> ())
	    ;
	  return tid;
}

double SumoNodeSensor::getX()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	//RSU
	//if(m_id >= getNumsofVehicles())
		//return m_sumodata->getVl().getVehicles()[m_id].trace.front().x;
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getX():Cannot find Trace!!");
	NS_ASSERT_MSG(m_sumodata->GetTrace(m_id,pos).x == pos.x,"Can not find coordinate x");
	return m_sumodata->GetTrace(m_id,pos).x;
}

double SumoNodeSensor::getY()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	//RSU
	//if(m_id >= getNumsofVehicles())
		//return m_sumodata->getVl().getVehicles()[m_id].trace.front().y;
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getY():Cannot find Trace!!");
	NS_ASSERT_MSG(m_sumodata->GetTrace(m_id,pos).y == pos.y,"Can not find coordinate y");
	return m_sumodata->GetTrace(m_id,pos).y;
}


double SumoNodeSensor::getPos()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	//RSU
	//if(m_id >= getNumsofVehicles())
		//return m_sumodata->getVl().getVehicles()[m_id].trace.front().pos;
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getPos():Cannot find Trace!!");
	return m_sumodata->GetTrace(m_id,pos).pos;
}

double SumoNodeSensor::getTime()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	//RSU
	//if(m_id >= getNumsofVehicles())
		//return m_sumodata->getVl().getVehicles()[m_id].trace.front().time;
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getTime():Cannot find Trace!!");
	return m_sumodata->GetTrace(m_id,pos).time;
}

double SumoNodeSensor::getAngle()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	//RSU
	//if(m_id >= getNumsofVehicles())
		//return m_sumodata->getVl().getVehicles()[m_id].trace.front().angle;
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getAngle():Cannot find Trace!!");
	return m_sumodata->GetTrace(m_id,pos).angle;
}

double SumoNodeSensor::getSlope()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	//RSU
	//if(m_id >= getNumsofVehicles())
		//return m_sumodata->getVl().getVehicles()[m_id].trace.front().slope;
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getSlope():Cannot find Trace!!");
	return m_sumodata->GetTrace(m_id,pos).slope;
}

double SumoNodeSensor::getSpeed()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	//RSU
	//if(m_id >= getNumsofVehicles())
		//return m_sumodata->getVl().getVehicles()[m_id].trace.front().speed;
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getSpeed():Cannot find Trace!!");
	return m_sumodata->GetTrace(m_id,pos).speed;
}

const std::string& SumoNodeSensor::getType()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	//RSU
	//if(m_id >= getNumsofVehicles())
		//return m_sumodata->getVl().getVehicles()[m_id].trace.front().type;
	if(&m_sumodata->GetTrace(m_id,pos)==NULL)
		return emptyType;
	return m_sumodata->GetTrace(m_id,pos).type;
}

const std::string& SumoNodeSensor::getLane()
{
	NodeSensor::getLane();
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t id = node->GetId();
	//RSU
	//if(id >= getNumsofVehicles())
		//return m_sumodata->getVl().getVehicles()[id].trace.front().lane;
	//cout << "(SumoNodeSensor.cc-getLane)" << " NodeId " << id << endl;
	
	if(&(m_sumodata->GetTrace(id,pos))==NULL)
	{
		//cout << "(SumoNodeSensor.cc-getLane) " << "m_sumodata == NULL" <<endl;
		m_lane.Set(emptyLane);
		m_sumoLane = m_lane.Get();
		//cout << "(SumoNodeSensor.cc-getLane) " << "m_sumoLane" << endl;
		return m_sumoLane;
	}
     
    if(&(m_sumodata->GetTrace(id,pos))==NULL)
	{
		//cout << "(SumoNodeSensor.cc-getLane) " << "id " <<id << " pos x " << pos.x <<" pos y " << pos.y <<" pos z " << pos.z << endl;
		//cout << "(SumoNodeSensor.cc-getLane) " << "m_lane.Set(emptyLane);" <<endl;
		//cout << "(SumoNodeSensor.cc-getLane) " << "Time now: "<<Simulator::Now().GetSeconds()<<endl;
		if(int(pos.x) == 10000 || int(pos.x) == -10000)
		{
			//cout << "(SumoNodeSensor.cc-getLane) " << "10000 or -10000" << endl;
			m_sumoLane = emptyLane; //NodeSensor.cc const std::string NodeSensor::emptyLane("UNKNOWN_LANE");
		//	cout <<"(SumoNodeSensor.cc-getLane) m_sumoLane "<< m_sumoLane << endl;
			return emptyLane;
		}

        m_lane.Set(emptyLane);
		//cout <<"SumoNodeSensor.cc: " << "m_lane.Set(emptyLane);" <<endl;
	}
    else
	{
		//cout << "SumoNodeSensor.cc: " << "m_lane.Set(m_sumodata->GetTrace(id,pos).lane):" <<m_sumodata->GetTrace(id,pos).lane << endl;
    	m_lane.Set(m_sumodata->GetTrace(id,pos).lane);
		//cout << "SumoNodeSensor.cc: " << "m_lane.Set(m_sumodata->GetTrace(id,pos).lane);" <<endl;
	}
	//cout <<"(SumoNodeSensor.cc-getLane) "<< "GetTraceid" << endl;
	//std::cout<<"(SumoNodeSensor.cc-getLane) NodeId "<<id<<"'s current lane is "<<m_lane<<std::endl;
	m_sumoLane = m_lane.Get();
	//cout <<"(SumoNodeSensor.cc-getLane) m_sumoLane " << m_sumoLane << endl;
    return m_sumoLane;
}

const uint32_t SumoNodeSensor::getNumsofVehicles()
{
	return m_sumodata->GetVehiclesSize();
}
	
const std::uint32_t SumoNodeSensor::getNodeId()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t id = node->GetId();
	return id;
}

const std::vector<std::string>& SumoNodeSensor::getNavigationRoute()
{
	if(!m_navigationRoute.empty())
		return m_navigationRoute;
	else
	{
		//Set the navigation route according to the m_sumodata at the first time invoked
		Ptr<Node> node		= this->GetObject<Node>();
		uint32_t id			= node->GetId();
		m_navigationRoute	= m_sumodata->getVl().getVehicles()[id].route.edgesID;
	}
	return m_navigationRoute;
}


const std::string SumoNodeSensor::RSUGetJunctionId(uint32_t id)
{
	std::vector<std::string> route = m_sumodata->getVl().getVehicles()[id].route.edgesID;
	const map<string,vanetmobility::sumomobility::Edge>& edges = m_sumodata->getRoadmap().getEdges();
	std::map<std::string,vanetmobility::sumomobility::Edge>::const_iterator eit;
	eit = edges.find(route[0]);
	std::string JunctionId = eit->second.to;
	return JunctionId;
}

std::string SumoNodeSensor::uriConvertToString(std::string str)
{
	//因为获取兴趣时使用toUri，避免出现类似[]的符号，进行编码转换
	std::string ret="";
	for(uint32_t i=0;i<str.size();i++)
	{
		if(i+2<str.size())
		{
			if(str[i]=='%'&&str[i+1]=='5'&&str[i+2]=='B')
			{
				ret+="[";
				i=i+2;
			}
			else if(str[i]=='%'&&str[i+1]=='5'&&str[i+2]=='D')
			{
				ret+="]";
				i=i+2;
			}
			else if(str[i]=='%'&&str[i+1]=='2'&&str[i+2]=='3')
			{
				ret+="#";
				i=i+2;
			}
			else
				ret+=str[i];
		}
		else
			ret+=str[i];
	}
	return ret;
}

std::pair<bool, double> SumoNodeSensor::getDistanceWith(const double& x,const double& y,const std::vector<std::string>& route)
{
	//cout << "进入(SumoNodeSensor.cc-getDistanceWith)" << endl;
	//当前节点所在路段和位置
	const string& localLane = getLane();
	const double& localPos  = getPos();
	//cout << "(SumoNodeSensor.cc-getPos )" << endl;

	vector<string>::const_iterator localLaneIterator;
	vector<string>::const_iterator remoteLaneIterator;

	std::pair<std::string, double> remoteInfo =
			convertCoordinateToLanePos(x,y);
	//cout << "(SuNodeSensor.cc-getDistanceWith) convertCoordinateToLanePos" <<endl;
	//另一节点所在路段和位置
	const string& remoteLane = remoteInfo.first;
	const double& remotePos  = remoteInfo.second;

	//当前节点所在路段在导航路线中的位置
	localLaneIterator  = std::find (route.begin(), route.end(), localLane);
	//另一节点所在路段在导航路线中的位置
	remoteLaneIterator = std::find (route.begin(), route.end(), remoteLane);

	if(remoteLaneIterator==route.end()||
			localLaneIterator==route.end())
	{
		Vector 	localPos = GetObject<MobilityModel>()->GetPosition();
		localPos.z=0;//Just in case
		Vector remotePos(x,y,0);
		
		return std::pair<bool, double>(false,CalculateDistance(localPos,remotePos));
	}

	uint32_t localIndex = distance(route.begin(),localLaneIterator);
	uint32_t remoteIndex= distance(route.begin(),remoteLaneIterator);
    
	//cout << "(SuNodeSensor.cc-getDistanceWith) localIndex==remoteIndex " << endl;
	if(localIndex==remoteIndex)//在同一条路上
		return pair<bool, double>(true, remotePos-localPos);

	double distance,beginLen, middleLen;
	distance = beginLen = middleLen = 0;
	const map<string,vanetmobility::sumomobility::Edge>& edges = m_sumodata->getRoadmap().getEdges();
	std::map<std::string,vanetmobility::sumomobility::Edge>::const_iterator eit;
	//当前节点位于另一节点前方
	if(localIndex>remoteIndex)
	{
		//cout << "(SuNodeSensor.cc-getDistanceWith) "<< "localIndex>remoteIndex " << endl;
		//From behind
		eit = edges.find(uriConvertToString(route.at(remoteIndex)));
		NS_ASSERT_MSG(eit!=edges.end(),"No edge info for "<<remoteLane << "注意查看uriConvertToString转码是否缺少情况");
		//=======.=======|=========|========|===============.=============
		//       remote(3)	(4)			(5)				local(6)
		//       |-------|------------------|---------------|
		//       beginLen     middleLen             localPos
		beginLen = eit->second.lane.length - remotePos ;
		for(uint32_t i = remoteIndex + 1; i<localIndex;++i)
		{
			eit = edges.find(uriConvertToString(route.at(i)));
			NS_ASSERT_MSG(eit!=edges.end(),"No edge info for "<<uriConvertToString(route.at(i)));
			middleLen+=eit->second.lane.length;
		}
		//cout << "(SuNodeSensor.cc-getDistanceWith) "<< "(beginLen+middleLen+localPos)" << endl;
		distance = -(beginLen+middleLen+localPos);
	}
	//当前节点位于另一节点后方
	else
	{
		//cout << "(SuNodeSensor.cc-getDistanceWith) "<< "else" << endl;
		//From front
		eit = edges.find(uriConvertToString(route.at(localIndex)));
		NS_ASSERT_MSG(eit!=edges.end(),"No edge info for "<<remoteLane);
		//=======.=======|=========|========|===============.=============
		//       local(3)	(4)			(5)				remote(6)
		//       |-------|------------------|---------------|
		//       beginLen     middleLen             remotePos
		beginLen = eit->second.lane.length - localPos ;
		for(uint32_t i = localIndex + 1; i<remoteIndex;++i)
		{
			eit = edges.find(uriConvertToString(route.at(i)));
			NS_ASSERT_MSG(eit!=edges.end(),"No edge info for "<<uriConvertToString(route.at(i)));
			middleLen+=eit->second.lane.length;
		}
		//cout << "(SuNodeSensor.cc-getDistanceWith) "<< " beginLen+middleLen+remotePos" << endl;
		distance = beginLen+middleLen+remotePos;

	}
	//cout << "(SuNodeSensor.cc-getDistanceWith) "<< "return" << endl;
	//getchar();
	return std::pair<bool, double>(true,distance);
}

//2017.12.13 判断车辆与其他车辆(及其发送消息包)的位置关系
std::pair<bool, double> SumoNodeSensor::VehicleGetDistanceWithVehicle(const double& x,const double& y)
{
	//cout << "进入(SumoNodeSensor.cc-getDistanceWith)" << endl;
	//当前节点所在路段和位置
	const string& localLane = getLane();
	const double& localPos  = getPos();
	cout<<"(SumoNodeSensor.cc-VehicleGetDistanceWithVehicle) localLane "<<localLane<<endl;
	cout<<"(SumoNodeSensor.cc-VehicleGetDistanceWithVehicle) localPos "<<localPos<<endl;

	std::pair<std::string, double> remoteInfo = convertCoordinateToLanePos(x,y);

	//另一节点所在路段和位置
	const string& remoteLane = remoteInfo.first;
	const double& remotePos  = remoteInfo.second;
	cout<<"(SumoNodeSensor.cc-VehicleGetDistanceWithVehicle) remoteLane "<<remoteLane<<endl;
	cout<<"(SumoNodeSensor.cc-VehicleGetDistanceWithVehicle) remotePos "<<remotePos<<endl;
	
	//位于同一路段
	if(remoteLane == localLane)
	{
		return pair<bool, double>(true, remotePos-localPos);
	}
	else//位于不同路段
	{
		Vector localPos = GetObject<MobilityModel>()->GetPosition();
		localPos.z=0;//Just in case
		Vector remotePos(x,y,0);
		return std::pair<bool, double>(false,CalculateDistance(localPos,remotePos));
	}
}

//2017.12.16 判断车辆与RSU(及其发送心跳包)的位置关系
std::pair<bool, double> SumoNodeSensor::VehicleGetDistanceWithRSU(const double& x,const double& y,const uint32_t& RSUID)
{
	//当前节点所在路段和位置
	const string& localLane = getLane();
	const double& localPos  = getPos();
	const map<string,vanetmobility::sumomobility::Edge>& edges = m_sumodata->getRoadmap().getEdges();
	std::map<std::string,vanetmobility::sumomobility::Edge>::const_iterator eit;
	eit = edges.find(localLane);
	//当前节点所在路段的起点
	std::string from = eit->second.from;
	//当前节点所在路段的终点
	std::string to = eit->second.to;
	//当前节点所在路段的长度
	double length = eit->second.lane.length;
	//cout<<"(SumoNodeSensor.cc-VehicleGetDistanceWithRSU) localLane "<<localLane<<" from "<<from<<" to "<<to<<" length "<<length<<endl;
	//RSU所在交点
	std::string junction = RSUGetJunctionId(RSUID);
	//cout<<"(SumoNodeSensor.cc-VehicleGetDistanceWithRSU) junction "<<junction<<endl;
	//RSU位于车辆所在路段起点
	if(from == junction)
	{
		return pair<bool,double>(true,-localPos);
	}
	//RSU位于车辆所在路段终点
	else if(to == junction)
	{
		return pair<bool,double>(true,length-localPos);
	}
	//RSU与车辆位于不同路段
	else
	{
		Vector localPos = GetObject<MobilityModel>()->GetPosition();
		localPos.z=0;//Just in case
		Vector remotePos(x,y,0);
		return std::pair<bool, double>(false,CalculateDistance(localPos,remotePos));
	}
}

/*2017.12.25 判断RSU与车辆的位置关系
 * x,y为另一节点的坐标
 */
std::pair<bool,double> SumoNodeSensor::RSUGetDistanceWithVehicle(const uint32_t RSUID,const double& x,const double& y)
{
	std::pair<std::string, double> remoteInfo = convertCoordinateToLanePos(x,y);

	//另一节点所在路段和位置
	const string& remoteLane = remoteInfo.first;
	const double& remotePos  = remoteInfo.second;
	//std::cout<<"(SumoNodeSensor.cc-RSUGetDistanceWithVehicle) 另一节点所在路段为 "<<remoteLane<<std::endl;
	
	const map<string,vanetmobility::sumomobility::Edge>& edges = m_sumodata->getRoadmap().getEdges();
	std::map<std::string,vanetmobility::sumomobility::Edge>::const_iterator eit;
	eit = edges.find(remoteLane);
	std::string from = eit->second.from;
	//std::cout<<"(SumoNodeSensor.cc-RSUGetDistanceWithVehicle) 另一节点所在路段的起点为 "<<from<<std::endl;
	
	std::string to = eit->second.to;
	//std::cout<<"(SumoNodeSensor.cc-RSUGetDistanceWithVehicle) 另一节点所在路段的终点为 "<<to<<std::endl;
	
	//另一节点所在路段的长度
	double length = eit->second.lane.length;
	
	std::string junction = RSUGetJunctionId(RSUID);
	//std::cout<<"(SumoNodeSensor.cc-RSUGetDistanceWithVehicle) RSU所在的交点为 "<<junction<<std::endl;
	if(from == junction)
	{
		//std::cout<<"(SumoNodeSensor.cc-RSUGetDistanceWithVehicle) 车辆在RSU前方 距离为 "<<remotePos<<std::endl;
		return pair<bool,double>(true,remotePos);
	}
	else if(to == junction)
	{
		//std::cout<<"(SumoNodeSensor.cc-RSUGetDistanceWithVehicle) 车辆在RSU后方 距离为 "<<length-remotePos<<std::endl;
		return pair<bool,double>(true,-(length-remotePos));
	}
	else
	{
		Vector localPos = GetObject<MobilityModel>()->GetPosition();
		localPos.z=0;//Just in case
		Vector remotePos(x,y,0);
		return std::pair<bool, double>(false,CalculateDistance(localPos,remotePos));
	}
}

/* 2017.12.25 判断RSU与RSU的位置关系 
 * lane代表兴趣包当前经过的路段
 */
std::pair<bool,double> SumoNodeSensor::RSUGetDistanceWithRSU(const uint32_t remoteid,std::string lane)
{
	const map<string,vanetmobility::sumomobility::Edge>& edges = m_sumodata->getRoadmap().getEdges();
	std::map<std::string,vanetmobility::sumomobility::Edge>::const_iterator eit;
	eit = edges.find(lane);
	//std::cout<<"(SumoNodeSensor.cc-RSUGetDistanceWithRSU) 节点所经过的路段为 "<<lane<<std::endl;
	std::string from = eit->second.from;
	//std::cout<<"(SumoNodeSensor.cc-RSUGetDistanceWithRSU) 另一节点所在路段的起点为 "<<from<<std::endl;
	
	std::string to = eit->second.to;
	//std::cout<<"(SumoNodeSensor.cc-RSUGetDistanceWithRSU) 另一节点所在路段的终点为 "<<to<<std::endl;
	
	//另一节点所在路段的长度
	double length = eit->second.lane.length;
	
	std::string remotejunction = RSUGetJunctionId(remoteid);
	//std::cout<<"(SumoNodeSensor.cc-RSUGetDistanceWithRSU) 另一节点的交点ID为 "<<remotejunction<<std::endl;
	
	std::string localjunction = RSUGetJunctionId(getNodeId());
	//std::cout<<"(SumoNodeSensor.cc-RSUGetDistanceWithRSU) 当前节点的交点ID为 "<<localjunction<<std::endl;
	
	if(remotejunction == localjunction)
	{
		//std::cout<<"(SumoNodeSensor.cc-RSUGetDistanceWithRSU) 两个节点属于同一节点"<<std::endl;
		return std::pair<bool,double>(true,0);
	}
	if(from == remotejunction && to == localjunction)
	{
		std::cout<<"(SumoNodeSensor.cc-RSUGetDistanceWithRSU) 另一节点位于路段起点，当前节点位于路段终点"<<std::endl;
		return std::pair<bool,double>(true,-length);
	}
	else if(from == localjunction && to == remotejunction)
	{
		std::cout<<"(SumoNodeSensor.cc-RSUGetDistanceWithRSU) 当前节点位于路段起点，另一节点位于路段终点"<<std::endl;
		return std::pair<bool,double>(true,length);
	}
	NS_ASSERT_MSG(false,"路段的起始点与RSU的交点ID不相同");
}

/*
 * 2017.12.27 
 * 获取路段的起点和终点
 */
std::pair<std::string,std::string> SumoNodeSensor::GetLaneJunction(const std::string lane)
{
	const map<string,vanetmobility::sumomobility::Edge>& edges = m_sumodata->getRoadmap().getEdges();
	std::map<std::string,vanetmobility::sumomobility::Edge>::const_iterator eit;
	eit = edges.find(lane);
	//std::cout<<"(SumoNodeSensor.cc-RSUGetDistanceWithRSU) 节点所经过的路段为 "<<lane<<std::endl;
	std::string from = eit->second.from;
	//std::cout<<"(SumoNodeSensor.cc-RSUGetDistanceWithRSU) 另一节点所在路段的起点为 "<<from<<std::endl;
	std::string to = eit->second.to;
	return std::pair<std::string,std::string>(from,to);
}


std::pair<std::string, double> SumoNodeSensor::convertCoordinateToLanePos(
		const double& x, const double& y)
{
	const vanetmobility::sumomobility::SumoMobility::CoordinateToLaneType& cvTable=
			m_sumodata->getCoordinateToLane();
	vanetmobility::sumomobility::SumoMobility::CoordinateToLaneType::const_iterator it;
	it=cvTable.find(Vector2D(x,y));
	NS_ASSERT_MSG(it!=cvTable.end(),"cannot find the trace of ("<<x<<","<<y<<")");
	return it->second;
}

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */
