#include <cstdio>
#include <limits>
#include <initializer_list>

#include "util/fp_convert.h"

int main()
{
	std::initializer_list<double> values = {
		16384,
		3.14159,
		1e-309,
		5e-324,
		2.5e-324,
		0,
		-0.0,
	};

	static_assert(2.5e-324 == 5e-324,
	              "your compiler is no longer rounding 1/2 the smallest "
	              "representable subnormal value away from zero");

	for (const auto & v : values)
	{
		char result[30];
		unsigned n = util::fp_convert(v, result);

		result[n] = '\0';

		printf("%20g %20s (%u)\n", v, result, n);
	}

	return 0;
}
