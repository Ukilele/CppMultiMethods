#include "MultiMethods.h"

#include <cassert>

//Create some type hierarchies. It is important that each type is virtual => it needs one virtual function
struct A0 { virtual ~A0() = default; };
struct A1 : A0 {};
struct A2 : A1 {};

struct B0 { virtual ~B0() = default; };
struct B1 : B0 {};

struct C0 { virtual ~C0() = default; };
struct C1 : C0 {};

//Createa functor that can be called with all combinations of A, B and C
struct ABC_Visitor
{
	int operator()(A0 const&, B0 const&, C0 const&) { return 0; }
	int operator()(A0 const&, B0 const&, C1 const&) { return 1; }
	int operator()(A0 const&, B1 const&, C0 const&) { return 2; }
	int operator()(A0 const&, B1 const&, C1 const&) { return 3; }

	int operator()(A1 const&, B0 const&, C0 const&) { return 4; }
	int operator()(A1 const&, B0 const&, C1 const&) { return 5; }
	int operator()(A1 const&, B1 const&, C0 const&) { return 6; }
	int operator()(A1 const&, B1 const&, C1 const&) { return 7; }

	int operator()(A2 const&, B0 const&, C0 const&) { return 8; }
	int operator()(A2 const&, B0 const&, C1 const&) { return 9; }
	int operator()(A2 const&, B1 const&, C0 const&) { return 10; }
	int operator()(A2 const&, B1 const&, C1 const&) { return 11; }

	ABC_Visitor(ABC_Visitor const&) = delete;
	void operator=(ABC_Visitor const&) = delete;
	ABC_Visitor() = default;
};

struct EqualTypes
{
	template<class T, class U>
	bool operator()(T const&, U const&) const { return false; }

	template<class T>
	bool operator()(T const&, T const&) const { return true; }
};



int main()
{
	A2 aObj;
	A0 const& a = aObj;
	B1 bObj;
	B0 const& b = bObj;
	C1 cObj;
	C0 const& c = cObj;

	using namespace cpp_test;

	//First pass all possible type-hierarchies
	int ret = for_type_hierarchies<
		type_list<A2, A1, A0>,
		type_list<B1, B0>,
		type_list<C1, C0>
	>::dispatch(ABC_Visitor{}, a, b, c); //then dispatch on a, b and c
	assert(ret == 11);

	bool equalRuntimeType = for_type_hierarchies<
		type_list<A2, A1, A0>,
		type_list<A2, A1, A0>
	>::dispatch(EqualTypes{}, aObj, a);
	assert(equalRuntimeType);

}