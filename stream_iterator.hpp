#include <streambuf>
#include <fstream>

class stream_range {
    streambuf *rd;
    streamoff pos;

public:

    class const_iterator {
        stream_range *r;
        streamoff pos;
        int sym;

    public:
        const_iterator() : r(nullptr), pos(0), sym(0) {}

        const_iterator(stream_range *r, streamoff pos) : r(r), pos(pos), sym(r->rd->sgetc()) {
            r->pos = pos;
        }

        int operator* () const {
            return sym;
        }

        bool operator== (const_iterator const& i) const {
            return (r == i.r) && (sym == i.sym) && (pos == i.pos);
        }
        
        bool operator!= (const_iterator const& i) const {
            return (r != i.r) || (sym != i.sym) || (pos != i.pos);
        }

        streamoff operator- (const_iterator const& i) const {
            return pos - i.pos;
        }

        streamoff operator- (streamoff const& i) const {
            return pos - i;
        }

        const_iterator& operator++ () {
            if (r->pos != pos) {
                r->pos = r->rd->pubseekoff(pos, ios_base::beg);
            } 
            sym = r->rd->snextc();
            ++(r->pos);
            ++pos;
            return *this;
        }

        /*const_iterator& operator-- () {
            --pos;
            if (r->pos != pos) { 
                r->pos = r->rd->pubseekoff(pos, ios_base::beg);
            }
            sym = r->rd->sgetc();
            return *this;
        }*/

        const_iterator& operator= (const_iterator const& i) {
            r = i.r;
            pos = i.pos;
            sym = i.sym;
            return *this;
        }

        const_iterator& operator= (streamoff const& i) {
            pos = i;
            if (r->pos != pos) { 
                r->pos = r->rd->pubseekoff(pos, ios_base::beg);
            }
            sym = r->rd->sgetc();
            return *this;
        }
    };

    friend class stream_range::const_iterator;

    stream_range(streambuf *rd) : rd(rd), pos(0) {}

    const_iterator cbegin() {
        return const_iterator(this, rd->pubseekoff(0, ios_base::beg));
    }

    const_iterator cend() {
        return const_iterator(this, rd->pubseekoff(0, ios_base::end));
    }
};
     
