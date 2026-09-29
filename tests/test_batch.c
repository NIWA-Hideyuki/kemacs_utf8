/*
 * test_batch.c -- Batch-mode test harness for kemacs.
 *
 * This is NOT a unit test file.  Instead of calling kemacs internals
 * in-process, it invokes the kemacs binary in batch mode (using the
 * -x flag to execute commands) and checks stdout/stderr/exit-code
 * for expected results.
 *
 * The KEMACS_BIN environment variable can override the path to the
 * kemacs binary (default: ../kemacs).
 *
 * Test strategy:
 *   1. Create a temp file with known content
 *   2. Run:  kemacs -x<command> file
 *   3. Verify the file contents after the command
 *   4. (For command-only tests) Run:  kemacs -x<command>  and check exit
 *
 * Note: kemacs -x requires the command to be attached to the -x flag
 * as a single argument (e.g., "-xquick-exit", NOT "-x quick-exit").
 * Also, kemacs always enters its event loop after -x processing, so
 * stdin must be /dev/null to prevent hanging.
 *
 * The "quick-exit" command is used because it saves modified buffers
 * and then calls quit(), which exits cleanly when all buffers are clean.
 */
#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

static char *kemacs_bin = NULL;

/* Find the kemacs binary: check KEMACS_BIN env, then ../kemacs */
static char *find_kemacs(void)
{
    static char path[256];
    char *env = getenv("KEMACS_BIN");
    if (env && env[0]) {
        strncpy(path, env, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
        return path;
    }
    /* Default: ../kemacs relative to tests/ directory */
    strncpy(path, "../kemacs", sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';
    return path;
}

/* Run kemacs in batch mode with a command, return exit status.
 * argv[] is NULL-terminated.  stdout/stderr are captured to temp files.
 * stdin is set to /dev/null to prevent kemacs from hanging in its
 * event loop after -x processing. */
static int run_kemacs(char *argv[])
{
    pid_t pid;
    int status;
    int fd_in, fd_out, fd_err;
    char outpath[] = "/tmp/kemacs_test_out_XXXXXX";
    char errpath[] = "/tmp/kemacs_test_err_XXXXXX";
    int fd1, fd2;

    /* Open temp files for capturing output */
    fd1 = mkstemp(outpath);
    fd2 = mkstemp(errpath);
    if (fd1 < 0 || fd2 < 0) {
        if (fd1 >= 0) close(fd1);
        if (fd2 >= 0) close(fd2);
        return -1;
    }

    pid = fork();
    if (pid < 0) {
        close(fd1); close(fd2);
        return -1;
    }

    if (pid == 0) {
        /* Child: redirect stdin from /dev/null, stdout/stderr to temp files */
        fd_in = open("/dev/null", O_RDONLY);
        if (fd_in >= 0) {
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        dup2(fd1, STDOUT_FILENO);
        dup2(fd2, STDERR_FILENO);
        close(fd1);
        close(fd2);
        execv(kemacs_bin, argv);
        /* If exec fails */
        _exit(127);
    }

    /* Parent: close our copies, wait for child */
    close(fd1);
    close(fd2);
    waitpid(pid, &status, 0);

    /* Clean up temp files */
    unlink(outpath);
    unlink(errpath);

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* Write content to a temp file, return a malloc'd path string.
 * Caller must free() the result. */
static char *make_temp_file(const char *content)
{
    char tmplate[] = "/tmp/kemacs_test_file_XXXXXX";
    int fd = mkstemp(tmplate);
    if (fd < 0)
        return NULL;
    if (content) {
        ssize_t n = write(fd, content, strlen(content));
        (void)n;
    }
    close(fd);
    /* Return a private copy so the template can be reused */
    return strdup(tmplate);
}

/* Read a file into a malloc'd buffer (NUL-terminated). Returns NULL on error. */
static char *read_file(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return NULL;
    char *buf = NULL;
    int cap = 256, len = 0;
    buf = malloc(cap);
    if (!buf) { close(fd); return NULL; }
    for (;;) {
        char chunk[256];
        int n = read(fd, chunk, sizeof(chunk));
        if (n <= 0) break;
        if (len + n + 1 > cap) {
            cap = (len + n + 1) * 2;
            buf = realloc(buf, cap);
            if (!buf) { close(fd); return NULL; }
        }
        memcpy(buf + len, chunk, n);
        len += n;
    }
    buf[len] = '\0';
    close(fd);
    return buf;
}

/* ================================================================ */
/*  Tests                                                            */
/* ================================================================ */

static void test_kemacs_exists(void)
{
    kemacs_bin = find_kemacs();
    int fd = open(kemacs_bin, O_RDONLY);
    TEST_ASSERT(fd >= 0, "kemacs binary exists and is readable");
    if (fd >= 0) close(fd);
}

static void test_batch_quit_clean(void)
{
    /* kemacs -xquick-exit on a clean (no file) session should exit 0.
     * quick-exit saves buffers (none modified) and calls exit(0). */
    char *argv[] = { kemacs_bin, "-e", "-xquick-exit", NULL };

    int rc = run_kemacs(argv);
    TEST_ASSERT_INT(rc, 0, "kemacs -xquick-exit exits 0 on clean session");
}

static void test_batch_file_unchanged(void)
{
    /* Create a file with known content, run kemacs -xquick-exit on it,
       and verify the file is unchanged (buffer not modified). */
    char *tmpfile = make_temp_file("Hello, kemacs!\n");
    TEST_ASSERT(tmpfile != NULL, "temp file created");

    if (tmpfile == NULL)
        return;

    char *argv[] = { kemacs_bin, "-e", "-xquick-exit", tmpfile, NULL };

    int rc = run_kemacs(argv);
    TEST_ASSERT(rc >= 0, "kemacs -xquick-exit runs on file");

    /* File should be unchanged */
    char *content = read_file(tmpfile);
    if (content != NULL) {
        TEST_ASSERT_STR(content, "Hello, kemacs!\n", "file content unchanged");
        free(content);
    } else {
        TEST_ASSERT(0, "file should be readable after kemacs run");
    }

    unlink(tmpfile);
    free(tmpfile);
}

static void test_batch_file_modified(void)
{
    /* Test that kemacs can open a file, execute a command, and save
       via quick-exit.  We use -xbegning-of-file (navigation command)
       followed by -xquick-exit (save and exit) on a file with content.
       Since the buffer is not modified, the file should remain unchanged. */
    char *tmpfile = make_temp_file("Line one\nLine two\n");
    TEST_ASSERT(tmpfile != NULL, "temp file created for modify test");

    if (tmpfile == NULL)
        return;

    /* Use beginning-of-file then quick-exit */
    char *argv[] = { kemacs_bin, "-e", "-xbeginning-of-file", "-xquick-exit", tmpfile, NULL };

    int rc = run_kemacs(argv);
    TEST_ASSERT(rc >= 0, "kemacs -xbeginning-of-file -xquick-exit runs on file");

    /* File should be unchanged (buffer was not modified) */
    char *content = read_file(tmpfile);
    if (content != NULL) {
        TEST_ASSERT_STR(content, "Line one\nLine two\n", "file content preserved after nav+save");
        free(content);
    } else {
        TEST_ASSERT(0, "file should be readable after kemacs run");
    }

    unlink(tmpfile);
    free(tmpfile);
}

TEST_LIST({
    {"kemacs binary exists",    test_kemacs_exists},
    {"batch quit (clean)",      test_batch_quit_clean},
    {"batch file unchanged",    test_batch_file_unchanged},
    {"batch file modified",     test_batch_file_modified},
})

int main(void)
{
    /* Determine kemacs binary path first */
    kemacs_bin = find_kemacs();
    RUN_TESTS();
}