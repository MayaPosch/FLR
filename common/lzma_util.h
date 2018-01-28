#include <stdlib.h>
#include "lzma.h"
#include <string>

// Level is between 0 (no compression), 9 (slow compression, small output).
std::string compressWithLzma(const std::string &in, int level) {
	std::string result;
	result.resize(in.size() + (in.size() >> 2) + 128);
	size_t out_pos = 0;
	if (LZMA_OK != lzma_easy_buffer_encode(level, LZMA_CHECK_CRC32, 
			0, reinterpret_cast<uint8_t*>(const_cast<char*>(in.data())), 
			in.size(), 	reinterpret_cast<uint8_t*>(&result[0]), 
										   &out_pos, result.size())) {
		// Return error.
		return std::string();
	}
	
	result.resize(out_pos);
	return result;
}

std::string decompressWithLzma(const std::string &in) {
	static const size_t kMemLimit = 1 << 30;  // 1 GB.
	lzma_stream strm = LZMA_STREAM_INIT;
	std::string result;
	result.resize(8192);
	size_t result_used = 0;
	lzma_ret ret;
	ret = lzma_stream_decoder(&strm, kMemLimit, LZMA_CONCATENATED);
	if (ret != LZMA_OK) {
		// Return error.
		return std::string();
	}

	size_t avail0 = result.size();
	strm.next_in = reinterpret_cast<const uint8_t*>(in.data());
	strm.avail_in = in.size();
	strm.next_out = reinterpret_cast<uint8_t*>(&result[0]);
	strm.avail_out = avail0;
	while (true) {
		ret = lzma_code(&strm, strm.avail_in == 0 ? LZMA_FINISH : LZMA_RUN);
		if (ret == LZMA_STREAM_END) {
			result_used += avail0 - strm.avail_out;
			if (0 != strm.avail_in) { // Guaranteed by lzma_stream_decoder().
				return std::string();
			}
			
			result.resize(result_used);
			lzma_end(&strm);
			
			return result;
		}
		
		if (ret != LZMA_OK) {
			// Return error.
			return std::string();
		}
		
		if (strm.avail_out == 0) {
			result_used += avail0 - strm.avail_out;
			result.resize(result.size() << 1);
			strm.next_out = reinterpret_cast<uint8_t*>(&result[0] + result_used);
			strm.avail_out = avail0 = result.size() - result_used;
		}
	}
}
