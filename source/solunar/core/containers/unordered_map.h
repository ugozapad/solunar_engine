// author: wh1t3lord

#ifndef SOLUNAR_CORE_CONTAINERS_UNORDERED_MAP_H
#define SOLUNAR_CORE_CONTAINERS_UNORDERED_MAP_H

#include <unordered_map>
#include <memory_resource>

namespace solunar
{
	// Helper function for linear interpolation
	constexpr size_t __pmr_hybrid_unordered_map_interpolate(size_t x0, size_t y0, size_t x1, size_t y1, size_t x) {
		if (x <= x0) return y0;
		if (x >= x1) return y1;
		return y0 + ((y1 - y0) * (x - x0)) / (x1 - x0);
	}

	// Helper function to find next prime number (for bucket count estimation)
	constexpr bool __pmr_hybrid_unordered_map_is_prime(size_t n) {
		if (n <= 1) return false;
		if (n == 2) return true;
		if (n % 2 == 0) return false;
		for (size_t i = 3; i * i <= n; i += 2)
			if (n % i == 0) return false;
		return true;
	}

	constexpr size_t __pmr_hybrid_unordered_map_next_prime(size_t n) {
		if (n <= 1) return 2;
		size_t candidate = n | 1;  // Start with next odd number
		while (!__pmr_hybrid_unordered_map_is_prime(candidate))
			candidate += 2;
		return candidate;
	}

	// Returns buffer size for std::pmr::unordered_map
	constexpr size_t __pmr_hybrid_unordered_map_buffer_size(size_t N) {
		if (N == 0) return 0;

#ifdef _MSC_VER
		// MSVC: Piecewise interpolation based on empirical data
		if (N <= 10) return __pmr_hybrid_unordered_map_interpolate(1, 179, 10, 1792, N);
		if (N <= 100) return __pmr_hybrid_unordered_map_interpolate(10, 1792, 100, 14656, N);
		if (N <= 1000) return __pmr_hybrid_unordered_map_interpolate(100, 14656, 1000, 80448, N);

		// For N > 1000: 80.4 bytes per element + bucket overhead
		const size_t node_size = 80;
		const size_t bucket_count = __pmr_hybrid_unordered_map_next_prime(N);
		return 80 * N + bucket_count * sizeof(void*);

#elif defined(__clang__) || defined(__GNUC__)
		// Clang/GCC: Piecewise interpolation based on empirical data
		if (N <= 10) return __pmr_hybrid_unordered_map_interpolate(1, 66, 10, 664, N);
		if (N <= 100) return __pmr_hybrid_unordered_map_interpolate(10, 664, 100, 6616, N);
		if (N <= 1000) return __pmr_hybrid_unordered_map_interpolate(100, 6616, 1000, 64872, N);

		// For N > 1000: 64.9 bytes per element + bucket overhead
		const size_t node_size = 56;  // 8 (pointer) + 8 (hash) + 40 (pair)
		const size_t bucket_count = __pmr_hybrid_unordered_map_next_prime(N);
		return node_size * N + bucket_count * sizeof(void*);

#else
		// Conservative default for other compilers
		return 100 * N + 4096;
#endif
	}

}

#endif