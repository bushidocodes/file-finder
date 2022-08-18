#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>

#include "worker.hpp"
#include "dumper.hpp"
#include "shell.hpp"
#include "matches.hpp"

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		std::cerr << "file-finder <dir> <substring1> [<substring2> [<substring3>]...]\n";
		std::exit(EXIT_FAILURE);
	}

	std::string root_directory{argv[1]};
	if (!std::filesystem::exists(root_directory))
	{
		std::cerr << "Failed to open " << root_directory << " with: ";
		std::perror("");
		std::exit(EXIT_FAILURE);
	}

	std::vector<std::string> substrings{};
	substrings.reserve(argc - 2);
	for (auto i = 2; i < argc; i++)
	{
		substrings.push_back(std::move(argv[i]));
	}

	std::vector<std::thread> workers;
	for (auto i = 0; i < substrings.size(); i++)
	{
		workers.push_back(std::thread{search_filenames, root_directory, substrings[i]});
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