/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Concepts.h>
#include <Util/Forward.h>

template<typename T>
class Position {
public:
    Position(T x, T y)
        : m_x(x)
        , m_y(y)
    {
    }

    template<typename U>
    requires(IsConvertibleTo<U, T>)
    Position(U x, U y)
        : m_x(x)
        , m_y(y)
    {
    }

    T x() const { return m_x; }
    T y() const { return m_y; }

    void set_x(T value) { m_x = value; }
    void set_y(T value) { m_y = value; }

private:
    T m_x;
    T m_y;
};
