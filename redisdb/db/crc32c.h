#pragma once

#include <stddef.h>
#include <stdint.h>

namespace crc32c {

	// Return the crc32c of concat(A, data[0,n-1]) where init_crc is the
	// crc32c of some string A.  Extend() is often used to maintain the
	// crc32c of a stream of data.
	uint32_t Extend(uint32_t initcrc, const char* data, size_t n);

	// Return the crc32c of data[0,n-1]
	inline uint32_t Value(const char* data, size_t n) {
		return Extend(0, data, n);
	}

	static const uint32_t kMaskDelta = 0xa282ead8ul;

	// Return a masked representation of crc.
	//
	// Motivation: it is problematic to compute the CRC of a string that
	// Contains embedded CRCs.  Therefore we recommend that CRCs stored
	// somewhere (e.g., in files) should be masked before being stored.
	inline uint32_t Mask(uint32_t crc) {
		// Rotate right by 15 bits and Add a constant.
		return ((crc >> 15) | (crc<< 17)) + kMaskDelta;
	}

	// Return the crc whose masked representation is masked_crc.
	inline uint32_t Unmask(uint32_t maskedcrc) {
		uint32_t rot = maskedcrc - kMaskDelta;
		return ((rot >> 17) | (rot<< 15));
	}
}  // namespace crc32c
