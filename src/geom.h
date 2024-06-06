#ifndef GEOM_H
#define GEOM_H

#include <compare>

namespace model {

struct Vec2D {
    Vec2D() = default;
    Vec2D(double x, double y)
        : dx(x)
        , dy(y) {
    }

    Vec2D& operator*=(double scale) {
        dx *= scale;
        dy *= scale;
        return *this;
    }

    auto operator<=>(const Vec2D&) const = default;

    double dx = 0;
    double dy = 0;
};

inline Vec2D operator*(Vec2D lhs, double rhs) {
    return lhs *= rhs;
}

inline Vec2D operator*(double lhs, Vec2D rhs) {
    return rhs *= lhs;
}

struct Point2D {
    Point2D() = default;
    Point2D(double x, double y)
        : x(x)
        , y(y) {
    }

    Point2D& operator+=(const Vec2D& rhs) {
        x += rhs.dx;
        y += rhs.dy;
        return *this;
    }

    auto operator<=>(const Point2D&) const = default;

    double x = 0;
    double y = 0;
};

inline Point2D operator+(Point2D lhs, const Vec2D& rhs) {
    return lhs += rhs;
}

inline Point2D operator+(const Vec2D& lhs, Point2D rhs) {
    return rhs += lhs;
}

}  // namespace geom

#endif  // GEOM_H