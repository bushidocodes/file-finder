#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>

#include "worker.hpp"
#include "dumper.hpp"
#include "shell.hpp"
#include "con_str_vec.hpp"

struct con_str_vec matches = {0};

std::filesystem::path root_directory;

static void
set_root_directory(char *dir)
{
	root_directory = dir;

	if (!std::filesystem::exists(root_directory))
	{
		std::cerr << "Failed to open " << dir << " with: ";
		std::perror("");
		std::exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		std::cerr << "file-finder <dir> <substring1> [<substring2> [<substring3>]...]\n";
		std::exit(EXIT_FAILURE);
	}

	set_root_directory(argv[1]);

	std::vector<std::string> substrings{};
	substrings.reserve(argc - 2);
	for (auto i = 2; i < argc; i++)
	{
		substrings.push_back(std::move(argv[i]));
	}

	std::vector<std::thread> workers;
	for (auto i = 0; i < substrings.size(); i++)
	{
		workers.push_back(std::thread{worker_main, substrings[i].c_str()});
	}

	std::thread dumper{dumper_main};
	std::thread shell{shell_main};

	dumper.detach();
	for (auto &w : workers)
	{
		w.detach();
	}

	shell.join();
	std::exit(EXIT_SUCCESS);
}