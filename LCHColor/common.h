#ifndef COMMON_H
#define COMMON_H

#include <map>
#include <cmath>
#include <vector>
#include <limits>
#include <memory>
#include <cstdint>
#include <algorithm>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using f32 = float;
using f64 = double;

#ifndef M_PI
#define M_PI 3.14159265359
#endif

#ifndef PI
#define PI M_PI
#endif

template<class T, class A, class B>
T clamp(T x, A l, B u) {
	return std::max(std::min(x, (T)u), (T)l);
}

#include "log.h"

#endif