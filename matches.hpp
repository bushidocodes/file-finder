#pragma once

#include <algorithm>
#include <mutex>
#include <string>
#include <vector>

class ResultBuilder
{
	std::vector<std::string> data;
	std::mutex lock;

public:
	void add(std::string &&match)
	{
		std::lock_guard<std::mutex> lk(lock);
		data.push_back(std::forward<std::string>(match));
	}

	/* TODO: Unique_ptr to heap value to avoid copy? */
	std::string dump()
	{
		std::lock_guard<std::mutex> lk(lock);

		/* TODO: Try to use fold or accumulate instead */
		std::string res{};
		std::for_each(data.cbegin(), data.cend(), [&res](auto val)
					  {
					res.append(val);
				res.append("\n"); });

		data.clear();

		return res;
	};
};

extern class ResultBuilder matches;