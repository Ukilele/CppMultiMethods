#pragma once

#include "CommonFunctionType.h"
#include "TypeList.h"

#include <memory> //addressof
#include <stdexcept> //default_error_policy
#include <type_traits>

namespace cpp_test
{
	struct default_error_policy
	{
		void operator()() const {
			throw std::runtime_error("MultiMethod got called with wrong arguments");
		}
	};

	struct dynamic_cast_policy
	{
		template<class To, class From>
		To operator()(From f) const {
			return dynamic_cast<To>(f);
		}
	};

	struct static_cast_policy
	{
		template<class To, class From>
		To operator()(From f) const {
			return static_cast<To>(f);
		}
	};

	namespace detail
	{
		template<class HierarchicType1, class HierarchicType2>
		using correctHierarchyComparator = std::bool_constant<
				(std::is_base_of_v<HierarchicType2, HierarchicType1>
				|| !std::is_base_of_v<HierarchicType1, HierarchicType2>)
			>;

		template<class TypeHierarchy>
		using is_corretcly_ordererd_type_hierarchy = typename is_sorted<TypeHierarchy, correctHierarchyComparator>::type;

		template<class ArgumentType>
		struct is_polymorphically_related_to
		{
			template<class HierarchyType>
			struct create
				: std::bool_constant<
					std::is_base_of_v<ArgumentType, HierarchyType> ||
					std::is_base_of_v<HierarchyType, ArgumentType>
				>
			{
			};
		};

		template<class SourceType, class TargetType>
		struct ref_const_volatile_applier
		{
			static constexpr bool use_ref = std::is_lvalue_reference_v<SourceType>;
			static constexpr bool use_const = std::is_const_v<std::remove_reference_t<SourceType>>;
			static constexpr bool use_volatile = std::is_volatile_v<std::remove_reference_t<SourceType>>;

			using volatile_type = std::conditional_t<use_volatile, TargetType volatile, TargetType>;
			using const_type = std::conditional_t<use_const, volatile_type const, volatile_type>;
			using ref_type = std::conditional_t<use_ref, const_type&, const_type>;

			using type = ref_type;
		};

		template<class SourceType, class TargetType>
		using ref_const_volatile_applier_t = typename ref_const_volatile_applier<SourceType, TargetType>::type;

		template<class TypeList>
		constexpr void ValidateTypeList() noexcept
		{
			static_assert(is_type_list<TypeList>::value);
			static_assert(size<TypeList>::value > 0);
			static_assert(all_of<TypeList, std::is_polymorphic>::value);
			//static_assert(is_unique<TypeList>::type::value); //Visual Studio scheint da nicht mit klarzukommen
			static_assert(detail::is_corretcly_ordererd_type_hierarchy<TypeList>::value);
		}

		template<class TypeList, class Argument>
		constexpr void ValidateTypeListAndTheArgument() noexcept
		{
			ValidateTypeList<TypeList>();
			//TODO:
			//Das hier klappt für int, std::string, char const(&)[1] nicht. Warum?
			//Clang compiliert es gar nicht erst.
			//static_assert(all_of<TypeList, is_polymorphically_related_to<std::decay_t<Argument>>::create>::value);
		}


		//MULTI DISPATCH


		//Castet ActualType&& arg zu dem tatsächlichen Typen Target_HierarchyType aus der Hierarchy.
		//Gibt einen functor zurück, der beliebige argument erwartet und die N_AryFunction m_f mit dem gecasteten
		//arg aufruft und den beliebigen argumenten.
		template<class...> struct SingleHierarchyDispatcher;

		template<class Target_HierarchyType>
		struct SingleHierarchyDispatcher<type_list<Target_HierarchyType>>
		{
			template<class N_AryFunction, class ActualType>
			static decltype(auto) getFunction(N_AryFunction&& f, ActualType&& arg)
			{
				static_assert(!(is_type_list<Target_HierarchyType>::value));
				using target_type = detail::ref_const_volatile_applier_t<decltype(arg), Target_HierarchyType>;
				auto const target_arg_ptr = dynamic_cast<std::remove_reference_t<target_type>*>(std::addressof(arg));
				if (target_arg_ptr == nullptr)
					default_error_policy{}();
				auto func = [p = target_arg_ptr, &f](auto&&... otherTypes) mutable {
					return std::forward<N_AryFunction>(f)(std::forward<target_type>(*p), std::forward<decltype(otherTypes)>(otherTypes)...);
				};
				return func;
			}
		};

		template<class CurrentTargetTypeOfMyHierarchy, class... RemainingTypesOfMyHierarchy>
		struct SingleHierarchyDispatcher<type_list<CurrentTargetTypeOfMyHierarchy, RemainingTypesOfMyHierarchy...>>
		{
			template<class N_AryFunction, class ActualType>
			static decltype(auto) getFunction(N_AryFunction&& f, ActualType&& arg)
			{
				using target_type = detail::ref_const_volatile_applier_t<decltype(arg), CurrentTargetTypeOfMyHierarchy>;
				auto const target_arg_ptr = dynamic_cast<std::remove_reference_t<target_type>*>(std::addressof(arg));

				using NextFuncType = decltype(SingleHierarchyDispatcher<type_list<RemainingTypesOfMyHierarchy...>>::getFunction(
					std::forward<N_AryFunction>(f), std::forward<ActualType>(arg)));
				using CurrentFuncType = decltype(SingleHierarchyDispatcher<type_list<CurrentTargetTypeOfMyHierarchy>>::getFunction(
					std::forward<N_AryFunction>(f), std::forward<ActualType>(arg)));
				using CommonFunctionReturnType = CommonFunctionType2D<NextFuncType, CurrentFuncType>;

				if (target_arg_ptr == nullptr) {
					auto nextFunc = SingleHierarchyDispatcher<type_list<RemainingTypesOfMyHierarchy...>>::getFunction(
						std::forward<N_AryFunction>(f), std::forward<ActualType>(arg));
					return CommonFunctionReturnType{ nextFunc };
				}
				else {
					auto currentFunc = SingleHierarchyDispatcher<type_list<CurrentTargetTypeOfMyHierarchy>>::getFunction(
						std::forward<N_AryFunction>(f), std::forward<ActualType>(arg));
					return CommonFunctionReturnType{ currentFunc };
				}
			}
		};

		template<class...> struct MultiMethodsDispatcher;

		template<class LastTypeHierarchy>
		struct MultiMethodsDispatcher<type_list<LastTypeHierarchy>>
		{
			template<class UnaryFunction, class ActualArgFromLastTypeHierarchy>
			static decltype(auto) dispatch(UnaryFunction&& f, ActualArgFromLastTypeHierarchy&& lastArg)
			{
				static_assert(is_type_list<LastTypeHierarchy>::value);
				static_assert(!is_type_list<ActualArgFromLastTypeHierarchy>::value);
				auto&& finalFunction = SingleHierarchyDispatcher<LastTypeHierarchy>::getFunction(std::forward<UnaryFunction>(f),
					std::forward<ActualArgFromLastTypeHierarchy>(lastArg));
				return std::forward<decltype(finalFunction)>(finalFunction)();
			}
		};

		template<class FirstTypeHierarchy, class... RemainingTypeHierarchies>
		struct MultiMethodsDispatcher<type_list<FirstTypeHierarchy, RemainingTypeHierarchies...>>
		{			
			template<class N_AryFunction, class ActualArgFromFirstTypeHierarchy, class... ActualArgsFromRemainingTypeHierarchies>
			static decltype(auto) dispatch(N_AryFunction&& f, ActualArgFromFirstTypeHierarchy&& a0, ActualArgsFromRemainingTypeHierarchies&&... args) {
				static_assert(sizeof...(ActualArgsFromRemainingTypeHierarchies) == sizeof...(RemainingTypeHierarchies));
				static_assert(is_type_list<FirstTypeHierarchy>::value);

				auto&& nMinus1AryFunc = SingleHierarchyDispatcher<FirstTypeHierarchy>::getFunction(std::forward<N_AryFunction>(f),
					std::forward<ActualArgFromFirstTypeHierarchy>(a0));
				
				using nextHierarchyDispatcher = MultiMethodsDispatcher<type_list<RemainingTypeHierarchies...>>;
				return nextHierarchyDispatcher::dispatch(std::forward<decltype(nMinus1AryFunc)>(nMinus1AryFunc), std::forward<ActualArgsFromRemainingTypeHierarchies>(args)...);
			}
		};
	}

	//Multiple dispatch
	template<class... TypeLists>
	struct for_type_hierarchies
	{
		template<class MultiFunction, class... Args>
		static decltype(auto) dispatch(MultiFunction&& f, Args&&... args)
		{
			//TODO: Cast-policy und error-policy einbauen
			static_assert(sizeof...(TypeLists) > 0);
			static_assert(sizeof...(TypeLists) == sizeof...(Args));
			using swallow = int[];
			(void)swallow{ (detail::ValidateTypeListAndTheArgument<TypeLists, Args>(), 0)... };

			return detail::MultiMethodsDispatcher<type_list<TypeLists...>>::dispatch(std::forward<MultiFunction>(f),
				std::forward<Args>(args)...);
		}
	};
}