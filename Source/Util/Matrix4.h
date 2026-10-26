/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Forward.h>

//
// Matrix4
//
// Matrices! Pretty sure I'm only going to use this for the camera projection,
// so certain things are missing - notably, we can only rotate in the z-axis
// because (for now) the game will be in 2D.
//
// TODO: When I move to 3D, implement full rotation of matrices!
//
class Matrix4 {
public:
    static Matrix4 identity();
    static Matrix4 orthographic(float left, float right, float top, float bottom, float near_clip, float far_clip);

    Matrix4 inverse() const;

    void translate(V3 const&);
    void scale(V3 const&);
    void rotate_z(float radians);

    Matrix4 operator*(Matrix4 const& other) const;
    Matrix4 operator*=(Matrix4 const& other);
    V4 operator*(V4 const&) const;

    float& at(u32 column, u32 row);
    float at(u32 column, u32 row) const;
    float const* flat() const { return m_flat; }

private:
    float m_flat[16] {}; // Column-major order, so equivalent to [COLUMN][ROW]
};
