#pragma once

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef void (*con_str_vec_foreach_cb)(char *);

struct con_str_vec
{
	char **buffer;
	size_t length;
	size_t capacity;
	pthread_mutex_t lock;

	con_str_vec(size_t capacity)
	{
		if (capacity == 0)
		{
			this->buffer = nullptr;
		}
		else
		{
			// TODO: throw exception
			this->buffer = (char **)calloc(capacity, sizeof(char *));
			if (this->buffer == nullptr)
				exit(EXIT_FAILURE);
		}

		this->length = 0;
		this->capacity = capacity;

		// No need to check rc. Always returns 0.
		pthread_mutex_init(&this->lock, nullptr);
	}

	~con_str_vec()
	{
		pthread_mutex_lock(&this->lock);
		if (this->capacity == 0)
		{
			assert(this->buffer == nullptr);
			assert(this->length == 0);
			return;
		}

		assert(this->buffer != nullptr);
		for (int i = 0; i < this->length; i++)
			free(this->buffer[i]);
		free(this->buffer);
		this->buffer = nullptr;
		this->length = 0;
		this->capacity = 0;

		pthread_mutex_unlock(&this->lock);
		pthread_mutex_destroy(&this->lock);
	}

	inline int
	resize(size_t capacity)
	{
		if (this->capacity != capacity)
		{
			// TODO: Throw exception if realloc fails
			char **temp = (char **)realloc(buffer, sizeof(char *) * capacity);
			if (temp == nullptr)
				return -1;
			buffer = temp;
			this->capacity = capacity;
		}
		return 0;
	}

	inline int
	grow()
	{
		size_t capacity = this->capacity == 0 ? 1 : this->capacity * 2;
		return resize(capacity);
	}

	inline int
	push(char *elem)
	{
		pthread_mutex_lock(&this->lock);

		if (this->length == this->capacity)
		{
			int rc = grow();
			if (rc != 0)
				return -1;
		}

		this->buffer[this->length] = elem;
		this->length++;

		pthread_mutex_unlock(&this->lock);

		return 0;
	}

	inline void
	foreach_del_nolock(con_str_vec_foreach_cb cb)
	{
		for (int i = 0; i < this->length; i++)
		{
			cb(buffer[i]);
			free(buffer[i]);
		}

		this->length = 0;
	}

	inline void
	foreach_del(con_str_vec_foreach_cb cb)
	{
		pthread_mutex_lock(&lock);
		foreach_del_nolock(cb);
		pthread_mutex_unlock(&lock);
	}
};
