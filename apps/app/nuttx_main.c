/*
 * Nuttx application entry
 */
#include <nuttx/config.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>

#include <nuttx/nuttx.h>
#include <nuttx/sched.h>
#include <nuttx/input/buttons.h>
#include <syslog.h>

#ifndef BIT
#define BIT(n) (0x1ul << (n))
#endif

#define BUTTON_DEV "/dev/buttons"

static char inputthrd_stack[2048] aligned_data(8);

static void task_backtrace(FAR struct tcb_s *tcb, FAR void *arg) {
    (void) arg;
    sched_dumpstack(tcb->pid);
}

static int input_thread(int argc, char *argv[]) {
    struct pollfd fds[1];
    btn_buttonset_t supported;
    btn_buttonset_t sample = 0;
    int fd;
    int ret;

    (void) argc;
    (void) argv;

    fd = open(BUTTON_DEV, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        printf("ERROR: Failed to open %s: %d\n", BUTTON_DEV, errno);
        goto _exit;
    }

    ret = ioctl(fd, BTNIOC_SUPPORTED, (unsigned long)((uintptr_t)&supported));
    if (ret < 0) {
        printf("ERROR: ioctl(BTNIOC_SUPPORTED) failed: %d\n", errno);
        goto _close;
    }

    for ( ; ; ) {
        memset(fds, 0, sizeof(fds));
        fds[0].fd = fd;
        fds[0].events = POLLIN;
        poll(fds, 1, -1);

        ret = read(fds[0].fd, (void *)&sample, sizeof(btn_buttonset_t));
        if (ret > 0) {
            if (fds[0].revents & POLLIN) {
                if (!(sample & BIT(0))) {
                    printf("Button pressed: 0x%x\n", sample);
                    nxsched_foreach(task_backtrace, NULL);
                }
            }
        }
    }

_close:
    close(fd);
_exit:
    return EXIT_FAILURE;
}

void nuttx_main(void) {
    int pid;

    pid = nxtask_create("InputThread", 100, inputthrd_stack, 
        sizeof(inputthrd_stack), input_thread, NULL, NULL);
    if (pid < 0) {
        syslog(LOG_ERR, "Create task failed(%d)\n", pid);
        return;
    }
                  
    syslog(LOG_INFO, "Nuttx application init...\n");
}
