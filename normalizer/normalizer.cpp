#include "normalizer.hpp"
#include <boost/gil/extension/numeric/sampler.hpp>
#include <boost/gil/extension/numeric/resample.hpp>

namespace gil = boost::gil;

gil::rgba8_image_t normalize(
    const NormalizationParamaters &params,
    const gil::rgba8_view_t &img,
    const CrnData &crnData)
{
    std::ptrdiff_t content_l = img.width(), content_r = 0, content_t = img.height(), content_b = 0;
    long long totalArea = 0;

    for (auto p = img.begin(); p != img.end(); p++)
    {
        const auto alpha = gil::get_color(*p, gil::alpha_t());
        if (alpha > 0)
        {
            auto x = p.x_pos(), y = p.y_pos();
            content_l = std::min(content_l, x);
            content_r = std::max(content_r, x + 1);
            content_t = std::min(content_t, y);
            content_b = std::max(content_b, y + 1);
            totalArea += 1;
        }
    }

    if (totalArea == 0)
        return gil::rgba8_image_t(1, 1);

    math::Point centerPoint;
    if (params.centeringHead)
    {
        centerPoint = crnData.headPosition;
    }
    else if (crnData.centerLine.size() > 0)
    {
        centerPoint = math::lineCentroid(crnData.centerLine);
    }
    else
    {
        centerPoint = {(content_l + content_r - 1) * 0.5,
                       (content_t + content_b - 1) * 0.5};
    }

    auto contentView = gil::subimage_view(img, content_l, content_t, content_r - content_l, content_b - content_t);
    const auto contentWidth = contentView.width(), contentHeight = contentView.height();

    centerPoint -= math::Point{static_cast<double>(content_l), static_cast<double>(content_t)};
    double paddingLeft = std::max((contentWidth - centerPoint.x) - (centerPoint.x), 0.0);
    double paddingRight = std::max((centerPoint.x) - (contentWidth - centerPoint.x), 0.0);
    double paddingTop = std::max((contentHeight - centerPoint.y) - (centerPoint.y), 0.0);
    double paddingBottom = std::max((centerPoint.y) - (contentHeight - centerPoint.y), 0.0);

    // scale

    const double scaleByArea = std::sqrt(params.targetTotalArea / totalArea);
    const double scaleByHeadSize = params.targetHeadSize / crnData.headRadius;
    const double scale = (scaleByArea * (1.0 - params.areaHeadRatio) +
                          scaleByHeadSize * (params.areaHeadRatio)) *
                         params.scaleAdjustment;

    // output

    gil::rgba8_image_t res(static_cast<std::ptrdiff_t>(std::round((paddingLeft + contentWidth + paddingRight) * scale)),
                           static_cast<std::ptrdiff_t>(std::round((paddingTop + contentHeight + paddingBottom) * scale)));
    gil::matrix3x2<double> mat =
        gil::matrix3x2<double>::get_scale(1 / scale) *
        gil::matrix3x2<double>::get_translate(-gil::point<double>(paddingLeft, paddingTop));
    gil::resample_pixels(contentView,
                         gil::view(res),
                         mat,
                         gil::bilinear_sampler());
    return res;
}
