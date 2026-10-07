/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Assert.h>
#include <Util/Badge.h>
#include <Util/Basic.h>

template<typename T>
class DoublyLinkedList;

template<typename T>
class DoublyLinkedListNode {
    friend DoublyLinkedList<T>;

public:
    void insert_other_before_this(DoublyLinkedListNode<T>& other)
    {
        auto* old_previous_node = m_previous_node;
        other.m_previous_node = old_previous_node;
        old_previous_node->m_next_node = &other;

        other.m_next_node = this;
        m_previous_node = &other;
    }

private:
    DoublyLinkedListNode* m_previous_node { this };
    DoublyLinkedListNode* m_next_node { this };
};

template<typename T>
class DoublyLinkedList {
public:
    u32 count() const
    {
        u32 count = 1;

        for (auto* node = m_sentinel.m_next_node; node != &m_sentinel; node = node->m_next_node) {
            count++;
        }

        return count;
    }

    bool is_empty() const
    {
        return m_sentinel.m_next_node == &m_sentinel;
    }

    void add(DoublyLinkedListNode<T>& node)
    {
        m_sentinel.insert_other_before_this(node);
    }

    void remove(DoublyLinkedListNode<T>& node)
    {
        auto* previous_node = node.m_previous_node;
        auto* next_node = node.m_next_node;

        previous_node->m_next_node = next_node;
        next_node->m_previous_node = previous_node;

        node.m_previous_node = &node;
        node.m_next_node = &node;
    }

    T& remove_first()
    {
        ASSERT(!is_empty());
        auto& node = *m_sentinel.m_next_node;
        remove(node);
        return static_cast<T&>(node);
    }

    T& remove_last()
    {
        ASSERT(!is_empty());
        auto& node = *m_sentinel.m_previous_node;
        remove(node);
        return static_cast<T&>(node);
    }

    void take_all(DoublyLinkedList& other)
    {
        if (other.is_empty())
            return;

        auto* previous_last_node = m_sentinel.m_previous_node;
        auto* other_first_node = other.m_sentinel.m_next_node;
        auto* other_last_node = other.m_sentinel.m_previous_node;

        // Insert other's items between our last node and our sentinel.
        other_first_node->m_previous_node = previous_last_node;
        previous_last_node->m_next_node = other_first_node;

        other_last_node->m_next_node = &m_sentinel;
        m_sentinel.m_previous_node = other_last_node;

        // Clear other
        other.m_sentinel.m_previous_node = &other.m_sentinel;
        other.m_sentinel.m_next_node = &other.m_sentinel;
    }

    template<typename ValueT, typename NodeT>
    class Iterator {
    public:
        Iterator(Badge<DoublyLinkedList>, NodeT& node)
            : m_current_node(&node)
        {
        }

        ValueT& operator*() { return static_cast<ValueT&>(*m_current_node); }
        ValueT* operator->() { return static_cast<ValueT*>(m_current_node); }

        Iterator& operator++()
        {
            m_current_node = m_current_node->m_next_node;
            return *this;
        }

        Iterator operator++(int)
        {
            auto result = *this;
            ++(*this);
            return result;
        }

        Iterator& operator--()
        {
            m_current_node = m_current_node->m_previous_node;
            return *this;
        }

        Iterator operator--(int)
        {
            auto result = *this;
            --(*this);
            return result;
        }

        bool operator==(Iterator const& other)
        {
            return m_current_node == other.m_current_node;
        }

        bool operator!=(Iterator const& other)
        {
            return !(*this == other);
        }

    private:
        NodeT* m_current_node;
    };

    using MutableIterator = Iterator<T, DoublyLinkedListNode<T>>;
    using ConstIterator = Iterator<T const, DoublyLinkedListNode<T> const>;

    MutableIterator begin()
    {
        return MutableIterator({}, *m_sentinel.m_next_node);
    }

    MutableIterator end()
    {
        return MutableIterator({}, m_sentinel);
    }

    ConstIterator begin() const
    {
        return ConstIterator({}, *m_sentinel.m_next_node);
    }

    ConstIterator end() const
    {
        return ConstIterator({}, m_sentinel);
    }

private:
    DoublyLinkedListNode<T> m_sentinel;
};
