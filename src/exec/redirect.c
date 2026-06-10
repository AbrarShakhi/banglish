#include "redirect.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int redirects_apply(const Redir *redirs, int n, RedirSave *save) {
  if (save) {
    save->saved[0] = dup(STDIN_FILENO);
    save->saved[1] = dup(STDOUT_FILENO);
    save->saved[2] = dup(STDERR_FILENO);
  }

  for (int i = 0; i < n; i++) {
    const Redir *r = &redirs[i];
    int dest = r->fd >= 0 ? r->fd : STDOUT_FILENO;
    int src = -1;
    int flags;

    switch (r->type) {
    case REDIR_IN:
      src = open(r->target, O_RDONLY);
      if (src < 0) {
        perror(r->target);
        goto fail;
      }
      dup2(src, STDIN_FILENO);
      close(src);
      break;

    case REDIR_OUT:
      if (r->target && r->target[0] == '&') {
        dup2(r->target[1] - '0', dest);
      } else {
        flags = O_WRONLY | O_CREAT | O_TRUNC;
        src = open(r->target, flags, 0644);
        if (src < 0) {
          perror(r->target);
          goto fail;
        }
        dup2(src, dest);
        close(src);
      }
      break;

    case REDIR_APPEND:
      if (r->target && r->target[0] == '&') {
        dup2(r->target[1] - '0', dest);
      } else {
        flags = O_WRONLY | O_CREAT | O_APPEND;
        src = open(r->target, flags, 0644);
        if (src < 0) {
          perror(r->target);
          goto fail;
        }
        dup2(src, dest);
        close(src);
      }
      break;

    case REDIR_ERR:
      /* "&1" means dup stderr → current stdout (used by |& pipe) */
      if (r->target && r->target[0] == '&') {
        int target_fd = r->target[1] - '0';
        dup2(target_fd, STDERR_FILENO);
      } else {
        flags = O_WRONLY | O_CREAT | O_TRUNC;
        src = open(r->target, flags, 0644);
        if (src < 0) {
          perror(r->target);
          goto fail;
        }
        dup2(src, STDERR_FILENO);
        close(src);
      }
      break;

    case REDIR_ERR_APPEND:
      flags = O_WRONLY | O_CREAT | O_APPEND;
      src = open(r->target, flags, 0644);
      if (src < 0) {
        perror(r->target);
        goto fail;
      }
      dup2(src, STDERR_FILENO);
      close(src);
      break;

    case REDIR_BOTH:
      flags = O_WRONLY | O_CREAT | O_TRUNC;
      src = open(r->target, flags, 0644);
      if (src < 0) {
        perror(r->target);
        goto fail;
      }
      dup2(src, STDOUT_FILENO);
      dup2(src, STDERR_FILENO);
      close(src);
      break;

    case REDIR_BOTH_APPEND:
      flags = O_WRONLY | O_CREAT | O_APPEND;
      src = open(r->target, flags, 0644);
      if (src < 0) {
        perror(r->target);
        goto fail;
      }
      dup2(src, STDOUT_FILENO);
      dup2(src, STDERR_FILENO);
      close(src);
      break;

    case REDIR_HEREDOC:
      /* Heredoc content is embedded in target; create a pipe */
      {
        int pfd[2];
        if (pipe(pfd) < 0)
          goto fail;
        const char *content = r->target;
        size_t len = 0;
        while (content[len])
          len++;
        write(pfd[1], content, len);
        close(pfd[1]);
        dup2(pfd[0], STDIN_FILENO);
        close(pfd[0]);
      }
      break;
    }
  }
  return 0;

fail:
  if (save)
    redirects_restore(save);
  return -1;
}

void redirects_restore(const RedirSave *save) {
  if (!save)
    return;
  if (save->saved[0] >= 0) {
    dup2(save->saved[0], STDIN_FILENO);
    close(save->saved[0]);
  }
  if (save->saved[1] >= 0) {
    dup2(save->saved[1], STDOUT_FILENO);
    close(save->saved[1]);
  }
  if (save->saved[2] >= 0) {
    dup2(save->saved[2], STDERR_FILENO);
    close(save->saved[2]);
  }
}
