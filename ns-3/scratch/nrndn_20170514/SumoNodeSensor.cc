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
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getPos():Cannot find Trace!!");
	return m_sumodata->GetTrace(m_id,pos).pos;
}

double SumoNodeSensor::getTime()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getTime():Cannot find Trace!!");
	return m_sumodata->GetTrace(m_id,pos).time;
}

double SumoNodeSensor::getAngle()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getAngle():Cannot find Trace!!");
	return m_sumodata->GetTrace(m_id,pos).angle;
}

double SumoNodeSensor::getSlope()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getSlope():Cannot find Trace!!");
	return m_sumodata->GetTrace(m_id,pos).slope;
}

double SumoNodeSensor::getSpeed()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getSpeed():Cannot find Trace!!");
	return m_sumodata->GetTrace(m_id,pos).speed;
}

const std::string& SumoNodeSensor::getType()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
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
	cout << "进入(SumoNodeSensor.cc-getDistanceWith)" << endl;
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
	getchar();
	return std::pair<bool, double>(true,distance);
}

//added by sy 2017.9.6
bool SumoNodeSensor::IsCoverThePath(const double& x,const double& y,const std::vector<std::string>& route)
{
	cout<<"进入(SumoNodeSensor.cc-IsCoverThePath)"<<endl;
	//当前节点所在路段和位置
	const string& localLane = getLane();
	const double& localPos = getPos();
	
	vector<string>::const_iterator localLaneIterator;
	vector<string>::const_iterator remoteLaneIterator;
	
	std::pair<std::string, double> remoteInfo = convertCoordinateToLanePos(x,y);
	
	//另一节点所在路段和位置
	const string& remoteLane = remoteInfo.first;
	const double& remotePos  = remoteInfo.second;
	
	//当前节点所在路段在导航路线中的位置
	localLaneIterator  = std::find (route.begin(), route.end(), localLane);
	//另一节点所在路段在导航路线中的位置
	remoteLaneIterator = std::find (route.begin(), route.end(), remoteLane);
	
	if(remoteLaneIterator==route.end()||localLaneIterator==route.end())
	{
		cout<<"(SumoNodeSensor.cc-IsCoverThePath) 当前节点所在路段或另一节点所在路段不在当前节点的导航路线中"<<endl;
		getchar();
	}
	
	uint32_t localIndex = distance(route.begin(),localLaneIterator);
	uint32_t remoteIndex= distance(route.begin(),remoteLaneIterator);
	
	//在同一条路上且另一节点位于当前节点前方
	if(localIndex==remoteIndex && remotePos-localPos > 0)
	{
		if(remotePos-localPos <= TransRange)
		{
			return true;
		}
		else
			return false;
	}
	
	const map<string,vanetmobility::sumomobility::Edge>& edges = m_sumodata->getRoadmap().getEdges();
	std::map<std::string,vanetmobility::sumomobility::Edge>::const_iterator eit;
	if(localIndex < remoteIndex)
	{
		eit = edges.find(uriConvertToString(route.at(localIndex)));
		//获取当前节点所在道路坐标
		string shape = eit->second.lane.shape;
		const char *pshape = shape.c_str();
		const char *sep = ", ";
		char *token = strtok(pshape,sep);
		int count = 1;
		vector<double> x_coordinates;
		vector<double> y_coordinates;
		double x;
		double y;
		while(token)
		{
			cout<<token<<endl;
			if(count%2)
			{
				x = atoi(token);
			}
			else
			{
				y = atoi(token);
				std::pair<std::string, double> pathInfo = convertCoordinateToLanePos(x,y);
				const double& pathPos = remoteInfo.second;
				if(pathPos > localPos)
				{
					x_coordinates.push_back(x);
					y_coordinates.push_back(y);
				}
			}
			token = strtok(NULL,sep);
			count += 1;
		}
		//获取当前节点与另一节点之间的道路坐标
		for(uint32_t i = localIndex+1;i < remoteIndex;++i)
		{
			eit = edges.find(uriConvertToString(route.at(i)));
			shape = eit->second.lane.shape;
			pshape = shape.c_str();
		    token = strtok(pshape,sep);
			count = 1;
			while(token)
			{
				cout<<token<<endl;
				if(count%2)
				{
					x_coordinates.push_back(atoi(token));
				}
				else
				{
					y_coordinates.push_back(atoi(token));
				}
				token = strtok(NULL,sep);
				count+=1;
			}
		}
		//获取另一节点所在道路坐标
		eit = edges.find(uriConvertToString(route.at(remoteIndex)));
		shape = eit->second.lane.shape;
		pshape = shape.c_str();
		token = strtok(pshape,sep);
		count = 1;
		while(token)
		{
			cout<<token<<endl;
			if(count%2)
			{
				x = atoi(token);
			}
			else
			{
				y = atoi(token);
				std::pair<std::string, double> pathInfo = convertCoordinateToLanePos(x,y);
				const double& pathPos = remoteInfo.second;
				if(pathPos<localPos)
				{
					x_coordinates.push_back(x);
					y_coordinates.push_back(y);
				}
			}
			token = strtok(NULL,sep);
			count += 1;
		}
		//判断以当前节点为圆心，传输长度为半径的圆是否可以覆盖到当前节点-转发节点之间的所有路段
		bool iscover = false;
		if(x_coordinates.size() == y_coordinates.size())
		{
			Vector localPos = GetObject<MobilityModel>()->GetPosition();
			localPos.z = 0;
			
			for(int i = 0;i < (signed)x_coordinates.size();i++)
			{
				x = x_coordinates[i];
				y = y_coordinates[i];
				Vector pathPos(x,y,0);
				if(CalculateDistance(localPos,pathPos) < TransRange)
				{
					iscover = true;
				}
				else
				{
					iscover = false;
					break;
				}
			}
		}
		return iscover;
	}
	else if(localIndex > remoteIndex)
	{
		cout<<"(SuNodeSensor.cc-IsCoverThePath) 当前节点位于另一节点前方"<<endl;
		getchar();
	}
	return false;
}

//added by sy 2017.9.5
void SumoNodeSensor::printVehicle()
{
	m_sumodata->PrintVehicle();
}

void SumoNodeSensor::printEdges()
{
	m_sumodata->PrintEdges();
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
