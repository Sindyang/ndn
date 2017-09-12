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

//added by sy 2017.9.6
bool SumoNodeSensor::IsCoverThePath(const double& x,const double& y,const std::vector<std::string>& route)
{
	//cout<<"进入(SumoNodeSensor.cc-IsCoverThePath)"<<endl;
	//当前节点所在路段和位置
	const string& localLane = getLane();
	const double& localPos = getPos();
	//cout<<"(SuNodeSensor-IsCoverThePath) 当前节点所在路段 "<<localLane<<", 所在位置 "<<localPos<<endl;
	
	vector<string>::const_iterator localLaneIterator;
	vector<string>::const_iterator remoteLaneIterator;
	
	std::pair<std::string, double> remoteInfo = convertCoordinateToLanePos(x,y);
	
	//另一节点所在路段和位置
	const string& remoteLane = remoteInfo.first;
	const double& remotePos  = remoteInfo.second;
	//cout<<"(SumoNodeSensor-IsCoverThePath) 另一节点所在路段 "<<remoteLane<<", 所在位置 "<<remotePos<<endl;
	
	//当前节点所在路段在导航路线中的下标
	localLaneIterator  = std::find (route.begin(), route.end(), localLane);
	//另一节点所在路段在导航路线中的下标
	remoteLaneIterator = std::find (route.begin(), route.end(), remoteLane);
	
	NS_ASSERT_MSG(remoteLaneIterator != route.end() && localLaneIterator != route.end(),"当前节点所在路段或另一节点所在路段不在当前节点的导航路线中");
	
	if(remoteLaneIterator==route.end()||localLaneIterator==route.end())
	{
		cout<<"(SumoNodeSensor.cc-IsCoverThePath) 当前节点所在路段或另一节点所在路段不在当前节点的导航路线中"<<endl;
		cout<<"当前节点所在路段为 "<<localLane<<endl;
		cout<<"另一节点所在路段为 "<<remoteLane<<endl;
		NS_ASSERT(remoteLaneIterator!=route.end() || localLaneIterator!=route.end());
		getchar();
	}
	
	uint32_t localIndex = distance(route.begin(),localLaneIterator);
	uint32_t remoteIndex= distance(route.begin(),remoteLaneIterator);
	
	//在同一条路上且另一节点位于当前节点前方
	if(localIndex==remoteIndex && remotePos-localPos > 0)
	{
		if(remotePos-localPos <= TransRange)
		{
			cout<<"(SumoNodeSensor-IsCoverThePath) 两节点位于同一段路且另一节点在当前节点前方 ,距离 "<<remotePos-localPos<<endl;
			return true;
		}
		else
		{
			cout<<"(SumoNodeSensor-IsCoverThePath) 两节点位于同一段路且另一节点在当前节点前方，但超出了传输范围。距离 "<<remotePos-localPos<<endl;
			return false;
		}
	}
	
	const map<string,vanetmobility::sumomobility::Edge>& edges = m_sumodata->getRoadmap().getEdges();
	std::map<std::string,vanetmobility::sumomobility::Edge>::const_iterator eit;
	if(localIndex < remoteIndex)
	{
		eit = edges.find(uriConvertToString(route.at(localIndex)));
		//获取当前节点所在道路坐标
		string shape = eit->second.lane.shape;
		cout<<"(SumoNodeSensor-IsCoverThePath) 当前节点所在道路为 "<<eit->second.lane.id<<" ,坐标为 "<<shape<<endl;
		vector<double> coordinates = split(shape," ,");
		int num = 0;
		cout<<"道路的坐标个数为 "<<coordinates.size()<<endl;
		
		if(coordinates.size()%2 == 0)
		{
			num = coordinates.size()/2;
		}
		else
		{
			cout<<"(SumoNodeSensor-IsCoverThePath) x与y的坐标个数不相同"<<endl;
		}
		
		vector<double> x_coordinates;
		vector<double> y_coordinates;
		
		if(num)
		{
			if(num == 1)
			{
				cout<<"(SumoNodeSensor-IsCoverThePath) 只有一个坐标"<<endl;
				getchar();
			}
			else if(num == 2)
			{
				x_coordinates.push_back(coordinates[2]);
				y_coordinates.push_back(coordinates[3]);
				cout<<"已添加坐标 ("<<coordinates[2]<<","<<coordinates[3]<<")"<<endl;
			}
			else
			{
				cout<<"(SumoNodeSensor-IsCoverThePath) x与y的个数大于2"<<endl;
				double x_begin = coordinates.front();
				double x_end = coordinates.back()-1;
				bool x_increase = false;
				if(x_begin < x_end)
				{
					x_increase = true;
				}
				
				double y_begin = coordinates.front()+1;
				double y_end = coordinates.back();
				bool y_increase = false;
				if(y_begin < y_end)
				{
					y_increase = true;
				}
				
				double x_local = getX();
				double y_local = getY();
				//寻找符合条件的道路坐标，即位于当前节点与道路末端之间的坐标
				for(int i = 0;i < num*2;i += 2)
				{
					bool flag = IsCorrectPosition(x_increase,y_increase,x_local,y_local,x_end,y_end,coordinates[i],coordinates[i+1]);
					if(flag)
					{
						x_coordinates.push_back(coordinates[i]);
						y_coordinates.push_back(coordinates[i+1]);
						cout<<"已添加坐标 ("<<coordinates[i]<<","<<coordinates[i+1]<<")"<<endl;
					}
				}
			}
		}
		else
		{
			cout<<"(SumoNodeSensor-IsCoverThePath)x与y的坐标个数不相同"<<endl;
			getchar();
		}			
		
		//获取当前节点与另一节点之间的道路坐标
		for(uint32_t i = localIndex+1;i < remoteIndex;++i)
		{
			eit = edges.find(uriConvertToString(route.at(i)));
			shape = eit->second.lane.shape;
			cout<<"(SumoNodeSensor-IsCoverThePath) 道路为 "<<eit->second.lane.id<<" ,坐标为 "<<shape<<endl;
			coordinates.clear();
			coordinates = split(shape," ,");
			for(int i = 0;i < (signed)coordinates.size();i+=2)
			{
				x_coordinates.push_back(coordinates[i]);
				y_coordinates.push_back(coordinates[i+1]);
				cout<<"已添加坐标 ("<<coordinates[i]<<","<<coordinates[i+1]<<")"<<endl;
			}
		}
		
		//获取另一节点所在道路坐标
		eit = edges.find(uriConvertToString(route.at(remoteIndex)));
		shape = eit->second.lane.shape;
		cout<<"(SumoNodeSensor-IsCoverThePath) 另一节点所在道路为 "<<eit->second.lane.id<<" ,坐标为 "<<shape<<endl;
		coordinates.clear();
		coordinates = split(shape," ,");
		num = 0;
		cout<<"道路的坐标个数为 "<<coordinates.size()<<endl;
		
		if(coordinates.size()%2 == 0)
		{
			num = coordinates.size()/2;
		}
		else
		{
			cout<<"(SumoNodeSensor-IsCoverThePath) x与y的个数不相同"<<endl;
		}
		
		if(num)
		{
			if(num == 1)
			{
				cout<<"(SumoNodeSensor-IsCoverThePath) x与y的坐标个数只有一个"<<endl;
				getchar();
			}
			else if(num == 2)
			{
				x_coordinates.push_back(coordinates[0]);
				y_coordinates.push_back(coordinates[1]);
				cout<<"已添加坐标 ("<<coordinates[0]<<","<<coordinates[1]<<")"<<endl;
			}
			else
			{
				cout<<"(SumoNodeSensor-IsCoverThePath) x与y的个数大于2"<<endl;
				double x_begin = coordinates.front();
				double x_end = coordinates.back()-1;
				bool x_increase = false;
				if(x_begin < x_end)
				{
					x_increase = true;
				}
				
				double y_begin = coordinates.front()+1;
				double y_end = coordinates.back();
				bool y_increase = false;
				if(y_begin < y_end)
				{
					y_increase = true;
				}
				
				//寻找符合条件的道路坐标，即位于当前节点与道路末端之间的坐标
				for(int i = 0;i < num*2;i+=2)
				{
					bool flag = IsCorrectPosition(x_increase,y_increase,x_begin,y_begin,x,y,coordinates[i],coordinates[i+1]);
					if(flag)
					{
						x_coordinates.push_back(coordinates[i]);
						y_coordinates.push_back(coordinates[i+1]);
						cout<<"已添加坐标 ("<<coordinates[i]<<","<<coordinates[i+1]<<")"<<endl;
					}
				}
			}
		}
		
		//判断以当前节点为圆心，传输长度为半径的圆是否可以覆盖到当前节点-转发节点之间的所有路段
		bool iscover = false;
		if(x_coordinates.size() == y_coordinates.size())
		{
			Vector localPos = GetObject<MobilityModel>()->GetPosition();
			localPos.z = 0;
			
			for(int i = 0;i < (signed)x_coordinates.size();i++)
			{
				double tempx = x_coordinates[i];
				double tempy = y_coordinates[i];
				Vector pathPos(tempx,tempy,0);
				cout<<"当前计算的坐标为 x "<<tempx<<" y "<<tempy<<" 距离为 "<<CalculateDistance(localPos,pathPos)<<endl;
			
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

bool SumoNodeSensor::IsCorrectPosition(bool x_increase,bool y_increase,int x_begin, int y_begin, int x_end,int y_end, int x,int y)
{		
	bool flag_x = false;
	bool flag_y = false;
	
	if(x_increase)
	{
		if(x >= x_begin && x <= x_end)
		{
			flag_x = true;
		}
	}
	else
	{
		if(x <= x_begin && x >= x_end)
		{
			flag_x = true;
		}
	}
		
	if(y_increase)
	{
		if(y >= y_begin && y <= y_end)
		{
			flag_y = true;
		}
	}
	else
	{
		if(y <= y_begin && y >= y_end)
		{
			flag_y = true;
		}
	}
		
	if(flag_x && flag_y)
	{
		return true;
	}
	return false;
}


std::vector<double> SumoNodeSensor::split(const std::string & s, const std::string & d)
{
	std::vector<double> v;
	char *str = new char[s.size()+1];
	strcpy(str,s.c_str());
	while(char *t = strsep(&str,d.c_str()))
		v.push_back(atof(t));
	delete []str;
	return v;
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
