#ifndef STREAM_ITERATOR_HPP
#define STREAM_ITERATOR_HPP

#include "parser_combinators.hpp"

using namespace std;

#ifdef USE_MMAP

#include "File-Vector/file_vector.hpp"

class stream_range {
    file_vector<char> file;

public:
    using iterator = file_vector<char>::const_iterator;

    iterator const last;
    iterator const first;

    stream_range(stream_range const&) = delete;

    stream_range(char const* name) : file(name), first(file.cbegin()), last(file.cend()) {}
    stream_range(string const& name) : stream_range(name.c_str()) {}
};

#else // USE_MMAP

#include <streambuf>
#include <fstream>

class stream_range {
    fstream file;
    streambuf *rd;
    streamoff pos;

public:
    class iterator {
        friend class stream_range;
 
        iterator(stream_range *r, streamoff pos) : r(r), pos(pos), sym(r->rd->sgetc()) {
            r->pos = pos;
        }

        stream_range *r;
        streamoff pos;
        int sym;

    public:
        int operator* () const {
            return sym;
        }

        bool operator== (iterator const& i) const {
            return (r == i.r) && (sym == i.sym) && (pos == i.pos);
        }
        
        bool operator!= (iterator const& i) const {
            return (r != i.r) || (sym != i.sym) || (pos != i.pos);
        }

        streamoff operator- (iterator const& i) const {
            return pos - i.pos;
        }

        iterator& operator++ () {
            if (r->pos != pos) {
                r->pos = r->rd->pubseekoff(++pos, ios_base::beg);
                sym = r->rd->sgetc();
            } else {
                r->pos = ++pos;
                sym = r->rd->snextc();
            }
            return *this;
        }

        iterator& operator-- () {
            --pos;
            if (r->pos != pos) { 
                r->pos = r->rd->pubseekoff(pos, ios_base::beg);
            }
            sym = r->rd->sgetc();
            return *this;
        }

        iterator& operator= (iterator const& i) {
            r = i.r;
            pos = i.pos;
            sym = i.sym;
            return *this;
        }
    };

    friend class stream_range::iterator;

    iterator const last;
    iterator const first;

    stream_range(stream_range const&) = delete;

    stream_range(char const* name) : file(name, ios_base::in),
        rd(file.rdbuf()), pos(0),
        last(this, rd->pubseekoff(0, ios_base::end)),
        first(this, rd->pubseekoff(0, ios_base::beg)
    ) {
        if (!file.is_open()) {
            throw runtime_error("unable to open file");
        }
    }

    stream_range(string const& name) : stream_range(name.c_str()) {}
};

#endif // USE_MMAP

template <typename T> using pstream_handle = parser_handle<stream_range::iterator, stream_range, T>;

#endif // STREAM_ITERATOR_HPP
