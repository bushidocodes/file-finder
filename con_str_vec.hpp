#pragma once

#include <mutex>
#include <vector>

typedef void (*con_str_vec_foreach_cb)(char *);

struct con_str_vec
{
	inline size_t getLength()
	{
		return data.size();
	}

	inline int
	push(char *elem)
	{
		std::lock_guard<std::mutex> lk(lock);
		data.push_back(elem);
		return 0;
	}

	inline void
	foreach_del_nolock(con_str_vec_foreach_cb cb)
	{
		while (data.size() > 0)
		{
			cb(data.back());
			data.pop_back();
		}
	}

	inline void
	foreach_del(con_str_vec_foreach_cb cb)
	{
		std::lock_guard<std::mutex> lk(lock);
		foreach_del_nolock(cb);
	}

private:
	std::vector<char *> data;
	std::mutex lock;
};
