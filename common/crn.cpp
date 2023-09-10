#include "crn.hpp"
#include "json.hpp"
#include <fstream>

void math::to_json(json &j, const math::Point &p)
{
    j = {p.x, p.y};
}

void math::from_json(const json &j, math::Point &p)
{
    p.x = j[0].template get<double>();
    p.y = j[1].template get<double>();
}

void to_json(json &j, const CrnData &crnData)
{
    j = json{
        {"version", 2},
        {"characterName", crnData.characterName},
        {"imageName", crnData.imageName},
        {"headPosition", crnData.headPosition},
        {"headRadius", crnData.headRadius},
        {"centerLine", crnData.centerLine},
        {"direction", static_cast<int>(crnData.direction)}};
}

void from_json(const json &j, CrnData &crnData)
{
    const auto dataVersion = j["version"].template get<int>();
    if (dataVersion == 1)
    {
        crnData.headPosition.x = j["circle_x"].template get<double>();
        crnData.headPosition.y = j["circle_y"].template get<double>();
        crnData.headRadius = j["circle_r"].template get<double>();

        const int pointsLength = j.contains("line_type") ? j["line_type"].template get<int>() : 2;
        if (pointsLength >= 1)
        {
            crnData.centerLine.push_back({j["p1_x"].template get<double>(),
                                          j["p1_y"].template get<double>()});
        }
        if (pointsLength >= 2)
        {
            crnData.centerLine.push_back({j["p2_x"].template get<double>(),
                                          j["p2_y"].template get<double>()});
        }
        if (pointsLength >= 3)
        {
            crnData.centerLine.push_back({j["p3_x"].template get<double>(),
                                          j["p3_y"].template get<double>()});
        }

        crnData.direction = static_cast<CrnData::Direction>(j.contains("direction") ? j["direction"].template get<int>() : 0);
    }
    else if (dataVersion == 2)
    {
        crnData.characterName = j["characterName"].template get<std::string>();
        crnData.imageName = j["imageName"].template get<std::string>();
        crnData.headPosition = j["headPosition"].template get<math::Point>();
        crnData.headRadius = j["headRadius"].template get<double>();
        crnData.centerLine = j["centerLine"].template get<std::vector<math::Point>>();
        crnData.direction = static_cast<CrnData::Direction>(j["direction"].template get<int>());
    }
    else
    {
        throw CrnData::jsonVersionException();
    }
}

CrnData CrnData::load(const std::string &crnFilePath)
{
    std::ifstream f(crnFilePath.c_str());
    return json::parse(f).template get<CrnData>();
};

void CrnData::save(const std::string &crnFilePath) const
{
    std::ofstream f(crnFilePath.c_str());
    f << json{*this}[0].dump() << std::endl;
};
