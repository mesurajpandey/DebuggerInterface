#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "parse.h"
#include <QDebug>

// TODO:
// presmerovani z/do souboru
// job control
// signal handling

int exec_cmds (struct command *cmd) {
    //dump_cmds(cmd);

    struct command * prev_cmd = NULL;

    while (cmd) {

        // new pipe
        if (pipe(cmd->fd)) {
            //err(1, "Pipe error");
            qDebug()<<"Pipe error";
            exit(1);
        }

        // fork
        switch (fork()) {

            // error
            case -1:
                //err(1, "Fork error");
                qDebug()<<"Fork error";
//                exit(1);
                break;

            // cmd
            case 0:
                // not first command

                if (prev_cmd) {
                    // read from prev. pipe
                    close(0);
                    dup(prev_cmd->fd[0]);

                    // close prev. pipe
                    close(prev_cmd->fd[0]);
                    close(prev_cmd->fd[1]);
                }

                // not last command
                if (cmd->next) {
                    // output to curr. pipe
                    close(1);
                    dup(cmd->fd[1]);

                    // close curr. pipe
                    close(cmd->fd[0]);
                    close(cmd->fd[1]);
                }
                execvp(cmd->argv[0], cmd->argv);
                break;

            // shell
            default:
                // if not first, close prev. pipe
                if (prev_cmd) {
                    close(prev_cmd->fd[0]);
                    close(prev_cmd->fd[1]);
                }

                // next
                prev_cmd = cmd;
                cmd = cmd->next;

                break;
        }	//switch
    }	// while

    // wait for all commands
    int status;
    while (wait(&status) != -1);

    return 0;
}

int readcommands(char* cmds,int len) {
    char line[len];
    strcpy(line,cmds);
    struct command *cmd;
    int quit = 0;

//    while (!quit) {
        //line = readline("myshell> ");
//        line = "ls";
        printf("%s",line);
        if (!line)
            return (0);
        add_history(line);
        cmd = parse_line(line);



        if (exec_cmds(cmd)) {
            fprintf(stderr, "Line execution failed.\n");
        }
        free_cmds(cmd);
//    }

    return (0);
}
