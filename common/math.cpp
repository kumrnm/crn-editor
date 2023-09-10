#include <utility>
#include <cmath>
#include "math.hpp"
#include <stdexcept>

namespace math
{

    // struct Point

    Point Point::operator+(const Point &p) const
    {
        return Point{x + p.x, y + p.y};
    }
    Point Point::operator-(const Point &p) const
    {
        return Point{x - p.x, y - p.y};
    }
    Point Point::operator-() const
    {
        return Point{-x, -y};
    }
    Point Point::operator*(const double a) const
    {
        return Point{x * a, y * a};
    }
    Point Point::operator/(const double a) const
    {
        return Point{x / a, y / a};
    }

    Point &Point::operator+=(const Point &p)
    {
        return *this = *this + p;
    }
    Point &Point::operator-=(const Point &p)
    {
        return *this = *this - p;
    }
    Point &Point::operator*=(const double a)
    {
        return *this = *this * a;
    }
    Point &Point::operator/=(const double a)
    {
        return *this = *this / a;
    }

    double Point::distanceTo(const Point &p) const
    {
        return sqrt((x - p.x) * (x - p.x) + (y - p.y) * (y - p.y));
    }

    // struct Transform

    Transform Transform::inverse() const
    {
        return Transform{-shift / scale, 1 / scale};
    }

    Point operator*(const Transform &t, const Point &p)
    {
        return p * t.scale + t.shift;
    }
    double operator*(const Transform &t, const double a)
    {
        return t.scale * a;
    }

    Point lineCentroid(const std::vector<Point> &points)
    {
        if (points.size() == 0)
        {
            throw std::invalid_argument("points must not be empty");
        }
        else if (points.size() == 1)
        {
            return points[0];
        }

        Point res;
        double totalLength = 0.0;
        for (int i = 1; i < points.size(); i++)
        {
            const auto &p = points[i - 1], &q = points[i];
            const auto d = p.distanceTo(q);
            res += (p + q) * (d / 2);
            totalLength += d;
        }
        res /= totalLength;
        return res;
    }
}
