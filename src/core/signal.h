#pragma once

void signals_init_interactive(
    void); /* parent: ignore SIGINT/SIGTSTP/SIGTTIN/SIGTTOU */
void signals_init_child(void);  /* child before exec: restore all to SIG_DFL */
void signals_init_script(void); /* non-interactive: SIG_DFL for SIGINT */
