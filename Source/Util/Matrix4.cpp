/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Matrix4.h"
#include <Util/Maths.h>
#include <Util/Vector.h>

Matrix4 Matrix4::identity()
{
    Matrix4 m = {};
    m.at(0, 0) = 1;
    m.at(1, 1) = 1;
    m.at(2, 2) = 1;
    m.at(3, 3) = 1;
    return m;
}

Matrix4 Matrix4::orthographic(float left, float right, float top, float bottom, float near_clip, float far_clip)
{
    Matrix4 m = {};
    m.at(0, 0) = 2.0f / (right - left);
    m.at(1, 1) = 2.0f / (top - bottom);
    m.at(2, 2) = -2.0f / (far_clip - near_clip);

    m.at(3, 0) = -(right + left) / (right - left);
    m.at(3, 1) = -(top + bottom) / (top - bottom);
    m.at(3, 2) = -(far_clip + near_clip) / (far_clip - near_clip);
    m.at(3, 3) = 1.0f;

    return m;
}

Matrix4 Matrix4::inverse() const
{
    // Shamelessly copied from http://stackoverflow.com/a/1148405/1178345
    // This is complicated! *_*
    Matrix4 result;

    result.m_flat[0] = m_flat[5] * m_flat[10] * m_flat[15] - m_flat[5] * m_flat[11] * m_flat[14] - m_flat[9] * m_flat[6] * m_flat[15] + m_flat[9] * m_flat[7] * m_flat[14] + m_flat[13] * m_flat[6] * m_flat[11] - m_flat[13] * m_flat[7] * m_flat[10];

    result.m_flat[4] = -m_flat[4] * m_flat[10] * m_flat[15] + m_flat[4] * m_flat[11] * m_flat[14] + m_flat[8] * m_flat[6] * m_flat[15] - m_flat[8] * m_flat[7] * m_flat[14] - m_flat[12] * m_flat[6] * m_flat[11] + m_flat[12] * m_flat[7] * m_flat[10];

    result.m_flat[8] = m_flat[4] * m_flat[9] * m_flat[15] - m_flat[4] * m_flat[11] * m_flat[13] - m_flat[8] * m_flat[5] * m_flat[15] + m_flat[8] * m_flat[7] * m_flat[13] + m_flat[12] * m_flat[5] * m_flat[11] - m_flat[12] * m_flat[7] * m_flat[9];

    result.m_flat[12] = -m_flat[4] * m_flat[9] * m_flat[14] + m_flat[4] * m_flat[10] * m_flat[13] + m_flat[8] * m_flat[5] * m_flat[14] - m_flat[8] * m_flat[6] * m_flat[13] - m_flat[12] * m_flat[5] * m_flat[10] + m_flat[12] * m_flat[6] * m_flat[9];

    result.m_flat[1] = -m_flat[1] * m_flat[10] * m_flat[15] + m_flat[1] * m_flat[11] * m_flat[14] + m_flat[9] * m_flat[2] * m_flat[15] - m_flat[9] * m_flat[3] * m_flat[14] - m_flat[13] * m_flat[2] * m_flat[11] + m_flat[13] * m_flat[3] * m_flat[10];

    result.m_flat[5] = m_flat[0] * m_flat[10] * m_flat[15] - m_flat[0] * m_flat[11] * m_flat[14] - m_flat[8] * m_flat[2] * m_flat[15] + m_flat[8] * m_flat[3] * m_flat[14] + m_flat[12] * m_flat[2] * m_flat[11] - m_flat[12] * m_flat[3] * m_flat[10];

    result.m_flat[9] = -m_flat[0] * m_flat[9] * m_flat[15] + m_flat[0] * m_flat[11] * m_flat[13] + m_flat[8] * m_flat[1] * m_flat[15] - m_flat[8] * m_flat[3] * m_flat[13] - m_flat[12] * m_flat[1] * m_flat[11] + m_flat[12] * m_flat[3] * m_flat[9];

    result.m_flat[13] = m_flat[0] * m_flat[9] * m_flat[14] - m_flat[0] * m_flat[10] * m_flat[13] - m_flat[8] * m_flat[1] * m_flat[14] + m_flat[8] * m_flat[2] * m_flat[13] + m_flat[12] * m_flat[1] * m_flat[10] - m_flat[12] * m_flat[2] * m_flat[9];

    result.m_flat[2] = m_flat[1] * m_flat[6] * m_flat[15] - m_flat[1] * m_flat[7] * m_flat[14] - m_flat[5] * m_flat[2] * m_flat[15] + m_flat[5] * m_flat[3] * m_flat[14] + m_flat[13] * m_flat[2] * m_flat[7] - m_flat[13] * m_flat[3] * m_flat[6];

    result.m_flat[6] = -m_flat[0] * m_flat[6] * m_flat[15] + m_flat[0] * m_flat[7] * m_flat[14] + m_flat[4] * m_flat[2] * m_flat[15] - m_flat[4] * m_flat[3] * m_flat[14] - m_flat[12] * m_flat[2] * m_flat[7] + m_flat[12] * m_flat[3] * m_flat[6];

    result.m_flat[10] = m_flat[0] * m_flat[5] * m_flat[15] - m_flat[0] * m_flat[7] * m_flat[13] - m_flat[4] * m_flat[1] * m_flat[15] + m_flat[4] * m_flat[3] * m_flat[13] + m_flat[12] * m_flat[1] * m_flat[7] - m_flat[12] * m_flat[3] * m_flat[5];

    result.m_flat[14] = -m_flat[0] * m_flat[5] * m_flat[14] + m_flat[0] * m_flat[6] * m_flat[13] + m_flat[4] * m_flat[1] * m_flat[14] - m_flat[4] * m_flat[2] * m_flat[13] - m_flat[12] * m_flat[1] * m_flat[6] + m_flat[12] * m_flat[2] * m_flat[5];

    result.m_flat[3] = -m_flat[1] * m_flat[6] * m_flat[11] + m_flat[1] * m_flat[7] * m_flat[10] + m_flat[5] * m_flat[2] * m_flat[11] - m_flat[5] * m_flat[3] * m_flat[10] - m_flat[9] * m_flat[2] * m_flat[7] + m_flat[9] * m_flat[3] * m_flat[6];

    result.m_flat[7] = m_flat[0] * m_flat[6] * m_flat[11] - m_flat[0] * m_flat[7] * m_flat[10] - m_flat[4] * m_flat[2] * m_flat[11] + m_flat[4] * m_flat[3] * m_flat[10] + m_flat[8] * m_flat[2] * m_flat[7] - m_flat[8] * m_flat[3] * m_flat[6];

    result.m_flat[11] = -m_flat[0] * m_flat[5] * m_flat[11] + m_flat[0] * m_flat[7] * m_flat[9] + m_flat[4] * m_flat[1] * m_flat[11] - m_flat[4] * m_flat[3] * m_flat[9] - m_flat[8] * m_flat[1] * m_flat[7] + m_flat[8] * m_flat[3] * m_flat[5];

    result.m_flat[15] = m_flat[0] * m_flat[5] * m_flat[10] - m_flat[0] * m_flat[6] * m_flat[9] - m_flat[4] * m_flat[1] * m_flat[10] + m_flat[4] * m_flat[2] * m_flat[9] + m_flat[8] * m_flat[1] * m_flat[6] - m_flat[8] * m_flat[2] * m_flat[5];

    float det = m_flat[0] * result.m_flat[0] + m_flat[1] * result.m_flat[4] + m_flat[2] * result.m_flat[8] + m_flat[3] * result.m_flat[12];
    if (det != 0) {
        det = 1.0f / det;
        for (int i = 0; i < 16; i++) {
            result.m_flat[i] *= det;
        }
    }

    return result;
}

void Matrix4::translate(V3 const& translation)
{
    at(3, 0) += translation.x;
    at(3, 1) += translation.y;
    at(3, 2) += translation.z;
}

void Matrix4::scale(V3 const& scale)
{
    at(0, 0) *= scale.x;
    at(1, 1) *= scale.y;
    at(2, 2) *= scale.z;
}

void Matrix4::rotate_z(float radians)
{
    Matrix4 rotation = identity();
    float c = cos32(radians);
    float s = sin32(radians);
    rotation.at(0, 0) = c;
    rotation.at(0, 1) = s;
    rotation.at(1, 0) = -s;
    rotation.at(1, 1) = c;

    *this *= rotation;
}

Matrix4 Matrix4::operator*(Matrix4 const& other) const
{
    Matrix4 result;

    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {

            result.at(col, row) = (at(0, row) * other.at(col, 0))
                + (at(1, row) * other.at(col, 1))
                + (at(2, row) * other.at(col, 2))
                + (at(3, row) * other.at(col, 3));
        }
    }

    return result;
}

Matrix4 Matrix4::operator*=(Matrix4 const& other)
{
    *this = *this * other;
    return *this;
}

V4 Matrix4::operator*(V4 const& v) const
{
    V4 result;

    result.x = (v.x * at(0, 0))
        + (v.y * at(1, 0))
        + (v.z * at(2, 0))
        + (v.w * at(3, 0));

    result.y = (v.x * at(0, 1))
        + (v.y * at(1, 1))
        + (v.z * at(2, 1))
        + (v.w * at(3, 1));

    result.z = (v.x * at(0, 2))
        + (v.y * at(1, 2))
        + (v.z * at(2, 2))
        + (v.w * at(3, 2));

    result.w = (v.x * at(0, 3))
        + (v.y * at(1, 3))
        + (v.z * at(2, 3))
        + (v.w * at(3, 3));

    return result;
}

float& Matrix4::at(u32 column, u32 row)
{
    return m_flat[(column * 4) + row];
}

float Matrix4::at(u32 column, u32 row) const
{
    return m_flat[(column * 4) + row];
}
