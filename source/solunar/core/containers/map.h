// author: wh1t3lord

#ifndef SOLUNAR_CORE_CONTAINERS_MAP_H
#define SOLUNAR_CORE_CONTAINERS_MAP_H

#include <map>
#include <memory_resource>
#include <type_traits>
#include <span>
#include <string>
#include <list>

namespace solunar
{
	// Helper function for linear interpolation
	constexpr size_t __pmr_hybrid_map_interpolate(size_t x0, size_t y0, size_t x1, size_t y1, size_t x) {
		if (x <= x0) return y0;
		if (x >= x1) return y1;
		return y0 + ((y1 - y0) * (x - x0)) / (x1 - x0);
	}

	// Helper function to find next prime number (for bucket count estimation)
	constexpr bool __pmr_hybrid_map_is_prime(size_t n) {
		if (n <= 1) return false;
		if (n == 2) return true;
		if (n % 2 == 0) return false;
		for (size_t i = 3; i * i <= n; i += 2)
			if (n % i == 0) return false;
		return true;
	}

	constexpr size_t __pmr_hybrid_map_next_prime(size_t n) {
		if (n <= 1) return 2;
		size_t candidate = n | 1;  // Start with next odd number
		while (!__pmr_hybrid_map_is_prime(candidate))
			candidate += 2;
		return candidate;
	}

	// Returns buffer size for std::pmr::map
	constexpr size_t __pmr_hybrid_map_map_buffer_size(size_t N) {
		if (N == 0) return 0;

#ifdef _MSC_VER
		// MSVC: 80 bytes per node + 80 byte base
		return 80 + 80 * N;
#else
		// Clang/GCC: 80 bytes per node
		return 80 * N;
#endif
	}

	template <typename T>
	struct is_fundamental_or_void :
		std::bool_constant<std::is_fundamental_v<T> || std::is_void_v<T>> {
	};

	template <typename T>
	struct is_trivial_no_alloc :
		std::bool_constant<std::is_trivial_v<T>&& std::is_trivially_copyable_v<T>> {
	};

	template <typename T>
	struct is_pmr_container : std::false_type {};

	template <typename T>
	struct is_pmr_container<std::pmr::vector<T>> : std::true_type {};
	template <typename T>
	struct is_pmr_container<std::pmr::basic_string<T>> : std::true_type {};
	template <typename T>
	struct is_pmr_container<std::pmr::list<T>> : std::true_type {};

	template <typename T>
	struct is_fixed_size_container : std::false_type {};

	template <typename T, std::size_t N>
	struct is_fixed_size_container<std::array<T, N>> : std::true_type {};

#ifdef _MSVC_LANG

#if _MSVC_LANG >= 202002L
	template <typename T>
	struct is_fixed_size_container<std::span<T>> : std::true_type {};
#endif

#elif defined(__cplusplus)

#if __cplusplus >= 202002L  
	template <typename T>
	struct is_fixed_size_container<std::span<T>> : std::true_type {};
#endif

#endif

	template <typename T>
	struct is_safe_for_buffer : std::bool_constant<
		is_fundamental_or_void<T>::value ||
		is_trivial_no_alloc<T>::value ||
		is_pmr_container<T>::value ||
		is_fixed_size_container<T>::value
	> {
	};

	template <typename T>
	inline constexpr bool is_safe_for_buffer_v = is_safe_for_buffer<T>::value;


	template<typename Key, typename Value, std::size_t ElementCount, bool IsRealloc, std::size_t _kBufferSize = __pmr_hybrid_map_map_buffer_size(ElementCount)
	>
	class hybrid_map
	{
		static_assert(is_safe_for_buffer_v<Value>,
			"Value type must be fundamental, trivial, PMR container, or fixed-size container");

		template<typename _K, typename _T, typename _C>
		using base_type = std::pmr::map<_K, _T, _C>;

		template<typename _K, typename _T, typename _C>
		using base_multimap_type = std::pmr::multimap<_K, _T, _C>;

		using container_type = std::pmr::map<Key, Value>;

	public:
		using key_type = typename container_type::key_type;
		using key_compare = typename container_type::key_compare;
		using value_compare = typename container_type::value_compare;
		using mapped_type = typename container_type::mapped_type;

		using reverse_iterator = typename container_type::reverse_iterator;
		using const_reverse_iterator = typename container_type::const_reverse_iterator;

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
		hybrid_map() : m_pool{ (ElementCount == 0) ? nullptr : m_memory, (ElementCount == 0) ? 0 : _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, map{ &m_pool }
		{
		}

		explicit hybrid_map(const key_compare& comp)
		{

		}

		template< class InputIt >
		hybrid_map(InputIt first, InputIt last,
			const key_compare& comp = key_compare()) : m_pool{ (ElementCount == 0) ? nullptr : m_memory, (ElementCount == 0) ? 0 : _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, map{ first, last, comp, &m_pool }
		{

		}

		template< class InputIt >
		hybrid_map(InputIt first, InputIt last)
			: m_pool{ (ElementCount == 0) ? nullptr : m_memory, (ElementCount == 0) ? 0 : _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, map(first, last, key_compare(), &m_pool) {
		}

		hybrid_map(const hybrid_map& other) : m_pool{ (ElementCount == 0) ? nullptr : m_memory, (ElementCount == 0) ? 0 : _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, map{ other.map, &m_pool }
		{
		}

		hybrid_map(hybrid_map&& other) : m_pool{ (ElementCount == 0) ? nullptr : m_memory, (ElementCount == 0) ? 0 : _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, map{ std::move(other.map), &m_pool }
		{

		}

		hybrid_map(std::initializer_list<value_type> init,
			const key_compare& comp = key_compare()) : m_pool{ (ElementCount == 0) ? nullptr : m_memory, (ElementCount == 0) ? 0 : _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, map{ init, comp, &m_pool }
		{

		}

		hybrid_map(std::initializer_list<value_type> init)
			: m_pool{ (ElementCount == 0) ? nullptr : m_memory, (ElementCount == 0) ? 0 : _kBufferSize, IsRealloc ? std::pmr::get_default_resource() : std::pmr::null_memory_resource() }, map(init, key_compare(), &m_pool) {
		}

		~hybrid_map() {}

	public:
		Value& at(const Key& key)
		{
			return map.at(key);
		}
		const Value& at(const Key& key) const
		{
			return map.at(key);
		}

		Value& operator[](const Key& key)
		{
			return map.operator[](key);
		}

		Value& operator[](Key&& key)
		{
			return map.operator[](std::move(key));
		}

	public:
		iterator begin()
		{
			return map.begin();
		}

		const_iterator begin() const
		{
			return map.begin();
		}

		const_iterator cbegin() const noexcept
		{
			return map.cbegin();
		}

		iterator end()
		{
			return map.end();
		}

		const_iterator end() const
		{
			return map.end();
		}

		const_iterator cend() const noexcept
		{
			return map.cend();
		}

		reverse_iterator rbegin()
		{
			return map.rbegin();
		}

		const_reverse_iterator rbegin() const
		{
			return map.rbegin();
		}

		const_reverse_iterator crbegin() const noexcept
		{
			return map.crbegin();
		}

		reverse_iterator rend()
		{
			return map.rend();
		}

		const_reverse_iterator rend() const
		{
			return map.rend();
		}

		const_reverse_iterator crend() const noexcept
		{
			return map.crend();
		}

	public:
		bool empty() const
		{
			return map.empty();
		}

		size_type size() const
		{
			return map.size();
		}

		size_type max_size() const
		{
			return map.max_size();
		}

	public:

		void clear()
		{
			map.clear();
		}

		std::pair<iterator, bool> insert(const value_type& value)
		{
			return map.insert(value);
		}

		template< class P >
		std::pair<iterator, bool> insert(P&& value)
		{
			return map.insert<P>(std::move(value));
		}

		std::pair<iterator, bool> insert(value_type&& value)
		{
			return map.insert(std::move(value));
		}

		iterator insert(iterator pos, const value_type& value)
		{
			return map.insert(pos, value);
		}

		iterator insert(const_iterator pos, const value_type& value)
		{
			return map.insert(pos, value);
		}

		template< class P >
		iterator insert(const_iterator pos, P&& value)
		{
			return map.insert<P>(pos, value);
		}

		iterator insert(const_iterator pos, value_type&& value)
		{
			return map.insert(pos, std::move(value));
		}

		template< class InputIt >
		void insert(InputIt first, InputIt last)
		{
			return map.insert<InputIt>(first, last);
		}

		void insert(std::initializer_list<value_type> ilist)
		{
			return map.insert(ilist);
		}

		insert_return_type insert(node_type&& nh)
		{
			return map.insert(std::move(nh));
		}

		iterator insert(const_iterator pos, node_type&& nh)
		{
			return map.insert(pos, std::move(nh));
		}

		template< class M >
		std::pair<iterator, bool> insert_or_assign(const Key& k, M&& obj)
		{
			return map.insert_or_assign<M>(k, obj);
		}

		template< class M >
		std::pair<iterator, bool> insert_or_assign(Key&& k, M&& obj)
		{
			return map.insert_or_assign<M>(std::move(k), obj);
		}

		template< class K, class M >
		std::pair<iterator, bool> insert_or_assign(K&& k, M&& obj)
		{
			return map.insert_or_assign<K, M>(std::move(k), std::move(obj));
		}

		template< class M >
		iterator insert_or_assign(const_iterator hint, const Key& k, M&& obj)
		{
			return map.insert_or_assign<M>(hint, k, obj);
		}

		template< class M >
		iterator insert_or_assign(const_iterator hint, Key&& k, M&& obj)
		{
			return map.insert_or_assign<M>(hint, k, obj);
		}

		template< class... Args >
		std::pair<iterator, bool> emplace(Args&&... args)
		{
			return map.emplace(std::forward<Args>(args)...);
		}

		template< class... Args >
		iterator emplace_hint(const_iterator hint, Args&&... args)
		{
			return map.emplace(std::forward<Args>(args)...);
		}

		template< class... Args >
		std::pair<iterator, bool> try_emplace(const Key& k, Args&&... args)
		{
			return map.try_emplace<Args>(k, std::forward<Args>(args)...);
		}

		template< class... Args >
		std::pair<iterator, bool> try_emplace(Key&& k, Args&&... args)
		{
			return map.try_emplace<Args>(std::move(k), std::forward<Args>(args)...);
		}

		template< class... Args >
		iterator try_emplace(const_iterator hint, const Key& k, Args&&... args)
		{
			return map.try_emplace<Args>(hint, k, std::forward<Args>(args)...);
		}

		template< class... Args >
		iterator try_emplace(const_iterator hint, Key&& k, Args&&... args)
		{
			return map.try_emplace(hint, std::move(k), std::forward<Args>(args)...);
		}

		iterator erase(iterator pos)
		{
			return map.erase(pos);
		}

		iterator erase(const_iterator pos)
		{
			return map.erase(pos);
		}

		iterator erase(const_iterator first, const_iterator last)
		{
			return map.erase(first, last);
		}

		size_type erase(const Key& key)
		{
			return map.erase(key);
		}

		void swap(hybrid_map& other) noexcept
		{
			map.swap(other);
		}

		node_type extract(const_iterator pos)
		{
			return map.extract(pos);
		}

		node_type extract(const Key& k)
		{
			return map.extract(k);
		}

		template< class C2 >
		void merge(base_type<Key, Value, C2>& source)
		{
			map.merge<C2>(source);
		}

		template< class C2 >
		void merge(base_type<Key, Value, C2>&& source)
		{
			map.merge<C2>(std::move(source));
		}

		template< class C2 >
		void merge(base_multimap_type<Key, Value, C2>& source)
		{
			map.merge<C2>(source);
		}

		template< class C2 >
		void merge(base_multimap_type<Key, Value, C2>&& source)
		{
			map.merge<C2>(std::move(source));
		}

	public:

		size_type count(const Key& key) const
		{
			return map.count(key);
		}

		template< class K >
		size_type count(const K& x) const
		{
			return map.count<K>(x);
		}

		iterator find(const Key& key)
		{
			return map.find(key);
		}

		const_iterator find(const Key& key) const
		{
			return map.find(key);
		}

		template< class K >
		iterator find(const K& x)
		{
			return map.find<K>(x);
		}

		template< class K >
		const_iterator find(const K& x) const
		{
			return map.find<K>(x);
		}

		std::pair<iterator, iterator> equal_range(const Key& key)
		{
			return map.equal_range(key);
		}

		std::pair<const_iterator, const_iterator>
			equal_range(const Key& key) const
		{
			return map.equal_range(key);
		}

		template< class K >
		std::pair<iterator, iterator> equal_range(const K& x)
		{
			return map.equal_range(x);
		}

		template< class K >
		std::pair<const_iterator, const_iterator>
			equal_range(const K& x) const
		{
			return map.equal_range(x);
		}

		iterator lower_bound(const Key& key)
		{
			return map.lower_bound(key);
		}

		const_iterator lower_bound(const Key& key) const
		{
			return map.lower_bound(key);
		}

		template< class K >
		iterator lower_bound(const K& x)
		{
			return map.lower_bound<K>(x);
		}

		template< class K >
		const_iterator lower_bound(const K& x) const
		{
			return map.lower_bound<K>(x);
		}

		iterator upper_bound(const Key& key)
		{
			return map.upper_bound(key);
		}

		const_iterator upper_bound(const Key& key) const
		{
			return map.upper_bound(key);
		}

		template< class K >
		iterator upper_bound(const K& x)
		{
			return map.upper_bound<K>(x);
		}

		template< class K >
		const_iterator upper_bound(const K& x) const
		{
			return map.upper_bound<K>(x);
		}

	public:
		key_compare key_comp() const
		{
			return map.key_comp();
		}

		value_compare value_comp() const
		{
			return map.value_comp();
		}


	public:

		constexpr std::size_t preallocated_memory_size() const noexcept { return _kBufferSize; }
		constexpr std::size_t preallocated_size() const noexcept { return ElementCount; }
		constexpr bool is_reallocation_supported() const noexcept { return IsRealloc; }

		const container_type& container() const noexcept { return map; }
		container_type& container() noexcept { return map; }
		container_type&& container_move_out() noexcept { return std::move(map); }

	private:
		unsigned char m_memory[(ElementCount == 0) ? 1 : _kBufferSize];
		std::pmr::monotonic_buffer_resource m_pool;
		container_type map;
	};
}

#endif