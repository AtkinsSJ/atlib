/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/DoublyLinkedList.h>

// Items are sorted so that the lowest number is the highest priority, eg 1, 2, 3, 4, 5
// This matches the current 1 use case, so I'll worry about making the sort order optional later.
template<typename Item, typename Priority = u64>
class PriorityQueue {
    struct Node : DoublyLinkedListNode<Node> {
        Node(Item&& item, Priority priority)
            : item(move(item))
            , priority(move(priority))
        {
        }
        Item item;
        Priority priority;
    };

public:
    ~PriorityQueue()
    {
        clear();
    }

    u32 count() const { return m_list.count(); }
    bool is_empty() const { return m_list.is_empty(); }

    void add(Item item, Priority priority)
    {
        auto& node = *new Node(move(item), priority);

        // Insert the new node into the list, before the first one with a bigger priority.
        // If there are existing nodes with the same priority, this new one comes after.
        // eg, if we want to insert a priority 3 into this list:
        // 1, 2, 3, 3, 3, 4, 5
        //               ^- it gets inserted here, before the first 4.
        for (auto& existing_node : m_list) {
            if (existing_node.priority > priority) {
                existing_node.insert_other_before_this(node);
                return;
            }
        }
        // If we got here, `priority` is bigger than any existing one, so add it to the end.
        m_list.add(node);
    }

    Item take_next()
    {
        auto& node = m_list.remove_first();
        auto item = move(node.item);
        delete &node;
        return item;
    }

    struct ItemAndPriority {
        Item item;
        Priority priority;
    };
    ItemAndPriority take_next_with_priority()
    {
        auto& node = m_list.remove_first();
        ItemAndPriority result {
            .item = move(node.item),
            .priority = move(node.priority),
        };
        delete &node;
        return result;
    }

    void clear()
    {
        while (!m_list.is_empty())
            (void)take_next();
    }

    // FIXME: These are really just here for testing convenience.
    //        A proper iterator would only expose the Item and Priority, not the whole DLL node.
    DoublyLinkedList<Node>::ConstIterator begin() const { return m_list.begin(); }
    DoublyLinkedList<Node>::ConstIterator end() const { return m_list.end(); }

private:
    DoublyLinkedList<Node> m_list;
};
