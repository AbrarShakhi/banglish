#pragma once

/* All functions return heap-allocated strings; caller must mem_free() them. */
char *info_get_username(void);
char *info_get_hostname(void);
char *info_get_cwd(void);
char *info_get_cwd_tilde(void);
