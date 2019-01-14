#include <math.h>

namespace ga {
    struct Vec;
    struct Bivec;
    struct Mat;

    struct Vec {
        float x, y, z, w;

        Vec(float x, float y, float z, float w) :
            x(x), y(y), z(z), w(w) {}

    };

    struct Bivec {
        float xy, yz, zw, wx, xz, yw;

        Bivec(float xy, float yz, float zw, float wx, float xz, float yw) :
            xy(xy), yz(yz), zw(zw), wx(wx), xz(xz), yw(yw) {}
    };

    struct Mat {
        Vec x, y, z, w;

        Mat(Vec x, Vec y, Vec z, Vec w) : x(x), y(y), z(z), w(w) {}
    };

    namespace unit {
        Vec x() { return Vec(1, 0, 0, 0); };

        Vec y() { return Vec(1, 0, 0, 0); };

        Vec z() { return Vec(1, 0, 0, 0); };

        Vec w() { return Vec(1, 0, 0, 0); };

        Bivec xy() { return Bivec(1, 0, 0, 0, 0, 0); }

        Bivec yz() { return Bivec(0, 1, 0, 0, 0, 0); }

        Bivec zw() { return Bivec(0, 0, 1, 0, 0, 0); }

        Bivec wx() { return Bivec(0, 0, 0, 1, 0, 0); }

        Bivec xz() { return Bivec(0, 0, 0, 0, 1, 0); }

        Bivec yw() { return Bivec(0, 0, 0, 0, 0, 1); }
    }

    float length2(Vec v) {
        return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
    }

    float length2(Bivec v) {
        return v.xy * v.xy + v.yz * v.yz + v.zw * v.zw
            + v.wx * v.wx + v.xz * v.xz + v.yw * v.yw;
    }

    float length(Vec v) { return sqrt(length2(v)); }

    float length(Bivec v) { return sqrt(length2(v)); }

    Vec add(Vec u, Vec v) {
        return Vec(u.x + v.x,
            u.y + v.y,
            u.z + v.z,
            u.w + v.w);
    }

    Bivec add(Bivec u, Bivec v) {
        Bivec(u.xy + v.xy,
            u.yz + v.yz,
            u.zw + v.zw,
            u.wx + v.wx,
            u.xz + v.xz,
            u.yw + v.yw);
    }

    Vec mul(float c, Vec v) {
        return Vec(c * v.x,
            c * v.y,
            c * v.z,
            c * v.w);
    }

    Bivec mul(float c, Bivec v) {
        Bivec(c * v.xy,
            c * v.yz,
            c * v.zw,
            c * v.wx,
            c * v.xz,
            c * v.yw);
    }

    Vec normalize(Vec v) {
        return mul(1 / length(v), v);
    }

    Bivec normalize(Bivec v) {
        return mul(1 / length(v), v);
    }

    Mat matrix(float c) {
        // c as a transformation
        // scale by c
        return Mat(Vec(c, 0, 0, 0), Vec(0, c, 0, 0), Vec(0, 0, c, 0), Vec(0, 0, 0, c));
    }

    Mat matrix(Vec v) {
        // v as a transformation
        // reflect along v
        return matrix(1); // todo reflection
    }

    Mat rotor(Bivec v) {
        // v as a transformation
        // rotate along v
        return matrix(1);
    }
}