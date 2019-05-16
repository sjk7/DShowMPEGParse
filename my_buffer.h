#pragma once
#ifndef MY_BUFFER_DEFINED__
#define MY_BUFFER_DEFINED__
#include <VECTOR>
#include <ATLBASE.H>

#define DEFAULT_BUFFER_CAPACITY 4096

namespace my{
template <typename T>
class buffer
{
	
	typedef std::vector<T> vec_t;
	vec_t m_vec;
	struct impl{
		size_t read_pos, writepos;
		LONGLONG written, read;
		BOOL at_end;
	};
	
	impl m_impl;

public:
	buffer(size_t reserve_size = DEFAULT_BUFFER_CAPACITY) {
		ASSERT(reserve_size);
		memset(&m_impl, 0, sizeof(m_impl) );
		m_vec.reserve(reserve_size);
	}
	buffer(const buffer&); // do not try to copy me!
	buffer& operator=(const buffer& ); // DO NOT try to copy me

	char* data(){
		if (m_vec.size() == 0){
			ASSERT("trying to get a pointer to an empty vector" == NULL);
			return NULL;
		}
		return &m_vec[0]
	}
	char* data_const () const{ return const_cast<T*>(data());}
	
	struct ptrs{
		T* ptr;
		int size;
	};
	struct slice{
		ptrs ptrs_1, ptrs_2;
	};

	slice get_slice_for_writing(int& n){
		int s = space();
		n > s ? n = s : n = n;
		if (n <= 0) return NULL;

	}
};
}// namespace my
#endif