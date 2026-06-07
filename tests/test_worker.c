/*
 * Functional tests for worker_main / search_filenames.
 *
 * worker.c is #include'd directly so its static search_filenames is
 * accessible without modifying production source.  main.c is NOT linked —
 * this file defines its own `matches` global.
 *
 * Build:
 *   gcc-14 -std=c23 -Wall -Wextra -pthread -Iinclude -Itests/unity \
 *       tests/test_worker.c tests/unity/unity.c -o tests/test_worker
 */
#define _POSIX_C_SOURCE 202311L
#define _DEFAULT_SOURCE

/* Pull in search_filenames (static) and worker_main without linking main.c */
#include "../src/worker.c"

#include "unity.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/* The global that worker.c references via globals.h                   */
/* ------------------------------------------------------------------ */

struct con_str_vec matches;

/* ------------------------------------------------------------------ */
/* Fixture helpers                                                     */
/* ------------------------------------------------------------------ */

static char tmpdir[256];

/* Create a file at tmpdir/relpath */
static void touch(const char *relpath)
{
    char full[512];
    snprintf(full, sizeof(full), "%s/%s", tmpdir, relpath);
    int fd = open(full, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    TEST_ASSERT_MESSAGE(fd >= 0, full);
    close(fd);
}

/* Create a subdirectory at tmpdir/relpath */
static void mksubdir(const char *relpath)
{
    char full[512];
    snprintf(full, sizeof(full), "%s/%s", tmpdir, relpath);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, mkdir(full, 0755), full);
}

/* Recursively remove tmpdir — simple shell fallback */
static void rmtmpdir(void)
{
    /* Re-enable permissions in case a test chmod'd a subdir to 000 */
    char restore[300], remove[300];
    snprintf(restore, sizeof(restore), "chmod -R u+rwx '%s' 2>/dev/null", tmpdir);
    snprintf(remove,  sizeof(remove),  "rm -rf '%s'", tmpdir);
    system(restore);
    system(remove);
}

/* Launch worker_main in a thread and join it; assert successful return */
static void run_worker(const char *const *substrings, size_t count)
{
    struct worker_args args = {
        .root_dir   = tmpdir,
        .substrings = substrings,
        .count      = count,
    };
    pthread_t t;
    TEST_ASSERT_EQUAL_INT(0, pthread_create(&t, NULL, worker_main, &args));
    void *retval = NULL;
    TEST_ASSERT_EQUAL_INT(0, pthread_join(t, &retval));
    TEST_ASSERT_NULL_MESSAGE(retval, "worker_main returned non-NULL (failure)");
}

/* Return 1 if the basename of any match equals name */
static int matches_basename(const char *name)
{
    for (size_t i = 0; i < matches.length; i++) {
        const char *base = strrchr(matches.buffer[i], '/');
        base = base ? base + 1 : matches.buffer[i];
        if (strcmp(base, name) == 0) return 1;
    }
    return 0;
}

void setUp(void)
{
    strcpy(tmpdir, "/tmp/test_worker_XXXXXX");
    TEST_ASSERT_NOT_NULL(mkdtemp(tmpdir));
    TEST_ASSERT_EQUAL_INT(0, con_str_vec_init(&matches, 0));
}

void tearDown(void)
{
    con_str_vec_destroy(&matches);
    rmtmpdir();
}

/* ------------------------------------------------------------------ */
/* Group 1: basic matching                                             */
/* ------------------------------------------------------------------ */

void test_single_match_in_flat_dir(void)
{
    touch("report_final.txt");
    touch("readme.md");
    const char *subs[] = { "report" };
    run_worker(subs, 1);
    TEST_ASSERT_EQUAL_size_t(1, matches.length);
    TEST_ASSERT_TRUE(matches_basename("report_final.txt"));
}

void test_no_match_in_flat_dir(void)
{
    touch("readme.md");
    const char *subs[] = { "report" };
    run_worker(subs, 1);
    TEST_ASSERT_EQUAL_size_t(0, matches.length);
}

void test_multiple_files_all_match(void)
{
    touch("foo_bar.c");
    touch("foo_baz.c");
    touch("foo_qux.h");
    const char *subs[] = { "foo" };
    run_worker(subs, 1);
    TEST_ASSERT_EQUAL_size_t(3, matches.length);
}

void test_multiple_files_partial_match(void)
{
    touch("alpha_beta.txt");
    touch("gamma_delta.txt");
    touch("alpha_gamma.txt");
    const char *subs[] = { "alpha" };
    run_worker(subs, 1);
    TEST_ASSERT_EQUAL_size_t(2, matches.length);
}

void test_empty_directory(void)
{
    const char *subs[] = { "anything" };
    run_worker(subs, 1);
    TEST_ASSERT_EQUAL_size_t(0, matches.length);
}

/* ------------------------------------------------------------------ */
/* Group 2: multiple substrings                                        */
/* ------------------------------------------------------------------ */

void test_or_semantics_two_substrings(void)
{
    touch("main.c");
    touch("test_main.c");
    touch("helper.c");
    const char *subs[] = { "test_", "helper" };
    run_worker(subs, 2);
    TEST_ASSERT_EQUAL_size_t(2, matches.length);
    TEST_ASSERT_TRUE(matches_basename("test_main.c"));
    TEST_ASSERT_TRUE(matches_basename("helper.c"));
    TEST_ASSERT_FALSE(matches_basename("main.c"));
}

void test_file_reported_once_when_matching_multiple_substrings(void)
{
    /* "test_helper.c" matches both "test_" and "helper" — must appear once */
    touch("test_helper.c");
    const char *subs[] = { "test_", "helper" };
    run_worker(subs, 2);
    TEST_ASSERT_EQUAL_size_t(1, matches.length);
}

void test_three_substrings(void)
{
    touch("aaa.c");
    touch("bbb.c");
    touch("ccc.c");
    touch("ddd.c");
    const char *subs[] = { "aaa", "bbb", "ccc" };
    run_worker(subs, 3);
    TEST_ASSERT_EQUAL_size_t(3, matches.length);
    TEST_ASSERT_FALSE(matches_basename("ddd.c"));
}

/* ------------------------------------------------------------------ */
/* Group 3: recursive traversal                                        */
/* ------------------------------------------------------------------ */

void test_recursive_single_level(void)
{
    mksubdir("sub");
    touch("sub/target.txt");
    touch("other.txt");
    const char *subs[] = { "target" };
    run_worker(subs, 1);
    TEST_ASSERT_EQUAL_size_t(1, matches.length);
    TEST_ASSERT_TRUE(matches_basename("target.txt"));
}

void test_recursive_two_levels_deep(void)
{
    mksubdir("a");
    mksubdir("a/b");
    touch("a/b/deep.txt");
    const char *subs[] = { "deep" };
    run_worker(subs, 1);
    TEST_ASSERT_EQUAL_size_t(1, matches.length);
    TEST_ASSERT_TRUE(matches_basename("deep.txt"));
}

void test_files_in_root_and_subdir_both_found(void)
{
    touch("match_root.c");
    mksubdir("sub");
    touch("sub/match_sub.c");
    const char *subs[] = { "match_" };
    run_worker(subs, 1);
    TEST_ASSERT_EQUAL_size_t(2, matches.length);
}

void test_dot_entries_not_reported(void)
{
    /* Searching for "." would match "." and ".." if not guarded */
    const char *subs[] = { "." };
    run_worker(subs, 1);
    TEST_ASSERT_EQUAL_size_t(0, matches.length);
}

void test_symlinks_are_skipped(void)
{
    touch("real.txt");
    char real_path[512], link_path[512];
    snprintf(real_path, sizeof(real_path), "%s/real.txt", tmpdir);
    snprintf(link_path, sizeof(link_path), "%s/link_real.txt", tmpdir);
    symlink(real_path, link_path);
    const char *subs[] = { "real" };
    run_worker(subs, 1);
    /* Only real.txt should match; the symlink is skipped */
    TEST_ASSERT_EQUAL_size_t(1, matches.length);
    TEST_ASSERT_TRUE(matches_basename("real.txt"));
}

void test_inaccessible_subdir_skipped_gracefully(void)
{
    mksubdir("noaccess");
    touch("noaccess/hidden.txt");
    char noaccess[512];
    snprintf(noaccess, sizeof(noaccess), "%s/noaccess", tmpdir);
    chmod(noaccess, 0000);

    const char *subs[] = { "hidden" };
    run_worker(subs, 1);  /* must not crash or return failure */
    TEST_ASSERT_EQUAL_size_t(0, matches.length);

    /* Restore so tearDown can remove the directory */
    chmod(noaccess, 0755);
}

void test_directory_name_not_reported_as_match(void)
{
    /* A directory whose name matches the substring should not appear */
    mksubdir("foobar");
    touch("foobar/inside.txt");
    const char *subs[] = { "foobar" };
    run_worker(subs, 1);
    TEST_ASSERT_EQUAL_size_t(0, matches.length);
}

/* ------------------------------------------------------------------ */
/* Group 4: path construction                                          */
/* ------------------------------------------------------------------ */

void test_result_contains_full_absolute_path(void)
{
    touch("needle.txt");
    const char *subs[] = { "needle" };
    run_worker(subs, 1);
    TEST_ASSERT_EQUAL_size_t(1, matches.length);
    /* Result must start with the absolute tmpdir */
    TEST_ASSERT_TRUE(strncmp(matches.buffer[0], tmpdir, strlen(tmpdir)) == 0);
    /* And end with the filename */
    TEST_ASSERT_TRUE(matches_basename("needle.txt"));
}

void test_path_has_no_double_slash(void)
{
    mksubdir("sub");
    touch("sub/file.txt");
    const char *subs[] = { "file" };
    run_worker(subs, 1);
    TEST_ASSERT_EQUAL_size_t(1, matches.length);
    TEST_ASSERT_NULL(strstr(matches.buffer[0], "//"));
}

/* ------------------------------------------------------------------ */
/* Group 5: worker_main return value contract                          */
/* ------------------------------------------------------------------ */

void test_worker_main_returns_null_on_success(void)
{
    touch("found.txt");
    struct worker_args args = {
        .root_dir   = tmpdir,
        .substrings = (const char *const[]){ "found" },
        .count      = 1,
    };
    pthread_t t;
    pthread_create(&t, NULL, worker_main, &args);
    void *retval = (void *)1; /* sentinel — must be overwritten */
    pthread_join(t, &retval);
    TEST_ASSERT_NULL(retval);
}

void test_matches_accumulates_across_calls(void)
{
    /* Pre-load one entry */
    TEST_ASSERT_EQUAL_INT(0, con_str_vec_push(&matches, strdup("pre-existing")));
    touch("extra.txt");
    const char *subs[] = { "extra" };
    run_worker(subs, 1);
    /* Worker appends; does not reset */
    TEST_ASSERT_EQUAL_size_t(2, matches.length);
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    UNITY_BEGIN();

    /* basic matching */
    RUN_TEST(test_single_match_in_flat_dir);
    RUN_TEST(test_no_match_in_flat_dir);
    RUN_TEST(test_multiple_files_all_match);
    RUN_TEST(test_multiple_files_partial_match);
    RUN_TEST(test_empty_directory);

    /* multiple substrings */
    RUN_TEST(test_or_semantics_two_substrings);
    RUN_TEST(test_file_reported_once_when_matching_multiple_substrings);
    RUN_TEST(test_three_substrings);

    /* recursive traversal */
    RUN_TEST(test_recursive_single_level);
    RUN_TEST(test_recursive_two_levels_deep);
    RUN_TEST(test_files_in_root_and_subdir_both_found);
    RUN_TEST(test_dot_entries_not_reported);
    RUN_TEST(test_symlinks_are_skipped);
    RUN_TEST(test_inaccessible_subdir_skipped_gracefully);
    RUN_TEST(test_directory_name_not_reported_as_match);

    /* path construction */
    RUN_TEST(test_result_contains_full_absolute_path);
    RUN_TEST(test_path_has_no_double_slash);

    /* worker_main contract */
    RUN_TEST(test_worker_main_returns_null_on_success);
    RUN_TEST(test_matches_accumulates_across_calls);

    return UNITY_END();
}
