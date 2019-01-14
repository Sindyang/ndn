/*
 * nrUtils.cpp
 *
 *  Created on: Dec 29, 2014
 *      Author: chen
 */

#include "nrUtils.h"
#include "nrProducer.h"

#include "ns3/network-module.h"

namespace ns3
{
namespace ndn
{
namespace nrndn
{

using namespace std;

nrUtils::MessageArrivalMap nrUtils::msgArrivalCounter;
nrUtils::ForwardCounterMap nrUtils::forwardCounter;
nrUtils::ForwardCounterMap nrUtils::interestForwardCounter;
nrUtils::ForwardCounterMap nrUtils::deleteForwardCounter;
nrUtils::TransmissionDelayMap nrUtils::TransmissionDelayRecord;
nrUtils::AppIndexType nrUtils::appIndex;
uint32_t nrUtils::ByteSent = 0;
uint32_t nrUtils::DataByteSent = 0;
uint32_t nrUtils::InterestByteSent = 0;
uint32_t nrUtils::HelloByteSent = 0;
uint32_t nrUtils::DeleteByteSent = 0;
uint32_t nrUtils::HelloCount = 0;

nrUtils::nrUtils()
{
	// TODO Auto-generated constructor stub
}

nrUtils::~nrUtils()
{
	// TODO Auto-generated destructor stub
}

std::pair<uint32_t, uint32_t> nrUtils::GetNodeSizeAndInterestNodeSize(
	const double &x, const double &y, uint32_t id, uint32_t signature, const std::string &lane)
{
	uint32_t nodeSize = 0;
	uint32_t interestSize = 0;
	NodeContainer c = NodeContainer::GetGlobal();
	NodeContainer::Iterator it;
	int idx = 0;
	cout << "(nrUtils.cc-GetNodeSizeAndInterestNodeSize) 修改RSU数目" << endl;
	getchar();

	for (it = c.Begin(); it != c.End(); ++it)
	{
		// 2018.1.15 忽略RSU
		if (idx >= 500)
			continue;
		Ptr<Application> app = (*it)->GetApplication(appIndex["ns3::ndn::nrndn::nrProducer"]);
		Ptr<nrndn::nrProducer> producer = DynamicCast<nrndn::nrProducer>(app);
		NS_ASSERT(producer);
		if (producer->IsActive())
			++nodeSize;
		else
		{ //非活跃节点直接跳过，避免段错误
			idx++;
			continue;
		}
		if (producer->IsInterestLane(x, y, lane))
		{
			++interestSize;
			cout << "(nrUtils.cc-GetNodeSizeAndInterestNodeSize) idx " << idx << endl;
		}
		idx++;
	}
	return std::pair<uint32_t, uint32_t>(nodeSize, interestSize);
}

//当前时刻，活跃节点个数
void nrUtils::SetNodeSize(uint32_t id, uint32_t signature, uint32_t nodesize)
{
	msgArrivalCounter[id][signature].NodeSize = nodesize;
}

//当前时刻，对该数据包感兴趣的节点个数
void nrUtils::SetInterestedNodeSize(uint32_t id,
									uint32_t signature, uint32_t InterestedNodeSize)
{
	msgArrivalCounter[id][signature].InterestedNodeSize = InterestedNodeSize;
}

//收到数据包的consumer对该数据包感兴趣的个数
void nrUtils::IncreaseInterestedNodeCounter(uint32_t id,
											uint32_t signature)
{
	msgArrivalCounter[id][signature].InterestedNodeReceiveCounter++;
}

//收到数据包的consumer对该数据包不感兴趣的个数
void nrUtils::IncreaseDisinterestedNodeCounter(uint32_t id,
											   uint32_t signature)
{
	msgArrivalCounter[id][signature].DisinterestedNodeReceiveCounter++;
}

//数据包的转发次数
void nrUtils::IncreaseForwardCounter(uint32_t id,
									 uint32_t signature)
{
	forwardCounter[id][signature]++;
}

//兴趣包的转发次数
void nrUtils::IncreaseInterestForwardCounter(uint32_t id, uint32_t nonce)
{
	interestForwardCounter[id][nonce]++;
}

void nrUtils::IncreaseDeleteForwardCounter(uint32_t id, uint32_t nonce)
{
	deleteForwardCounter[id][nonce]++;
}

//数据包从发出到收到的时间
void nrUtils::InsertTransmissionDelayItem(uint32_t id,
										  uint32_t signature, double delay)
{
	TransmissionDelayRecord[id][signature].push_back(delay);
}

//平均到达率表示的是所有车辆节点中收到数据包车辆的平均占比，用于评价数据包的覆盖范围
double nrUtils::GetAverageArrivalRate()
{
	MessageArrivalMap::iterator it1;
	std::unordered_map<uint32_t, MsgAttribute>::iterator it2;
	vector<double>::iterator rit;
	vector<double> result;

	//每一个Producer
	for (it1 = msgArrivalCounter.begin(); it1 != msgArrivalCounter.end(); ++it1)
	{
		//Producer的每个Signature
		for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{
			//每一个Singature对应的信息
			const MsgAttribute &msgAttr = it2->second;
			//活跃节点个数
			double size = msgAttr.NodeSize;
			//该数据包被consumer收到的个数(包括感兴趣节点个数+不感兴趣节点个数)
			double receiveNodesNum = msgAttr.InterestedNodeReceiveCounter + msgAttr.DisinterestedNodeReceiveCounter;
			if (size != 0)
			{
				double arrivalRate = receiveNodesNum / size;
				result.push_back(arrivalRate);
			}
		}
	}
	return GetAverage(result);
}

//平均准确率表示所有收到数据包的车辆中对数据包感兴趣的车辆的平均占比
double nrUtils::GetAverageAccurateRate()
{
	MessageArrivalMap::iterator it1;
	std::unordered_map<uint32_t, MsgAttribute>::iterator it2;
	vector<double> result;
	//每一个Producer
	for (it1 = msgArrivalCounter.begin(); it1 != msgArrivalCounter.end(); ++it1)
	{
		//Producer的每个Signature
		for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{
			const MsgAttribute &msgAttr = it2->second;
			//收到数据包的车辆中，对数据包感兴趣的节点个数
			double interestedNodeNum = msgAttr.InterestedNodeReceiveCounter;
			//收到数据包的车辆中，对数据包不感兴趣的节点个数
			double receiveNodesNum = msgAttr.InterestedNodeReceiveCounter + msgAttr.DisinterestedNodeReceiveCounter;
			if (receiveNodesNum != 0)
			{
				double accurateRate = interestedNodeNum / receiveNodesNum;
				result.push_back(accurateRate);
			}
		}
	}

	return GetAverage(result);
}

//表示所有收到数据包的车辆中对数据包不感兴趣的车辆的平均占比
double nrUtils::GetAverageDisinterestedRate()
{
	MessageArrivalMap::iterator it1;
	std::unordered_map<uint32_t, MsgAttribute>::iterator it2;
	vector<double> result;

	for (it1 = msgArrivalCounter.begin(); it1 != msgArrivalCounter.end(); ++it1)
	{
		for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{
			const MsgAttribute &msgAttr = it2->second;
			double disinterestedNodeNum = msgAttr.DisinterestedNodeReceiveCounter;
			double receiveNodesNum = msgAttr.InterestedNodeReceiveCounter + msgAttr.DisinterestedNodeReceiveCounter;
			if (receiveNodesNum != 0)
			{
				double accurateRate = disinterestedNodeNum / receiveNodesNum;
				result.push_back(accurateRate);
			}
		}
	}
	return GetAverage(result);
}

double nrUtils::GetAverageHitRate()
{
	MessageArrivalMap::iterator it1;
	std::unordered_map<uint32_t, MsgAttribute>::iterator it2;
	vector<double> result;

	//每一个Producer
	for (it1 = msgArrivalCounter.begin(); it1 != msgArrivalCounter.end(); ++it1)
	{
		//每一个Singature
		for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{
			//每一个Singature对应的信息
			const MsgAttribute &msgAttr = it2->second;
			//收到感兴趣数据包的节点个数
			double interestedNodeNum = msgAttr.InterestedNodeReceiveCounter;
			//对该数据包感兴趣的节点个数
			double interestedNodeSum = msgAttr.InterestedNodeSize;
			if (interestedNodeSum != 0)
			{
				double hitRate = interestedNodeNum / interestedNodeSum;
				if (hitRate > 1.0)
				{
					hitRate = 1.0;
				}
				result.push_back(hitRate);
				cout << "车辆 " << it1->first << " 数据包 " << it2->first << " 收到个数 " << interestedNodeNum << " 总数 " << interestedNodeSum
					 << " hitRate " << hitRate << endl;
			}
		}
	}
	return GetAverage(result);
}

//数据包平均转发次数
pair<uint32_t, double> nrUtils::GetAverageForwardTimes()
{
	ForwardCounterMap::iterator it1;
	std::unordered_map<uint32_t, uint32_t>::iterator it2;
	uint32_t forwardTimes = 0;
	double messageNum = 0;

	//每一个Producer
	for (it1 = forwardCounter.begin(); it1 != forwardCounter.end(); ++it1)
	{
		//producer发出的数据包的个数
		messageNum += it1->second.size();
		for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{
			//每一个数据包转发的次数
			forwardTimes += it2->second;
		}
	}

	if (messageNum == 0)
		return make_pair(forwardTimes, 0);

	double average = forwardTimes / messageNum;

	return make_pair(forwardTimes, average);
}

//兴趣包平均转发次数
pair<uint32_t, double> nrUtils::GetAverageInterestForwardTimes()
{
	ForwardCounterMap::iterator it1;
	std::unordered_map<uint32_t, uint32_t>::iterator it2;
	uint32_t forwardTimes = 0;
	double messageNum = 0;

	for (it1 = interestForwardCounter.begin(); it1 != interestForwardCounter.end(); ++it1)
	{
		messageNum += it1->second.size();
		for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{
			forwardTimes += it2->second;
		}
	}

	if (messageNum == 0)
		return make_pair(forwardTimes, 0);

	double average = forwardTimes / messageNum;

	return make_pair(forwardTimes, average);
}

//删除包平均转发次数
pair<uint32_t, double> nrUtils::GetAverageDeleteForwardTimes()
{
	ForwardCounterMap::iterator it1;
	std::unordered_map<uint32_t, uint32_t>::iterator it2;
	uint32_t forwardTimes = 0;
	double messageNum = 0;

	for (it1 = deleteForwardCounter.begin(); it1 != deleteForwardCounter.end(); ++it1)
	{
		messageNum += it1->second.size();
		for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{
			forwardTimes += it2->second;
		}
	}

	if (messageNum == 0)
		return make_pair(forwardTimes, 0);

	double average = forwardTimes / messageNum;

	return make_pair(forwardTimes, average);
}

//数据包从发出到收到的时间
double nrUtils::GetAverageDelay()
{
	TransmissionDelayMap::iterator it1;
	std::unordered_map<uint32_t, std::vector<double>>::iterator it2;
	vector<double> result;

	//每一个Producer
	for (it1 = TransmissionDelayRecord.begin(); it1 != TransmissionDelayRecord.end(); ++it1)
	{
		//每一个Singature
		for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{
			//该数据包从发出到接收的平均时间
			double averageDelayOfOneMsg = GetAverage(it2->second);
			result.push_back(averageDelayOfOneMsg);
			cout << "车辆 " << it1->first << " 数据包 " << it2->first << " 平均延迟 " << averageDelayOfOneMsg << endl;
		}
	}
	return GetAverage(result);
}

double nrUtils::GetAverageTransmissionDelay()
{
	TransmissionDelayMap::iterator it1;
	unordered_map<uint32_t, vector<double>>::iterator it2;
	vector<double>::iterator it3;
	double SumSize = 0;
	double SumDelay = 0;
	//每一个Producer
	for (it1 = TransmissionDelayRecord.begin(); it1 != TransmissionDelayRecord.end(); ++it1)
	{
		//每一个Singature
		for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{
			//延迟的个数即为数据包被收到的个数
			SumSize += it2->second.size();
			//数据包从发出到被consumer接收的时间相加
			for (it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
				SumDelay += (*it3);
		}
	}

	NS_ASSERT(SumSize != 0);

	//总共的延迟/被接收的次数
	return SumDelay / SumSize;
}

void nrUtils::AggrateDataPacketSize(Ptr<const Data> data)
{
	Ptr<Packet> packet = Wire::FromData(data);
	uint32_t size = packet->GetSize();
	ByteSent += size;
	DataByteSent += size;
}

void nrUtils::AggrateInterestPacketSize(Ptr<const Interest> interest)
{
	Ptr<Packet> packet = Wire::FromInterest(interest);
	uint32_t size = packet->GetSize();
	ByteSent += size;
	if (2 == interest->GetScope()) //Hello message
	{
		HelloByteSent += size;
		HelloCount += 1;
		//cout << "(nrUtils-AggrateInterestPacketSize) Hello size " << size << endl;
	}
	else if (3 == interest->GetScope())
	{
		DeleteByteSent += size;
		//cout << "(nrUtils-AggrateInterestPacketSize) Delete size " << size << endl;
	}
	else
	{
		InterestByteSent += size;
		//cout << "(nrUtils-AggrateInterestPacketSize) Interest size " << size << endl;
	}
}

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */
