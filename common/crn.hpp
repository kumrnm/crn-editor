#pragma once

#include "math.hpp"
#include <vector>
#include <string>
#include <json.hpp>

using json = nlohmann::json;

struct CrnData
{
    std::string characterName;
    std::string imageName;

    math::Point headPosition;
    double headRadius;
    std::vector<math::Point> centerLine;

    enum struct Direction : int
    {
        None = 0,
        Left = 1,
        Right = 2
    };
    Direction direction;

    static CrnData load(const std::string &crnFilePath);
    void save(const std::string &crnFilePath) const;

    class jsonVersionException : public std::exception {
    };
};

namespace math
{
    void to_json(json &, const math::Point &);
    void from_json(const json &, math::Point &);
}

void to_json(json &, const CrnData &);
void from_json(const json &, CrnData &);
