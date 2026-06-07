/*
 * Unit tests for con_str_vec — the thread-safe growable string vector.
 *
 * Build:
 *   gcc -std=c11 -Wall -Wextra -pthread -Iinclude -Itests/unity \
 *       tests/test_con_str_vec.c tests/unity/unity.c -o tests/test_con_str_vec
 */
#define _POSIX_C_SOURCE 200809

#include "unity.h"
#include "con_str_vec.h"

#include <pthread.h>
#include <string.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/* Fixture                                                             */
/* ------------------------------------------------------------------ */

static struct con_str_vec vec;

void setUp(void)    { TEST_ASSERT_EQUAL_INT(0, con_str_vec_init(&vec, 0)); }
void tearDown(void) { con_str_vec_destroy(&vec); }

/* ------------------------------------------------------------------ */
/* Group 1: init                                                       */
/* ------------------------------------------------------------------ */

void test_init_zero_capacity(void)
{
    struct con_str_vec v;
    TEST_ASSERT_EQUAL_INT(0, con_str_vec_init(&v, 0));
    TEST_ASSERT_NULL(v.buffer);
    TEST_ASSERT_EQUAL_size_t(0, v.length);
    TEST_ASSERT_EQUAL_size_t(0, v.capacity);
    con_str_vec_destroy(&v);
}

void test_init_nonzero_capacity(void)
{
    struct con_str_vec v;
    TEST_ASSERT_EQUAL_INT(0, con_str_vec_init(&v, 8));
    TEST_ASSERT_NOT_NULL(v.buffer);
    TEST_ASSERT_EQUAL_size_t(0, v.length);
    TEST_ASSERT_EQUAL_size_t(8, v.capacity);
    con_str_vec_destroy(&v);
}

void test_init_mutex_is_functional(void)
{
    /* A freshly initialised mutex must be unlocked — trylock succeeds */
    int rc = pthread_mutex_trylock(&vec.lock);
    TEST_ASSERT_EQUAL_INT(0, rc);
    pthread_mutex_unlock(&vec.lock);
}

/* ------------------------------------------------------------------ */
/* Group 2: destroy                                                    */
/* ------------------------------------------------------------------ */

void test_destroy_zeroes_struct_fields(void)
{
    /* Push something so destroy has real work to do */
    TEST_ASSERT_EQUAL_INT(0, con_str_vec_push(&vec, strdup("hello")));
    con_str_vec_destroy(&vec);
    TEST_ASSERT_NULL(vec.buffer);
    TEST_ASSERT_EQUAL_size_t(0, vec.length);
    TEST_ASSERT_EQUAL_size_t(0, vec.capacity);
    /* Re-init so tearDown doesn't double-destroy */
    con_str_vec_init(&vec, 0);
}

void test_destroy_empty_vec_no_crash(void)
{
    /* tearDown calls destroy again — ensure it's idempotent after an empty init */
    con_str_vec_destroy(&vec);
    /* Re-init so tearDown is happy */
    con_str_vec_init(&vec, 0);
}

/* ------------------------------------------------------------------ */
/* Group 3: resize                                                     */
/* ------------------------------------------------------------------ */

void test_resize_grows(void)
{
    struct con_str_vec v;
    con_str_vec_init(&v, 4);
    TEST_ASSERT_EQUAL_INT(0, con_str_vec_resize(&v, 16));
    TEST_ASSERT_EQUAL_size_t(16, v.capacity);
    TEST_ASSERT_NOT_NULL(v.buffer);
    con_str_vec_destroy(&v);
}

void test_resize_shrinks(void)
{
    struct con_str_vec v;
    con_str_vec_init(&v, 16);
    /* Push two items so length stays 2 after shrink */
    con_str_vec_push(&v, strdup("a"));
    con_str_vec_push(&v, strdup("b"));
    TEST_ASSERT_EQUAL_INT(0, con_str_vec_resize(&v, 4));
    TEST_ASSERT_EQUAL_size_t(4,  v.capacity);
    TEST_ASSERT_EQUAL_size_t(2,  v.length);
    con_str_vec_destroy(&v);
}

void test_resize_same_capacity_is_noop(void)
{
    struct con_str_vec v;
    con_str_vec_init(&v, 4);
    char **before = v.buffer;
    TEST_ASSERT_EQUAL_INT(0, con_str_vec_resize(&v, 4));
    /* Implementation guards with capacity != capacity, so no realloc */
    TEST_ASSERT_EQUAL_PTR(before, v.buffer);
    TEST_ASSERT_EQUAL_size_t(4, v.capacity);
    con_str_vec_destroy(&v);
}

/* ------------------------------------------------------------------ */
/* Group 4: grow                                                       */
/* ------------------------------------------------------------------ */

void test_grow_from_zero_capacity(void)
{
    struct con_str_vec v;
    con_str_vec_init(&v, 0);
    TEST_ASSERT_EQUAL_INT(0, con_str_vec_grow(&v));
    TEST_ASSERT_EQUAL_size_t(1, v.capacity);
    con_str_vec_destroy(&v);
}

void test_grow_doubles_capacity(void)
{
    struct con_str_vec v;
    con_str_vec_init(&v, 1);
    TEST_ASSERT_EQUAL_INT(0, con_str_vec_grow(&v));
    TEST_ASSERT_EQUAL_size_t(2, v.capacity);
    TEST_ASSERT_EQUAL_INT(0, con_str_vec_grow(&v));
    TEST_ASSERT_EQUAL_size_t(4, v.capacity);
    con_str_vec_destroy(&v);
}

/* ------------------------------------------------------------------ */
/* Group 5: push                                                       */
/* ------------------------------------------------------------------ */

void test_push_single_element(void)
{
    char *s = strdup("hello");
    TEST_ASSERT_EQUAL_INT(0, con_str_vec_push(&vec, s));
    TEST_ASSERT_EQUAL_size_t(1, vec.length);
    TEST_ASSERT_EQUAL_STRING("hello", vec.buffer[0]);
}

void test_push_triggers_grow(void)
{
    struct con_str_vec v;
    con_str_vec_init(&v, 1);
    TEST_ASSERT_EQUAL_INT(0, con_str_vec_push(&v, strdup("first")));
    /* Capacity is full — next push must grow */
    TEST_ASSERT_EQUAL_INT(0, con_str_vec_push(&v, strdup("second")));
    TEST_ASSERT_EQUAL_size_t(2, v.length);
    TEST_ASSERT_TRUE(v.capacity >= 2);
    con_str_vec_destroy(&v);
}

void test_push_many_elements(void)
{
    char buf[32];
    for (int i = 0; i < 64; i++) {
        snprintf(buf, sizeof(buf), "item-%d", i);
        TEST_ASSERT_EQUAL_INT(0, con_str_vec_push(&vec, strdup(buf)));
    }
    TEST_ASSERT_EQUAL_size_t(64, vec.length);
    for (int i = 0; i < 64; i++) {
        snprintf(buf, sizeof(buf), "item-%d", i);
        TEST_ASSERT_EQUAL_STRING(buf, vec.buffer[i]);
    }
}

void test_push_stores_pointer_directly(void)
{
    char *s = strdup("owner_test");
    con_str_vec_push(&vec, s);
    /* vec must store the exact pointer, not a copy */
    TEST_ASSERT_EQUAL_PTR(s, vec.buffer[0]);
}

/* ------------------------------------------------------------------ */
/* Group 6: foreach_del                                                */
/* ------------------------------------------------------------------ */

static int  g_cb_count;
static char g_cb_last[64];

static void counting_cb(const char *s)
{
    g_cb_count++;
    strncpy(g_cb_last, s, sizeof(g_cb_last) - 1);
}

static int  g_seq_count;
static char g_seq_received[8][64];

static void sequence_cb(const char *s)
{
    if (g_seq_count < 8)
        strncpy(g_seq_received[g_seq_count], s, 63);
    g_seq_count++;
}

void test_foreach_del_calls_cb_for_each_element(void)
{
    g_cb_count = 0;
    con_str_vec_push(&vec, strdup("x"));
    con_str_vec_push(&vec, strdup("y"));
    con_str_vec_push(&vec, strdup("z"));
    con_str_vec_foreach_del(&vec, counting_cb);
    TEST_ASSERT_EQUAL_INT(3, g_cb_count);
    TEST_ASSERT_EQUAL_size_t(0, vec.length);
}

void test_foreach_del_passes_correct_strings(void)
{
    g_seq_count = 0;
    con_str_vec_push(&vec, strdup("a"));
    con_str_vec_push(&vec, strdup("b"));
    con_str_vec_push(&vec, strdup("c"));
    con_str_vec_foreach_del(&vec, sequence_cb);
    TEST_ASSERT_EQUAL_INT(3, g_seq_count);
    TEST_ASSERT_EQUAL_STRING("a", g_seq_received[0]);
    TEST_ASSERT_EQUAL_STRING("b", g_seq_received[1]);
    TEST_ASSERT_EQUAL_STRING("c", g_seq_received[2]);
}

void test_foreach_del_resets_length_not_capacity(void)
{
    con_str_vec_push(&vec, strdup("p"));
    con_str_vec_push(&vec, strdup("q"));
    size_t cap_before = vec.capacity;
    con_str_vec_foreach_del(&vec, counting_cb);
    TEST_ASSERT_EQUAL_size_t(0, vec.length);
    /* Buffer is kept — capacity unchanged so next batch needs no realloc */
    TEST_ASSERT_EQUAL_size_t(cap_before, vec.capacity);
}

void test_foreach_del_nolock_same_behaviour(void)
{
    g_cb_count = 0;
    con_str_vec_push(&vec, strdup("1"));
    con_str_vec_push(&vec, strdup("2"));
    con_str_vec_foreach_del_nolock(&vec, counting_cb);
    TEST_ASSERT_EQUAL_INT(2, g_cb_count);
    TEST_ASSERT_EQUAL_size_t(0, vec.length);
}

void test_foreach_del_on_empty_vec(void)
{
    g_cb_count = 0;
    con_str_vec_foreach_del(&vec, counting_cb);
    TEST_ASSERT_EQUAL_INT(0, g_cb_count);
    TEST_ASSERT_EQUAL_size_t(0, vec.length);
}

void test_push_after_drain_works(void)
{
    con_str_vec_push(&vec, strdup("first"));
    con_str_vec_foreach_del(&vec, counting_cb);
    TEST_ASSERT_EQUAL_size_t(0, vec.length);
    /* Push again after drain — must succeed */
    TEST_ASSERT_EQUAL_INT(0, con_str_vec_push(&vec, strdup("second")));
    TEST_ASSERT_EQUAL_size_t(1, vec.length);
    TEST_ASSERT_EQUAL_STRING("second", vec.buffer[0]);
}

void test_foreach_del_nulls_freed_slots(void)
{
    con_str_vec_push(&vec, strdup("slot0"));
    con_str_vec_foreach_del(&vec, counting_cb);
    /* Implementation nulls buffer[i] after free — verify */
    TEST_ASSERT_NULL(vec.buffer[0]);
}

/* ------------------------------------------------------------------ */
/* Group 7: concurrency                                                */
/* ------------------------------------------------------------------ */

#define CONCURRENT_THREADS 8
#define PUSHES_PER_THREAD  100

static void *push_thread(void *arg)
{
    struct con_str_vec *v = (struct con_str_vec *)arg;
    char buf[32];
    for (int i = 0; i < PUSHES_PER_THREAD; i++) {
        snprintf(buf, sizeof(buf), "item");
        con_str_vec_push(v, strdup(buf));
    }
    return NULL;
}

void test_concurrent_push_correct_count(void)
{
    pthread_t threads[CONCURRENT_THREADS];
    for (int i = 0; i < CONCURRENT_THREADS; i++)
        pthread_create(&threads[i], NULL, push_thread, &vec);
    for (int i = 0; i < CONCURRENT_THREADS; i++)
        pthread_join(threads[i], NULL);

    TEST_ASSERT_EQUAL_size_t(CONCURRENT_THREADS * PUSHES_PER_THREAD, vec.length);
    /* Verify every stored pointer is non-NULL */
    for (size_t i = 0; i < vec.length; i++)
        TEST_ASSERT_NOT_NULL(vec.buffer[i]);
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    UNITY_BEGIN();

    /* init */
    RUN_TEST(test_init_zero_capacity);
    RUN_TEST(test_init_nonzero_capacity);
    RUN_TEST(test_init_mutex_is_functional);

    /* destroy */
    RUN_TEST(test_destroy_zeroes_struct_fields);
    RUN_TEST(test_destroy_empty_vec_no_crash);

    /* resize */
    RUN_TEST(test_resize_grows);
    RUN_TEST(test_resize_shrinks);
    RUN_TEST(test_resize_same_capacity_is_noop);

    /* grow */
    RUN_TEST(test_grow_from_zero_capacity);
    RUN_TEST(test_grow_doubles_capacity);

    /* push */
    RUN_TEST(test_push_single_element);
    RUN_TEST(test_push_triggers_grow);
    RUN_TEST(test_push_many_elements);
    RUN_TEST(test_push_stores_pointer_directly);

    /* foreach_del */
    RUN_TEST(test_foreach_del_calls_cb_for_each_element);
    RUN_TEST(test_foreach_del_passes_correct_strings);
    RUN_TEST(test_foreach_del_resets_length_not_capacity);
    RUN_TEST(test_foreach_del_nolock_same_behaviour);
    RUN_TEST(test_foreach_del_on_empty_vec);
    RUN_TEST(test_push_after_drain_works);
    RUN_TEST(test_foreach_del_nulls_freed_slots);

    /* concurrency */
    RUN_TEST(test_concurrent_push_correct_count);

    return UNITY_END();
}
