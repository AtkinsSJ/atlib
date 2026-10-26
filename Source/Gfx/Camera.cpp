/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Camera.h"

#include <Gfx/Renderer.h>
#include <Util/Maths.h>
#include <Util/Rectangle.h>

Camera::Camera(Rect2I screen_rect, float near_clipping_plane, float far_clipping_plane, V2 position)
    : m_position(position)
    , m_screen_rect(screen_rect)
    , m_near_clipping_plane(near_clipping_plane)
    , m_far_clipping_plane(far_clipping_plane)
{
}

Camera::Camera() = default;

void Camera::set_position(V2 new_position)
{
    m_position = new_position;
}

void Camera::move_by(V2 delta)
{
    m_position += delta;
}

void Camera::set_zoom(float new_zoom)
{
    m_zoom = snap_zoom(new_zoom);
}

void Camera::zoom_by(float delta)
{
    m_zoom = snap_zoom(m_zoom + delta);
}

float Camera::snap_zoom(float zoom)
{
    return clamp(round_float(10 * (zoom)) * 0.1f, 0.1f, 10.0f);
}

void Camera::set_screen_rect(Rect2I screen_rect)
{
    m_screen_rect = screen_rect;
}

void Camera::update_mouse_position(V2 input_mouse_position)
{
    m_mouse_position = unproject(input_mouse_position);
}

void Camera::update_projection_matrix()
{
    float left_offset = ((static_cast<float>(m_screen_rect.width()) / 2.0f) + static_cast<float>(m_screen_rect.x())) / m_zoom;
    float top_offset = ((static_cast<float>(m_screen_rect.height()) / 2.0f) + static_cast<float>(m_screen_rect.y())) / m_zoom;

    m_projection_matrix = Matrix4::orthographic(
        m_position.x - left_offset,
        m_position.x + (static_cast<float>(the_renderer().window_width()) / m_zoom) - left_offset,
        m_position.y - top_offset,
        m_position.y + (static_cast<float>(the_renderer().window_height()) / m_zoom) - top_offset,
        m_near_clipping_plane, m_far_clipping_plane);

    m_inverse_projection_matrix = m_projection_matrix.inverse();
}

V2 Camera::unproject(V2 screen_position) const
{
    // Convert into scene space
    V4 unprojected = m_inverse_projection_matrix * v4(screen_position.x, screen_position.y, 0.0f, 1.0f);
    return { unprojected.x, unprojected.y };
}
