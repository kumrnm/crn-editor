#pragma once

#include <vector>

namespace math
{

    struct Point
    {
        double x, y;

        Point operator+(const Point &) const;
        Point operator-(const Point &) const;
        Point operator-() const;
        Point operator*(const double) const;
        Point operator/(const double) const;
        Point &operator+=(const Point &);
        Point &operator-=(const Point &);
        Point &operator*=(const double);
        Point &operator/=(const double);

        double distanceTo(const Point &) const;
    };

    struct Transform
    {
        Point shift;
        double scale = 1.0;

        Transform inverse() const;
    };

    Point operator*(const Transform &, const Point &);
    double operator*(const Transform &, const double);

    Point lineCentroid(const std::vector<Point> &);
}
