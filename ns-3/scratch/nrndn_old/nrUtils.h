/*
 * nrUtils.h
 *
 *  Created on: Dec 29, 2014
 *      Author: chen
 */

#ifndef NRUTILS_H_
#define NRUTILS_H_

#include "ns3/core-module.h"
#include "ns3/ndn-wire.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <utility>

namespace ns3
{
namespace ndn
{
namespace nrndn
{

/**
 *求容器的平均值
 * The class implements the universal Method for nodes to use the navigation route
 * And some global variables
 *
 */
template <class T>
double GetAverage(std::vector<T> &v)
{
	if (v.empty())
		return 0;
	typename std::vector<T>::iterator rit;
	double average = 0;
	double sum = 0;
	for (rit = v.begin(); rit != v.end(); ++rit)
		sum += *rit;
	average = sum / v.size();
	return average;
}

struct MsgAttribute
{
	MsgAttribute() : NodeSize(0),
					 InterestedNodeSize(0),
					 InterestedNodeReceiveCounter(0),
					 DisinterestedNodeReceiveCounter(0) {}
	uint32_t NodeSize;
	uint32_t InterestedNodeSize;
	uint32_t InterestedNodeReceiveCounter;	//节点收到感兴趣数据的数量
	uint32_t DisinterestedNodeReceiveCounter; //节点收到不感兴趣数据的数量
};

class nrUtils
{
  public:
	nrUtils();
	~nrUtils();

	typedef std::unordered_map<std::string, uint32_t> AppIndexType;
	typedef std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::vector<double>>> TransmissionDelayMap;
	typedef std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t>> ForwardCounterMap;
	typedef std::unordered_map<uint32_t, std::unordered_map<uint32_t, MsgAttribute>> MessageArrivalMap;

	//1. Arrival Condition
	static MessageArrivalMap msgArrivalCounter;
	//producer在发送数据包时，需要记录当前活跃节点个数和对该数据包感兴趣的节点个数
	static std::pair<uint32_t, uint32_t> GetNodeSizeAndInterestNodeSize(const double &x, const double &y, uint32_t id, uint32_t signature, const std::string &lane);
	//设置节点个数
	static void SetNodeSize(uint32_t id, uint32_t signature, uint32_t nodesize);
	//设置感兴趣节点个数
	static void SetInterestedNodeSize(uint32_t id, uint32_t signature, uint32_t InterestedNodeSize);
	//consumer对该数据包感兴趣的个数
	static void IncreaseInterestedNodeCounter(uint32_t id, uint32_t signature);
	//consumer对该数据包不感兴趣的个数
	static void IncreaseDisinterestedNodeCounter(uint32_t id, uint32_t signature);

	//2. forward times
	static ForwardCounterMap forwardCounter; //数据包被转发的次数
	static void IncreaseForwardCounter(uint32_t id, uint32_t signature);
	static ForwardCounterMap interestForwardCounter; //兴趣包被转发的次数
	static void IncreaseInterestForwardCounter(uint32_t id, uint32_t nonce);
	//2018.3.23
	static ForwardCounterMap deleteForwardCounter; //删除包被转发的次数
	static void IncreaseDeleteForwardCounter(uint32_t id, uint32_t nonce);

	//3. Delay Record
	static TransmissionDelayMap TransmissionDelayRecord; //\brief TransmissionDelayRecord[nodeid][signature]=Delay time;
	//consumer在收到data后，记录传输时间
	static void InsertTransmissionDelayItem(uint32_t id, uint32_t signature, double delay);
	static double GetAverageTransmissionDelay();

	static double GetAverageArrivalRate();
	static double GetAverageAccurateRate();
	static double GetAverageDisinterestedRate();
	static double GetAverageHitRate();

	/*
	 * @brief get the average forward times
	 * \return (ForwardTimesSum,averageForwardTimes)
	 * */
	static std::pair<uint32_t, double> GetAverageForwardTimes();

	/*
	 * @brief get the average interest packet forward times
	 * \return (InterestForwardTimesSum,averageInterestForwardTimes)
	 * */
	static std::pair<uint32_t, double> GetAverageInterestForwardTimes();

	//2017.2.23
	static std::pair<uint32_t, double> GetAverageDeleteForwardTimes();

	static double GetAverageDelay();

	//4 . appIndex
	static AppIndexType appIndex;

	static uint32_t ByteSent;
	static uint32_t DataByteSent;
	static uint32_t InterestByteSent;
	static uint32_t HelloByteSent;
	static uint32_t DeleteByteSent;
	static uint32_t HelloCount;
	static void AggrateDataPacketSize(Ptr<const Data> data);
	static void AggrateInterestPacketSize(Ptr<const Interest> interest);
};

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NRUTILS_H_ */
