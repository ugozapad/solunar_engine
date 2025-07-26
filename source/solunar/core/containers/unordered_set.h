// author: wh1t3lord

#ifndef SOLUNAR_CORE_CONTAINERS_UNORDERED_SET_H
#define SOLUNAR_CORE_CONTAINERS_UNORDERED_SET_H


#include <unordered_set>
#include <memory_resource>

namespace solunar
{
	// Helper function to compute next power of two
	inline constexpr size_t __next_power_of_two(size_t n) noexcept {
		if (n == 0) return 1;
		n--;
		n |= n >> 1;
		n |= n >> 2;
		n |= n >> 4;
		n |= n >> 8;
		n |= n >> 16;
		if constexpr (sizeof(size_t) > 4) {
			n |= n >> 32;
		}
		return n + 1;
	}

	// Main buffer size calculation
	inline constexpr size_t __calculate_unordered_set_size(size_t element_count) noexcept {
		// Minimum buffer size for empty container
		if (element_count == 0) return 64;

		// Worst-case node size (MSVC: 24 bytes)
		constexpr size_t node_size = 24;

		// Worst-case bucket array calculation
		size_t bucket_array_size{};
		if (element_count < 8) {
			// Small collection case (matches MSVC minimum)
			bucket_array_size = 128;
		}
		else {
			// MSVC-style bucket array (power-of-two * sizeof(void*))
			const size_t min_buckets = element_count * 2;
			const size_t bucket_count = __next_power_of_two(min_buckets);
			bucket_array_size = bucket_count * sizeof(void*);
		}

		// Base memory requirements
		const size_t total_base = bucket_array_size + (element_count * node_size);

		// Safety margin: 30% extra + 64 bytes padding
		return total_base + (total_base * 3 + 9) / 10 + 64;  // Ceiling division
	}

	template<typename Type, typename H, typename P, std::size_t ElementCount, bool IsRealloc, std::size_t _kBufferSize

#ifdef _DEBUG
		= sizeof(std::pmr::unordered_set<Type>::node_type)* ElementCount * 2
#else
		= sizeof(std::pmr::unordered_set<Type>::node_type) * ElementCount
#endif
	>
	class hybrid_unordered_set
	{
		using container_type = std::pmr::unordered_set<Type, H, P>;

	public:
		using key_type = typename container_type::key_type;
		using hasher = typename container_type::hasher;
		using key_equal = typename container_type::key_equal;

		using local_iterator = typename container_type::local_iterator;
		using const_local_iterator = typename container_type::const_local_iterator;

		using value_type = typename container_type::value_type;
		using size_type = typename container_type::size_type;
		using difference_type = typename container_type::difference_type;
		using reference = typename container_type::reference;
		using const_reference = typename container_type::const_reference;
		using pointer = typename container_type::pointer;
		using const_pointer = typename container_type::const_pointer;
		using iterator = typename container_type::iterator;
		using const_iterator = typename container_type::const_iterator;
		using allocator_type = typename container_type::allocator_type;

		using node_type = typename container_type::node_type;
		using insert_return_type = typename container_type::insert_return_type;

	public:
		hybrid_unordered_set() : m_pool{ m_memory, _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, set{ &m_pool }
		{
			set.reserve(ElementCount);
		}

		hybrid_unordered_set(size_type bucket_count)
			: m_pool{ m_memory, _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, set(bucket_count, H(), P(), &m_pool)
		{
			set.reserve(ElementCount);
		}

		hybrid_unordered_set(size_type bucket_count,
			const H& hash)
			: m_pool{ m_memory, _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, set(bucket_count, hash, P(), &m_pool)
		{
			set.reserve(ElementCount);
		}

		//explicit unordered_set(const allocator_type& alloc);

		template< class InputIt >
		hybrid_unordered_set(InputIt first, InputIt last,
			size_type bucket_count,
			const H& hash = H(),
			const key_equal& equal = key_equal()) : m_pool{ m_memory, _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, set(first, last, bucket_count, hash, equal, &m_pool)
		{
			set.reserve(ElementCount);
		}

		template< class InputIt >
		hybrid_unordered_set(InputIt first, InputIt last,
			size_type bucket_count)
			: m_pool{ m_memory, _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, set(first, last,
				bucket_count, H(), key_equal(), &m_pool)
		{
			set.reserve(ElementCount);
		}

		template< class InputIt >
		hybrid_unordered_set(InputIt first, InputIt last,
			size_type bucket_count,
			const H& hash)
			: m_pool{ m_memory, _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, set(first, last,
				bucket_count, hash, key_equal(), &m_pool)
		{
			set.reserve(ElementCount);
		}

		/*
		template<typename Type2, typename H2, typename P2, std::size_t Count, bool Realloc, typename = std::enable_if_t<(ElementCount < Count) && (Realloc == false)>>
		hybrid_unordered_set(const hybrid_unordered_set<Type2, H2, P2, Count, Realloc>&) = delete;

		template<typename Type2, typename H2, typename P2, std::size_t Count, bool Realloc, typename = std::enable_if_t<(ElementCount < Count) && (Realloc == false)>>
		hybrid_unordered_set(hybrid_unordered_set<Type2, H2, P2, Count, Realloc>&&) = delete;*/

		template<typename Type2, typename H2, typename P2, std::size_t Count, bool Realloc, typename = std::enable_if_t<(ElementCount >= Count || IsRealloc == true) && std::is_same_v<Type, Type2>>>
		hybrid_unordered_set(const hybrid_unordered_set<Type2, H2, P2, Count, Realloc>& other) : m_pool{ m_memory, _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, set{ other.container(), &m_pool }
		{
			set.reserve(ElementCount);
		}

		hybrid_unordered_set(const hybrid_unordered_set& other) : m_pool{ m_memory, _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, set{ other.set, &m_pool }
		{
			set.reserve(ElementCount);
		}

		hybrid_unordered_set(hybrid_unordered_set&& other) : m_pool{ m_memory, _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, set{ std::move(other.set), &m_pool }
		{
			set.reserve(ElementCount);
		}

		template<typename Type2, typename H2, typename P2, std::size_t Count, bool Realloc, typename = std::enable_if_t<(ElementCount >= Count || IsRealloc == true) && std::is_same_v<Type, Type2>>>
		hybrid_unordered_set(hybrid_unordered_set<Type2, H2, P2, Count, Realloc>&& other) : m_pool{ m_memory, _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, set{ std::move(other.container_move_out()), &m_pool }
		{

		}

		hybrid_unordered_set(std::initializer_list<value_type> init,
			size_type bucket_count,
			const H& hash = H(),
			const key_equal& equal = key_equal()) : m_pool{ m_memory, _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, set{ init, bucket_count, hash, equal, &m_pool }
		{
			set.reserve(ElementCount);
		}

		hybrid_unordered_set(std::initializer_list<value_type> init,
			size_type bucket_count)
			: m_pool{ m_memory, _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, set(init, bucket_count,
				H(), key_equal(), &m_pool)
		{
			set.reserve(ElementCount);
		}

		hybrid_unordered_set(std::initializer_list<value_type> init,
			size_type bucket_count,
			const H& hash)
			: m_pool{ m_memory, _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, set(init, bucket_count,
				hash, key_equal(), &m_pool) {
			set.reserve(ElementCount);
		}


		~hybrid_unordered_set() {}

	public:
		hybrid_unordered_set& operator=(const hybrid_unordered_set& other)
		{
			set.operator=(other.set);
			return *this;
		}

		hybrid_unordered_set& operator=(const container_type& other)
		{
			set.operator=(other);
			return *this;
		}

		hybrid_unordered_set& operator=(hybrid_unordered_set&& other)
			noexcept
		{
			set.operator=(std::move(other.set));
			return *this;
		}

		hybrid_unordered_set& operator=(container_type&& other)
		{
			set.operator=(std::move(other));
			return *this;
		}

		hybrid_unordered_set& operator=(std::initializer_list<value_type> ilist);


		allocator_type get_allocator() const noexcept { return set.get_allocator(); }

		iterator begin() noexcept
		{
			return set.begin();
		}

		const_iterator begin() const noexcept
		{
			return set.begin();
		}

		const_iterator cbegin() const noexcept
		{
			return set.cbegin();
		}

		iterator end() noexcept
		{
			return set.end();
		}

		const_iterator end() const noexcept
		{
			return set.end();
		}

		const_iterator cend() const noexcept
		{
			return set.cend();
		}

		bool empty() const noexcept { return set.empty(); }

		size_type size() const noexcept { return set.size(); }

		size_type max_size() const noexcept { return set.max_size(); }

		void clear() noexcept { return set.clear(); }

		std::pair<iterator, bool> insert(const value_type& value)
		{
			return set.insert(value);
		}

		std::pair<iterator, bool> insert(value_type&& value)
		{
			return set.insert(std::move(value));
		}

		iterator insert(const_iterator hint, const value_type& value)
		{
			return set.insert(hint, value);
		}

		iterator insert(const_iterator hint, value_type&& value)
		{
			return set.insert(hint, std::move(value));
		}

		template< class InputIt >
		void insert(InputIt first, InputIt last)
		{
			set.insert<InputIt>(first, last);
		}

		void insert(std::initializer_list<value_type> ilist)
		{
			set.insert(ilist);
		}

		insert_return_type insert(node_type&& nh)
		{
			return set.insert(nh);
		}

		iterator insert(const_iterator hint, node_type&& nh)
		{
			return set.insert(hint, nh);
		}

		template< class... Args >
		std::pair<iterator, bool> emplace(Args&&... args)
		{
			return set.emplace(std::forward<Args>(args)...);
		}

		template< class... Args >
		iterator emplace_hint(const_iterator hint, Args&&... args)
		{
			return set.emplace_hint(hint, std::forward<Args>(args)...);
		}

		/*
		iterator erase(iterator pos)
		{
			return set.erase(pos);
		}
		*/

		iterator erase(const_iterator pos)
		{
			return set.erase(pos);
		}

		iterator erase(const_iterator first, const_iterator last)
		{
			return set.erase(first, last);
		}

		size_type erase(const Type& key)
		{
			return set.erase(key);
		}

		void swap(hybrid_unordered_set& other) noexcept
		{
			set.swap(other.set);
		}

		void swap(container_type& other)noexcept
		{
			set.swap(other);
		}

		node_type extract(const_iterator pos)
		{
			return set.extract(pos);
		}

		node_type extract(const Type& k)
		{
			return set.extract(k);
		}

		template<class H2, class P2>
		void merge(hybrid_unordered_set<Type, H2, P2, ElementCount, IsRealloc>& source)
		{
			set.merge(source.set);
		}

		template< class H2, class P2 >
		void merge(std::pmr::unordered_set<Type, H2, P2>& source)
		{
			set.merge(source);
		}


		template< class H2, class P2 >
		void merge(hybrid_unordered_set<Type, H2, P2, ElementCount, IsRealloc>&& source)
		{
			set.merge(std::move(source.set));
		}

		template< class H2, class P2 >
		void merge(std::pmr::unordered_set<Type, H2, P2>&& source)
		{
			set.merge(std::move(source));
		}

		template< class H2, class P2 >
		void merge(std::pmr::unordered_multiset<Type, H2, P2>& source)
		{
			set.merge(source);
		}

		template< class H2, class P2 >
		void merge(std::pmr::unordered_multiset<Type, H2, P2>&& source)
		{
			set.merge(std::move(source));
		}

		size_type count(const Type& key) const
		{
			return set.count(key);
		}

		iterator find(const Type& key)
		{
			return set.find(key);
		}

		const_iterator find(const Type& key) const
		{
			return set.find(key);
		}

		std::pair<iterator, iterator> equal_range(const Type& key)
		{
			return set.equal_range(key);
		}

		std::pair<const_iterator, const_iterator> equal_range(const Type& key) const
		{
			return set.equal_range(key);
		}

		local_iterator begin(size_type n)
		{
			return set.begin(n);
		}

		const_local_iterator begin(size_type n) const
		{
			return set.begin(n);
		}

		const_local_iterator cbegin(size_type n) const
		{
			return set.cbegin(n);
		}

		local_iterator end(size_type n)
		{
			return set.end(n);
		}

		const_local_iterator end(size_type n) const
		{
			return set.end(n);
		}
		const_local_iterator cend(size_type n) const
		{
			return set.cend(n);
		}

		size_type bucket_count() const
		{
			return set.bucket_count();
		}

		size_type max_bucket_count() const
		{
			return set.max_bucket_count();
		}

		size_type bucket_size(size_type n) const
		{
			return set.bucket_size(n);
		}

		size_type bucket(const Type& key) const
		{
			return set.bucket(key);
		}

		float load_factor() const
		{
			return set.load_factor();
		}

		float max_load_factor() const
		{
			return set.max_load_factor();
		}

		void max_load_factor(float ml)
		{
			return set.max_load_factor(ml);
		}

		void rehash(size_type count)
		{
			return set.rehash(count);
		}

		void reserve(size_type count)
		{
			return set.reserve(count);
		}

		hasher hash_function() const
		{
			return set.hash_function();
		}

		key_equal key_eq() const
		{
			return set.key_eq();
		}
	public:

		constexpr std::size_t preallocated_memory_size() const noexcept { return _kBufferSize; }
		constexpr std::size_t preallocated_size() const noexcept { return ElementCount; }
		constexpr bool is_reallocation_supported() const noexcept { return IsRealloc; }

		const container_type& container() const noexcept { return set; }
		container_type& container() noexcept { return set; }
		container_type&& container_move_out() noexcept { return std::move(str); }

	private:
		unsigned char m_memory[_kBufferSize];
		std::pmr::monotonic_buffer_resource m_pool;
		container_type set;
	};

	template<typename Key, std::size_t ElementCount>
	using unordered_set = hybrid_unordered_set<Key, std::hash<Key>, std::equal_to<Key>, ElementCount, true>;

	template<typename Key, std::size_t ElementCount>
	using static_unordered_set = hybrid_unordered_set<Key, std::hash<Key>, std::equal_to<Key>, ElementCount, false>;
}

template< class Key, class Hash, class KeyEqual, std::size_t ElementCount, bool Realloc >
bool operator==(const solunar::hybrid_unordered_set<Key, Hash, KeyEqual, ElementCount, Realloc>& lhs,
	const solunar::hybrid_unordered_set<Key, Hash, KeyEqual, ElementCount, Realloc>& rhs)
{
	return lhs.container() == rhs.container();
}

template< class Key, class Hash, class KeyEqual, std::size_t ElementCount, bool Realloc >
bool operator!=(const solunar::hybrid_unordered_set<Key, Hash, KeyEqual, ElementCount, Realloc>& lhs,
	const solunar::hybrid_unordered_set<Key, Hash, KeyEqual, ElementCount, Realloc>& rhs)
{
	return lhs.container() != rhs.container();
}



#endif