
#ifndef NRNDN_FORWARDED_H_
#define NRNDN_FORWARDED_H_

#include <set>
#include <map>
#include <string>
#include <unordered_set>

namespace ns3
{
namespace ndn
{
namespace fw
{
namespace nrndn
{

class Forwarded
{
  public:
    void setForwardedRoads(const uint32_t RSUId, const uint32_t signature, std::string road);
    std::set<std::string> getForwardedRoads(const uint32_t RSUId, const uint32_t signature);
    bool IsAllForwarded(const uint32_t RSUId, const uint32_t signature, std::unordered_set<std::string> allinteresRoutes);
    void clearAllRoads(const uint32_t RSUId, const uint32_t signature);
    void clearOneRoad(const uint32_t RSUId, const uint32_t signature, std::string road);
    void printAllRoads(const uint32_t RSUId, const uint32_t signature);


  private:
    std::map<std::pair<uint32_t, uint32_t>, std::set<std::string>>
        RSUForwarded;
    std::map<std::pair<uint32_t, uint32_t>, std::set<std::string>>::iterator it;
};

} /* namespace nrndn */
} /* namespace fw */
} /* namespace ndn */
} /* namespace ns3 */
#endif
