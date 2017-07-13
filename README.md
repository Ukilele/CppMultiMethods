# CppMultiMethods
C++17 Open Multi-Methods

This is a header-only library for open Multi-Methods in C++. The library only compiles with C++17 (tested with VS2017 and clang).
The example.cpp shows two examples of usage. It provides a lot of flexibility.
You neither need any virtual "acceptMyVisitor"-function nor a macro.
It is protable to C++14 but you need to write some templates like std::bool_constant and std::conjunction by hand.
As I am using lambdas with variadic callable operator we need C++14 at least.
