all : test_simple test_combinators test.csv example_expression

test.csv:
	printf "" > test.csv; \
	for j in $$(seq 1 1000); do \
	for i in $$(seq 1 999); do \
	printf "$$[($$RANDOM % 10) + 1], " >> test.csv; \
	done; \
	printf "$$[($$RANDOM % 10) + 1]\n" >> test.csv; \
	done;

clean:
	rm -f test_combinators test_simple example_expression

test_combinators: test_combinators.cpp templateio.hpp parser_combinators.hpp function_traits.hpp profile.hpp
	clang++ -ggdb -march=native -O3 -flto -std=c++11 -o test_combinators test_combinators.cpp

test_simple: test_simple.cpp templateio.hpp parser_simple.hpp profile.hpp
	clang++ -ggdb -march=native -O3 -flto -std=c++11 -o test_simple test_simple.cpp

example_expression: example_expression.cpp templateio.hpp parser_combinators.hpp function_traits.hpp profile.hpp
	clang++ -ggdb -march=native -O3 -flto -std=c++11 -o example_expression example_expression.cpp

