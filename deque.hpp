#pragma once

//---------------------------------------------------------------------------//
//
// deque.hpp
//  両端キュー
//   Copyright (C) tapetums 2016
//
//---------------------------------------------------------------------------//

namespace tapetums { template<typename T> class deque; }

//---------------------------------------------------------------------------//

template<typename T> class tapetums::deque
{
private:
    struct node
    {
        T     data;
        node* prev { nullptr };
        node* next { nullptr };

        explicit node(const T& data) : data(data) { }
        explicit node(T&& data)      : data(std::move(data)) { }
    };

private:
    size_t m_size  { 0 };
    node*  m_front { nullptr };
    node*  m_back  { nullptr };

public:
    struct iterator
    {
        node* n;
        explicit iterator(node* n) : n(n) { }

        T&   operator * () { return n->data; }
        T*   operator ->() { return &n->data; }
        void operator ++() { n = n->next; }
        bool operator !=(const iterator& lhs) { return n != lhs.n; }
    };

    iterator begin() { return iterator(m_front); }
    iterator end()   { return iterator(nullptr); }

public:
    deque() : m_front(nullptr), m_back(nullptr) { }
    ~deque() { clear(); }

public:
    bool   empty() const noexcept { return m_size == 0; }
    size_t size()  const noexcept { return m_size; }

    const T& front() const { return m_front->data; }
    const T& back()  const { return m_back->data; }

    T& front() { return m_front->data; }
    T& back()  { return m_back->data; }

public:
    void push_front(const T& data)
    {
        auto n = new node(data);
        n->prev = nullptr;
        n->next = m_front;

        if ( m_front ) { m_front->prev = n; }
        else           { m_back = n; }

        m_front = n;
        ++m_size;
    }

    void push_front(T&& data)
    {
        auto n = new node(std::move(data));
        n->prev = nullptr;
        n->next = m_front;

        if ( m_front ) { m_front->prev = n; }
        else           { m_back = n; }

        m_front = n;
        ++m_size;
    }

    void push_back(const T& data)
    {
        auto n = new node(data);
        n->prev = m_back;
        n->next = nullptr;

        if ( m_back ) { m_back->next = n; }
        else          { m_front = n; }

        m_back = n;
        ++m_size;
    }

    void push_back(T&& data)
    {
        auto n = new node(std::move(data));
        n->prev = m_back;
        n->next = nullptr;

        if ( m_back ) { m_back->next = n; }
        else          { m_front = n; }

        m_back = n;
        ++m_size;
    }

    void pop_front()
    {
        if ( nullptr == m_front ) { return; }

        auto n = m_front;

        m_front->prev = nullptr;
        m_front = n->next;

        if ( n == m_back ) { m_back = nullptr; }

        delete n;
        --m_size;
    }

    void pop_back()
    {
        if ( nullptr == m_back ) { return; }

        auto n = m_back;

        m_back->prev->next = nullptr;
        m_back = m_back->prev;

        if ( n == m_front ) { m_front = nullptr; }

        delete n;
        --m_size;
    }

    void clear()
    {
        while ( size() ) { pop_front(); }
    }

    iterator erase(const iterator& it)
    {
        auto n = it.n;

        if ( n->prev ) { n->prev->next = n->next; }
        if ( n->next ) { n->next->prev = n->prev; }

        if ( n == m_front ) { m_front = n->next; }
        if ( n == m_back )  { m_back  = n->prev; }

        auto next = n->next;
        delete n;
        --m_size;

        return iterator(next);
    }
};

//---------------------------------------------------------------------------//

// deque.hpp