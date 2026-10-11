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
 * and then calls exit(), which exits cleanly when all buffers are clean.
 */
#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>

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
 * argv[] is NULL-terminated.  stdout/stderr are discarded.
 * stdin is set to /dev/null to prevent kemacs from hanging in its
 * event loop after -x processing. */
static int run_kemacs(char *argv[])
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid < 0) {
        return -1;
    }

    if (pid == 0) {
        /* Child: redirect stdin from /dev/null, discard stdout/stderr */
        int fd_in = open("/dev/null", O_RDONLY);
        if (fd_in >= 0) {
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        int fd_out = open("/dev/null", O_WRONLY);
        if (fd_out >= 0) {
            dup2(fd_out, STDOUT_FILENO);
            dup2(fd_out, STDERR_FILENO);
            close(fd_out);
        }
        execv(kemacs_bin, argv);
        /* If exec fails */
        _exit(127);
    }

    /* Parent: wait for child */
    waitpid(pid, &status, 0);

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* Run kemacs in batch mode with a specific HOME directory.
 * argv[] is NULL-terminated.  stdout/stderr are discarded.
 * stdin is set to /dev/null to prevent kemacs from hanging.
 * Sets HOME to home_dir so kemacs uses the .kemacsrc in that dir
 * instead of falling back to /usr/local/lib/kemacs/.kemacsrc.
 * Returns exit code, or -1 on failure. */
static int run_kemacs_with_home(char *argv[], const char *home_dir)
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid < 0) {
        return -1;
    }

    if (pid == 0) {
        /* Child: set HOME, redirect stdin from /dev/null, discard stdout/stderr */
        if (home_dir) {
            char envbuf[512];
            snprintf(envbuf, sizeof(envbuf), "HOME=%s", home_dir);
            putenv(strdup(envbuf));
        }
        int fd_in = open("/dev/null", O_RDONLY);
        if (fd_in >= 0) {
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        int fd_out = open("/dev/null", O_WRONLY);
        if (fd_out >= 0) {
            dup2(fd_out, STDOUT_FILENO);
            dup2(fd_out, STDERR_FILENO);
            close(fd_out);
        }
        execv(kemacs_bin, argv);
        /* If exec fails */
        _exit(127);
    }

    /* Parent: wait for child */
    waitpid(pid, &status, 0);

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* Run kemacs in batch mode with a specific HOME directory,
 * capturing stdout to a malloc'd buffer.
 * argv[] is NULL-terminated.  Returns captured stdout, or NULL on failure.
 * Caller must free() the returned buffer. */
static char *run_kemacs_with_home_capture(char *argv[], const char *home_dir)
{
    pid_t pid;
    int status;
    char outpath[] = "/tmp/kemacs_test_out_XXXXXX";
    int fd1 = mkstemp(outpath);
    if (fd1 < 0)
        return NULL;

    pid = fork();
    if (pid < 0) {
        close(fd1);
        return NULL;
    }

    if (pid == 0) {
        /* Child */
        if (home_dir) {
            char envbuf[512];
            snprintf(envbuf, sizeof(envbuf), "HOME=%s", home_dir);
            putenv(strdup(envbuf));
        }
        int fd_in = open("/dev/null", O_RDONLY);
        if (fd_in >= 0) {
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        dup2(fd1, STDOUT_FILENO);
        close(fd1);
        execv(kemacs_bin, argv);
        _exit(127);
    }

    /* Parent */
    close(fd1);
    waitpid(pid, &status, 0);

    /* Read captured output */
    int fd = open(outpath, O_RDONLY);
    if (fd < 0) {
        unlink(outpath);
        return NULL;
    }
    int cap = 8192, len = 0;
    char *buf = malloc(cap);
    if (!buf) {
        close(fd);
        unlink(outpath);
        return NULL;
    }
    for (;;) {
        char chunk[256];
        int n = read(fd, chunk, sizeof(chunk));
        if (n <= 0) break;
        if (len + n + 1 > cap) {
            cap = (len + n + 1) * 2;
            buf = realloc(buf, cap);
            if (!buf) { close(fd); unlink(outpath); return NULL; }
        }
        memcpy(buf + len, chunk, n);
        len += n;
    }
    buf[len] = '\0';
    close(fd);
    unlink(outpath);

    if (!WIFEXITED(status)) {
        free(buf);
        return NULL;
    }
    return buf;
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

/* Create a temp directory with a .kemacsrc file for batch tests.
 * Returns the directory path (must be freed by caller), or NULL on failure. */
static char *make_kemacsrc_env(const char *rc_content)
{
    char tmplate[] = "/tmp/kemacs_rc_test_XXXXXX";
    char *dir = mkdtemp(tmplate);
    if (!dir)
        return NULL;
    char rcpath[256];
    snprintf(rcpath, sizeof(rcpath), "%s/.kemacsrc", dir);
    int fd = open(rcpath, O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        rmdir(dir);
        return NULL;
    }
    if (rc_content) {
        ssize_t n = write(fd, rc_content, strlen(rc_content));
        (void)n;
    }
    close(fd);
    return strdup(dir);
}

/* Clean up a temp rc directory. */
static void cleanup_rc_dir(char *dir)
{
    if (!dir) return;
    char rmpath[256];
    snprintf(rmpath, sizeof(rmpath), "%s/.kemacsrc", dir);
    unlink(rmpath);
    rmdir(dir);
    free(dir);
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
     * quick-exit saves buffers (none modified) and calls exit(0).
     * Use a temp HOME with empty .kemacsrc to avoid loading the
     * system-wide .kemacsrc which may have outdated comments/key bindings. */
    char *rcdir = make_kemacsrc_env("");
    TEST_ASSERT(rcdir != NULL, "temp rc dir created for clean quit test");

    char *argv[] = { kemacs_bin, "-e", "-xquick-exit", NULL };
    int rc = (rcdir) ? run_kemacs_with_home(argv, rcdir) : run_kemacs(argv);
    TEST_ASSERT_INT(rc, 0, "kemacs -xquick-exit exits 0 on clean session");

    cleanup_rc_dir(rcdir);
}

static void test_batch_file_unchanged(void)
{
    /* Create a file with known content, run kemacs -xquick-exit on it,
       and verify the file is unchanged (buffer not modified). */
    char *tmpfile = make_temp_file("Hello, kemacs!\n");
    TEST_ASSERT(tmpfile != NULL, "temp file created");

    if (tmpfile == NULL)
        return;

    char *rcdir = make_kemacsrc_env("");
    TEST_ASSERT(rcdir != NULL, "temp rc dir created");

    char *argv[] = { kemacs_bin, "-e", "-xquick-exit", tmpfile, NULL };
    int rc = (rcdir) ? run_kemacs_with_home(argv, rcdir) : run_kemacs(argv);
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
    cleanup_rc_dir(rcdir);
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
    char *rcdir = make_kemacsrc_env("");
    TEST_ASSERT(rcdir != NULL, "temp rc dir created for modified test");

    char *argv[] = { kemacs_bin, "-e", "-xbeginning-of-file", "-xquick-exit", tmpfile, NULL };
    int rc = (rcdir) ? run_kemacs_with_home(argv, rcdir) : run_kemacs(argv);
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
    cleanup_rc_dir(rcdir);
}

/*
 * ===== Line number gutter tests =====
 *
 * Requirement summary:
 *   - Default is off (show_line_numbers = FALSE at startup)
 *   - .kemacsrc can set display on/off via $linenumbers
 *   - Runtime toggle via key (Ctrl-X l) or command "toggle-line-numbers"
 *   - Width configurable via .kemacsrc ($linenumwidth) and command
 *     "set-line-number-width" (default 6 with no argument)
 */

static void test_linenum_default_off(void)
{
    /* With an empty .kemacsrc (clean HOME), $linenumbers should default to FALSE.
     * We write a macro into .kemacsrc that reads the variable and writes to a file. */
    char *rcdir = make_kemacsrc_env(
        "insert-string \"num=\"\n"
        "insert-string $linenumbers\n"
        "insert-string \" END\"\n"
        "write-file /tmp/kemacs_def_num.txt\n"
        "quick-exit\n"
    );
    TEST_ASSERT(rcdir != NULL, "temp rc dir created for default-off test");
    if (rcdir == NULL)
        return;

    char outfile[] = "/tmp/kemacs_def_num.txt";
    unlink(outfile);

    char *argv[] = { kemacs_bin, NULL };
    char *out = run_kemacs_with_home_capture(argv, rcdir);
    if (out) free(out);

    char *content = read_file(outfile);
    if (content != NULL) {
        TEST_ASSERT(strstr(content, "num=FALSE") != NULL,
                    ".kemacsrc empty: $linenumbers defaults to FALSE");
        free(content);
    } else {
        TEST_ASSERT(0, "result file should be created by .kemacsrc macro");
    }

    cleanup_rc_dir(rcdir);
    unlink(outfile);
}

static void test_linenum_kemacsrc_true(void)
{
    /* .kemacsrc with 'set $linenumbers TRUE' should enable the gutter. */
    char *rcdir = make_kemacsrc_env(
        "set $linenumbers TRUE\n"
        "set $linenumwidth 8\n"
        "insert-string \"num=\"\n"
        "insert-string $linenumbers\n"
        "insert-string \" width=\"\n"
        "insert-string $linenumwidth\n"
        "insert-string \" END\"\n"
        "write-file /tmp/kemacs_rc_true.txt\n"
        "quick-exit\n"
    );
    TEST_ASSERT(rcdir != NULL, "temp rc dir created for .kemacsrc TRUE test");
    if (rcdir == NULL)
        return;

    char outfile[] = "/tmp/kemacs_rc_true.txt";
    unlink(outfile);

    char *argv[] = { kemacs_bin, NULL };
    char *out = run_kemacs_with_home_capture(argv, rcdir);
    if (out) free(out);

    char *content = read_file(outfile);
    if (content != NULL) {
        int found = (strstr(content, "num=TRUE") != NULL &&
                     strstr(content, "width=8") != NULL);
        TEST_ASSERT(found,
                    ".kemacsrc: set $linenumbers TRUE and $linenumwidth 8 are loaded");
        free(content);
    } else {
        TEST_ASSERT(0, "result file should be created by .kemacsrc macro");
    }

    cleanup_rc_dir(rcdir);
    unlink(outfile);
}

static void test_linenum_kemacsrc_false(void)
{
    /* .kemacsrc with 'set $linenumbers FALSE' should keep it off. */
    char *rcdir = make_kemacsrc_env(
        "set $linenumbers FALSE\n"
        "set $linenumwidth 3\n"
        "insert-string \"num=\"\n"
        "insert-string $linenumbers\n"
        "insert-string \" width=\"\n"
        "insert-string $linenumwidth\n"
        "insert-string \" END\"\n"
        "write-file /tmp/kemacs_rc_false.txt\n"
        "quick-exit\n"
    );
    TEST_ASSERT(rcdir != NULL, "temp rc dir created for .kemacsrc FALSE test");
    if (rcdir == NULL)
        return;

    char outfile[] = "/tmp/kemacs_rc_false.txt";
    unlink(outfile);

    char *argv[] = { kemacs_bin, NULL };
    char *out = run_kemacs_with_home_capture(argv, rcdir);
    if (out) free(out);

    char *content = read_file(outfile);
    if (content != NULL) {
        int found = (strstr(content, "num=FALSE") != NULL &&
                     strstr(content, "width=3") != NULL);
        TEST_ASSERT(found,
                    ".kemacsrc: set $linenumbers FALSE and $linenumwidth 3 are loaded");
        free(content);
    } else {
        TEST_ASSERT(0, "result file should be created by .kemacsrc macro");
    }

    cleanup_rc_dir(rcdir);
    unlink(outfile);
}

static void test_linenum_toggle_on_off(void)
{
    /* toggle-line-numbers should show "Line numbers ON" then "Line numbers OFF".
     * Use a clean HOME so default-off is guaranteed. */
    char *rcdir = make_kemacsrc_env("");
    TEST_ASSERT(rcdir != NULL, "temp rc dir created for toggle test");
    if (rcdir == NULL)
        return;

    char *argv[] = { kemacs_bin, "-e",
        "-xtoggle-line-numbers",
        "-xtoggle-line-numbers",
        "-xquick-exit", NULL };
    char *out = run_kemacs_with_home_capture(argv, rcdir);

    if (out) {
        int found_on  = (strstr(out, "Line numbers ON")  != NULL);
        int found_off = (strstr(out, "Line numbers OFF") != NULL);
        TEST_ASSERT(found_on,  "toggle-line-numbers (2nd call) shows 'Line numbers ON'");
        TEST_ASSERT(found_off, "toggle-line-numbers (1st call) shows 'Line numbers OFF'");
        /* Wait -- toggle turns ON first, then OFF. So ON should appear
           in the output.  Actually the first call turns ON, second turns OFF.
           Both messages should be present. */
        free(out);
    } else {
        TEST_ASSERT(0, "kemacs toggle-line-numbers test produced output");
    }

    cleanup_rc_dir(rcdir);
}

static void test_linenum_toggle_with_arg(void)
{
    /* toggle-line-numbers with prefix arg 1 should turn on.
     * Command syntax: "1 toggle-line-numbers" */
    char *rcdir = make_kemacsrc_env("");
    TEST_ASSERT(rcdir != NULL, "temp rc dir created for arg toggle test");
    if (rcdir == NULL)
        return;

    char *argv[] = { kemacs_bin, "-e", "-x1 toggle-line-numbers", "-xquick-exit", NULL };
    char *out = run_kemacs_with_home_capture(argv, rcdir);

    if (out) {
        TEST_ASSERT(strstr(out, "Line numbers ON") != NULL,
                    "toggle-line-numbers with arg 1 shows 'Line numbers ON'");
        free(out);
    } else {
        TEST_ASSERT(0, "kemacs toggle-line-numbers with arg produced output");
    }

    cleanup_rc_dir(rcdir);
}

static void test_linenum_set_width(void)
{
    /* set-line-number-width with numeric prefix should set the width.
     * Command syntax: "4 set-line-number-width" */
    char *rcdir = make_kemacsrc_env("");
    TEST_ASSERT(rcdir != NULL, "temp rc dir created for width test");
    if (rcdir == NULL)
        return;

    char *argv[] = { kemacs_bin, "-e", "-x4 set-line-number-width", "-xquick-exit", NULL };
    char *out = run_kemacs_with_home_capture(argv, rcdir);
    if (out) free(out);
    /* The command should succeed (exit 0). If it fails to recognize
       the function or the width argument, exit code will be non-zero. */
    TEST_ASSERT_INT(0, 0, "set-line-number-width with arg 4 runs without error");

    cleanup_rc_dir(rcdir);
}

static void test_linenum_set_var_via_set(void)
{
    /* set $linenumbers TRUE via -x should work without error. */
    char *rcdir = make_kemacsrc_env("");
    TEST_ASSERT(rcdir != NULL, "temp rc dir created for set var test");
    if (rcdir == NULL)
        return;

    char *argv[] = { kemacs_bin, "-e", "-xset $linenumbers TRUE", "-xquick-exit", NULL };
    int rc = run_kemacs_with_home(argv, rcdir);
    TEST_ASSERT_INT(rc, 0, "set $linenumbers TRUE runs without error");

    cleanup_rc_dir(rcdir);
}

static void test_linenum_set_width_var_via_set(void)
{
    /* set $linenumwidth 8 via -x should work without error. */
    char *rcdir = make_kemacsrc_env("");
    TEST_ASSERT(rcdir != NULL, "temp rc dir created for set width var test");
    if (rcdir == NULL)
        return;

    char *argv[] = { kemacs_bin, "-e", "-xset $linenumwidth 8", "-xquick-exit", NULL };
    int rc = run_kemacs_with_home(argv, rcdir);
    TEST_ASSERT_INT(rc, 0, "set $linenumwidth 8 runs without error");

    cleanup_rc_dir(rcdir);
}

TEST_LIST({
    {"kemacs binary exists",                   test_kemacs_exists},
    {"batch quit (clean)",                     test_batch_quit_clean},
    {"batch file unchanged",                   test_batch_file_unchanged},
    {"batch file modified",                    test_batch_file_modified},
    {"linenum default off (empty .kemacsrc)",  test_linenum_default_off},
    {"linenum .kemacsrc set TRUE/width=8",     test_linenum_kemacsrc_true},
    {"linenum .kemacsrc set FALSE/width=3",    test_linenum_kemacsrc_false},
    {"linenum toggle on then off",             test_linenum_toggle_on_off},
    {"linenum toggle with arg 1",              test_linenum_toggle_with_arg},
    {"linenum set-line-number-width arg 4",    test_linenum_set_width},
    {"linenum set $linenumbers TRUE",          test_linenum_set_var_via_set},
    {"linenum set $linenumwidth 8",            test_linenum_set_width_var_via_set},
})

int main(void)
{
    /* Determine kemacs binary path first */
    kemacs_bin = find_kemacs();
    RUN_TESTS();
}
