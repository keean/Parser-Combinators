all: test_simple test_combinators stream_expression vector_expression prolog test.csv test.exp

debug: CFLAGS+="-DDEBUG"
debug: all

clean:
	rm -f test_combinators test_simple stream_expression vector_expression prolog test.csv mkexp test.exp mkcsv 

test_combinators: test_combinators.cpp templateio.hpp parser_combinators.hpp function_traits.hpp profile.hpp stream_iterator.hpp
	clang++ ${CFLAGS} -ggdb -march=native -O3 -flto -std=c++11 -o test_combinators test_combinators.cpp

test_simple: test_simple.cpp templateio.hpp parser_simple.hpp profile.hpp
	clang++ ${CFLAGS} -ggdb -march=native -O3 -flto -std=c++11 -o test_simple test_simple.cpp

stream_expression: example_expression.cpp templateio.hpp parser_combinators.hpp function_traits.hpp profile.hpp stream_iterator.hpp
	clang++ ${CFLAGS} -ggdb -march=native -O3 -flto -std=c++11 -o stream_expression example_expression.cpp

vector_expression: example_expression.cpp templateio.hpp parser_combinators.hpp function_traits.hpp profile.hpp File-Vector/file_vector.hpp
	clang++ ${CFLAGS} -DUSE_MMAP -ggdb -march=native -O3 -flto -std=c++11 -o vector_expression example_expression.cpp

prolog: prolog.cpp templateio.hpp parser_combinators.hpp function_traits.hpp profile.hpp File-Vector/file_vector.hpp
	clang++ ${CFLAGS} -DUSE_MMAP -ggdb -march=native -O3 -flto -std=c++11 -o prolog prolog.cpp

mkexp: mkexp.cpp
	clang++ ${CFLAGS} -ggdb -march=native -O3 -flto -std=c++11 -o mkexp mkexp.cpp

mkcsv: mkcsv.cpp
	clang++ ${CFLAGS} -ggdb -march=native -O3 -flto -std=c++11 -o mkcsv mkcsv.cpp

test.csv: mkcsv
	./mkcsv > test.csv

test.exp: mkexp
	./mkexp > test.exp
	
