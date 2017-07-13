#pragma once

#include <cstddef>
#include <type_traits>

namespace cpp_test
{
	//TYPE_LIST
	template<class...>
	struct type_list;

	//IS_TYPE_LIST
	template<class...>
	struct is_type_list : std::false_type {};

	template<class... Types>
	struct is_type_list<type_list<Types...>> : std::true_type {};

	//PUSH_BACK
	template<class...> struct push_back;

	template<class... Types, class TNew>
	struct push_back<type_list<Types...>, TNew>
	{
		using type = type_list<Types..., TNew>;
	};

	//AT
	template<class, std::size_t> struct at;

	template<class T0, class... Types>
	struct at<type_list<T0, Types...>, 0>
	{
		using type = T0;
	};

	template<std::size_t Index, class T0, class... Types>
	struct at<type_list<T0, Types...>, Index>
	{
		using type = typename at<type_list<Types...>, Index - 1>::type;
	};

	//SIZE
	template<class...> struct size;

	template<class... Types>
	struct size<type_list<Types...>> : std::integral_constant<std::size_t, sizeof...(Types)>
	{
	};

	//CONTAINS
	template<class...> struct contains;

	template<class... Types, class T>
	struct contains<type_list<Types...>, T>
		: std::disjunction<std::is_same<Types, T>...>
	{
	};

	//INDEX_OF
	template<class...> struct index_of;

	template<class... Types, class SearchedType>
	struct index_of<type_list<SearchedType, Types...>, SearchedType>
	{
		static constexpr std::size_t value = 0;
	};

	template<class T0, class... Types, class SearchedType>
	struct index_of<type_list<T0, Types...>, SearchedType>
	{
		static constexpr std::size_t value = 1 + index_of<type_list<Types...>, SearchedType>::value;
	};

	//ALL_OF
	template<class, template<class> class> struct all_of;

	template<class... Types, template<class> class UnaryPredicate>
	struct all_of<type_list<Types...>, UnaryPredicate>
		: std::conjunction<UnaryPredicate<Types>...>
	{
	};

	//ANY_OF
	template<class, template<class> class> struct any_of;

	template<class... Types, template<class> class UnaryPredicate>
	struct any_of<type_list<Types...>, UnaryPredicate>
		: std::disjunction<UnaryPredicate<Types>...>
	{
	};

	//NONE_OF
	template<class, template<class> class> struct none_of;

	template<class... Types, template<class> class UnaryPredicate>
	struct none_of<type_list<Types...>, UnaryPredicate>
		: std::bool_constant<!std::disjunction_v<UnaryPredicate<Types>...>>
	{
	};

	//SWAP_AT, only works for type_lists with a set of types
	template<class, std::size_t, std::size_t> struct swap_at;

	template<class... Types, std::size_t IndexA, std::size_t IndexB>
	struct swap_at<type_list<Types...>, IndexA, IndexB>
	{
		using type =
			type_list<
				std::conditional_t<
					index_of<type_list<Types...>, Types>::value == IndexA,
					typename at<type_list<Types...>, IndexB>::type,

					std::conditional_t<
						index_of<type_list<Types...>, Types>::value == IndexB,
						typename at<type_list<Types...>, IndexA>::type,
					Types
					>
				>...
			>;
	};

	//SWAP, swaps each occurrence of a type A or B with the other one
	template<class...> struct swap;

	template<class... Types, class TypeA, class TypeB>
	struct swap<type_list<Types...>, TypeA, TypeB>
	{
		using type =
			type_list<
				std::conditional_t<
					std::is_same_v<Types, TypeA>,
					TypeB,

					std::conditional_t<
						std::is_same_v<Types, TypeB>,
						TypeA,
						Types
					>
				>...
			>;
	};

	//CONCATENATE
	template<class...> struct concatenate_internal;

	template<class... TypesLHS, class... TypesRHS>
	struct concatenate_internal<type_list<TypesLHS...>, type_list<TypesRHS...>>
	{
		using type = type_list<TypesLHS..., TypesRHS...>;
	};

	template<class...> struct concatenate;

	template<class TypeList>
	struct concatenate<TypeList>
	{
		using type = std::enable_if_t<is_type_list<TypeList>::value, TypeList>;
	};

	template<class TypeList0, class TypeList1>
	struct concatenate<TypeList0, TypeList1>
	{
		using type = typename concatenate_internal<TypeList0, TypeList1>::type;
	};

	template<class TypeList0, class TypeList1, class... TypeLists>
	struct concatenate<TypeList0, TypeList1, TypeLists...>
	{
		using type = typename concatenate<
			typename concatenate<TypeList0, TypeList1>::type,
			TypeLists...
		>::type;
	};

	//REMOVE_IF
	template<class, template<class> class> struct remove_if;

	template<class... Types, template<class> class UnaryPredicate>
	struct remove_if<type_list<Types...>, UnaryPredicate>
	{
		using type = typename concatenate<
			std::conditional_t<
				UnaryPredicate<Types>::value,
				type_list<>,
				type_list<Types>
			>...
		>::type;
	};

	//REMOVE
	template<class TypeList, class TypeToRemove>
	struct remove
	{
		template<class OtherType>
		using isTypeToRemove = std::is_same<OtherType, TypeToRemove>;

		using type = typename remove_if<TypeList, isTypeToRemove>::type;
	};

	//COUNT_IF
	template<class, template<class> class> struct count_if;

	template<template<class> class UnaryPredicate>
	struct count_if<type_list<>, UnaryPredicate>
		: std::integral_constant<std::size_t, 0>
	{		
	};

	template<class T0, class... Types, template<class> class UnaryPredicate>
	struct count_if<type_list<T0, Types...>, UnaryPredicate>
		: std::integral_constant<std::size_t,
			count_if<type_list<Types...>, UnaryPredicate>::value +
			(UnaryPredicate<T0>::value ? 1 : 0)>
	{
	};

	//COUNT
	template<class...> struct count;

	template<class... Types, class TypeToCount>
	struct count<type_list<Types...>, TypeToCount>
	{
		template<class OtherType>
		using isTypeToCount = std::is_same<OtherType, TypeToCount>;

		using type = count_if<type_list<Types...>, isTypeToCount>;
		static constexpr std::size_t value = type::value;
	};

	//IS_UNIQUE
	template<class...> struct is_unique;

	template<class... Types>
	struct is_unique<type_list<Types...>>
	{
		template<class SomeTypeFromTypeList>
		using appears_once = std::bool_constant<
			count<type_list<Types...>, SomeTypeFromTypeList>::value == 1
		>;

		using type = std::conjunction<appears_once<Types>...>;
		static constexpr bool value = type::value;
	};

	//IS_SORTED
	template<class, template<class, class> class> struct is_sorted;

	template<template<class, class> class Compare>
	struct is_sorted<type_list<>, Compare>
		: std::bool_constant<true>
	{
	};

	template<class T0, template<class, class> class Compare>
	struct is_sorted<type_list<T0>, Compare>
		: std::bool_constant<true>
	{
	};

	template<class T0, class T1, template<class, class> class Compare>
	struct is_sorted<type_list<T0, T1>, Compare>
		: std::bool_constant<Compare<T0, T1>::value>
	{
	};

	template<class T0, class T1, class... Types, template<class, class> class Compare>
	struct is_sorted<type_list<T0, T1, Types...>, Compare>
		: std::bool_constant<
			is_sorted<type_list<T0, T1>, Compare>::value &&
			is_sorted<type_list<T1, Types...>, Compare>::value
		>
	{
	};


	//TODO: SORT
	//MERGE SORT:


	//TRANSFORM
	template<class, template<class> class> struct transform;

	template<class... Types, template<class> class Func>
	struct transform<type_list<Types...>, Func>
	{
		using type = type_list<Func<Types...>>;
	};
}