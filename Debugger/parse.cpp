
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "parse.h"

struct command * parse_line(char *line) {
    char *str1, *str2, *saveptr1, *saveptr2, *one_cmd, *one_arg;
    struct command *cmd, **cmdp, *new_cmd;
    int j;

    cmd = NULL;
    cmdp = &cmd;

    for (str1 = line; ; str1 = NULL) {
        one_cmd = strtok_r(str1, "|", &saveptr1);
        if (one_cmd == NULL)
            break;
#ifndef LIFO
        *cmdp = (struct command *)malloc(sizeof (struct command));
        new_cmd = *cmdp;
        new_cmd->next = NULL;
        cmdp = &new_cmd->next;
#else
        new_cmd = (struct command *)malloc(sizeof (struct command));
        new_cmd->next = cmd;
        cmd = new_cmd;
#endif /* LIFO */

        for (j = 0, str2 = one_cmd; ; j++, str2 = NULL) {
            if (j >= MAX_ARG) {
                fprintf(stderr, "Too many arguments.\n");
                exit(1);
            }
            one_arg = strtok_r(str2, " ", &saveptr2);
            if (one_arg == NULL)
                break;
            new_cmd->argv[j] = one_arg;
        }
        new_cmd->argv[j] = '\0';

    }
    return (cmd);
}

void dump_cmds(const struct command *cmd) {
    int i;
    while (cmd) {
        printf("command:");
        for (i = 0; cmd->argv[i]; i++) {
            printf("%s-", cmd->argv[i]);
        }
        printf("\n");
        cmd = cmd->next;
    }

}

void free_cmds(struct command *cmd) {
    struct command *aux;
    for (aux = cmd; cmd; cmd = cmd->next, free(aux), aux=cmd)
        ;
}
