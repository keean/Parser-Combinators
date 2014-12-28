#include <streambuf>
#include <fstream>

class stream_range {
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
                r->pos = r->rd->pubseekoff(pos, ios_base::beg);
            } 
            sym = r->rd->snextc();
            ++(r->pos);
            ++pos;
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

    stream_range(streambuf *rd) : rd(rd), pos(0),
        last(this, rd->pubseekoff(0, ios_base::end)),
        first(this, rd->pubseekoff(0, ios_base::beg)) {}
};
     
