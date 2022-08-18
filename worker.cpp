#include <filesystem>
#include <string>
#include <iostream>

#include "matches.hpp"

void search_filenames(std::filesystem::path dir_path, const std::string &substring)
{
	for (auto const &dir_entry : std::filesystem::directory_iterator{dir_path})
	{
		if (dir_entry.is_symlink() || dir_entry.path().compare(".") == 0 || dir_entry.path().compare("..") == 0)
		{
			continue;
		}

		if (dir_entry.is_directory())
		{
			search_filenames(dir_entry.path(), substring);
		}

		auto filename = dir_entry.path().filename().string();
		if (filename.find(substring, 0) != std::string::npos)
		{
			matches.add(std::move(filename));
		}
	}
}
