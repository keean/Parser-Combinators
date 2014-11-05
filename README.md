Parser-Combinators
==================

A high performance C++ parser combinator library, focusing static instantiation of combinators, which differentiates it from other libraries such as Boost.Spirit. The library design ensures that all combinator composition occurs at compile time, with a special construct (a parser-handle) used to allow dynamic runtime polymorphism at specific points.

As backtraking is supported, parsers can generally consist of a set of independent static parse rules, and a single parser-handle to enable polymorphic recursion. However higher level parser combinators can also be implemented that take parser-handles as their arguments.

This gives the programmer control over whether polymorphism is static or dynamic, and allows optimal run-time performance. Because the combinators are implemented as static template function-objects, they can be inlined by the compiler, which results in performance better than the simple recursive-descent parser, combined with more readable and maintainable code.

See "test_combinators.cpp" and "example_expression.cpp" for usage examples.
