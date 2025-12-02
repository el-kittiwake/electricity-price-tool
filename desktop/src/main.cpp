#include "leccy.hpp"

int main(int ac, char **av)
{
	(void)av;
	if (ac != 2)
	{
		std::cerr << "Error: invalid argument.\nUsage: ./leccy <param> (--help for list)" << std::endl;
		return (EXIT_FAILURE);
	}

	try
	{
		leccy test;
		test.chooseAction(av[1]);
	}
	catch(const std::exception& e)
	{
		std::cerr << "Exception caught: " << e.what() << "\nAttempting clean exit." << std::endl;
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}