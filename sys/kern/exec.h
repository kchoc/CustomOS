#ifndef EXEC_H
#define EXEC_H

typedef struct ps_strings {
    char**       ps_argvstr;
    unsigned int ps_nargvstr;
    char**       ps_envstr;
    unsigned int ps_nenvstr;
} ps_strings_t;

int execve(const char* path, char* const argv[], char* const envp[]);

#endif
