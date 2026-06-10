#pragma once

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct con_str_vec {
	char          **buffer;
	size_t          length;
	size_t          capacity;
	pthread_mutex_t lock;
};

typedef void (*con_str_vec_foreach_cb)(const char *);

[[nodiscard]] static inline int
con_str_vec_init(struct con_str_vec *self, size_t capacity)
{
	if (capacity == 0) {
		self->buffer = nullptr;
	} else {
		self->buffer = calloc(capacity, sizeof(char *));
		if (self->buffer == nullptr) return -1;
	}

	self->length   = 0;
	self->capacity = capacity;

	int rc = pthread_mutex_init(&self->lock, nullptr);
	if (rc != 0) {
		free(self->buffer);
		errno = rc;
		return -1;
	}

	return 0;
}

/*
 * Must be called only after all threads that access this vector have
 * been joined or cancelled — no concurrent access is possible at that
 * point, so no lock is taken.
 */
static inline void
con_str_vec_destroy(struct con_str_vec *self)
{
	for (size_t i = 0; i < self->length; i++) {
		free(self->buffer[i]);
		self->buffer[i] = nullptr;
	}
	free(self->buffer);
	self->buffer   = nullptr;
	self->length   = 0;
	self->capacity = 0;
	pthread_mutex_destroy(&self->lock);
}

[[nodiscard]] static inline int
con_str_vec_resize(struct con_str_vec *self, size_t capacity)
{
	if (self->capacity != capacity) {
		char **temp = realloc(self->buffer, sizeof(char *) * capacity);
		if (temp == nullptr) return -1;
		self->buffer   = temp;
		self->capacity = capacity;
	}
	return 0;
}

[[nodiscard]] static inline int
con_str_vec_grow(struct con_str_vec *self)
{
	size_t capacity;
	if (self->capacity == 0) {
		capacity = 1;
	} else if (self->capacity > SIZE_MAX / 2) {
		/* Doubling would overflow; cap at SIZE_MAX */
		if (self->capacity == SIZE_MAX) {
			errno = ENOMEM;
			return -1;
		}
		capacity = SIZE_MAX;
	} else {
		capacity = self->capacity * 2;
	}
	return con_str_vec_resize(self, capacity);
}

/*
 * Append elem to the vector, transferring ownership to it.  The vector
 * will free elem when it is drained (foreach_del*) or destroyed.
 * On failure (-1), elem is NOT freed — the caller retains ownership.
 */
[[nodiscard]] static inline int
con_str_vec_push(struct con_str_vec *self, char *elem)
{
	pthread_mutex_lock(&self->lock);

	if (self->length == self->capacity) {
		int rc = con_str_vec_grow(self);
		if (rc != 0) { pthread_mutex_unlock(&self->lock); return -1; }
	}

	self->buffer[self->length] = elem;
	self->length++;

	pthread_mutex_unlock(&self->lock);

	return 0;
}

/* Caller must hold self->lock.  Frees every element and resets length to 0. */
static inline void
con_str_vec_foreach_del_nolock(struct con_str_vec *self, con_str_vec_foreach_cb cb)
{
	for (size_t i = 0; i < self->length; i++) {
		cb(self->buffer[i]);
		free(self->buffer[i]);
		self->buffer[i] = nullptr;
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

static inline void
con_str_vec_puts(const char *s) { puts(s); }
