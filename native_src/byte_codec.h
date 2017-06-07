// Copyright 2017, Erlang Solutions Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

// Tool library assists with parsing and writing of simple integer values
// with respect to endianness

//
// Endian Swap helpers
//
#include <endian.h>

namespace codec {

#if __BYTE_ORDER == __BIG_ENDIAN
  constexpr uint16_t big_to_native(uint16_t x) { return x; }

  constexpr uint32_t big_to_native(uint32_t x) { return x; }

  constexpr uint64_t big_to_native(uint64_t x) { return x; }
#else
  // GCC 4.3+ builtins
  constexpr uint16_t big_to_native(uint16_t x) { return __builtin_bswap16(x); }

  constexpr uint32_t big_to_native(uint32_t x) { return __builtin_bswap32(x); }

  constexpr uint64_t big_to_native(uint64_t x) { return __builtin_bswap64(x); }
#endif

// ARM compilers have this keyword to specify unaligned memory pointer
#ifdef __arm__
#define PACKED __packed
#else
#define PACKED
#endif

// portable unaligned memory read
template<typename VALUE, typename PTRTYPE>
inline VALUE unaligned_read(const PTRTYPE* src) {
  PACKED const VALUE* src2 = reinterpret_cast<const VALUE*>(src);
  return *src2;
}

uint16_t read_big_u16(const char* ptr) {
  uint16_t r = big_to_native(
    unaligned_read<uint16_t>(ptr));
  return r;
}


uint32_t read_big_u32(const char* ptr) {
  uint32_t r = big_to_native(
    unaligned_read<uint32_t>(ptr));
  return r;
}


uint64_t read_big_u64(const char* ptr) {
  uint64_t r = big_to_native(
    unaligned_read<uint64_t>(ptr));
  return r;
}


double read_big_float64(const char* ptr) {
  uint64_t r = big_to_native(
    unaligned_read<uint64_t>(ptr));
  return *((double *)&r); // Boom!
}

} // ns codec
