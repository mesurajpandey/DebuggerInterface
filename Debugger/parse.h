#ifndef _PARSE_H_
#define	_PARSE_H_

#define	MAX_ARG 32

struct command {
    char *argv[MAX_ARG+1];	/* command line arguments */
    int fd[2];		/* pipe to next command */
    struct command *next;	/* next command in the pipeline */
};

struct command * parse_line(char* line);

void dump_cmds(const struct command *cmd);

void free_cmds(struct command *cmd);

#endif /* _PARSE_H_ */
