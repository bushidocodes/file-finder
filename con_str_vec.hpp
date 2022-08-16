#pragma once

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct con_str_vec
{
	char **buffer;
	size_t length;
	size_t capacity;
	pthread_mutex_t lock;
};

typedef void (*con_str_vec_foreach_cb)(char *);

static inline int
con_str_vec_init(struct con_str_vec *self, size_t capacity)
{
	if (capacity == 0)
	{
		self->buffer = nullptr;
	}
	else
	{
		self->buffer = (char **)calloc(capacity, sizeof(char *));
		if (self->buffer == nullptr)
			return -1;
	}

	self->length = 0;
	self->capacity = capacity;

	// No need to check rc. Always returns 0.
	pthread_mutex_init(&self->lock, nullptr);

	return 0;
}

static inline void
con_str_vec_destroy(struct con_str_vec *self)
{
	pthread_mutex_lock(&self->lock);
	if (self->capacity == 0)
	{
		assert(self->buffer == nullptr);
		assert(self->length == 0);
		return;
	}

	assert(self->buffer != nullptr);
	for (int i = 0; i < self->length; i++)
		free(self->buffer[i]);
	free(self->buffer);
	self->buffer = nullptr;
	self->length = 0;
	self->capacity = 0;

	pthread_mutex_unlock(&self->lock);
	pthread_mutex_destroy(&self->lock);
}

static inline int
con_str_vec_resize(struct con_str_vec *self, size_t capacity)
{
	if (self->capacity != capacity)
	{
		char **temp = (char **)realloc(self->buffer, sizeof(char *) * capacity);
		if (temp == nullptr)
			return -1;
		self->buffer = temp;
		self->capacity = capacity;
	}
	return 0;
}

static inline int
con_str_vec_grow(struct con_str_vec *self)
{
	size_t capacity = self->capacity == 0 ? 1 : self->capacity * 2;
	return con_str_vec_resize(self, capacity);
}

static inline int
con_str_vec_push(struct con_str_vec *self, char *elem)
{
	pthread_mutex_lock(&self->lock);

	if (self->length == self->capacity)
	{
		int rc = con_str_vec_grow(self);
		if (rc != 0)
			return -1;
	}

	self->buffer[self->length] = elem;
	self->length++;

	pthread_mutex_unlock(&self->lock);

	return 0;
}

static inline void
con_str_vec_foreach_del_nolock(struct con_str_vec *self, con_str_vec_foreach_cb cb)
{
	for (int i = 0; i < self->length; i++)
	{
		cb(self->buffer[i]);
		free(self->buffer[i]);
	}

	self->length = 0;
}

static inline void
con_str_vec_foreach_del(struct con_str_vec *self, con_str_vec_foreach_cb cb)
{
	pthread_mutex_lock(&self->lock);
	con_str_vec_foreach_del_nolock(self, cb);
	pthread_mutex_unlock(&self->lock);
}