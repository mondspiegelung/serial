#include <chrono>

#include "serial/json.h"

int main(int argc, char ** argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "Need config file\n");
		exit(1);
	}

	serial::json::parser p;
//	serial::value_store data;
	serial::printing_visitor data;
	p.read_file(argv[1], data);

//	data.print(std::cout);
	std::cout << std::endl;
}
