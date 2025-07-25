// author: wh1t3lord

#ifndef SOLUNAR_CORE_CONTAINERS_VECTOR_H
#define SOLUNAR_CORE_CONTAINERS_VECTOR_H

#include <vector>
#include <memory_resource>

namespace sr
{
	template<typename Type, std::size_t ElementCount, bool IsRealocatable,
		std::size_t _kBufferSize
#ifdef _DEBUG
		= sizeof(Type)* ElementCount * 2
#else
		= sizeof(Type) * ElementCount
#endif
	>
	class hybrid_vector
	{
		using container_type = std::pmr::vector<Type>;

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


		hybrid_vector() : pool{ memory, _kBufferSize, IsRealocatable ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, vec{ &pool }
		{
			vec.reserve(ElementCount);
		}

		virtual ~hybrid_vector()
		{
		}

		reference at(size_type pos) { return vec.at(pos); }
		const_reference at(size_type pos) const { return vec.at(pos); }
		reference operator[](size_type pos) { return vec[pos]; }
		const_reference operator[](size_type pos) const { return vec[pos]; }
		reference front() { return vec.front(); }
		const_reference front() const { return vec.front(); }
		reference back() { return vec.back(); }
		const_reference back() const { return vec.back(); }
		pointer data() noexcept { return vec.data(); }
		const_pointer data() const noexcept { return vec.data(); }

		iterator begin() noexcept { return vec.begin(); }
		const_iterator begin() const noexcept { return vec.begin(); }
		const_iterator cbegin() const noexcept { return vec.cbegin(); }

		iterator end() noexcept { return vec.end(); }
		const_iterator end() const noexcept { return vec.end(); }
		const_iterator cend() const noexcept { return vec.cend(); }

		reverse_iterator rbegin() noexcept { return vec.rbegin(); }
		const_reverse_iterator rbegin() const noexcept { return vec.rbegin(); }
		const_reverse_iterator crbegin() const noexcept { return vec.crbegin(); }

		reverse_iterator rend() noexcept { return vec.rend(); }
		const_reverse_iterator rend() const noexcept { return vec.rend(); }
		const_reverse_iterator crend() const noexcept { return vec.crend(); }

		bool empty() const noexcept { return vec.empty(); }
		size_type size() const noexcept { return vec.size(); }
		size_type max_size() const noexcept { return vec.max_size(); }
		void reserve(size_type new_cap) { vec.reserve(new_cap); }
		size_type capacity() const noexcept { return vec.capacity(); }
		void shrink_to_fit() { vec.shrink_to_fit(); }

		void clear() noexcept { vec.clear(); }

		template<typename... Args>
		reference emplace_back(Args&&... args) {
			return vec.emplace_back(std::forward<Args>(args)...);
		}

		template< class... Args >
		iterator emplace(const_iterator pos, Args&&... args)
		{
			return vec.emplace(std::forward<Args>(args)...);
		}

		iterator insert(const_iterator pos, const Type& value) { return vec.insert(pos, value); }
		iterator insert(const_iterator pos, Type&& value) { return vec.insert(pos, std::move(value)); }
		iterator insert(const_iterator pos, size_type count, const Type& value) { return vec.insert(pos, count, value); }

		template< class InputIt >
		iterator insert(const_iterator pos, InputIt first, InputIt last) { return vec.insert<InputIt>(pos, first, last); }

		iterator insert(const_iterator pos, std::initializer_list<Type> ilist) { return vec.insert(pos, ilist); }

		void push_back(const Type& value) { vec.push_back(value); }
		void push_back(Type&& value) { vec.push_back(std::move(value)); }
		void pop_back() { vec.pop_back(); }

		void resize(size_type count) { vec.resize(count); }
		void resize(size_type count, const Type& value) { vec.resize(count, value); }

		void swap(hybrid_vector& other) noexcept {
			std::swap(vec, other.vec);
		}

		iterator erase(iterator pos) { return vec.erase(pos); }
		iterator erase(const_iterator pos) { return vec.erase(pos); }
		iterator erase(iterator first, iterator last) { return vec.erase(first, last); }
		iterator erase(const_iterator first, const_iterator last) { return vec.erase(first, last); }

		void assign(size_type count, const Type& value) { return vec.assign(count, value); }

		template< class InputIt >
		void assign(InputIt first, InputIt last) { return vec.assign<InputIt>(first, last); }

		void assign(std::initializer_list<Type> ilist) { return vec.assign(ilist); }

		allocator_type get_allocator() const { return vec.get_allocator(); }


		hybrid_vector& operator=(const hybrid_vector& other)
		{
			vec.operator=(other.vec);
			return *this;
		}
		hybrid_vector& operator=(const container_type& other)
		{
			vec.operator=(other);
			return *this;
		}

		hybrid_vector& operator=(hybrid_vector&& other)
		{
			vec.operator=(std::move(other.vec));
			return *this;
		}

		hybrid_vector& operator=(container_type&& other)
		{
			vec.operator=(std::move(other));
			return *this;
		}

		hybrid_vector& operator=(std::initializer_list<value_type> ilist)
		{
			vec.operator=(ilist);
			return *this;
		}

		const container_type& container() const { return vec; }
		container_type& container() noexcept { return vec; }

		// returns size of buffer that used for memory allocations before reallocation
		constexpr std::size_t preallocated_memory_size() const noexcept { return _kBufferSize; }
		constexpr std::size_t preallocated_size() const noexcept { return ElementCount; }
		constexpr bool is_reallocation_supported() const noexcept { return IsRealocatable; }

	private:
		unsigned char memory[_kBufferSize];
		std::pmr::monotonic_buffer_resource pool;
		container_type vec;
	};

	template<typename Type, std::size_t ElementCount>
	using static_vector = hybrid_vector<Type, ElementCount, false>;

	template<typename Type, std::size_t ElementCount>
	using vector = hybrid_vector<Type, ElementCount, true>;
}

template< class T, std::size_t E>
bool operator==(const sr::vector<T, E>& lhs,
	const sr::vector<T, E>& rhs)
{
	return std::operator==(lhs.container(), rhs.container());
}

template< class T, std::size_t E>
bool operator==(const sr::vector<T, E>& lhs,
	const sr::static_vector<T, E>& rhs)
{
	return std::operator==(lhs.container(), rhs.container());
}

template< class T, std::size_t E>
bool operator==(const sr::static_vector<T, E>& lhs,
	const sr::vector<T, E>& rhs)
{
	return std::operator==(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator!=(const sr::vector<T, E>& lhs,
	const sr::vector<T, E>& rhs)
{
	return std::operator!=(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator!=(const sr::static_vector<T, E>& lhs,
	const sr::vector<T, E>& rhs)
{
	return std::operator!=(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator!=(const sr::vector<T, E>& lhs,
	const sr::static_vector<T, E>& rhs)
{
	return std::operator!=(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator< (const sr::vector<T, E>& lhs,
	const sr::vector<T, E>& rhs)
{
	return std::operator<(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator< (const sr::static_vector<T, E>& lhs,
	const sr::vector<T, E>& rhs)
{
	return std::operator<(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator< (const sr::vector<T, E>& lhs,
	const sr::static_vector<T, E>& rhs)
{
	return std::operator<(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator<=(const sr::vector<T, E>& lhs,
	const sr::vector<T, E>& rhs)
{
	return operator<=(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator<=(const sr::static_vector<T, E>& lhs,
	const sr::vector<T, E>& rhs)
{
	return operator<=(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator<=(const sr::vector<T, E>& lhs,
	const sr::static_vector<T, E>& rhs)
{
	return operator<=(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator> (const sr::vector<T, E>& lhs,
	const sr::vector<T, E>& rhs)
{
	return operator>(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator> (const sr::static_vector<T, E>& lhs,
	const sr::vector<T, E>& rhs)
{
	return operator>(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator> (const sr::vector<T, E>& lhs,
	const sr::static_vector<T, E>& rhs)
{
	return operator>(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator>=(const sr::vector<T, E>& lhs,
	const sr::vector<T, E>& rhs)
{
	return operator>=(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator>=(const sr::static_vector<T, E>& lhs,
	const sr::vector<T, E>& rhs)
{
	return operator>=(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator>=(const sr::vector<T, E>& lhs,
	const sr::static_vector<T, E>& rhs)
{
	return operator>=(lhs.container(), rhs.container());
}

template< class T, std::size_t E>
bool operator==(const sr::static_vector<T, E>& lhs,
	const sr::static_vector<T, E>& rhs)
{
	return std::operator==(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator!=(const sr::static_vector<T, E>& lhs,
	const sr::static_vector<T, E>& rhs)
{
	return std::operator!=(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator< (const sr::static_vector<T, E>& lhs,
	const sr::static_vector<T, E>& rhs)
{
	return std::operator<(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator<=(const sr::static_vector<T, E>& lhs,
	const sr::static_vector<T, E>& rhs)
{
	return operator<=(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator> (const sr::static_vector<T, E>& lhs,
	const sr::static_vector<T, E>& rhs)
{
	return operator>(lhs.container(), rhs.container());
}

template< class T, std::size_t E >
bool operator>=(const sr::static_vector<T, E>& lhs,
	const sr::static_vector<T, E>& rhs)
{
	return operator>=(lhs.container(), rhs.container());
}

#endif