/*
 * main.cpp
 *
 *  Created on: 2018年2月22日
 *      Author: wangsy
 */
#include "tinyxml/tinyxml.h"
#include <iostream>
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <vector>
#include <fstream>

using namespace std;
#define ATTR_ID        1
#define ATTR_FROM      2
#define ATTR_TO        3
#define ATTR_PRIORITY  4
#define ATTR_INDEX     5
#define ATTR_SPEED     6
#define ATTR_LENGTH    7
#define ATTR_SHAPE     8
#define ATTR_DEPART    9
#define ATTR_EDGES     10
#define ATTR_TIME      11
#define ATTR_X         12
#define ATTR_Y         13
#define ATTR_ANGLE     14
#define ATTR_TYPE      15
#define ATTR_POS       16
#define ATTR_LANE      17
#define ATTR_SLOPE     18

const std::string originLanCharactor("/");
const std::string changeLaneCharactor("-");

void StringReplace(std::string&, const std::string&, const std::string&);

int getAttribuutID(const char* attribute);

struct Lane {
	std::string id;
	int index;
	double speed;
	double length;
	std::string shape;
};

struct Edge {
	std::string id;
	std::string from;
	std::string to;
	double priority;
	Lane lane;
};

class RoadMap {
public:
	RoadMap();
	RoadMap(const RoadMap& r);
	virtual ~RoadMap();
	void Clear() {
		edges.clear();
	}
	;
	void LoadNetXMLFile(const char* pFilename);
	void printedges();
	void getdata();
	const std::map<std::string, Edge>& getEdges() const; //warning: the key is lane's id, not edges

private:
	std::map<std::string, Edge> edges;
	Edge m_temp_edge;
	bool edge_with_attribs(TiXmlElement* pElement, const char* str); //check whether "pElement" has "str" attribute
	int Read_edges(TiXmlElement* pElement);
	int Read_lane(TiXmlElement* pElement);
	void InitializeEdges(TiXmlNode* pParent);
	void ChangeLaneCharactor(Edge& edge);
	void ChangeLaneCharactor(Lane& lane);
};

//将"/"转化为"-"
void StringReplace(std::string& base, const std::string& src,
		const std::string& dst) {
	std::string::size_type pos = 0;
	std::string::size_type srcLen = src.size();
	std::string::size_type dstLen = dst.size();
	pos = base.find(src, pos);
	//If no matches were found, the function returns string::npos
	while ((pos != std::string::npos)) {
		base.replace(pos, srcLen, dst);
		pos = base.find(src, (pos + dstLen));
	}
}

int getAttribuutID(const char* attribute) {
	if (0 == strcmp(attribute, "id"))
		return ATTR_ID;
	if (0 == strcmp(attribute, "from"))
		return ATTR_FROM;
	if (0 == strcmp(attribute, "to"))
		return ATTR_TO;
	if (0 == strcmp(attribute, "priority"))
		return ATTR_PRIORITY;
	if (0 == strcmp(attribute, "index"))
		return ATTR_INDEX;
	if (0 == strcmp(attribute, "speed"))
		return ATTR_SPEED;
	if (0 == strcmp(attribute, "length"))
		return ATTR_LENGTH;
	if (0 == strcmp(attribute, "shape"))
		return ATTR_SHAPE;
	if (0 == strcmp(attribute, "depart"))
		return ATTR_DEPART;
	if (0 == strcmp(attribute, "edges"))
		return ATTR_EDGES;
	if (0 == strcmp(attribute, "time"))
		return ATTR_TIME;
	if (0 == strcmp(attribute, "x"))
		return ATTR_X;
	if (0 == strcmp(attribute, "y"))
		return ATTR_Y;
	if (0 == strcmp(attribute, "angle"))
		return ATTR_ANGLE;
	if (0 == strcmp(attribute, "type"))
		return ATTR_TYPE;
	if (0 == strcmp(attribute, "speed"))
		return ATTR_SPEED;
	if (0 == strcmp(attribute, "pos"))
		return ATTR_POS;
	if (0 == strcmp(attribute, "lane"))
		return ATTR_LANE;
	if (0 == strcmp(attribute, "slope"))
		return ATTR_SLOPE;
	return 0;
}

RoadMap::RoadMap() {
	// TODO Auto-generated constructor stub

}

RoadMap::~RoadMap() {
	// TODO Auto-generated destructor stub
}

RoadMap::RoadMap(const RoadMap& r) :
		edges(r.edges) {

}

//读入的文件为input_net.net.xml
void RoadMap::LoadNetXMLFile(const char* pFilename) {
	TiXmlDocument doc(pFilename);
	bool loadOkay = doc.LoadFile();
	Edge edge;
	if (loadOkay) {
		//	printf("\n%s:\n", pFilename);
		InitializeEdges(&doc); // defined later in the tutorial
		printedges();
		getdata();
	} else {
		printf("Failed to load file \"%s\"\n", pFilename);
	}
}

void RoadMap::printedges() {
	//map<string, Edge>::iterator edge;
	/*for (edge = edges.begin(); edge != edges.end(); edge++) {
	 cout << (*edge).second.id << "  " << (*edge).second.from << "  "
	 << (*edge).second.to << "  " << (*edge).second.priority << endl;
	 cout << "    " << (*edge).second.lane.id << "  "
	 << (*edge).second.lane.index << "  "
	 << (*edge).second.lane.speed << "  "
	 << (*edge).second.lane.length << "  "
	 << (*edge).second.lane.shape << endl;
	 }*/
	cout << "The Size of Edges " << edges.size() << endl;
}

void RoadMap::getdata() {
	map<string, Edge>::iterator edge;
	set<string> junctions;
	set<string>::iterator it;
	for (edge = edges.begin(); edge != edges.end(); edge++) {
		string from = (*edge).second.from;
		string to = (*edge).second.to;
		junctions.insert(from);
		junctions.insert(to);
	}

	//存放顶点与下标的对应关系
	map<string, int> m;
	int num = 0;
	ofstream in;
	in.open("junctions.txt", ios::trunc);
	in << junctions.size() << endl;
	for (it = junctions.begin(); it != junctions.end(); it++) {
		m[*it] = num;
		in << *it << " " << num << endl;
		cout << *it << " " << num << endl;
		num++;
	}
	in.close();

	cout << "The size of Junctions " << num << endl;

	in.open("data.txt", ios::trunc);
	in << num << endl << edges.size() << endl;
	for (edge = edges.begin(); edge != edges.end(); edge++) {
		string from = (*edge).second.from;
		string to = (*edge).second.to;
		int cost = (*edge).second.lane.length;
		string id = (*edge).second.id;
		in << m[from] << " " << m[to] << " " << cost << " " << id << endl;
	}
	in.close();
}

const map<string, Edge>& RoadMap::getEdges() const {
	return edges;
}

bool RoadMap::edge_with_attribs(TiXmlElement* pElement, const char* str) //���pElement��ǩ��û��str����
		{
	if (!pElement)
		return 0;
	TiXmlAttribute* pAttrib = pElement->FirstAttribute();
	while (pAttrib) {
		if (0 == strcmp(str, pAttrib->Name()))
			return true;
		pAttrib = pAttrib->Next();
	}
	return false;
}

//读入的文件为input_net.net.xml
int RoadMap::Read_edges(TiXmlElement* pElement) {

	if (!pElement)
		return 0;
	TiXmlAttribute* pAttrib = pElement->FirstAttribute();
	int i = 0;
	int attributeID;
	while (pAttrib) {
		attributeID = getAttribuutID(pAttrib->Name());
		switch (attributeID) {
		case ATTR_ID:
			m_temp_edge.id = pAttrib->Value();
			break;
		case ATTR_FROM:
			m_temp_edge.from = pAttrib->Value();
			break;
		case ATTR_TO:
			m_temp_edge.to = pAttrib->Value();
			break;
		case ATTR_PRIORITY:
			m_temp_edge.priority = atof(pAttrib->Value());
			break; //atof:字符串转化为浮点数
		default:
			break;
		}
		i++;
		pAttrib = pAttrib->Next();
	}
	ChangeLaneCharactor(m_temp_edge);
	return i;
}

//将"/"更改为"-"
void RoadMap::ChangeLaneCharactor(Edge& edge) {
	StringReplace(edge.id, originLanCharactor, changeLaneCharactor);
	StringReplace(edge.from, originLanCharactor, changeLaneCharactor);
	StringReplace(edge.to, originLanCharactor, changeLaneCharactor);
	return;
}

//将"/"更改为"-"
void RoadMap::ChangeLaneCharactor(Lane& lane) {
	StringReplace(lane.id, originLanCharactor, changeLaneCharactor);
	return;
}

//读入的文件为input_net.net.xml
int RoadMap::Read_lane(TiXmlElement* pElement) {
	if (!pElement)
		return 0;
	TiXmlAttribute* pAttrib = pElement->FirstAttribute();
	int i = 0;
	int attributeID;
	while (pAttrib) {
		attributeID = getAttribuutID(pAttrib->Name());
		switch (attributeID) {
		case ATTR_ID: {
			m_temp_edge.lane.id = pAttrib->Value();
			//lane的id去掉的最后两个字符
			m_temp_edge.lane.id.erase(m_temp_edge.lane.id.end() - 2,
					m_temp_edge.lane.id.end());
			break;
		}

		case ATTR_INDEX:
			m_temp_edge.lane.index = atoi(pAttrib->Value());
			break;
		case ATTR_SPEED:
			m_temp_edge.lane.speed = atof(pAttrib->Value());
			break;
		case ATTR_LENGTH:
			m_temp_edge.lane.length = atof(pAttrib->Value());
			break;
		case ATTR_SHAPE: {
			m_temp_edge.lane.shape = pAttrib->Value();
			break;
		}

		default:
			break;
		}
		i++;
		pAttrib = pAttrib->Next();
	}
	ChangeLaneCharactor(m_temp_edge.lane);
	return i;
}

//对应的文件为input_net.net.xml
void RoadMap::InitializeEdges(TiXmlNode* pParent) {
	if (!pParent)
		return;

	TiXmlNode* pChild;
	int t = pParent->Type();
	//int num;
	bool getin = true;

	switch (t) {
	case TiXmlNode::TINYXML_ELEMENT: {
		const char *element = pParent->Value();
		int elementID = 0;
		if (0 == strcmp(element, "edge"))
			elementID = 1;
		if (0 == strcmp(element, "lane"))
			elementID = 2;
		switch (elementID) {
		case 1: {
			if (edge_with_attribs(pParent->ToElement(), "function")) //��Ҫfunction��ǩ��edge
					{
				getin = false;
				break;
			}
			Read_edges(pParent->ToElement());
			break;
		} //edge
		case 2: {
			Read_lane(pParent->ToElement());
			//map insert 重复插入，第二次插入的无效
			//导致多车道的道路只能插入第一个车道的信息
			edges.insert(
					map<string, Edge>::value_type(m_temp_edge.lane.id,
							m_temp_edge));
			//edges.push_back(m_temp_edge);
			break;
		}					//lane
		default:
			break;
		}
		break;
	}
	default:
		break;
	}

	for (pChild = pParent->FirstChild(); pChild != 0;
			pChild = pChild->NextSibling()) {
		if (getin)
			InitializeEdges(pChild);
	}
}

int main() {
	RoadMap roadmap;
	string netxmlpath = "E:/ndn_input_data/input_daxuecheng_800/input_net.net.xml";
	roadmap.LoadNetXMLFile(netxmlpath.data());
	return 0;
}
