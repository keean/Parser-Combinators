all : test_simple test_combinators

clean:
	rm -f test_combinators test_simple

test_combinators: test_combinators.cpp templateio.hpp parser_combinators.hpp function_traits.hpp
	clang++ -ggdb -march=native -O3 -flto -std=c++11 -o test_combinators test_combinators.cpp

test_simple: test_simple.cpp templateio.hpp parser_simple.hpp 
	clang++ -ggdb -march=native -O3 -flto -std=c++11 -o test_simple test_simple.cpp

