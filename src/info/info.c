#include "info.h"
#include "utils/memory.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

char *info_get_username(void) {
  const char *user = getenv("USER");
  if (!user || !*user)
    user = "user";
  return mem_strdup(user);
}

char *info_get_hostname(void) {
  char buf[HOST_NAME_MAX + 1];
  if (gethostname(buf, sizeof(buf)) != 0)
    return mem_strdup("localhost");
  buf[HOST_NAME_MAX] = '\0';
  /* Trim domain: keep only up to first '.' */
  char *dot = strchr(buf, '.');
  if (dot)
    *dot = '\0';
  return mem_strdup(buf);
}

char *info_get_cwd(void) {
  char buf[PATH_MAX];
  if (!getcwd(buf, sizeof(buf)))
    return mem_strdup("?");
  return mem_strdup(buf);
}

char *info_get_cwd_tilde(void) {
  char *cwd = info_get_cwd();
  const char *home = getenv("HOME");
  if (!home || !*home)
    return cwd;
  size_t hlen = strlen(home);
  if (strncmp(cwd, home, hlen) == 0 &&
      (cwd[hlen] == '/' || cwd[hlen] == '\0')) {
    char *rel = mem_alloc(strlen(cwd) - hlen + 2);
    rel[0] = '~';
    strcpy(rel + 1, cwd + hlen);
    mem_free(cwd);
    return rel;
  }
  return cwd;
}
