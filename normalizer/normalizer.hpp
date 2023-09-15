#pragma once

#include <boost/gil.hpp>
#include "../common/crn.hpp"
#include <string>

struct NormalizationParamaters
{
    double areaHeadRatio = 0.0; // 0.0 (reslze by total area only) ~ 1.0 (by head size only)
    bool centeringHead = false;

    double targetTotalArea = 250000.0;
    double targetHeadSize = 200.0;

    double scaleAdjustment = 1.0;
};

boost::gil::rgba8_image_t normalize(
    const NormalizationParamaters &params,
    const boost::gil::rgba8_view_t &img,
    const CrnData &crnData);
