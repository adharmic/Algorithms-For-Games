#ifndef _VECTOR2D_H
#define _VECTOR2D_H
#pragma once

#include <math.h>
#include <d2d1.h>


// VECTOR MATH LIBRARY FROM ALLEGRO PORTED FOR C++ PURPOSES BY DANIEL SOLTYKA
// http://www.danielsoltyka.com/programming/2010/05/30/c-vector2d-rectangle-classes/

class Vector2D
{

public:
    Vector2D(float x = 0, float y = 0);
    ~Vector2D() {};

    void Rotate(const float angle);
    float Magnitude() const;
    void Normalize();
    float DotProduct(const Vector2D& v2) const;
    float CrossProduct(const Vector2D& v2) const;

    //Vector2D Normalize();

    static Vector2D Zero();
    static float Distance(const Vector2D& v1, const Vector2D& v2);

    Vector2D& operator= (const Vector2D& v2);

    Vector2D& operator+= (const Vector2D& v2);
    Vector2D& operator-= (const Vector2D& v2);
    Vector2D& operator*= (const float scalar);
    Vector2D& operator/= (const float scalar);

    const Vector2D operator+(const Vector2D& v2) const;
    const Vector2D operator-(const Vector2D& v2) const;
    const Vector2D operator*(const float scalar) const;
    const Vector2D operator/(const float scalar) const;

    bool operator== (const Vector2D& v2) const;
    bool operator!= (const Vector2D& v2) const;

public:
    float x, y;
};
#endif