// author: wh1t3lord


#ifndef SOLUNAR_CORE_CONTAINERS_STRING_H
#define SOLUNAR_CORE_CONTAINERS_STRING_H

#include <string>
#include <memory_resource>
#include <utility>

namespace solunar
{
	template<typename Type, std::size_t ElementCount, bool IsRealocatable, std::size_t _kBufferSize
#ifdef _DEBUG
		= sizeof(Type)* ElementCount * 2
#else
		= sizeof(Type) * ElementCount
#endif

	>
	class hybrid_string
	{
		using container_type = std::pmr::basic_string<Type>;
		using container_view_type = std::basic_string_view<Type>;

	public:
		using value_type = typename container_type::value_type;
		using size_type = typename container_type::size_type;
		using difference_type = typename container_type::difference_type;
		using reference = typename container_type::reference;
		using const_reference = typename container_type::const_reference;
		using pointer = typename container_type::pointer;
		using const_pointer = typename container_type::const_pointer;
		using iterator = typename container_type::iterator;
		using const_iterator = typename container_type::const_iterator;
		using reverse_iterator = typename container_type::reverse_iterator;
		using const_reverse_iterator = typename container_type::const_reverse_iterator;
		using allocator_type = typename container_type::allocator_type;

		template< class InputIt >
		hybrid_string(InputIt first, InputIt last) : m_pool{ m_memory, _kBufferSize, IsRealocatable ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, str{ first,last, &m_pool }
		{

		}


		hybrid_string(const Type* s, size_type count) : m_pool{ m_memory, _kBufferSize, IsRealocatable ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, str{ s, count, &m_pool }
		{

		}

		hybrid_string(const Type* s) : m_pool{ m_memory, _kBufferSize, IsRealocatable ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, str{ s, &m_pool }
		{

		}


		template< class StringViewLike >
		explicit hybrid_string(const StringViewLike& t) : m_pool{ m_memory, _kBufferSize, IsRealocatable ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, str{ t, &m_pool }
		{

		}

		template< class StringViewLike >
		hybrid_string(const StringViewLike& t,
			size_type pos, size_type count) : m_pool{ m_memory, _kBufferSize, IsRealocatable ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, str{ t,pos,count,&m_pool }
		{
		}

		//hybrid_string(const hybrid_string& other);
		//hybrid_string(hybrid_string&& other) noexcept;

	//	hybrid_string(const hybrid_string& other, const allocator_type& alloc);

	//	hybrid_string(hybrid_string&& other, const allocator_type& alloc);

	//	hybrid_string(const hybrid_string& other, size_type pos,
	//	const allocator_type& alloc = allocator_type());

		//	hybrid_string(const hybrid_string& other,
		//		size_type pos, size_type count,
		//		const allocator_type& alloc = allocator_type());

		hybrid_string(std::initializer_list<Type> ilist) : m_pool{ m_memory, _kBufferSize, IsRealocatable ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, str{ ilist, &m_pool }
		{

		}


		hybrid_string() : m_pool{ m_memory, _kBufferSize, IsRealocatable ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, str{ &m_pool }
		{
			str.reserve(ElementCount);
		}
		~hybrid_string() {}

		inline const_iterator begin() const { return str.begin(); }
		inline const_iterator end() const { return str.end(); }

		inline const_iterator cbegin() const { return str.cbegin(); }
		inline const_iterator cend() const { return str.cend(); }

		inline reverse_iterator rbegin() { return str.rbegin(); }
		inline reverse_iterator rend() { return str.rend(); }

		inline const_reverse_iterator rbegin() const { return str.rbegin(); }
		inline const_reverse_iterator rend() const { return str.rend(); }

		inline const_reverse_iterator crbegin() const { return str.crbegin(); }
		inline const_reverse_iterator crend() const { return str.crend(); }

		inline const value_type* c_str(void) const { return str.c_str(); }
		inline size_type max_size(void) const { return str.max_size(); }
		inline bool empty(void) const { return str.empty(); }
		inline reference at(size_type index)
		{
			return str.at(index);
		}

		inline const_reference at(const size_type index) const {
			return str.at(index);
		}

		hybrid_string substr(size_type pos = 0, size_type count = npos) const
		{
			return str.substr(pos, count);
		}

		size_type copy(Type* dest, size_type count, size_type pos = 0) const
		{
			return str.copy(dest, count, pos);
		}

		size_type find(const hybrid_string& istr, size_type pos = 0) const
		{
			return str.find(istr.str, pos);
		}

		size_type find(const container_type& istr, size_type pos = 0) const
		{
			return str.find(istr, pos);
		}

		size_type find(const Type* s, size_type pos, size_type count) const
		{
			return str.find(s, pos, count);
		}

		size_type find(const Type* s, size_type pos = 0) const
		{
			return str.find(s, pos);
		}

		size_type find(Type ch, size_type pos = 0) const
		{
			return str.find(ch, pos);
		}

		template< class StringViewLike >
		size_type find(const StringViewLike& t,
			size_type pos = 0) const noexcept
		{
			return str.find<StringViewLike>(t, pos);
		}

		size_type rfind(const hybrid_string& istr, size_type pos = container_type::npos) const
		{
			return str.rfind(istr.str, pos);
		}

		size_type rfind(const container_type& str, size_type pos = container_type::npos) const
		{
			return str.rfind(str, pos);
		}

		size_type rfind(const Type* s, size_type pos, size_type count) const
		{
			return str.rfind(s, pos, count);
		}

		size_type rfind(const Type* s, size_type pos = container_type::npos) const
		{
			return str.rfind(s, pos);
		}


		size_type rfind(Type ch, size_type pos = container_type::npos) const
		{
			return str.rfind(ch, pos);
		}

		template< class StringViewLike >
		size_type rfind(const StringViewLike& t,
			size_type pos = npos) const noexcept
		{
			return str.rfind<StringViewLike>(t, pos);
		}

		size_type find_first_of(const hybrid_string& istr, size_type pos = 0) const
		{
			return str.find_first_of(istr.str, pos);
		}

		size_type find_first_of(const container_type& str, size_type pos = 0) const { return str.find_first_of(str, pos); }

		size_type find_first_of(const Type* s,
			size_type pos, size_type count) const
		{
			return str.find_first_of(s, pos, count);
		}

		size_type find_first_of(const Type* s, size_type pos = 0) const
		{
			return str.find_first_of(s, pos);
		}

		size_type find_first_of(Type ch, size_type pos = 0) const
		{
			return str.find_first_of(ch, pos);
		}

		template< class StringViewLike >
		size_type find_first_of(const StringViewLike& t,
			size_type pos = 0) const noexcept
		{
			return str.find_first_of<StringViewLike>(t, pos);
		}

		size_type find_first_not_of(const hybrid_string& istr,
			size_type pos = 0) const
		{
			return str.find_first_not_of(istr.str, pos);
		}

		size_type find_first_not_of(const container_type& str, size_type pos = 0) const
		{
			return str.find_first_not_of(str, pos);
		}

		size_type find_first_not_of(const Type* s,
			size_type pos, size_type count) const
		{
			return str.find_first_not_of(s, pos, count);
		}

		size_type find_first_not_of(const Type* s,
			size_type pos = 0) const
		{
			return str.find_first_not_of(s, pos);
		}

		size_type find_first_not_of(Type ch, size_type pos = 0) const
		{
			return str.find_first_not_of(ch, pos);
		}

		template< class StringViewLike >
		size_type
			find_first_not_of(const StringViewLike& t,
				size_type pos = 0) const noexcept
		{
			return str.find_first_not_of<StringViewLike>(t, pos);
		}

		size_type find_last_of(const hybrid_string& istr,
			size_type pos = container_type::npos) const
		{
			return str.find_last_of(istr.str, pos);
		}

		size_type find_last_of(const container_type& str, size_type pos = container_type::npos) const
		{
			return str.find_last_of(str, pos);
		}

		size_type find_last_of(const Type* s,
			size_type pos, size_type count) const
		{
			return str.find_last_of(s, pos, count);
		}

		size_type find_last_of(const Type* s, size_type pos = container_type::npos) const
		{
			return str.find_last_of(s, pos);
		}

		size_type find_last_of(Type ch, size_type pos = container_type::npos) const
		{
			return str.find_last_of(ch, pos);
		}

		template< class StringViewLike >
		size_type
			find_last_of(const StringViewLike& t,
				size_type pos = container_type::npos) const noexcept
		{
			return str.find_last_of<StringViewLike>(t, pos);
		}

		size_type find_last_not_of(const hybrid_string& istr,
			size_type pos = container_type::npos) const
		{
			return str.find_last_not_of(istr.str, pos);
		}

		size_type find_last_not_of(const container_type& str, size_type pos = container_type::npos) const
		{
			return str.find_last_not_of(str, pos);
		}

		size_type find_last_not_of(const Type* s,
			size_type pos, size_type count) const
		{
			return str.find_last_not_of(s, pos, count);
		}


		size_type find_last_not_of(const Type* s, size_type pos = container_type::npos) const
		{
			return str.find_last_not_of(s, pos);
		}

		size_type find_last_not_of(Type ch, size_type pos = container_type::npos) const
		{
			return str.find_last_not_of(ch, pos);
		}

		template< class StringViewLike >
		size_type
			find_last_not_of(const StringViewLike& t,
				size_type pos = container_type::npos) const noexcept
		{
			return str.find_last_not_of<StringViewLike>(t, pos);
		}

		// non-const
		inline iterator begin() { return str.begin(); }
		inline iterator end() { return str.end(); }

		inline void clear(void) { str.clear(); }
		inline pointer data(void) { return str.data(); }
		inline size_type size(void) const
		{
			return str.size();
		}

		size_type length() const { return str.length(); }

		hybrid_string& append(size_type count, Type ch)
		{
			str.append(count, ch);
			return *this;
		}

		hybrid_string& append(const Type* s, size_type count)
		{
			str.append(s, count);
			return *this;
		}

		hybrid_string& append(const Type* s)
		{
			str.append(s);
			return *this;
		}

		template< class SV >
		hybrid_string& append(const SV& t)
		{
			str.append<SV>(t);
			return *this;
		}

		template< class SV >
		hybrid_string& append(const SV& t, size_type pos,
			size_type count = container_type::npos)
		{
			str.append<SV>(t, pos, count);
			return *this;
		}

		hybrid_string& append(const hybrid_string& istr)
		{
			str.append(istr.str);
			return *this;
		}

		hybrid_string& append(const container_type& istr)
		{
			str.append(istr);
			return *this;
		}

		hybrid_string& append(const hybrid_string& istr, size_type pos, size_type count = container_type::npos)
		{
			str.append(istr.str, pos, count);
			return *this;
		}

		hybrid_string& append(const container_type& istr, size_type pos, size_type count = container_type::npos)
		{
			str.append(istr, pos, count);
			return *this;
		}

		template< class InputIt >
		hybrid_string& append(InputIt first, InputIt last)
		{
			str.append<InputIt>(first, last);
			return *this;
		}

		hybrid_string& append(std::initializer_list<Type> ilist)
		{
			str.append(ilist);
			return *this;
		}

		inline void swap(const hybrid_string& istr)
		{
			return str.swap(istr.str);
		}

		inline void swap(const container_type& istr)
		{
			return str.swap(istr);
		}

		inline void push_back(Type c)
		{
			str.push_back(c);
		}

		inline void pop_back()
		{
			str.pop_back();
		}

		hybrid_string& operator+=(const hybrid_string& istr)
		{
			str.operator+=(istr.str);
			return *this;
		}

		hybrid_string& operator+=(const container_type& istr)
		{
			str.operator+=(istr);
			return *this;
		}

		hybrid_string& operator+=(Type ch)
		{
			str.operator+=(ch);
			return *this;
		}

		hybrid_string& operator+=(const Type* s)
		{
			str.operator+=(s);
			return *this;
		}

		hybrid_string& operator+=(std::initializer_list<Type> ilist)
		{
			str.operator+=(ilist);
			return *this;
		}

		template< class StringViewLike >
		hybrid_string& operator+=(const StringViewLike& t)
		{
			str.operator+=(t);
			return *this;
		}

		hybrid_string& operator=(const hybrid_string& istr)
		{
			str.operator=(istr.str);
			return *this;
		}

		hybrid_string& operator=(const container_type& istr)
		{
			str.operator=(istr);
			return *this;
		}

		hybrid_string& operator=(hybrid_string&& istr) noexcept
		{
			str.operator=(std::move(istr.str));
			return *this;
		}
		hybrid_string& operator=(container_type&& istr) noexcept
		{
			str.operator=(std::move(istr));
			return *this;
		}

		template<size_type Size>
		hybrid_string& operator=(const Type(&test)[Size])
		{
			this->operator=(static_cast<const Type*>(test));
			return *this;
		}

		hybrid_string& operator=(const Type* s)
		{
			str.operator=(s);
			return *this;
		}

		hybrid_string& operator=(Type ch)
		{
			str.operator=(ch);
			return *this;
		}

		hybrid_string& operator=(std::initializer_list<Type> ilist)
		{
			str.operator=(ilist);
			return *this;
		}

		template<class StringViewLike>
		hybrid_string& operator=(const StringViewLike& t)
		{
			str.operator=<StringViewLike>(t);
			return *this;
		}

		operator container_view_type() const noexcept
		{
			return container_view_type(str);
		}

		allocator_type get_allocator() const noexcept { return str.get_allocator(); }

		value_type& front() { return str.front(); }
		const value_type& front() const { return str.front(); }

		value_type& back() { return str.back(); }
		const value_type& back() const { return str.back(); }

		inline reference operator[](size_type pos) noexcept { return str.operator[](pos); }
		inline const_reference operator[](size_type pos) const noexcept { return str.operator[](pos); }

		container_type& container() noexcept { return str; }
		const container_type& container() const noexcept { return str; }

		constexpr std::size_t preallocated_memory_size() const noexcept { return _kBufferSize; }
		constexpr std::size_t preallocated_size() const noexcept { return ElementCount; }
		constexpr bool is_reallocation_supported() const noexcept { return IsRealocatable; }
	private:
		unsigned char m_memory[_kBufferSize];
		std::pmr::monotonic_buffer_resource m_pool;
		container_type str;
	};

	template<std::size_t Length>
	using string = hybrid_string<char, Length, true>;

	template<std::size_t Length>
	using static_string = hybrid_string<char, Length, false>;
}

template<std::size_t E>
bool operator==(const solunar::string<E>& lhs,
	const solunar::string<E>& rhs)
{
	return std::operator==(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator==(const solunar::string<E>& lhs,
	const solunar::static_string<E>& rhs)
{
	return std::operator==(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator==(const solunar::static_string<E>& lhs,
	const solunar::string<E>& rhs)
{
	return std::operator==(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator!=(const solunar::string<E>& lhs,
	const solunar::string<E>& rhs)
{
	return std::operator!=(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator!=(const solunar::static_string<E>& lhs,
	const solunar::string<E>& rhs)
{
	return std::operator!=(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator!=(const solunar::string<E>& lhs,
	const solunar::static_string<E>& rhs)
{
	return std::operator!=(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator< (const solunar::string<E>& lhs,
	const solunar::string<E>& rhs)
{
	return std::operator<(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator< (const solunar::static_string<E>& lhs,
	const solunar::string<E>& rhs)
{
	return std::operator<(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator< (const solunar::string<E>& lhs,
	const solunar::static_string<E>& rhs)
{
	return std::operator<(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator<=(const solunar::string<E>& lhs,
	const solunar::string<E>& rhs)
{
	return operator<=(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator<=(const solunar::static_string<E>& lhs,
	const solunar::string<E>& rhs)
{
	return operator<=(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator<=(const solunar::string<E>& lhs,
	const solunar::static_string<E>& rhs)
{
	return operator<=(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator> (const solunar::string<E>& lhs,
	const solunar::string<E>& rhs)
{
	return operator>(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator> (const solunar::static_string<E>& lhs,
	const solunar::string<E>& rhs)
{
	return operator>(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator> (const solunar::string<E>& lhs,
	const solunar::static_string<E>& rhs)
{
	return operator>(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator>=(const solunar::string<E>& lhs,
	const solunar::string<E>& rhs)
{
	return operator>=(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator>=(const solunar::static_string<E>& lhs,
	const solunar::string<E>& rhs)
{
	return operator>=(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator>=(const solunar::string<E>& lhs,
	const solunar::static_string<E>& rhs)
{
	return operator>=(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator==(const solunar::static_string<E>& lhs,
	const solunar::static_string<E>& rhs)
{
	return std::operator==(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator!=(const solunar::static_string<E>& lhs,
	const solunar::static_string<E>& rhs)
{
	return std::operator!=(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator< (const solunar::static_string<E>& lhs,
	const solunar::static_string<E>& rhs)
{
	return std::operator<(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator<=(const solunar::static_string<E>& lhs,
	const solunar::static_string<E>& rhs)
{
	return operator<=(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator> (const solunar::static_string<E>& lhs,
	const solunar::static_string<E>& rhs)
{
	return operator>(lhs.container(), rhs.container());
}

template<std::size_t E>
bool operator>=(const solunar::static_string<E>& lhs,
	const solunar::static_string<E>& rhs)
{
	return operator>=(lhs.container(), rhs.container());
}

#endif