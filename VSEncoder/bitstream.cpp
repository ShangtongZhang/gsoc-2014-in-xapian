#include "bitstream.h"

#include <cmath>
#include <vector>

using namespace std;

static const unsigned char flstab[256] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

const unsigned char Xapian::OrdinaryDecoder::mask_nbits[8][9] =
{
	0, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff,
	0, 0x40, 0x60, 0x70, 0x78, 0x7c, 0x7e, 0x7f, 0,
	0, 0x20, 0x30, 0x38, 0x3c, 0x3e, 0x3f, 0, 0,
	0, 0x10, 0x18, 0x1c, 0x1e, 0x1f, 0, 0, 0,
	0, 0x8, 0xc, 0xe, 0xf, 0, 0, 0, 0,
	0, 0x4, 0x6, 0x7, 0, 0, 0, 0, 0,
	0, 0x2, 0x3, 0, 0, 0, 0, 0, 0,
	0, 0x1, 0, 0, 0, 0, 0, 0, 0
};

const unsigned char Xapian::UnaryEncoder::mask_1s[8] = {
    0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff
};

const unsigned int Xapian::Encoder::mask_low_n_bits[33] = {
	0,0x1,0x3,0x7,0xf,0x1f,0x3f,0x7f,0xff,
    0x1ff,0x3ff,0x7ff,0xfff,0x1fff,0x3fff,0x7fff,0xffff,
    0x1ffff,0x3ffff,0x7ffff,0xfffff,0x1fffff,0x3fffff,0x7fffff,0xffffff,
    0x1ffffff,0x3ffffff,0x7ffffff,0xfffffff,0x1fffffff,0x3fffffff,0x7fffffff,0xffffffff
};

// @return : the value returned is an integer which is no less than log2(@val) when @up is ture, 
//				or is no more than log2(@val) when @up is false
inline int log2(unsigned val, bool up )
{
    int result = 0;
    if (val >= 0x10000u) {
	val >>= 16;
	result = 16;
    }
    if (val >= 0x100u) {
	val >>= 8;
	result += 8;
    }
    result += flstab[val];
	if ( up )
	{
		if ( val == 1<<(result-1) )
		{
			result--;
		}
	}
	else
	{
		result--;
	}

	return result;
}

unsigned int get_Unary_encode_length( unsigned int n )
{
	return n;
}

unsigned int get_Gamma_encode_length( unsigned int n )
{
	return 2*log2(n,false)+1;
}

namespace Xapian {

//Encoder.cpp


inline bool Encoder::check_acc()
{
	if ( bits == 8 )
	{
		buf += acc;
		acc = 0;
		bits = 0;
		return true;
	}
	return false;
}

    void UnaryEncoder::encode(unsigned int n) {
        int num_of_1s = n-1;
        if (n == 1) {
            acc <<= 1;
            bits++;
            check_acc();
            return;
        }
        if (bits + num_of_1s <= 8) {
            acc <<= num_of_1s;
            bits += num_of_1s;
            acc |= mask_1s[num_of_1s-1];
            check_acc();
            acc <<= 1;
            bits++;
            check_acc();
            return;
        }
        
        acc <<= 8-bits;
        acc |= mask_1s[7-bits];
        buf += acc;
        num_of_1s -= 8-bits;
        acc = 0;
        bits = 0;
        while (num_of_1s > 8) {
            buf += (char)0xff;
            num_of_1s -= 8;
        }
        
        acc |= mask_1s[num_of_1s-1];
        bits = num_of_1s;
        check_acc();
        
        acc = acc << 1;
        bits++;
        check_acc();
    }

    void GammaEncoder::encode(unsigned int n) {
        
        // get |bin(@n)|
        int n_bin_bits = log2(n,false)+1;
        
        // encode |bin(@n)| using Unary Encoder
        UnaryEncoder u(buf, acc, bits);
        u.encode(n_bin_bits);
        
        // get low |bin(@n)|-1 bits of @n
        n = n & mask_low_n_bits[n_bin_bits-1];
        
        // the highest bit needn't encoding
        n_bin_bits--;
        
        // encoding one bit every time
        for (int i = n_bin_bits-1 ; i >= 0 ; i--) {
            acc <<= 1;
            acc |= (n&(1<<i))>>i;
            bits++;
            check_acc();
        }
    }

void OrdinaryEncoder::encode( unsigned int n )
{
	unsigned int cur_width = num_of_bits;
	while ( cur_width )
	{
		if ( cur_width + bits <= 8 )
		{
			acc <<= cur_width;
			acc |= mask_low_n_bits[cur_width]&n;
			bits += cur_width;
			if ( bits == 8 )
			{
				buf += acc;
				acc = 0;
				bits = 0;
			}
			return;
		}
		else
		{
			acc <<= (8-bits);
			acc |= mask_low_n_bits[8-bits]&(n>>(cur_width-8+bits));
			cur_width -= 8-bits;
			buf += acc;
			acc = 0;
			bits = 0;
		}

	}
}

VSEncoder::VSEncoder( std::string& chunk_, int maxK_ )
	: chunk(chunk_), maxK(maxK_)
{
	acc = 0;
	bits = 0;


}

unsigned int VSEncoder::get_optimal_split2(const std::vector<unsigned int>& L, std::vector<unsigned int>& S)
{
	unsigned int n = (unsigned)L.size();
	double min_good_bytes_ratio = 0.3; 
	int pre_p, cur_p;
	pre_p = cur_p = 0;
	int max_bits = 0;
	int cur_bits = 0;
	int used_bits = 0;
	int good_bits = 0;
	S.push_back(0);
	while (true)
	{
		if (cur_p == n) {
			S.push_back(cur_p);
			break;
		}
		cur_bits = log2(L[cur_p]);
		if (max_bits < cur_bits) {
			max_bits = cur_bits;
			used_bits = (cur_p-pre_p+1)*max_bits;
		} else {
			used_bits += max_bits;
		}
		good_bits += cur_bits;
		if ((float)good_bits/(float)used_bits < min_good_bytes_ratio) {
			S.push_back(cur_p);
			max_bits = 0;
			cur_bits = 0;
			used_bits = 0;
			good_bits = 0;
			pre_p = cur_p;
			continue;
		}
		++cur_p;
	}
	return 0;
}

unsigned int VSEncoder::get_optimal_split( const std::vector<unsigned int>& L, std::vector<unsigned int>& S )
{
	unsigned int n = (unsigned)L.size();
	std::vector<unsigned int> E,P;
	std::vector<unsigned> log2L;
	log2L.resize(n);
	for (int i=0 ; i<(int)n ; ++i )
	{
		log2L[i] = log2(L[i]);
	}
	E.resize( n+1 );
	P.resize( n+1 );
	E[0] = 0;
	P[0] = 0;
	int down_edge = 0;
	int cur_bits = 0;
	unsigned int cost_j_i = 0;
	for ( int i = 1 ; i <= (int)n ; ++i )
	{
		int b = 0;
		E[i] = ~0;
		if ( i-maxK > 0 )
		{
			down_edge = i-maxK;
		}
		else
		{
			down_edge = 0;
		}
		for ( int j = i-1 ; j >= down_edge ; --j )
		{
			cur_bits = log2L[j];
			if( b< cur_bits )
			{
				b = cur_bits;
			}
			//unsigned int cost_j_i = (i-j)*b+get_Gamma_encode_length(b+1)+get_Unary_encode_length(i-j);
			cost_j_i = (i-j)*b+2*log2(b+1,false)+1+i-j;
			if ( E[j]+cost_j_i < E[i] )
			{
				E[i] = E[j]+cost_j_i;
				P[i] = j;
			}
		}
	}
	unsigned int cur = n;
	while ( P[cur] != cur )
	{
		S.push_back(cur);
		cur = P[cur];
	}
	S.push_back(cur);
	return E[n];
}

void VSEncoder::encode( const std::vector<unsigned int>& L_ )
{
	vector<unsigned int> L;
	L.push_back( L_[0] );
	for ( int i = 0 ; i < (int)L_.size()-1 ; ++i )
	{
		L.push_back( L_[i+1]-L_[i] );
	}
	std::vector<unsigned int> S;
	get_optimal_split2( L, S );
	for ( int i = 0 ; i < (int)S.size()-1 ; ++i )
	{
		encode( L, S[i], S[i+1] );
	}
	//get_optimal_split( L, S );
	//for ( int i = S.size() - 1; i > 0; --i )
	//{
	//	encode( L, S[i], S[i-1] );
	//}
    string header;
	header += acc;
    header += (char)bits&0xff;
	unsigned int last_entry = L_.back();
	unsigned int n_entry = (unsigned)L_.size();
    pack_uint(header, n_entry);
    pack_uint(header, last_entry);
    chunk += header+buf;
}

void VSEncoder::encode( const std::vector<unsigned int>& L, int pstart, int pend )
{
	int b = 0;
	for ( int i = pstart ; i < pend ; ++i )
	{
		int tmp = log2(L[i],false)+1;
		b = tmp > b ? tmp : b;
	}
	int k = pend-pstart;
	GammaEncoder g( buf, acc, bits );
	g.encode( b+1 );
	UnaryEncoder u( buf, acc, bits );
	u.encode( k );
	OrdinaryEncoder o( buf, acc, bits, b );
	for ( int i = pstart ; i < pend ; ++i )
	{
		o.encode( L[i] );
	}

}


//Decoder.cpp

unsigned int Decoder::mask[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };

inline unsigned int Decoder::get_bit_value( unsigned int n, int i )
{
	if ( n & mask[i] )
	{
		return 1;
	}
	return 0;
}

unsigned int UnaryDecoder::decode()
{
	unsigned int n = 0;
    char cur_data = 0;
    if (pos != end)
	{
		cur_data = *pos;
	}
	else
	{
		cur_data = (char)acc;
	}
	unsigned char cur_bit = 0;
	cur_bit = cur_data&mask[p_bit];
	while ( cur_bit )
	{
		n++;
		p_bit++;
		if ( p_bit < 8)
		{
			cur_bit = cur_data&mask[p_bit];
		}
		if ( p_bit == 8 )
		{
            pos++;
			if (pos != end)
			{
				p_bit = 0;
                cur_data = *pos;
				cur_bit = cur_data&mask[p_bit];
			}
			else
			{
                p_bit = 8-acc_bits;
				cur_data = (char)acc;
				cur_bit = cur_data&mask[p_bit];
			}
		}
	}
	n++;
	p_bit++;
	if ( p_bit == 8 )
	{
        pos++;
        if (pos != end)
        {
            p_bit = 0;
            cur_data = *pos;
            cur_bit = cur_data&mask[p_bit];
        }
        else
        {
            p_bit = 8-acc_bits;
            cur_data = (char)acc;
            cur_bit = cur_data&mask[p_bit];
        }
	}	
	return n;
}

unsigned int GammaDecoder::decode()
{
	unsigned int n_bits = p_ud->decode();
	unsigned int n = 1;
    char cur_data = 0;
	n_bits--;
	if (pos == end)
	{
		while ( n_bits-- )
		{
			int tmp = get_bit_value( acc, p_bit );
			p_bit++;
			n = n*2 + tmp;
		}
	}
	else
	{
        cur_data = *pos;
		while ( n_bits-- )
		{
			int tmp = get_bit_value(cur_data, p_bit);
			n = 2*n + tmp;
			p_bit++;
			if ( p_bit == 8 )
			{
				p_bit = 0;
				pos++;
				if (pos == end)
				{
					cur_data = acc;
					p_bit = 8-acc_bits;
                    while (n_bits--)
                    {
                        int tmp = get_bit_value(acc, p_bit);
                        p_bit++;
                        n = 2*n + tmp;
                    }
					break;
				}
                cur_data = *pos;
			}
		}
	}
	return n;
}

unsigned int OrdinaryDecoder::decode()
{
	unsigned int n_bits = width;
	unsigned int n = 0;
	while ( n_bits )
	{
		if (pos != end)
		{
			if (n_bits <= (unsigned int)8-p_bit)
			{
				n <<= n_bits;
				unsigned char tmp = (*pos)&mask_nbits[p_bit][n_bits];
				tmp >>= ( 8-p_bit-n_bits );
				n |= tmp;
				p_bit += n_bits;
				if ( p_bit == 8 )
				{
                    pos++;
					if (pos == end)
					{
						p_bit = 8-acc_bits;
					}
					else
					{
                        p_bit = 0;
					}
				}
				return n;
			}
			else
			{
				unsigned char tmp = (*pos)&mask_nbits[p_bit][8-p_bit];
				n <<= (8-p_bit);
				n |= tmp;
				n_bits -= (8-p_bit);
                pos++;
				if (pos == end)
				{
					p_bit = 8-acc_bits;
				}
				else
				{
					p_bit = 0;
				}
				continue;
			}
		}
		else
		{
			n <<= n_bits;
			unsigned char tmp = acc&mask_nbits[p_bit][n_bits];
			tmp >>= ( 8-p_bit-n_bits );
			n |= tmp;
			p_bit += n_bits;
			n_bits = 0;
			return n;
		}

	}
	return n;
}


/*void VSDecoder::decode( std::vector< unsigned int >& R )
{
	while ( !( p_buf == -1 && p_bit == 8 ) )
	{
		GammaDecoder gd( buf, acc, bits, p_buf, p_bit );
		unsigned int num_of_bits = gd.decode()-1;
		UnaryDecoder ud( buf, acc, bits, p_buf, p_bit );
		unsigned int k = ud.decode();
		while ( k-- )
		{
			OrdinaryDecoder od( buf, acc, bits, p_buf, p_bit, num_of_bits );
			R.push_back( od.decode() );
		}

	}
}*/

VSDecoder::VSDecoder( std::string& buf_ )
	: buf(buf_), acc(buf[0]), acc_bits(buf[1])
{
	bias = -1;
    pos = buf.data()+2;
    end = buf.data()+buf.size();
    unpack_uint(&pos, end, &n_entry);
    unpack_uint(&pos, end, &last_entry);
    
	if (pos == end)
	{
		p_bit = 8-acc_bits;
	}
	else
	{
		p_bit = 0;
	}
	p_gd = new GammaDecoder(pos, end, acc, acc_bits, p_bit);
	cur_num_width = p_gd->decode()-1;
	p_ud = new UnaryDecoder(pos, end, acc, acc_bits, p_bit);
	cur_remaining_nums = p_ud->decode();
	p_od = new OrdinaryDecoder(pos, end, acc, acc_bits, p_bit, cur_num_width);
	p_od->setWidth(cur_num_width);

}

unsigned int VSDecoder::get_first_entry()
{
	bias = next();
	return bias;
}

unsigned int VSDecoder::get_next_entry()
{
	bias += next();
	return bias;
}

unsigned int VSDecoder::next()
{
	if ( pos == end && p_bit == 8 )
	{
		return ~0;
	}
	if ( cur_remaining_nums )
	{
		cur_remaining_nums--;
		return p_od->decode();
	}
	else
	{
		cur_num_width = p_gd->decode()-1;
		cur_remaining_nums = p_ud->decode();
		cur_remaining_nums--;
		p_od->setWidth( cur_num_width );
		return p_od->decode();
	}
}

unsigned int VSDecoder::get_n_entry()
{
	return n_entry;
}

unsigned int VSDecoder::get_last_entry()
{
	return last_entry;
}
VSDecoder::~VSDecoder()
{
	if ( p_ud != NULL )
	{
		delete p_ud;
		p_ud = NULL;
	}
	if ( p_gd != NULL )
	{
		delete p_gd;
		p_gd = NULL;
	}
	if ( p_od != NULL )
	{
		delete p_od;
		p_od = NULL;
	}
}

}