/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Concepts.h>
#include <Util/Forward.h>
#include <Util/Vector.h>

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

    bool operator==(Position const& other) const
    {
        return m_x == other.m_x
            && m_y == other.m_y;
    }

    operator V2I() const
    requires(IsIntegral<T>)
    {
        return { static_cast<s32>(m_x), static_cast<s32>(m_y) };
    }

    operator V2() const
    {
        return { static_cast<float>(m_x), static_cast<float>(m_y) };
    }

private:
    T m_x;
    T m_y;
};
