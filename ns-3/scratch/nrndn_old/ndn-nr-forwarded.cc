/*
 * ndn-nr-forwarded.cc
 *
 *  Created on: Dec 20, 2018
 *      Author: WSY
 */
#include "ns3/log.h"
#include "ndn-nr-forwarded.h"

NS_LOG_COMPONENT_DEFINE("NrndnForwarded");

namespace ns3
{
namespace ndn
{
namespace fw
{
namespace nrndn
{

void Forwarded::setForwardedRoads(const uint32_t RSUId, const uint32_t signature, std::string road)
{
    std::pair<uint32_t, uint32_t> id(RSUId, signature);
    it = RSUForwarded.find(id);
    std::set<std::string> forwardedRoads;
    if (it != RSUForwarded.end())
    {
        forwardedRoads = it->second;
    }

    forwardedRoads.insert(road);
    RSUForwarded[id] = forwardedRoads;
    std::cout << "(forwarded.cc-setForwardedRoads) RSU " << RSUId << " signature " << signature << " 已转发过路段 " << road << std::endl;
}

std::set<std::string> Forwarded::getForwardedRoads(const uint32_t RSUId, const uint32_t signature)
{
    std::set<std::string> forwardedRoads;
    std::pair<uint32_t, uint32_t> id(RSUId, signature);
    it = RSUForwarded.find(id);
    if (it != RSUForwarded.end())
    {
        forwardedRoads = it->second;
    }
    return forwardedRoads;
}

bool Forwarded::IsAllForwarded(const uint32_t RSUId, const uint32_t signature, std::unordered_set<std::string> allinteresRoutes)
{
    std::pair<uint32_t, uint32_t> id(RSUId, signature);
    it = RSUForwarded.find(id);
    if (it != RSUForwarded.end())
    {
        std::set<std::string> forwardedRoads = it->second;
        std::set<std::string>::iterator itroads = forwardedRoads.begin();
        for (; itroads != forwardedRoads.end(); itroads++)
        {
            std::unordered_set<std::string>::iterator itfind = allinteresRoutes.find(*itroads);
            if (itfind != allinteresRoutes.end())
            {
                allinteresRoutes.erase(itfind);
            }
        }
        if (allinteresRoutes.empty())
        {
            return true;
        }

        std::cout << "(forwarded.cc-IsAllForwarded) RSU " << RSUId << " signature " << signature << " 未转发过的路段为 ";
        std::unordered_set<std::string>::iterator itfind = allinteresRoutes.begin();
        for (; itfind != allinteresRoutes.end(); itfind++)
        {
            std::cout << *itfind << " ";
        }
        std::cout << std::endl;
        return false;
    }
    NS_ASSERT_MSG(it != RSUForwarded.end(), "RSU未转发过该数据包");
}

void Forwarded::clearAllRoads(const uint32_t RSUId, const uint32_t signature)
{
    std::pair<uint32_t, uint32_t> id(RSUId, signature);
    it = RSUForwarded.find(id);
    if (it != RSUForwarded.end())
    {
        std::set<std::string> forwardedRoads;
        RSUForwarded[id] = forwardedRoads;
        std::cout << "(forwarded.cc-clearAllRoads) RSU " << RSUId << " signature " << signature << "全部清空" << std::endl;
    }
}

void Forwarded::printAllRoads(const uint32_t RSUId, const uint32_t signature)
{
    std::pair<uint32_t, uint32_t> id(RSUId, signature);
    it = RSUForwarded.find(id);
    if (it != RSUForwarded.end())
    {
        std::cout << "(forwarded.cc) RSU " << RSUId << " signature " << signature << " 转发过的路段为";
        std::set<std::string> forwardedRoads = it->second;
        std::set<std::string>::iterator itforward = forwardedRoads.begin();
        for (; itforward != forwardedRoads.end(); itforward++)
        {
            std::cout << *itforward << " ";
        }
        std::cout << std::endl;
        return;
    }
}

} /* namespace nrndn */
} /* namespace fw */
} /* namespace ndn */
} /* namespace ns3 */
