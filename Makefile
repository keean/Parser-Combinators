all : test-simple test-combinators

clean:
	rm -f test-combinators test-simple

test-combinators: test-combinators.cpp parser-combinators.h
	clang++ -ggdb -march=native -O3 -flto -std=c++11 -o test-combinators test-combinators.cpp

test-simple: test-simple.cpp parser-simple.h
	clang++ -ggdb -march=native -O3 -flto -std=c++11 -o test-simple test-simple.cpp

