/* Q1: cat /etc/passwd | cut -d: -f7 | sort -u  using fork/pipe/dup2/execvp */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

static void die(const char *msg) { perror(msg); exit(EXIT_FAILURE); }

int main(void)
{
    int p1[2], p2[2], p3[2];          /* cat->cut, cut->sort, sort->parent */
    if (pipe(p1) == -1 || pipe(p2) == -1 || pipe(p3) == -1) die("pipe");

    char *c1[] = {"cat", "/etc/passwd", NULL};
    char *c2[] = {"cut", "-d:", "-f7", NULL};
    char *c3[] = {"sort", "-u", NULL};
    char **cmds[3] = {c1, c2, c3};
    int in_fd[3]  = {-1, p1[0], p2[0]};
    int out_fd[3] = {p1[1], p2[1], p3[1]};
    pid_t pids[3];

    for (int i = 0; i < 3; i++) {
        pids[i] = fork();
        if (pids[i] < 0) die("fork");
        if (pids[i] == 0) {                       /* child i */
            if (in_fd[i] != -1 && dup2(in_fd[i], STDIN_FILENO) == -1) die("dup2 stdin");
            if (dup2(out_fd[i], STDOUT_FILENO) == -1) die("dup2 stdout");
            /* close every pipe end: the needed ones now live on fd 0/1 */
            close(p1[0]); close(p1[1]);
            close(p2[0]); close(p2[1]);
            close(p3[0]); close(p3[1]);
            execvp(cmds[i][0], cmds[i]);
            perror("execvp");
            _exit(127);
        }
    }

    /* parent: keep only the read end of the last pipe */
    close(p1[0]); close(p1[1]);
    close(p2[0]); close(p2[1]);
    close(p3[1]);                                 /* so read() can see EOF */

    int out = open("result.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out == -1) die("open result.txt");

    char buf[4096];
    ssize_t n;
    while ((n = read(p3[0], buf, sizeof buf)) != 0) {
        if (n == -1) { if (errno == EINTR) continue; die("read pipe"); }
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(out, buf + off, n - off);
            if (w == -1) { if (errno == EINTR) continue; die("write result.txt"); }
            off += w;
        }
    }
    close(p3[0]);
    close(out);

    for (int i = 0; i < 3; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) == -1) die("waitpid");
        if (WIFEXITED(status))
            printf("child %d (%s, pid %d) exited with status %d\n",
                   i + 1, cmds[i][0], (int)pids[i], WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("child %d (%s, pid %d) killed by signal %d\n",
                   i + 1, cmds[i][0], (int)pids[i], WTERMSIG(status));
    }

    int in = open("result.txt", O_RDONLY);
    if (in == -1) die("reopen result.txt");
    long lines = 0, shown = 0;
    int last = '\n';
    printf("--- first 5 lines of result.txt ---\n");
    while ((n = read(in, buf, sizeof buf)) != 0) {
        if (n == -1) { if (errno == EINTR) continue; die("read result.txt"); }
        for (ssize_t i = 0; i < n; i++) {
            if (lines < 5) putchar(buf[i]);
            if (buf[i] == '\n') lines++;
            last = buf[i];
        }
    }
    if (last != '\n') lines++;                    /* final line without newline */
    close(in);
    (void)shown;
    printf("--- total lines: %ld ---\n", lines);
    return 0;
}
