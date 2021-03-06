// This source file is a heavily modified derivative work of
//    https://github.com/night-shift/fpconv
// night-shift's code was distributed under the Boost software licence
// and as such, at a user's option, I give my permission to use it under
// the Boost software license instead of GNU LGPL.
//
// Due to the terms of the Boost license, this notice must be included in this
// file:
//
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "fp_convert.h"

#include <initializer_list>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <type_traits>

// This include is also dual-licenced under the LGPL & Boost software license
#include "leading_zeros.h"

#define npowers     87
#define steppowers  8
#define firstpower -348 /* 10 ^ -348 */
#define expmax     -32
#define expmin     -60
#define fracmask  0x000FFFFFFFFFFFFFU
#define expmask   0x7FF0000000000000U
#define hiddenbit 0x0010000000000000U
#define signmask  0x8000000000000000U
#define expbias   (1023 + 52)

#ifndef likely
#define likely(pred) __builtin_expect(!!(pred), 1)
#endif
#ifndef unlikely
#define unlikely(pred) __builtin_expect(!!(pred), 0)
#endif

namespace util {

static constexpr uint64_t tens[] = {
    10000000000000000000U, 1000000000000000000U, 100000000000000000U,
    10000000000000000U, 1000000000000000U, 100000000000000U, 10000000000000U,
	1000000000000U, 100000000000U, 10000000000U, 1000000000U, 100000000U,
    10000000U, 1000000U, 100000U, 10000U, 1000U, 100U, 10U, 1U
};


inline uint64_t to_bits(double d)
{
	uint64_t i;
	std::memcpy(&i, &d, sizeof(i));
    return i;
}

struct decomposed
{
 protected:
	constexpr decomposed()
	  : mantissa(0)
	  , exp(0)
	  , negative(false)
	  , is_nan(false)
	  , is_infinite(false)
	  , is_zero(true)
	{ }

 public:
	// TODO: detect zero and nan
	constexpr decomposed(uint64_t m, int32_t e, bool s = false)
	  : mantissa(m), exp(e), negative(s)
	  , is_nan(false), is_infinite(false), is_zero(false)
	{ }

	decomposed(double d) : decomposed()
	{
		uint64_t bits = to_bits(d);

		mantissa = bits & fracmask;
		exp = (bits & expmask) >> 52;
		negative = bits & signmask;

		if (unlikely(exp == 0))
		{
			exp = -expbias + 1;
			is_zero = (mantissa == 0);
		} else if (unlikely(exp == (expmask >> 52)))
		{
			if (mantissa)
				is_nan = true;
			else
				is_infinite = true;
		} else
		{
			mantissa += hiddenbit;
			exp -= expbias;
			is_zero = false;
		}
	}

	std::array<decomposed, 2> clamp()
	{
		decomposed upper, lower;
		upper.mantissa = (mantissa << 1) + 1;
		upper.exp  = exp - 1;

		upper.normalize();

		int l_shift = mantissa == hiddenbit ? 2 : 1;

		lower.mantissa = (mantissa << l_shift) - 1;
		lower.exp = exp - l_shift;

		lower.mantissa <<= lower.exp - upper.exp;
		lower.exp = upper.exp;

		return { lower, upper };
	}

	void normalize()
	{
		uint64_t shift = util::leading_zeros(mantissa);
		mantissa <<= shift;
		exp -= shift;
	}

    uint64_t mantissa;
    int32_t exp;
	bool negative;
	bool is_nan;
	bool is_infinite;
	bool is_zero;
};

static constexpr decomposed powers_ten[] = {
    { 18054884314459144840U, -1220 }, { 13451937075301367670U, -1193 },
    { 10022474136428063862U, -1166 }, { 14934650266808366570U, -1140 },
    { 11127181549972568877U, -1113 }, { 16580792590934885855U, -1087 },
    { 12353653155963782858U, -1060 }, { 18408377700990114895U, -1034 },
    { 13715310171984221708U, -1007 }, { 10218702384817765436U, -980 },
    { 15227053142812498563U, -954 },  { 11345038669416679861U, -927 },
    { 16905424996341287883U, -901 },  { 12595523146049147757U, -874 },
    { 9384396036005875287U,  -847 },  { 13983839803942852151U, -821 },
    { 10418772551374772303U, -794 },  { 15525180923007089351U, -768 },
    { 11567161174868858868U, -741 },  { 17236413322193710309U, -715 },
    { 12842128665889583758U, -688 },  { 9568131466127621947U,  -661 },
    { 14257626930069360058U, -635 },  { 10622759856335341974U, -608 },
    { 15829145694278690180U, -582 },  { 11793632577567316726U, -555 },
    { 17573882009934360870U, -529 },  { 13093562431584567480U, -502 },
    { 9755464219737475723U,  -475 },  { 14536774485912137811U, -449 },
    { 10830740992659433045U, -422 },  { 16139061738043178685U, -396 },
    { 12024538023802026127U, -369 },  { 17917957937422433684U, -343 },
    { 13349918974505688015U, -316 },  { 9946464728195732843U,  -289 },
    { 14821387422376473014U, -263 },  { 11042794154864902060U, -236 },
    { 16455045573212060422U, -210 },  { 12259964326927110867U, -183 },
    { 18268770466636286478U, -157 },  { 13611294676837538539U, -130 },
    { 10141204801825835212U, -103 },  { 15111572745182864684U, -77 },
    { 11258999068426240000U, -50 },   { 16777216000000000000U, -24 },
    { 12500000000000000000U,   3 },   { 9313225746154785156U,   30 },
    { 13877787807814456755U,  56 },   { 10339757656912845936U,  83 },
    { 15407439555097886824U, 109 },   { 11479437019748901445U, 136 },
    { 17105694144590052135U, 162 },   { 12744735289059618216U, 189 },
    { 9495567745759798747U,  216 },   { 14149498560666738074U, 242 },
    { 10542197943230523224U, 269 },   { 15709099088952724970U, 295 },
    { 11704190886730495818U, 322 },   { 17440603504673385349U, 348 },
    { 12994262207056124023U, 375 },   { 9681479787123295682U,  402 },
    { 14426529090290212157U, 428 },   { 10748601772107342003U, 455 },
    { 16016664761464807395U, 481 },   { 11933345169920330789U, 508 },
    { 17782069995880619868U, 534 },   { 13248674568444952270U, 561 },
    { 9871031767461413346U,  588 },   { 14708983551653345445U, 614 },
    { 10959046745042015199U, 641 },   { 16330252207878254650U, 667 },
    { 12166986024289022870U, 694 },   { 18130221999122236476U, 720 },
    { 13508068024458167312U, 747 },   { 10064294952495520794U, 774 },
    { 14996968138956309548U, 800 },   { 11173611982879273257U, 827 },
    { 16649979327439178909U, 853 },   { 12405201291620119593U, 880 },
    { 9242595204427927429U,  907 },   { 13772540099066387757U, 933 },
    { 10261342003245940623U, 960 },   { 15290591125556738113U, 986 },
    { 11392378155556871081U, 1013 },  { 16975966327722178521U, 1039 },
    { 12648080533535911531U, 1066 }
};

static decomposed find_cachedpow10(int exp, int* k)
{
	// note: 15051499783/50000000000 is an exact fraction of this
	static constexpr double one_log_ten = 0.30102999566398114;

	int approx = -(exp + npowers) * one_log_ten;
	int idx = (approx - firstpower) / steppowers;

	int current = exp + powers_ten[idx].exp + 64;

	while (current < expmin)
	{
		++idx;
		current = exp + powers_ten[idx].exp + 64;
	}

	while (current > expmax)
	{
		--idx;
		current = exp + powers_ten[idx].exp + 64;
	}

	*k = (firstpower + idx * steppowers);

	return powers_ten[idx];
}

typedef unsigned __int128 uint128_t;

static decomposed multiply(const decomposed & a, const decomposed & b)
{
#if 0
    static constexpr uint64_t lomask = 0x00000000FFFFFFFF;

    uint64_t ah_bl = (a.mantissa >> 32)    * (b.mantissa & lomask);
    uint64_t al_bh = (a.mantissa & lomask) * (b.mantissa >> 32);
    uint64_t al_bl = (a.mantissa & lomask) * (b.mantissa & lomask);
    uint64_t ah_bh = (a.mantissa >> 32)    * (b.mantissa >> 32);

    uint64_t tmp = (ah_bl & lomask) + (al_bh & lomask) + (al_bl >> 32); 
    /* round up */
    tmp += 1U << 31;

    return decomposed{
        ah_bh + (ah_bl >> 32) + (al_bh >> 32) + (tmp >> 32),
        a.exp + b.exp + 64
    };
#else
	uint128_t p = static_cast<uint128_t>(a.mantissa)
	            * static_cast<uint128_t>(b.mantissa);
	uint64_t h = p >> 64;
	uint64_t l = static_cast<uint64_t>(p);
	if (l & (uint64_t(1) << 63)) ++h;

	return decomposed(h, a.exp + b.exp + 64);
#endif
}

static void round_digit(char* digits, int ndigits, uint64_t delta, uint64_t rem, uint64_t kappa, uint64_t frac)
{
    while (rem < frac && delta - rem >= kappa &&
           (rem + kappa < frac || frac - rem > rem + kappa - frac)) {

        digits[ndigits - 1]--;
        rem += kappa;
    }
}

static int generate_digits(const decomposed & fp, const decomposed & upper, const decomposed & lower, char* digits, int* K)
{
    uint64_t wfrac = upper.mantissa - fp.mantissa;
    uint64_t delta = upper.mantissa - lower.mantissa;

    decomposed one(1ULL << -upper.exp, upper.exp);

    uint64_t part1 = upper.mantissa >> -one.exp;
    uint64_t part2 = upper.mantissa & (one.mantissa - 1);

	static constexpr char ascii_digits[] = "0123456789";

    int idx = 0, kappa = 10;
    const uint64_t* divp;

    for(divp = tens + 10; kappa > 0; divp++) {

        uint64_t div = *divp;
        unsigned digit = part1 / div;

        if (digit || idx) {
            digits[idx++] = ascii_digits[digit];
        }

        part1 -= digit * div;
        kappa--;

        uint64_t tmp = (part1 <<-one.exp) + part2;
        if (tmp <= delta) {
            *K += kappa;
            round_digit(digits, idx, delta, tmp, div << -one.exp, wfrac);

            return idx;
        }
    }

    /* 10 */
    const uint64_t* unit = tens + 18;
    while(true) {
        part2 *= 10;
        delta *= 10;
        kappa--;

        unsigned digit = part2 >> -one.exp;
        if (digit || idx) {
            digits[idx++] = ascii_digits[digit]; // digit + '0';
        }

        part2 &= one.mantissa - 1;
        if (part2 < delta) {
            *K += kappa;
            round_digit(digits, idx, delta, part2, one.mantissa, wfrac * *unit);

            return idx;
        }

        unit--;
    }
}

static int grisu2(decomposed & v, char* digits, int* K)
{
	decomposed w(v.mantissa, v.exp);
	auto [lower, upper] = w.clamp();

	w.normalize();

    int k;
    decomposed cp = find_cachedpow10(upper.exp, &k);

    w     = multiply(w,     cp);
    upper = multiply(upper, cp);
    lower = multiply(lower, cp);

    lower.mantissa++;
    upper.mantissa--;

    *K = -k;

	v.mantissa = w.mantissa;
	v.exp = w.exp;
    return generate_digits(v, upper, lower, digits, K);
}

static int emit_digits(char* digits, int ndigits, char* dest, int K, bool neg)
{
    int exp = std::abs(K + ndigits - 1);

    /* write plain integer */
    if(K >= 0 && (exp < (ndigits + 7))) {
        memcpy(dest, digits, ndigits);
        memset(dest + ndigits, '0', K);

        return ndigits + K;
    }

    /* write decimal w/o scientific notation */
    if(K < 0 && (K > -7 || exp < 4)) {
        int offset = ndigits - std::abs(K);
        /* fp < 1.0 -> write leading zero */
        if(offset <= 0) {
            offset = -offset;
            dest[0] = '0';
            dest[1] = '.';
            memset(dest + 2, '0', offset);
            memcpy(dest + offset + 2, digits, ndigits);

            return ndigits + 2 + offset;

        /* fp > 1.0 */
        } else {
            memcpy(dest, digits, offset);
            dest[offset] = '.';
            memcpy(dest + offset + 1, digits + offset, ndigits - offset);

            return ndigits + 1;
        }
    }

    /* write decimal w/ scientific notation */
    ndigits = std::min(ndigits, 18 - neg);

    int idx = 0;
    dest[idx++] = digits[0];

    if(ndigits > 1) {
        dest[idx++] = '.';
        memcpy(dest + idx, digits + 1, ndigits - 1);
        idx += ndigits - 1;
    }

    dest[idx++] = 'e';

    char sign = K + ndigits - 1 < 0 ? '-' : '+';
    dest[idx++] = sign;

    int cent = 0;

    if(exp > 99) {
        cent = exp / 100;
        dest[idx++] = cent + '0';
        exp -= cent * 100;
    }
    if(exp > 9) {
        int dec = exp / 10;
        dest[idx++] = dec + '0';
        exp -= dec * 10;

    } else if(cent) {
        dest[idx++] = '0';
    }

    dest[idx++] = exp % 10 + '0';

    return idx;
}

int fp_convert(double d, char * dest)
{
    char digits[18];
    int str_len = 0;

	decomposed value(d);

	dest[0] = '-';

	if (unlikely(value.is_nan))
	{
		memcpy(dest, "NaN", 3);
		return 3;
	} else if (unlikely(value.is_infinite))
	{
		memcpy(dest + value.negative, "Inf", 3);
		return 3 + value.negative;
	} else if (unlikely(value.is_zero))
	{
		dest[value.negative] = '0';
		return 1 + value.negative;
	}


    int K = 0;
    int ndigits = grisu2(value, digits, &K);

    str_len += emit_digits(digits, ndigits, dest + value.negative, K, value.negative);

    return str_len + value.negative;
}

} // namespace util
