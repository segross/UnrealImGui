// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "Range.h"

#include <array>
#include <iterator>
#include <type_traits>


// Utilities to work with one-dimensional, statically bound arrays. Code relying on those utilities should work without
// modifications with fixed-sized arrays (currently used in ImGui) and with standard arrays.

namespace Utilities
{
	//====================================================================================================
	// Helper functions
	//====================================================================================================

	// Function to determine number of elements in fixed size array.
	template<class T, std::size_t N>
	constexpr std::size_t GetArraySize(const T(&)[N])
	{
		return N;
	}

	// Function to determine number of elements in std array.
	template<class T, std::size_t N>
	constexpr std::size_t GetArraySize(const std::array<T, N>&)
	{
		return N;
	}


	//====================================================================================================
	// Traits
	//====================================================================================================

	template<typename TArray>
	struct ArraySize;

	// Struct to determine number of elements in fixed size array.
	template<typename T, std::size_t N>
	struct ArraySize<T[N]> : std::extent<T[N]>
	{
	};

	// Struct to determine number of elements in std array.
	template<typename T, std::size_t N>
	struct ArraySize<std::array<T, N>> : std::tuple_size<std::array<T, N>>
	{
	};


	//====================================================================================================
	// Ranges
	//====================================================================================================

	// Array indices range. Limited by 0 and array size.
	template<typename TArray, typename SizeType>
	using TArrayIndexRange = TBoundedRange<SizeType, 0, ArraySize<TArray>::value>;
}
