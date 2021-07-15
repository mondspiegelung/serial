#include <cstdio>
#include <limits>
#include <initializer_list>
#include <random>

#include "util/fp_convert.h"

void stress_test()
{
	char buffer[40];
	std::random_device rd;

	std::uniform_real_distribution<double> dist;

	for (unsigned i = 0; i < 100'000; ++i)
	{
		auto val = dist(rd);

		util::fp_convert(val, buffer);
	}
}

int main()
{
	std::initializer_list<double> values = {
		1.23456e0,
		1.23456e1,
		1.23456e2,
		1.23456e3,
		1.23456e4,
		1.23456e5,
		1.23456e6,
		1.23456e7,
		1.23456e8,
		1.23456e9,
		1.23456e10,
		1.23456e11,
		1.23456e12,
		1.23456e13,
		1.23456e14,
		1.23456e15,
		1.23456e16,
		16384,
		3.14159,
		1e-309,
		5e-324,
		2.5e-324,
		0,
		-0.0,
		-5,
		-1.2345,
	};

	static_assert(2.5e-324 == 5e-324,
	              "your compiler is no longer rounding 1/2 the smallest "
	              "representable subnormal value away from zero");

	constexpr size_t buflen = 64;

	for (const auto & v : values)
	{
		char printf_result[buflen];
		char conv_result[buflen];

		unsigned printf_n = snprintf(printf_result, buflen, "%g", v);
		unsigned conv_n = util::fp_convert(v, conv_result);

		conv_result[conv_n] = '\0';

		printf("%20s (%u) %20s (%u)\n",
		       printf_result, printf_n,
		       conv_result, conv_n);
	}

	stress_test();

	return 0;
}
