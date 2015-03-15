all: test_simple test_combinators stream_expression vector_expression prolog test.csv test.exp

CFLAGS=-ggdb -march=native -O3 -flto -std=c++11

debug: CFLAGS+=-DDEBUG
debug: all

clang: CXX=clang++
clang: all

clean:
	rm -f test_combinators test_simple stream_expression vector_expression prolog test.csv mkexp test.exp mkcsv 

test_combinators: test_combinators.cpp templateio.hpp parser_combinators.hpp function_traits.hpp profile.hpp stream_iterator.hpp
	${CXX} ${CFLAGS} -o test_combinators test_combinators.cpp

test_simple: test_simple.cpp templateio.hpp parser_simple.hpp profile.hpp
	${CXX} ${CFLAGS} -o test_simple test_simple.cpp

stream_expression: example_expression.cpp templateio.hpp parser_combinators.hpp function_traits.hpp profile.hpp stream_iterator.hpp
	${CXX} ${CFLAGS} -o stream_expression example_expression.cpp

vector_expression: example_expression.cpp templateio.hpp parser_combinators.hpp function_traits.hpp profile.hpp File-Vector/file_vector.hpp
	${CXX} ${CFLAGS} -DUSE_MMAP -o vector_expression example_expression.cpp

prolog: prolog.cpp templateio.hpp parser_combinators.hpp function_traits.hpp profile.hpp File-Vector/file_vector.hpp
	${CXX} ${CFLAGS} -DUSE_MMAP -o prolog prolog.cpp

mkexp: mkexp.cpp
	${CXX} ${CFLAGS} -o mkexp mkexp.cpp

mkcsv: mkcsv.cpp
	${CXX} ${CFLAGS} -o mkcsv mkcsv.cpp

test.csv: mkcsv
	./mkcsv > test.csv

test.exp: mkexp
	./mkexp > test.exp
	
