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

void NavigationRouteHeuristic::OnInterest(Ptr<Face> face,
		Ptr<Interest> interest)
{
	//NS_LOG_UNCOND("Here is NavigationRouteHeuristic dealing with OnInterest");
	//NS_LOG_FUNCTION (this);
	if(!m_running) return;
	//getchar();
	//cout<<endl<<"进入(forwarding.cc-OnInterest)"<<endl;
	
	if(Face::APPLICATION==face->GetFlags())
	{
		//consumer产生兴趣包，在路由层进行转发
		cout << "(forwarding.cc-OnInterest)该兴趣包来自应用层。当前节点为 "<<m_node->GetId() <<endl;
		NS_LOG_DEBUG("Get interest packet from APPLICATION");
		// This is the source interest from the upper node application (eg, nrConsumer) of itself
		// 1.Set the payload
		interest->SetPayload(GetNrPayload(HeaderHelper::INTEREST_NDNSIM,interest->GetPayload(),999999999));
        
        //added by sy
        ndn::nrndn::nrHeader nrheader;
        interest->GetPayload()->PeekHeader(nrheader);
        uint32_t nodeId = nrheader.getSourceId();
		
		// 2017.12.23 added by sy
		const std::vector<uint32_t>& pri=nrheader.getPriorityList();
		if(pri.empty())
		{
			cout<<"(forwarding.cc-OnInterest) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<m_node->GetId()<<"准备缓存自身的兴趣包 "<<interest->GetNonce()<<endl;
			//getchar();
			Simulator::Schedule(MilliSeconds(m_uniformRandomVariable->GetInteger(0,100)),&NavigationRouteHeuristic::CachingInterestPacket,this,interest->GetNonce(),interest);
			return;
		}
		
		//2017.12.13 输出兴趣包实际转发路线
		//std::string routes = interest->GetRoutes();
		//std::cout<<"(forwarding.cc-OnInterest) routes "<<routes<<std::endl;
		//getchar();
	 
		
		//cout<<"(forwarding.cc-OnInterest) 兴趣包序列号为 "<<interest->GetNonce()<<endl;
		//getchar();

		// 2. record the Interest Packet
		m_interestNonceSeen.Put(interest->GetNonce(),true);
		//cout<<"(forwarding.cc-OnInterest) 记录兴趣包 nonce "<<interest->GetNonce()<<" from NodeId "<<nodeId<<endl;

		// 3. Then forward the interest packet directly
		Simulator::Schedule(MilliSeconds(m_uniformRandomVariable->GetInteger(0,100)),
				&NavigationRouteHeuristic::SendInterestPacket,this,interest);
		
		
	    cout<<"(forwarding.cc-OnInterest)来自应用层的兴趣包处理完毕。源节点 "<<nodeId<<endl;
		//getchar();
		
		//判断RSU是否发送兴趣包
		const std::string& currentType = m_sensor->getType();
		if(currentType == "RSU")
		{
			cout<<"(forwarding.cc-OnInterest) RSU不该发送兴趣包。出错！！"<<endl;
			//getchar();
		}
		return;
	}
	
	if(HELLO_MESSAGE==interest->GetScope())
	{		
		//cout << "(forwarding.cc-OnInterest) 心跳包" <<endl;
		if(m_sensor->getType() == "RSU")
		{
			ProcessHelloRSU(interest);
		}
		else
		{
			ProcessHello(interest);
		}
		return;
	}
	
	Ptr<const Packet> nrPayload	= interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	//获取发送兴趣包节点的ID
	uint32_t nodeId = nrheader.getSourceId();
	//获取兴趣的随机编码
	uint32_t seq = interest->GetNonce();
	//获取当前节点Id
	uint32_t myNodeId = m_node->GetId();
	//获取兴趣包的转发节点id
	uint32_t forwardId = nrheader.getForwardId();
	
	cout<<endl<<"(forwarding.cc-OnInterest)At Time "<<Simulator::Now().GetSeconds()<<" 当前车辆Id为 "<<myNodeId<<",源节点 "<<nodeId<<",转发节点 "<<forwardId<<endl;
	
	//std::string routes = interest->GetRoutes();
	//std::cout<<"(forwarding.cc-OnInterest) routes "<<routes<<std::endl;
	//std::cout<<"(forwarding.cc-OnInterest) seq "<<seq<<std::endl;
	
	//getchar();
		
	if(nodeId == myNodeId)
	{
		ForwardNodeList.insert(forwardId);
		cout<<"(forwarding.cc-OnInterest) 源节点 "<<nodeId <<" 收到了自己发送的兴趣包,转发节点 "<<forwardId<<endl;
		//getchar();
	} 

	//If the interest packet has already been sent, do not proceed the packet
	if(m_interestNonceSeen.Get(interest->GetNonce()))
	{
		cout<<"(forwarding.cc-OnInterest) 源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",该兴趣包已经被发送, nonce为 "<<interest->GetNonce()<<endl;
		NS_LOG_DEBUG("The interest packet has already been sent, do not proceed the packet of "<<interest->GetNonce());
		return;
	}
	
	//获取优先列表
	cout << "(forwarding.cc-OnInterest) 兴趣包的转发优先级列表为: ";
	const std::vector<uint32_t>& pri=nrheader.getPriorityList();
    for(auto it = pri.begin();it != pri.end();it++)
	{
		cout<<*it<<" ";
	}
	cout<<endl;
	//getchar();

	//Deal with the stop message first
	//避免回环
	if(Interest::NACK_LOOP==interest->GetNack())
	{
		cout<<"(forwarding.cc-OnInterest) 该兴趣包为NACK_LOOP。源节点 "<<nodeId<<endl;
		ExpireInterestPacketTimer(nodeId,seq);
		return;
	}

	//If it is not a stop message, prepare to forward:
	pair<bool, double> msgdirection = packetFromDirection(interest);
	if(!msgdirection.first || // from other direction
			msgdirection.second > 0)// or from front
	{
		NS_LOG_DEBUG("Get interest packet from front or other direction");
		if(!isDuplicatedInterest(nodeId,seq))// Is new packet
		{
			//wsy:对于从前方收到的兴趣包，若是第一次收到的，直接丢弃即可
			NS_LOG_DEBUG("Get interest packet from front or other direction and it is new packet");
			cout<<"(forwarding.cc-OnInterest) 该兴趣包从前方或其他路线得到，且该兴趣包是新的。源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",转发节点 "<<forwardId<<endl<<endl;
			//getchar();
			DropInterestePacket(interest);
		}
		else // Is old packet
		{
			//wsy:对于从前方收到的兴趣包，若之前已经收到过，则没有必要再转发该兴趣包
			NS_LOG_DEBUG("Get interest packet from front or other direction and it is old packet");
			cout<<"(forwarding.cc-OnInterest) 该兴趣包从前方或其他路线得到，且该兴趣包是旧的。源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",转发节点 "<<forwardId<<endl<<endl;
			//getchar();
			ExpireInterestPacketTimer(nodeId,seq);
		}
	}
	else// it is from nodes behind
	{
		NS_LOG_DEBUG("Get interest packet from nodes behind");
		cout<<"(forwarding.cc-OnInterest) 该兴趣包从后方得到。源节点 "<<nodeId<<",当前节点 "<<myNodeId<<",转发节点 "<<forwardId<<endl;
		//getchar();
		const vector<string> remoteRoute=
							ExtractRouteFromName(interest->GetName());

		//changed by sy:这里需要判断当前节点为RSU还是普通车辆
		// Update the PIT here
		const std::string& currentType = m_sensor->getType();
		
		if(currentType == "DEFAULT_VEHTYPE")
		{
			cout<<"(forwarding.cc-OnInterest) 当前节点 "<<myNodeId<<" 的PIT为："<<endl;
		    m_nrpit->UpdateCarPit(remoteRoute, nodeId);
		}
		else if(currentType == "RSU")
		{
			cout<<"(forwarding.cc-OnInterest) At Time "<<Simulator::Now().GetSeconds()<<" 当前RSU "<<myNodeId<<" 的PIT为："<<endl;
			m_nrpit->UpdateRSUPit(remoteRoute,nodeId);
		}
		
		// Update finish

		//evaluate whether receiver's id is in sender's priority list
		bool idIsInPriorityList;
		vector<uint32_t>::const_iterator idit;
		idit = find(pri.begin(), pri.end(), m_node->GetId());
		idIsInPriorityList = (idit != pri.end());

		//evaluate end

		if (idIsInPriorityList)
		{
			cout<<"(forwarding.cc-OnInterest) Node id is in PriorityList"<<endl;
			NS_LOG_DEBUG("Node id is in PriorityList");

			bool IsPitCoverTheRestOfRoute=PitCoverTheRestOfRoute(remoteRoute);

			NS_LOG_DEBUG("IsPitCoverTheRestOfRoute?"<<IsPitCoverTheRestOfRoute);
			if(NoFwStop)
				IsPitCoverTheRestOfRoute = false;

			if (IsPitCoverTheRestOfRoute)
			{
				BroadcastStopMessage(interest);
				return;
			}
			else
			{
				//Start a timer and wait
				double index = distance(pri.begin(), idit);
				double random = m_uniformRandomVariable->GetInteger(0, 20);
				Time sendInterval(MilliSeconds(random) + index * m_timeSlot);
				//构造转发优先级列表，并判断前方邻居是否为空
				std::vector<uint32_t> newPriorityList = VehicleGetPriorityListOfInterest();
				if(newPriorityList.empty())
				{
					cout<<"(forwarding.cc-OnInterest) At Time "<<Simulator::Now().GetSeconds()<<" 节点 "<<myNodeId<<"准备缓存兴趣包 "<<seq<<endl;
					//getchar();
					Simulator::Schedule(sendInterval,&NavigationRouteHeuristic::CachingInterestPacket,this,seq,interest);
				}
				else
				{
					m_sendingInterestEvent[nodeId][seq] = Simulator::Schedule(sendInterval,&NavigationRouteHeuristic::ForwardInterestPacket,this,interest,newPriorityList);
				}
			}
		}
		else
		{
			cout<<"(forwarding.cc-OnInterest) Node id is not in PriorityList"<<endl;
			NS_LOG_DEBUG("Node id is not in PriorityList");
			DropInterestePacket(interest);
		}
		//getchar();
		//cout<<endl;
	}
}

void NavigationRouteHeuristic::ProcessHello(Ptr<Interest> interest)
{
	if(!m_running){
		return;
	}
	
	if(m_HelloLogEnable)
		NS_LOG_DEBUG (this << interest << "\tReceived HELLO packet from "<<interest->GetNonce());
	
	Ptr<const Packet> nrPayload	= interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t sourceId = nrheader.getSourceId();
	uint32_t nodeId = m_node->GetId();
	int m_nbChange_mode = 0;
	
	cout<<"(forwarding.cc-ProcessHello) 当前节点 "<<nodeId<<" 发送心跳包的节点 "<<sourceId<<" At time "<<Simulator::Now().GetSeconds()<<endl;
	
	//更新邻居列表
	m_nb.Update(sourceId,nrheader.getX(),nrheader.getY(),Time (AllowedHelloLoss * HelloInterval));
	

	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator nb = m_nb.getNb().begin();
	std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator prenb = m_preNB.getNb().begin();
	
	
	
	if(m_preNB.getNb().size()<m_nb.getNb().size())
	{   
		m_nbChange_mode = 2;
		//cout<<"邻居增加"<<endl;
	}
	else if(m_preNB.getNb().size()>m_nb.getNb().size())
	{
		m_nbChange_mode = 1;
		//cout<<"邻居减少"<<endl;f
	}
	else
	{
		bool nbChange=false;
		for(prenb = m_preNB.getNb().begin();nb!=m_nb.getNb().end() && prenb!=m_preNB.getNb().end();++prenb,++nb)
		{
			//寻找上次的邻居，看看能不能找到，找不到证明变化了
			if(m_nb.getNb().find(prenb->first) == m_nb.getNb().end())
			{  
				nbChange=true;
				break;
				
			}
		}
		if(nbChange)
		{   //邻居变化，发送兴趣包
			m_nbChange_mode = 3;
			//cout<<"邻居变化"<<endl;
		}
	}
	
	prenb = m_preNB.getNb().begin();
	nb = m_nb.getNb().begin();
	/*cout<<"原来的邻居：";
	for(; prenb!=m_preNB.getNb().end();++prenb)
	{
		cout<<prenb->first<<" ";
	}
	cout<<"\n现在的邻居：";
	for(;nb != m_nb.getNb().end();++nb)
	{
		cout<<nb->first<<" ";
	}*/
	//getchar();
	
	const uint32_t numsofvehicles = m_sensor->getNumsofVehicles();
	
	// 2017.12.21 获取当前前方邻居数目
	uint32_t nums_car_current = 0;
	for(;nb != m_nb.getNb().end();++nb)
	{
		if(nb->first >= numsofvehicles)
		{
			std::pair<bool,double> result = m_sensor->getDistanceWithRSU(nb->second.m_x,nb->second.m_y,nb->first);
			cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			if(result.first && result.second > 0)
			{
				nums_car_current += 1;
			}
			//getchar();
		}
		else
		{
			std::pair<bool, double> result = m_sensor->getDistanceWithVehicle(nb->second.m_x,nb->second.m_y);
			cout<<"("<<nb->first<<" "<<result.first<<" "<<result.second<<")"<<" ";
			if(result.first && result.second > 0)
			{
				nums_car_current += 1;
			}
		}
	}
	cout<<endl<<"(forwarding.cc-ProcessHello) nums_car_current "<<nums_car_current<<endl;
	
	
	//前方道路有车辆
	if(nums_car_current > 0)
	{
		cout<<"(forwarding.cc-ProcessHello) 前方道路有车辆"<<endl;
		//有兴趣包在缓存中
		if(m_csinterest->GetSize() > 0)
		{
			cout<<"(forwarding.cc-ProcessHello) 有兴趣包在缓存中"<<endl;
			const string& localLane = m_sensor->getLane();
			cout<<"(forwarding.cc-ProcessHello) 车辆当前所在路段为 "<<localLane<<endl;
			//获得缓存的兴趣包
			map<uint32_t,Ptr<const Interest> > interestcollection = m_csinterest->GetInterest(localLane);
			cout<<"(forwarding.cc-ProcessHello) 获得缓存的兴趣包"<<endl;
			SendInterestInCache(interestcollection);
		}
		else
		{
			cout<<"(forwarding.cc-ProcessHello) 无兴趣包在缓存中"<<endl;
		}
		//getchar();
	}
	
	
	//判断心跳包的来源方向
	pair<bool, double> msgdirection = packetFromDirection(interest);
	//cout<<"(forwarding.cc-ProcessHello) 心跳包的位置为 "<<msgdirection.first<<" "<<msgdirection.second<<endl;
	
	//added by sy
	//发送心跳包的节点位于当前节点后方
	if(msgdirection.first && msgdirection.second < 0)
	{
		overtake.insert(sourceId);
	}
	else if(msgdirection.first && msgdirection.second >0)
	{
		std::unordered_set<uint32_t>::iterator it = overtake.find(sourceId);
		//该车辆超车
		if(it != overtake.end())
		{
			const vector<string> remoteroutes = ExtractRouteFromName(interest->GetName());
			//获取心跳包所在路段
			string remoteroute = remoteroutes.front();
			m_nrpit->DeleteFrontNode(remoteroute,sourceId,"DEFAULT_VEHTYPE");
			overtake.erase(it);
			//cout<<"(forwarding.cc-ProcessHello) 车辆 "<<sourceId<<"超车，从PIT中删除该表项"<<endl;
		}
		else
		{
			//cout<<"(forwarding.cc-ProcessHello) 车辆 "<<sourceId<<"一直位于前方"<<endl;
		}
	}
	
	//判断转发列表是否为空 2017.9.10
	if(ForwardNodeList.size() == 0)
	{
		//cout<<"(forwarding.cc-ProcessHello) ForwardNodeList中的节点个数为0"<<endl;
		if(msgdirection.first && msgdirection.second >= 0)
		{
			notifyUpperOnInterest();
			//cout<<"(forwarding.cc-ProcessHello) notifyUpperOnInterest"<<endl;
		}
	}
	
	//判断转发节点是否丢失
	std::unordered_set<uint32_t>::iterator it;
	bool lostforwardnode = false;
	bool resend = false;
	//cout<<"(forwarding.cc-ProcessHello) ForwardNodeList中的节点为 ";
	for(it = ForwardNodeList.begin();it != ForwardNodeList.end();)
	{
		//cout<<*it<<endl;
		std::unordered_map<uint32_t, Neighbors::Neighbor>::const_iterator itnb = m_nb.getNb().find(*it);
		if(itnb == m_nb.getNb().end())
		{
			//cout<<"转发节点丢失 "<<*it<<endl;
			it = ForwardNodeList.erase(it);
			lostforwardnode = true;
		}
		else
		{
			//cout<<"转发节点存在"<<endl;
			it++;
		}
	}
	
	//转发节点丢失
	if(lostforwardnode)
	{
		if(ForwardNodeList.empty() && msgdirection.first && msgdirection.second)
		{
			//cout<<"转发节点全部丢失"<<endl;
			notifyUpperOnInterest();
		}
		else if(msgdirection.first && msgdirection.second && m_nbChange_mode > 1)
		{
			//cout<<"转发节点部分丢失，但有新的邻居进入"<<endl;
			notifyUpperOnInterest();
		}
	}
	
	m_preNB = m_nb;
	//cout<<endl;
}

void NavigationRouteHeuristic::notifyUpperOnInterest()
{
	//增加一个时间限制，超过1s才进行转发
	double interval = Simulator::Now().GetSeconds() - m_resendInterestTime;
    if(interval >= 1)
	{
		m_resendInterestTime =  Simulator::Now().GetSeconds();	
		cout<<"源节点 "<<m_node->GetId() << " 允许发送兴趣包。间隔" <<interval << " 时间 "<<Simulator::Now().GetSeconds() << endl;
	}
	else
	{
		//cout<<"源节点 "<<m_node->GetId()<< " 禁止发送兴趣包。间隔 " <<interval << " 时间 "<<Simulator::Now().GetSeconds() <<endl;
		return;
	}
	vector<Ptr<Face> >::iterator fit;
	Ptr<Interest> interest = Create<Interest> ();
	int count=0;
	for (fit = m_inFaceList.begin(); fit != m_inFaceList.end(); ++fit)
	{   //只有一个Face？有两个，一个是consumer，一个是producer
		//App::OnInterest() will be executed,
		//including nrProducer::OnInterest.
		count++;
		(*fit)->SendInterest(interest);
	}
	if(count>2)
	{
		cout<<"(forwarding.cc)notifyUpperOnInterest中的Face数量大于2："<<count<<endl;
		NS_ASSERT_MSG(count <= 2,"notifyUpperOnInterest:Face数量大于2！");
	}
}

void EntryNrImpl::AddNeighborTimeoutEvent(uint32_t id)
{
	if(m_nbTimeoutEvent.find(id)!=m_nbTimeoutEvent.end())
	{
		m_nbTimeoutEvent[id].Cancel();
		Simulator::Remove (m_nbTimeoutEvent[id]); // slower, but better for memory
	}
	//Schedule a new cleaning event
	//std::cout<<"(ndn-pit-entry-nrimpl.cc)m_infaceTimeout "<<m_infaceTimeout<<std::endl;
	m_nbTimeoutEvent[id]
	                   = Simulator::Schedule(m_infaceTimeout,
	                		   &EntryNrImpl::CleanExpiredIncomingNeighbors,this,id);
}

//删除超时的邻居
void EntryNrImpl::CleanExpiredIncomingNeighbors(uint32_t id)
{
	std::unordered_set< uint32_t >::iterator it;

	NS_LOG_DEBUG("At PIT Entry:"<<GetInterest()->GetName().toUri()<<" To delete neighbor:"<<id);

	std::unordered_set< uint32_t >::iterator incomingnb  = m_incomingnbs.find(id);
	
	if (incomingnb != m_incomingnbs.end())
	{
		m_incomingnbs.erase(incomingnb);
		std::cout<<"删除邻居 "<<id<<std::endl;
	}	
	listPitEntry();
}

void EntryNrImpl::RemoveAllTimeoutEvent()
{
	//std::cout<<"(forwarding.cc-RemoveAllTimeoutEvent)"<<std::endl;
	std::unordered_map< uint32_t,EventId>::iterator it;
	for(it=m_nbTimeoutEvent.begin();it!=m_nbTimeoutEvent.end();++it)
	{
		it->second.Cancel();
		Simulator::Remove (it->second); // slower, but better for memory
	}
}