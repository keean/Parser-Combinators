Parser-Combinators
==================

<b>The latest version now supports functionality equivalent to an attribute grammar, where the parser result (synthesized attribute) is returned bottom up, and state (inherited attribute) is threaded trough the parsers accumulating values left-to-right. This needs some care to interact with backtracking, as the state needs to be backtracked as well. The 'attempt' parser combinator deals with saving the current result and state and restoring it on backtracking, but this relies on the state being copyable. If there is only single-character look-ahead failed parsers do not consume characters, and so backtracking is unnecessary, and the state does not need to by copyable.</b>

A high performance C++ parser combinator library, focusing static instantiation of combinators, which differentiates it from other libraries such as Boost.Spirit. The library design ensures that all combinator composition occurs at compile time, with a special construct (a parser-handle) used to allow dynamic runtime polymorphism at specific points.

As backtraking is supported, parsers can generally consist of a set of independent static parse rules, and a single parser-handle to enable polymorphic recursion. However higher level parser combinators can also be implemented that take parser-handles as their arguments.

This gives the programmer control over whether polymorphism is static or dynamic, and allows optimal run-time performance. Because the combinators are implemented as static template function-objects, they can be inlined by the compiler, which results in performance better than the simple recursive-descent parser, combined with more readable and maintainable code.

The library now uses an Iterator and Range pair, and provides a stream_range that makes backtracking much neater in the implementation, results in a 25% performance improvement compared to the pre-iterator version on non-backtracking parsers, and even more (40% improvement) on backtracking parsers. The combinator parser with stream iterator is now about twice the speed of the simple recursive descent parser, and the iterator interface can be used with the File-Vector which doubles the performance again. Swapping between the stream range/iterator and the file_vector range/iterator is now controlled by defining USE_MMAP, without needing to change the source code.

See "test_combinators.cpp", "example_expression.cpp", and "prolog.cpp" for usage examples.
