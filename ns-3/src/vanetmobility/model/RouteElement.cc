/*
 * RouteElement.cc
 *
 *  Created on: Dec 31, 2014
 *      Author: chen
 */


#include "ns3/RouteElement.h"

namespace ns3
{
namespace vanetmobility
{
namespace sumomobility
{

using namespace std;

//将"/"转化为"-"
void StringReplace(std::string& base,const std::string& src,const std::string& dst)
{
    std::string::size_type pos = 0;
    std::string::size_type srcLen = src.size();
    std::string::size_type dstLen = dst.size();
    pos = base.find(src,pos);
	//If no matches were found, the function returns string::npos
    while((pos!=std::string::npos))
    {
        base.replace(pos,srcLen,dst);
        pos = base.find(src,(pos+dstLen));
    }
}

int getAttribuutID(const char* attribute)
{
	if(0==strcmp(attribute,"id"))      return ATTR_ID ;
	if(0==strcmp(attribute,"from"))    return ATTR_FROM;
	if(0==strcmp(attribute,"to"))      return ATTR_TO;
	if(0==strcmp(attribute,"priority"))return ATTR_PRIORITY;
	if(0==strcmp(attribute,"index"))   return ATTR_INDEX;
	if(0==strcmp(attribute,"speed"))   return ATTR_SPEED;
	if(0==strcmp(attribute,"length"))  return ATTR_LENGTH ;
	if(0==strcmp(attribute,"shape"))   return ATTR_SHAPE;
	if(0==strcmp(attribute,"depart"))  return ATTR_DEPART;
	if(0==strcmp(attribute,"edges"))   return ATTR_EDGES;
	if(0==strcmp(attribute,"time"))    return ATTR_TIME;
	if(0==strcmp(attribute,"x"))       return ATTR_X;
	if(0==strcmp(attribute,"y"))       return ATTR_Y;
	if(0==strcmp(attribute,"angle"))   return ATTR_ANGLE;
	if(0==strcmp(attribute,"type"))    return ATTR_TYPE;
	if(0==strcmp(attribute,"speed"))   return ATTR_SPEED;
	if(0==strcmp(attribute,"pos"))     return ATTR_POS;
	if(0==strcmp(attribute,"lane"))    return ATTR_LANE;
	if(0==strcmp(attribute,"slope"))   return ATTR_SLOPE;
	return 0;
}

RoadMap::RoadMap()
{
	// TODO Auto-generated constructor stub

}

RoadMap::~RoadMap()
{
	// TODO Auto-generated destructor stub
}

RoadMap::RoadMap(const RoadMap& r):edges(r.edges)
{
	
}

//读入的文件为input_net.net.xml
void RoadMap::LoadNetXMLFile(const char* pFilename)
{
	TiXmlDocument doc(pFilename);
	bool loadOkay = doc.LoadFile();
	Edge edge;
	if (loadOkay)
	{
	//	printf("\n%s:\n", pFilename);
		InitializeEdges( &doc ); // defined later in the tutorial
	}
	else
	{
		printf("Failed to load file \"%s\"\n", pFilename);
	}
}

void RoadMap::printedges()
{
	map<string,Edge>::iterator edge;
	for (edge=edges.begin();edge!=edges.end();edge++)
	{
		cout<<(*edge).second.id<<"  "<<(*edge).second.from<<"  "<<(*edge).second.to<<"  "<<(*edge).second.priority<<endl;
		cout<<"    "<<(*edge).second.lane.id<<"  "<<(*edge).second.lane.index<<"  "<<(*edge).second.lane.speed<<"  "<<(*edge).second.lane.length<<"  "<<(*edge).second.lane.shape<<endl;

	}
	cout<<edges.size()<<endl;
}

const map<string,Edge>& RoadMap::getEdges()const
{
	return edges;
}

bool RoadMap::edge_with_attribs(TiXmlElement* pElement,const char* str)//���pElement��ǩ��û��str����
{
	if ( !pElement ) return 0;
	TiXmlAttribute* pAttrib=pElement->FirstAttribute();
	while (pAttrib)
	{
		if (0==strcmp(str,pAttrib->Name()))
			return true;
		pAttrib=pAttrib->Next();
	}
	return false;
}

//读入的文件为input_net.net.xml
int RoadMap::Read_edges(TiXmlElement* pElement)
{

	if ( !pElement ) return 0;
	TiXmlAttribute* pAttrib=pElement->FirstAttribute();
	int i=0;
	int attributeID;
	while (pAttrib)
	{
		attributeID=getAttribuutID(pAttrib->Name());
		switch(attributeID)
		{
		case ATTR_ID      :m_temp_edge.id      =pAttrib->Value();break;
		case ATTR_FROM    :m_temp_edge.from    =pAttrib->Value();break;
		case ATTR_TO      :m_temp_edge.to      =pAttrib->Value();break;
		case ATTR_PRIORITY:m_temp_edge.priority=atof(pAttrib->Value());break;//atof:字符串转化为浮点数
		default:break;
		}
		i++;
		pAttrib=pAttrib->Next();
	}
    ChangeLaneCharactor(m_temp_edge);
	return i;
}

//将"/"更改为"-"
void RoadMap::ChangeLaneCharactor(Edge& edge)
{
   StringReplace(edge.id,originLanCharactor,changeLaneCharactor);
   StringReplace(edge.from,originLanCharactor,changeLaneCharactor);
   StringReplace(edge.to,originLanCharactor,changeLaneCharactor);
   return;
}

//将"/"更改为"-"
void RoadMap::ChangeLaneCharactor(Lane& lane)
{
    StringReplace(lane.id,originLanCharactor,changeLaneCharactor);
    return;
}

//读入的文件为input_net.net.xml
int RoadMap::Read_lane(TiXmlElement* pElement)
{
	if ( !pElement ) return 0;
	TiXmlAttribute* pAttrib=pElement->FirstAttribute();
	int i=0;
	int attributeID;
	while (pAttrib)
	{
		attributeID=getAttribuutID(pAttrib->Name());
		switch(attributeID)
		{
		case ATTR_ID    :
			{
				m_temp_edge.lane.id      =pAttrib->Value();
				//lane的id去掉的最后两个字符
				m_temp_edge.lane.id.erase(m_temp_edge.lane.id.end()-2,m_temp_edge.lane.id.end());
				break;
			}

		case ATTR_INDEX :m_temp_edge.lane.index   =atoi(pAttrib->Value());break;
		case ATTR_SPEED :m_temp_edge.lane.speed   =atof(pAttrib->Value());break;
		case ATTR_LENGTH:m_temp_edge.lane.length  =atof(pAttrib->Value());break;
		case ATTR_SHAPE :
		{
			m_temp_edge.lane.shape   =pAttrib->Value();
			break;
		}
		
		default:break;
		}
		i++;
		pAttrib=pAttrib->Next();
	}
    ChangeLaneCharactor(m_temp_edge.lane);
	return i;
}

//对应的文件为input_net.net.xml
void RoadMap::InitializeEdges( TiXmlNode* pParent)
{
	if ( !pParent ) return;

	TiXmlNode* pChild;
	int t = pParent->Type();
	//int num;
	bool getin = true;

	switch ( t )
	{
	case TiXmlNode::TINYXML_ELEMENT:
		{
			const char *element = pParent->Value();
			int elementID=0;
			if (0==strcmp(element,"edge"))elementID=1;
			if (0==strcmp(element,"lane"))elementID=2;
			switch(elementID)
			{
			case 1:
				{
					if (edge_with_attribs(pParent->ToElement(),"function"))//��Ҫfunction��ǩ��edge
					{
						getin=false;
						break;
					}
					Read_edges(pParent->ToElement());
					break;
				}//edge
			case 2:
				{
					Read_lane(pParent->ToElement());
					//map insert 重复插入，第二次插入的无效
					//导致多车道的道路只能插入第一个车道的信息
					edges.insert(map<string,Edge>::value_type(m_temp_edge.lane.id,m_temp_edge));
					//edges.push_back(m_temp_edge);
					break;
				}//lane
			default:break;
			}
			break;
		}
	default:break;
	}

	for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling())
	{
		if(getin)
			InitializeEdges(pChild);
	}
}

Route::Route()
{
	// TODO Auto-generated constructor stub

}

Route::~Route()
{
	// TODO Auto-generated destructor stub
}

Route::Route(const vector<string>& e)
{
		this->edgesID=e;
}

Route::Route(const Route& r)
{
	this->edgesID=r.edgesID;
}

void Route::LoadRouteString(const char* pRouteStr)
{
		string line(pRouteStr);
        StringReplace(line,originLanCharactor,changeLaneCharactor);
		string split(" ");
		size_t found;
		size_t last_pos = 0;

		found = line.find(split);
		while (found != string::npos) 
		{
			string s = line.substr(last_pos, found - last_pos);
			last_pos = found + split.length();
			found = line.find(split, last_pos);

			if (s.compare("") == 0) continue;
		    //	s+="_0";
			edgesID.push_back(s);
		}

		if (line.substr(last_pos).length() > 0)
		{
			string add=line.substr(last_pos);
		    //	add+="_0";
			edgesID.push_back(add);
		}

}

void Route::printroute()
{
		vector<string>::iterator i;
		for (i=edgesID.begin();i!=edgesID.end();i++)
		{
			cout<<(*i)<<" ";
		}
		cout<<endl;
}

VehicleLoader::VehicleLoader()
{
	// TODO Auto-generated constructor stub
	numofvehicles = 0;
}

VehicleLoader::~VehicleLoader()
{
	// TODO Auto-generated destructor stub
}

VehicleLoader::VehicleLoader(const VehicleLoader& v)
{
	vehicles=v.vehicles;
	numofvehicles = v.numofvehicles;
	m_temp_vehicle=NULL;
}

//对应的文件为routes.rou.xml
void VehicleLoader::LoadRouteXML(const char *  pXMLFilename)
{
	TiXmlDocument doc(pXMLFilename);
	bool loadOkay = doc.LoadFile();
	if (loadOkay)
	{
	//	printf("\n%s:\n", pXMLFilename);
		initialize_vehicles( &doc ); // defined later in the tutorial
		ReadMapIntoVector();
	}
	else
	{
		printf("Failed to load file \"%s\"\n", pXMLFilename);
	}
}

//对应的文件为fcdoutput.xml
void VehicleLoader::LoadFCDOutputXML(const char *  pXMLFilename)
{
	cout<<"进入(RouteElement.cc-LoadFCDOutputXML)"<<endl;
	TiXmlDocument doc(pXMLFilename);
	bool loadOkay = doc.LoadFile();
	if (loadOkay)
	{
//		printf("\n%s:\n", pXMLFilename);
		initialize_trace(&doc);
	}
	else
	{
		printf("Failed to load file \"%s\"\n", pXMLFilename);
	}
}

void VehicleLoader::print_vehicle()
{
	vector<Vehicle>::iterator v;
	for (v=vehicles.begin();v!=vehicles.end();v++)
	{
		cout<<(*v).id<<"  "<<(*v).depart<<endl;
		//(*v).route.printroute();
		vector<Trace>::iterator t;
		for (t=(*v).trace.begin();t!=(*v).trace.end();t++)
		{
			cout<<"trace:  "<<(*t).time<<"  "<<(*t).x<<"  "<<(*t).y<<"  "<<(*t).angle<<(*t).type
				<<(*t).speed<<(*t).pos<<"  "<<(*t).lane<<"  "<<"  "<<(*t).slope<<"  "<<"  "<<endl;
		}
	}
}

const vector<Vehicle>& VehicleLoader::getVehicles()const
{
	return vehicles;
}

//对应的文件为routes.rou.xml
void VehicleLoader::initialize_vehicles( TiXmlNode* pParent)
{
	if (!pParent) 
		return;
	TiXmlNode* pChild;
	int t = pParent->Type();
	//int num;
	switch (t)
	{
	case TiXmlNode::TINYXML_ELEMENT:
		{
			const char *element = pParent->Value();
			int elementID=0;
			if (0==strcmp(element,"vehicle"))elementID=1;
			if (0==strcmp(element,"route"))elementID=2;
			switch(elementID)
			{
			case 1:
				{
					m_temp_vehicle = new Vehicle();
					read_vehicle(pParent->ToElement());
					break;
				}//vehicle
			case 2:
				{
					read_vehicle(pParent->ToElement());
				//	vehicles.push_back(*m_temp_vehicle);
					mapvehicles[m_temp_vehicle->id]=*m_temp_vehicle;
					delete m_temp_vehicle;
					break;
				}//route
			default:break;
			}
			break;
		}
	default:break;
	}

	for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling())
		initialize_vehicles(pChild);

}

//对应的文件为routes.rou.xml 
int VehicleLoader::read_vehicle(TiXmlElement* pElement)
{
	if ( !pElement ) return 0;
	TiXmlAttribute* pAttrib=pElement->FirstAttribute();
	int i=0;
	int attributeID;
	while (pAttrib)
	{
		attributeID=getAttribuutID(pAttrib->Name());
		switch(attributeID)
		{
		case ATTR_ID    :m_temp_vehicle->id      =atoi(pAttrib->Value());break;
		case ATTR_DEPART:m_temp_vehicle->depart  =atof(pAttrib->Value());break;
		case ATTR_EDGES :m_temp_vehicle->route.LoadRouteString(pAttrib->Value());break;
		default:break;
		}
		i++;
		pAttrib=pAttrib->Next();
	}
	return i;
}

//对应的文件为fcdoutput.xml
void VehicleLoader::initialize_trace( TiXmlNode* pParent)
{
	if (!pParent) 
		return;
	cout<<"进入(RouteElement.cc-initialize_trace)"<<endl;
	TiXmlNode* pChild;
	int t = pParent->Type();
	int vid;
	switch (t)
	{
	case TiXmlNode::TINYXML_ELEMENT:
		{
			const char *element = pParent->Value();
			int elementID = 0;
			if (0 == strcmp(element,"timestep"))
				elementID = 1;
			if (0 == strcmp(element,"vehicle"))
				elementID = 2;
			switch(elementID)
			{
			case 1:
				{
					read_trace(pParent->ToElement());
					cout<<"(RouteElement.cc-Initialize_trace) timestep"<<endl;
					break;
				}//timestep
			case 2:
				{
					vid = read_trace(pParent->ToElement());
					cout<<"(RouteElement.cc-initialize_trace) vid "<<vid<<endl;
					/*cout<<m_temp_trace.time<<endl;
					cout<<m_temp_trace.x<<endl;
					cout<<m_temp_trace.y<<endl;
					cout<<m_temp_trace.angle<<endl;
					cout<<m_temp_trace.type<<endl;
					cout<<m_temp_trace.speed<<endl;
					cout<<m_temp_trace.pos<<endl;
					cout<<m_temp_trace.lane<<endl;
					cout<<m_temp_trace.slope<<endl;*/
					//cout<<vehicles[101].id<<" "<<vehicles[101].depart<<endl;
					/*2017.12.15 判断该车辆是否为RSU*/
					if(vid >= FIRST_RSU_ID)
					{
						/*确认RSU对应的下标*/
						vid = vid - FIRST_RSU_ID + numofvehicles;
					}
					vehicles[vid].trace.push_back(m_temp_trace);
					cout<<"(RouteElement.cc-initialize_trace) already push_back "<<vid<<endl;
					break;
				}//vehicle
			default:break;
			}
			break;
		}
	default:break;
	}
	for (pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling())
		initialize_trace(pChild);
}

int VehicleLoader::read_trace(TiXmlElement* pElement)//Return vehicle ID value
{
	
	if (!pElement) 
		return 0;
	//cout<<"进入(RouteElement.cc-read_trace)"<<endl;
	TiXmlAttribute* pAttrib=pElement->FirstAttribute();
	int i = 0;
	int attributeID;
	int vid = -1;
	while (pAttrib)
	{
		attributeID=getAttribuutID(pAttrib->Name());
		//cout<<"(RouteElement.cc-read_trace) attributeID "<<attributeID<<endl;
		switch(attributeID)
		{
		case ATTR_ID    :vid               =atoi(pAttrib->Value());break;
		case ATTR_TIME  :m_temp_trace.time =atof(pAttrib->Value());break;
		case ATTR_X     :m_temp_trace.x    =atof(pAttrib->Value());break;
		case ATTR_Y     :m_temp_trace.y    =atof(pAttrib->Value());break;
		case ATTR_ANGLE :m_temp_trace.angle=atof(pAttrib->Value());break;
		case ATTR_SPEED :m_temp_trace.speed=atof(pAttrib->Value());break;
		case ATTR_POS   :m_temp_trace.pos  =atof(pAttrib->Value());break;
		case ATTR_SLOPE :m_temp_trace.slope=atof(pAttrib->Value());break;
		case ATTR_LANE  :
			{
				m_temp_trace.lane = pAttrib->Value();
				//删除lane的最后两位：_x
				m_temp_trace.lane.erase(m_temp_trace.lane.end()-2,m_temp_trace.lane.end());
				//将"/"替换为"-"
                StringReplace(m_temp_trace.lane,originLanCharactor,changeLaneCharactor);
				//cout<<m_temp_trace.lane<<endl;
				break;
			}
		case ATTR_TYPE  :m_temp_trace.type =     pAttrib->Value();break;
		default:break;
		}
		i++;
		pAttrib=pAttrib->Next();
	}
	return vid;
}

void VehicleLoader::Clear()
{
	vehicles.clear();
	mapvehicles.clear();
}

void VehicleLoader::ReadMapIntoVector()
{
	map<int,Vehicle>::iterator it;
	for(it = mapvehicles.begin();it!= mapvehicles.end();it++)
	{
		vehicles.push_back(it->second);
		cout<<"(RouteElement.cc-ReadMapIntoVector)"<<it->second.id<<" "<<it->second.depart<<endl;
		/*
		 * added by sy:
		 * 记录普通车辆的数目 
		 * RSU的编号从10000开始
		 */
		if(it->second.id < FIRST_RSU_ID)
			numofvehicles+=1;
	}
	cout<<"(RouteElement.cc-ReadMapIntoVector) 普通车辆的数目为 "<<numofvehicles<<endl;
}

int VehicleLoader::getNumOfVehicles() const
{
	return numofvehicles;
}

} /* namespace sumomobility */
} /* namespace vanetmobility */
} /* namespace ns3 */
