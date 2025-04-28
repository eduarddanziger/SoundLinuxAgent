#pragma once

#include <array>

template <typename Ch, size_t S>
constexpr auto any_string_array(const char(&literal)[S]) -> std::array<Ch, S> {
	std::array<Ch, S> r = {};

	for (size_t i = 0; i < S; i++)
		r[i] = literal[i];

	return r;
}

