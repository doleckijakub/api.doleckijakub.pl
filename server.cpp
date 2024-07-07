#include <assert.h>
#include <stdio.h>
#define eprintf(format, ...) fprintf(stderr, format, ##__VA_ARGS__)
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

__attribute__((noreturn)) void fatal(const char *format, ...) {
    eprintf("Error: ");

    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    eprintf("\n");
    exit(1);
}

const char *exec(const char *argv0, const char *argv[]) {
    int link[2];
    if (pipe(link) < 0) fatal("pipe failed: %s", strerror(errno));

    pid_t pid = fork();
    if (pid < 0) fatal("fork failed: %s", strerror(errno));

    if (pid) {
        close(link[1]);

        static char buf[4096];
        int sz = read(link[0], buf, sizeof(buf));
        buf[sz] = 0;
        
        wait(NULL);

        return buf;
    } else {
        dup2(link[1], STDOUT_FILENO);
        close(link[0]);
        close(link[1]);
        int ret = execvp(argv0, (char *const *) argv);
        fatal("exec failed: %s", strerror(errno));
    }
}

const char *ssh_cmd(const char *host, ...) {
    const char *argv[32] = { 0 };
    int argc = 0;

    va_list ap;
    va_start(ap, host);

    if (strcmp(host, "localhost") == 0 || strcmp(host, "127.0.0.1") == 0) {
        const char *file = va_arg(ap, const char *);
        argv[argc++] = file;
        argv[argc++] = file;
    } else {
        argv[argc++] = "ssh";
        argv[argc++] = "ssh";
        argv[argc++] = host;
    }

    const char *arg = NULL;
    do {
        arg = va_arg(ap, const char *);
        argv[argc++] = arg;
    } while (arg);

    va_end(ap);

    return exec(argv[0], &argv[1]);
}

const char *get_host_uptime(const char *host) {
    return ssh_cmd(host, "uptime", "-p", NULL);
}

#define next_arg() (assert(argc--), *argv++)
int main(int argc, const char **argv) {
    const char *argv0 = next_arg();

    if (argc) {
        const char *command = next_arg();
        if (strcmp(command, "uptime") == 0) {
            if (argc) {
                const char *host = next_arg();
                printf("%s\n", get_host_uptime(host));
            } else {
                fatal("no ip provided");
            }
        } else {
            fatal("unknown subcommand");
        }
    } else {
        fatal("no subcommand provided");
    }

    return 0;
}