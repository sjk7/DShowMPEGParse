#pragma once
#include <vector>
#include "my_debug.h"

// my_membuf.h
namespace my {

template <typename T> class mem_buf {

    typedef std::vector<T> stor_t;
    stor_t m_v;

    public:
    typedef T type;
    typedef T* ptr_type;

    mem_buf(size_t capacity = 8192) { m_v.reserve(capacity); }
    mem_buf(const mem_buf& rhs);
    mem_buf& operator=(const mem_buf& rhs);

    size_t capacity() const noexcept { return m_v.capacity(); }
    void resize(size_t new_size) { m_v.resize(new_size); }
    size_t size() const noexcept { return m_v.size(); }
};

template <typename BUF> struct mem_view {
    private:
    BUF& m_buf;
    size
};

} // namespace my
