#pragma once

#include <variant>

namespace cpp_test
{
	template<class Func1, class Func2>
	struct CommonFunctionType2D
	{
		bool use_first;
		std::variant<Func1, Func2> m_f;

		CommonFunctionType2D(Func1 f) : m_f(f), use_first(true) {}
		CommonFunctionType2D(Func2 f) : m_f(f), use_first(false) {}

		template<class... Args>
		decltype(auto) operator()(Args... a) {
			if (use_first)
				return std::get<Func1>(m_f)(a...);
			else
				return std::get<Func2>(m_f)(a...);
		}
	};
}