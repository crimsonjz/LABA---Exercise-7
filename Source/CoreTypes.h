#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Fixed-size primitive aliases (Unreal Engine naming).
using int8 = std::int8_t;
using uint8 = std::uint8_t;
using int16 = std::int16_t;
using uint16 = std::uint16_t;
using int32 = std::int32_t;
using uint32 = std::uint32_t;
using int64 = std::int64_t;
using uint64 = std::uint64_t;

using FString = std::string;

template <typename InElementType>
using TArray = std::vector<InElementType>;

constexpr int32 INDEX_NONE = -1;
