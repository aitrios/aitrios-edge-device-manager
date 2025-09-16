/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

static volatile sig_atomic_t s_child_pid = 0;
static void SigHandler(int sig) {
  if (0 < s_child_pid) {
    kill(s_child_pid, sig); // send signal to child.
  }
}

int main(int argc, char *argv[]) {
  signal(SIGINT,  SigHandler);
  signal(SIGTERM, SigHandler);
  s_child_pid = fork();
  int esf_ret = -1;
  if (s_child_pid == 0) { // child
    extern int esf_main(int argc, char *argv[]);
    int ret = esf_main(argc, argv);
    exit(ret);
  } else if (0 < s_child_pid) { // parent
    int stat;
    int ret;
    do {
      ret = waitpid(s_child_pid, &stat, 0);
    } while ((ret == -1) && (errno == EINTR));
    if (ret == -1) {
      fprintf(stderr, "waitpid failed errno=%d\n", errno);
    } else {
      if (WIFSIGNALED(stat)) {
        printf("esf_main end with sig=%d\n", WTERMSIG(stat));
      }
      if (WIFEXITED(stat)) {
        esf_ret = WEXITSTATUS(stat);
        printf("esf_main exited with code=%d\n", esf_ret);
      }
    }
  } else {
    printf("Failed to fork:%d errno:%d\n", s_child_pid, errno);
  }

#ifdef CONFIG_ENABLE_REBOOT_ON_EDC_ASSERT
  execl("/sbin/reboot", "reboot", (char *)NULL);
#endif
  return esf_ret;
}
