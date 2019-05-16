#pragma once
#ifndef MY_BUFFER_DEFINED__
#define MY_BUFFER_DEFINED__

#include <VECTOR>
#include <ATLBASE.H>

#ifndef TRACE
#define TRACE ATLTRACE
#endif

#ifndef MY_DEFAULT_BUFFER_CAPACITY
#define MY_DEFAULT_BUFFER_CAPACITY 4096
#endif

namespace my{

	template<class T> 
	const T& mymin(const T& a, const T& b)
	{
		return (b < a) ? b : a;
	}

	struct buf_state_t{
		int read_pos, write_pos;
		LONGLONG written, read;
		BOOL at_end;
	};

	template <typename T>
	struct slice_method_read {
		T& m_buf;
		buf_state_t& m_state;

		slice_method_read(T& buf) : m_buf(buf), m_state(buf.m_state){};
		slice_method_read(const slice_method_read& rhs) : m_buf(rhs.buf), m_state(rhs.m_state){}

		int nbytes_avail() const {
			int n = m_buf.delta();
			if (n <= 0) {
				
				TRACE("Nothing to read in buffer!\n");
				if (n < 0){
					ASSERT("Deadly calculation in bytes_avail!\n" == NULL);
				}
				return 0;
			}

			return n;
		}

		void pos_set(const int newpos){
			m_state.read_pos = newpos % m_buf.size();
		}
		int pos() const{
			return m_state.read_pos;
		}
	};
	template <typename T>
	struct slice_method_write{
		T& m_buf;
		buf_state_t& m_state;
		
		slice_method_write(T& buf) : m_buf(buf), m_state(buf.m_state){};
		slice_method_write(const slice_method_read& rhs) : m_buf(rhs.buf), m_state(rhs.m_state){}
		slice_method_write(); // no impl;
		slice_method_write& operator=(const slice_method_write&); // no impl

		int nbytes_avail() const {
			int n = m_buf.space();
			if (n <= 0) {
				ASSERT("No room in buffer!\n" == NULL);
				TRACE("No more room in buffer!\n");
				return 0;
			}
			return n;
		}

		
		void pos_set(const int newpos){
			m_state.write_pos = newpos % m_buf.size();
		}
		int pos() const{
			return m_state.write_pos;
		}
	};

	template <typename T>
	class buffer
	{

		typedef std::vector<T> vec_t;
		vec_t m_vec;


	public:

		buf_state_t m_state;
		buffer(size_t reserve_size = MY_DEFAULT_BUFFER_CAPACITY) {
			ASSERT(sizeof(T) == 1); // didn't really build it with other types' sizes in mind.
			ASSERT(reserve_size);
			memset(&m_state, 0, sizeof(m_state) );
			m_vec.resize(reserve_size);
		}
		buffer(const buffer&); // do not try to copy me!
		buffer& operator=(const buffer& ); // DO NOT try to copy me


		T* data(){
			if (m_vec.size() == 0){
				ASSERT("trying to get a pointer to an empty vector" == NULL);
				return NULL;
			}
			return &m_vec[0];
		}
		T* data_const () const {
			buffer* naughty_me = const_cast<buffer*>(this);
			return  (T*)(naughty_me->data());
		}
		void clear() { m_vec.clear(); memset(&m_state, 0, sizeof(impl)); }


		inline	int size() const{
			return (int)m_vec.size();
		}
		// get count of available Ts to read:
		inline int delta() const{
			return (int)(m_state.written - m_state.read);
		}
		// get space for writing:
		inline int space() const {
			return (int)m_vec.size() - delta(); 
		}

		// non owning ptr range:
		struct ptrs{
			ptrs(T* p, int size_in_bytes) : ptr(p), size(size_in_bytes){}
			ptrs() : ptr(0), size(0){}
			ptrs(const ptrs& rhs): ptr(rhs.ptr), size(rhs.size){}
			ptrs& operator=(const ptrs& rhs){
				ptr = rhs.ptr;
				size = rhs.size;
			}
			T* ptr;
			int size;
		};
		struct slice{

			ptrs ptrs_1, ptrs_2;
		};

		template <typename M>
		inline slice get_slice(M& m , int nwanted){

			int spare = m.nbytes_avail();
			if (!spare){
				memset(&m, 0, sizeof(m));
			}
			spare = mymin(nwanted, spare);


			slice sli;
			const int pos = m.pos();
			if (pos + spare > size()){
				sli.ptrs_2.size = -1;
			}else{
				sli.ptrs_2.size= 0;
			}
			{
				sli.ptrs_1.size = spare;
				ASSERT(pos >= 0 && pos <= m_vec.size());
				sli.ptrs_1.ptr = &m_vec[pos];
				ASSERT(sli.ptrs_1.size > 0);
				
				m.pos_set(pos + sli.ptrs_1.size);
				m_state.written += sli.ptrs_1.size;
			}
			if (sli.ptrs_2.size == -1){
				sli.ptrs_2.size = spare - sli.ptrs_1.size;
				ASSERT(sli.ptrs_2.size > 0);
				sli.ptrs_2.ptr = data() + m.pos();
				m.pos_set(pos + sli.ptrs_2.size);
				m_state.written += sli.ptrs_2.size;
			}
			ASSERT(m.pos() < size());
			ASSERT(sli.ptrs_1.size + sli.ptrs_2.size <= size());

			return sli;
		}


	};

#ifdef _DEBUG
	void buffer_test(){
		typedef my::buffer<BYTE> buf_t;
		buf_t buf;
		ASSERT(MY_DEFAULT_BUFFER_CAPACITY == buf.size());
		ASSERT(buf.data());
		ASSERT(buf.data_const());

		typedef buf_t::ptrs ptrs_t;
		typedef buf_t::slice slice_t;
		typedef my::slice_method_read<buf_t> read_method_t;
		typedef my::slice_method_write<buf_t> write_method_t; 

		read_method_t read_method(buf);
		write_method_t write_method(buf);

		typedef buf_t::slice slice_t;


		slice_t write_slice = buf.get_slice(write_method, 77);
		ASSERT(write_slice.ptrs_1.ptr && ! write_slice.ptrs_2.ptr);
		ASSERT(write_slice.ptrs_1.size == 77);
		const char* hello = "Hello, World!";
		memcpy(write_slice.ptrs_1.ptr, hello, strlen(hello) + 1);

		slice_t read_slice = buf.get_slice(read_method , 77);
		ASSERT(read_slice.ptrs_1.size == 77 &&
			read_slice.ptrs_2.size == 0 && read_slice.ptrs_1.ptr != 0
			&& read_slice.ptrs_2.ptr == 0);

		int mem = memcmp(read_slice.ptrs_1.ptr, hello, strlen(hello) + 1);
		ASSERT(mem == 0);

	}
#else
	void buffer_test(){}
#endif
}// namespace my
#endif