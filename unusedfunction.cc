//changed by sy 2017.9.6
std::vector<uint32_t> NavigationRouteHeuristic::GetPriorityList(const vector<string>& route)
{
	std::vector<uint32_t> PriorityList;
	std::ostringstream str;
	str<<"PriorityList is";
	cout<<"(forwarding.cc-GetPriorityList)节点 "<<m_node->GetId()<<" 的转发优先级列表为 "<<endl;

	// The default order of multimap is ascending order,
	// but I need a descending order
	std::multimap<double,uint32_t,std::greater<double> > sortlist;

	// step 1. 寻找位于导航路线前方的一跳邻居列表,m_nb为邻居列表
	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb;
	
	for(nb = m_nb.getNb().begin() ; nb != m_nb.getNb().end();++nb)
	{
		std::pair<bool, double> result=
				m_sensor->getDistanceWith(nb->second.m_x,nb->second.m_y,route);
		//cout<<"All "<<nb->first<<" ("<<result.first<<" "<<result.second<<") "<<endl;
		
		// Be careful with the order, increasing or descending?
		if(result.first && result.second > 0)
		{
			bool iscover = m_sensor->IsCoverThePath(nb->second.m_x,nb->second.m_y,route);
			cout<<"iscover "<<iscover<<endl;
			if(iscover)
			{
				sortlist.insert(std::pair<double,uint32_t>(result.second,nb->first));
				cout<<"Front "<<nb->first<<" ("<<result.first<<" "<<result.second<<") ";
			}
			else
			{
				cout<<"Disgard "<<nb->first<<" ("<<result.first<<" "<<result.second<<") ";
			}
			//getchar();
		}
	}
	//cout<<endl;
	//getchar();
	// step 2. Sort By Distance Descending
	std::multimap<double,uint32_t>::iterator it;
	for(it=sortlist.begin();it!=sortlist.end();++it)
	{
		PriorityList.push_back(it->second);

		str<<" "<<it->second;
		//cout<<" "<<it->second;
	}
	//cout<<endl;
	NS_LOG_DEBUG(str.str());
	//cout<<endl<<"(forwarding.cc-GetPriorityList) 邻居数目为 "<<m_nb.getNb().size()<<" At time "<<Simulator::Now().GetSeconds()<<endl;
	return PriorityList;
}

//added by sy 2017.9.6
bool SumoNodeSensor::IsCoverThePath(const double& x,const double& y,const std::vector<std::string>& route)
{
	//cout<<"进入(SumoNodeSensor.cc-IsCoverThePath)"<<endl;
	//当前节点所在路段和位置
	const string& localLane = getLane();
	const double& localPos = getPos();
	cout<<"(SuNodeSensor-IsCoverThePath) 当前节点所在路段 "<<localLane<<", 所在位置 "<<localPos<<endl;
	
	vector<string>::const_iterator localLaneIterator;
	vector<string>::const_iterator remoteLaneIterator;
	
	std::pair<std::string, double> remoteInfo = convertCoordinateToLanePos(x,y);
	
	//另一节点所在路段和位置
	const string& remoteLane = remoteInfo.first;
	const double& remotePos  = remoteInfo.second;
	cout<<"(SumoNodeSensor-IsCoverThePath) 另一节点所在路段 "<<remoteLane<<", 所在位置 "<<remotePos<<endl;
	
	//当前节点所在路段在导航路线中的下标
	localLaneIterator  = std::find (route.begin(), route.end(), localLane);
	//另一节点所在路段在导航路线中的下标
	remoteLaneIterator = std::find (route.begin(), route.end(), remoteLane);
	
	if(localLaneIterator == route.end() || remoteLaneIterator == route.end())
	{
		cout<<"另一节点所在路段不在当前节点导航路线中"<<endl;
		return false;
	}
	
	NS_ASSERT_MSG(remoteLaneIterator != route.end() && localLaneIterator != route.end(),"当前节点所在路段为 "<<localLane<<"另一节点所在路段为 "<< remoteLane);
	
	uint32_t localIndex = distance(route.begin(),localLaneIterator);
	uint32_t remoteIndex= distance(route.begin(),remoteLaneIterator);
	
	//在同一条路上且另一节点位于当前节点前方
	if(localIndex==remoteIndex)
	{
		if(remotePos-localPos <= TransRange && remotePos-localPos >=0)
		{
			cout<<"(SumoNodeSensor-IsCoverThePath) 两节点位于同一段路且另一节点在当前节点前方 ,距离 "<<remotePos-localPos<<endl;
			return true;
		}
		else if(remotePos-localPos > TransRange)
		{
			cout<<"(SumoNodeSensor-IsCoverThePath) 两节点位于同一段路且另一节点在当前节点前方，但超出了传输范围。距离 "<<remotePos-localPos<<endl;
			return false;
		}
		else if(remotePos-localPos < 0)
		{
			cout<<"(SumoNodeSensor-IsCoverThePath) 两节点位于同一段路且另一节点在当前节点后方"<<endl;
			return false;
		}
	}
	
	//位于不同路段
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
		
		NS_ASSERT_MSG(coordinates.size()%2 == 0,"x与y的坐标个数不相同");
		
		num = coordinates.size()/2;
		vector<double> x_coordinates;
		vector<double> y_coordinates;
		
		NS_ASSERT_MSG(num != 1,"只有一个坐标");
		
		if(num == 2)
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
		
		NS_ASSERT_MSG(coordinates.size()%2 == 0,"x与y的坐标个数不相同");
		num = coordinates.size()/2;
		NS_ASSERT_MSG(num != 1,"只有一个坐标");
		
		
		if(num == 2)
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
		
		//加入另一节点坐标
		x_coordinates.push_back(x);
		y_coordinates.push_back(y);
		cout<<"已添加坐标 ("<<x<<","<<y<<")"<<endl;
		
		//判断以当前节点为圆心，传输长度为半径的圆是否可以覆盖到当前节点-转发节点之间的所有路段
		bool iscover = false;
		NS_ASSERT_MSG(x_coordinates.size() == y_coordinates.size(),"x_coordinates.size() == y_coordinates.size()");
		
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
		return false;
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