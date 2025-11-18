#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_LINE_LEN 2048 // POSIX max line length
#define MAX_PATH_LEN 4096
#define STATUS_EXIT -2
#define STATUS_DIE -3

typedef struct {
    char **tokens;
    int size;
    int cap;
} ArrayList;

void make_list(ArrayList *list, int cap) {
    list->tokens = (char **)malloc(cap * sizeof(char *));
    if(list->tokens == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    list->size = 0;
    list->cap = cap;
}

void list_append(ArrayList *list, char *token) {
    // capacity reached, allocate more space
    if(list->size >= list->cap) {
        int new_cap = list->cap * 2;
        list->tokens = (char **)realloc(list->tokens, new_cap * sizeof(char *));
        if(list->tokens == NULL) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        list->cap = new_cap;
    }

    list->tokens[list->size] = token;
    list->size++;
}

void free_list(ArrayList *list) {
    free(list->tokens);
    list->tokens = NULL;
    list->size = 0;
    list->cap = 0;
}

char * my_strdup(char *c) {
    if(c == NULL) {
        return NULL;
    }
    int len = strlen(c) + 1;
    char *new_c = malloc(len);
    if(new_c == NULL) {
        perror("malloc strdup");
        exit(EXIT_FAILURE);
    }
    memcpy(new_c, c, len);
    return new_c;
}

char * find_path(char *cmd) {
    // check if it's a bare name
    int slash_found = 0;
    char *c = cmd;
    while(*c != '\0') {
        if(*c == '/') {
            slash_found = 1;
            break;
        }
        c++;
    }

    if(slash_found == 1) {
        if(access(cmd, X_OK) == 0) {
            return my_strdup(cmd);
        } else {
            return NULL;
        }
    } else {
        char *search_dirs[] = {"/usr/local/bin", "/usr/bin", "/bin", NULL};
        char path_buf[MAX_PATH_LEN];

        for(int i = 0; search_dirs[i] != NULL; i++) {
            snprintf(path_buf, MAX_PATH_LEN, "%s/%s", search_dirs[i], cmd);
            if(access(path_buf, X_OK) == 0) {
                return my_strdup(path_buf);
            }
        }
    }

    return NULL; // no path found, not a valid executable
}

// handle actually executing a given command as a child process
void execute_child(char **args, char *input_file, char *output_file, int devnull) {
    // set input file if specified, or /dev/null if reading from non-terminal standard input
    if(input_file != NULL) {
        int input_fd = open(input_file, O_RDONLY);
        if(input_fd == -1) {
            perror("input open");
            exit(EXIT_FAILURE);
        }
        if(dup2(input_fd, STDIN_FILENO) == -1) {
            perror("input dup2");
            exit(EXIT_FAILURE);
        }
        close(input_fd);
    } else if(devnull) {
        int input_fd = open("/dev/null", O_RDONLY);
        if(input_fd != -1) {
            if(dup2(input_fd, STDIN_FILENO) == -1) {
                perror("/dev/null dup2");
                exit(EXIT_FAILURE);
            }
            close(input_fd);
        } else {
            perror("/dev/null open");
            exit(EXIT_FAILURE);
        }
    }

    // set output file if specified
    if(output_file != NULL) {
        int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if(output_fd == -1) {
            perror("output open");
            exit(EXIT_FAILURE);
        }
        if(dup2(output_fd, STDOUT_FILENO) == -1) {
            perror("output dup2");
            exit(EXIT_FAILURE);
        }
        close(output_fd);
    }

    // handle built in commands
    if(strcmp(args[0], "pwd") == 0) {
        if(args[1] != NULL) {
            fprintf(stderr, "Error: pwd does not take arguments\n");
            exit(EXIT_FAILURE);
        }

        char path[MAX_PATH_LEN];
        if(getcwd(path, sizeof(path)) != NULL) {
            printf("%s\n", path);
            exit(EXIT_SUCCESS);
        } else {
            perror("pwd");
            exit(EXIT_FAILURE);
        }
    } else if(strcmp(args[0], "which") == 0) {
        if(args[1] == NULL || args[2] != NULL) {
            exit(EXIT_FAILURE);
        }

        // can't be a built in command
        if(strcmp(args[1], "exit") == 0 || strcmp(args[1], "die") == 0 || strcmp(args[1], "which") == 0 || strcmp(args[1], "pwd") == 0 || strcmp(args[1], "cd") == 0) {
            exit(EXIT_FAILURE);
        }

        char *path = find_path(args[1]);
        if(path == NULL) {
            exit(EXIT_FAILURE);
        } else {
            printf("%s\n", path);
            free(path);
            exit(EXIT_SUCCESS);
        }
    } else if(strcmp(args[0], "exit") == 0) { // execute_cmd() already marked current line for exit, so exit can be run as a child process
        for(int i = 1; args[i] != NULL; i++) {
            printf("%s ", args[i]);
        }

        if(args[1] != NULL) {
            printf("\n");
        }

        exit(EXIT_SUCCESS);
    } else if(strcmp(args[0], "die") == 0) { // execute_cmd() already marked current line for die, so die can be run as a child process
        for(int i = 1; args[i] != NULL; i++) {
            printf("%s ", args[i]);
        }

        if(args[1] != NULL) {
            printf("\n");
        }

        exit(EXIT_FAILURE);
    } else if(strcmp(args[0], "cd") == 0) { // cd in a pipeline will not affect parent process
        if(args[1] == NULL || args[2] != NULL) {
            fprintf(stderr, "Error: cd takes one argument\n");
            exit(EXIT_FAILURE);
        }
        if(chdir(args[1]) != 0) {
            perror("cd");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }

    // check if its a command that isn't built in
    char *path = find_path(args[0]);
    if(path == NULL) {
        fprintf(stderr, "Error: command not found\n");
        exit(EXIT_FAILURE);
    }

    execv(path, args);

    // error if execv returns
    perror("execv");
    free(path);
    exit(EXIT_FAILURE);
}

// fork, call function to execute child process, and have parent wait for it
int forked_cmd(char **args, char *input_file, char *output_file, int devnull) {
    pid_t child = fork();
    if(child == -1) {
        perror("fork");
        return 1;
    } else if(child == 0) {
        execute_child(args, input_file, output_file, devnull);
    } else {
        int status;
        waitpid(child, &status, 0);

        if(WIFEXITED(status)) { 
            return WEXITSTATUS(status);
        } else {
            return 1;
        }
    }
    return 1;
}

int execute_cmd(ArrayList *list, int prev_status, int devnull) {
    // no cmd to execute, don't change prev_status
    if(list->size <= 1) {
        return prev_status;
    }

    char **args = list->tokens;
    int new_status = prev_status;
    int exit_flag = 0;
    int die_flag = 0;
    char *cd_flag = NULL;

    // don't count first token as an arg if it's "and" or "or"
    if(strcmp(args[0], "and") == 0) {
        // only execute if previous cmd was successful
        if(prev_status != 0) {
            return prev_status;
        }
        args++;
    } else if(strcmp(args[0], "or") == 0) {
        // only execute if previous cmd failed
        if(prev_status == 0) {
            return prev_status;
        }
        args++;
    }

    // check if there was only one arg, the "and" or "or"
    if(args[0] == NULL) {
        return prev_status;
    }

    // handle piping
    int num_cmds = 1;
    int *pipe_inds = malloc(list->size * sizeof(int));
    if(pipe_inds == NULL) {
        perror("malloc");
        return 1;
    }
    pipe_inds[0] = -1;

    // find the indices of all the pipes
    for(int i = 0; args[i] != NULL; i++) {
        if(strcmp(args[i], "|") == 0) {
            if(args[i + 1] == NULL || strcmp(args[i + 1], "|") == 0 || strcmp(args[i + 1], ">") == 0 || strcmp(args[i + 1], "<") == 0) {
                fprintf(stderr, "Error: pipe with no command after it\n");
                free(pipe_inds);
                return 1;
            }
            if(i > 0) {
                if(strcmp(args[i - 1], "<") == 0) {
                    fprintf(stderr, "Error: did not specify input file\n");
                    free(pipe_inds);
                    return 1;
                } else if(strcmp(args[i - 1], ">") == 0) {
                    fprintf(stderr, "Error: did not specify output file\n");
                    free(pipe_inds);
                    return 1;
                }
            }
            
            pipe_inds[num_cmds] = i;
            num_cmds++;
            args[i] = NULL; // set the pipe to NULL so that it marks the end of the subcommand's args
        }
    }

    // check if there's a pipeline
    if(num_cmds > 1) {
        int (*pipe_fds)[2] = malloc((num_cmds - 1) * sizeof(int[2]));
        if(pipe_fds == NULL) {
            perror("malloc");
            free(pipe_inds);
            return 1;
        }
        for(int i = 0; i < num_cmds - 1; i++) {
            if(pipe(pipe_fds[i]) == -1) {
                perror("pipe");
                free(pipe_inds);
                free(pipe_fds);
                return 1;
            }
        }

        pid_t *pids = malloc(num_cmds * sizeof(pid_t));
        if(pids == NULL) {
            perror("malloc");
            free(pipe_inds);
            free(pipe_fds);
            return 1;
        }
        for(int i = 0; i < num_cmds; i++) {
            char **curr_args = &args[pipe_inds[i] + 1];

            if(strcmp(curr_args[0], "die") == 0) {
                die_flag = 1;
            } else if(strcmp(curr_args[0], "exit") == 0) {
                exit_flag = 1;
            } else if(strcmp(curr_args[0], "cd") == 0) {
                cd_flag = my_strdup(curr_args[1]);
            }

            pids[i] = fork();
            if(pids[i] == -1) {
                perror("fork");
                free(pipe_inds);
                free(pipe_fds);
                free(pids);
                if (cd_flag) free(cd_flag);
                return 1;
            }

            if(pids[i] == 0) {
                if(i > 0) {
                    dup2(pipe_fds[i - 1][0], STDIN_FILENO);
                } 
                if(i < num_cmds - 1) {
                    dup2(pipe_fds[i][1], STDOUT_FILENO);
                }

                for(int j = 0; j < num_cmds - 1; j++) {
                    close(pipe_fds[j][0]);
                    close(pipe_fds[j][1]);
                }

                free(pipe_inds);
                free(pipe_fds);
                free(pids);
                if (cd_flag) free(cd_flag);

                execute_child(curr_args, NULL, NULL, devnull);
                exit(EXIT_FAILURE); // should not be reached, fail if it does
            }
        }

        for(int i = 0; i < num_cmds - 1; i++) {
            close(pipe_fds[i][0]);
            close(pipe_fds[i][1]);
        }

        for(int i = 0; i < num_cmds; i++) {
            int status;
            waitpid(pids[i], &status, 0);
            if(i == num_cmds - 1) {
                if(WIFEXITED(status)) {
                    new_status = WEXITSTATUS(status);
                } else {
                    new_status = 1;
                }
            }
        }
        free(pipe_fds);
        free(pids);

        // actually change directory of parent if there's a cd in the pipeline
        if(cd_flag != NULL) {
            chdir(cd_flag); // don't print out an error, child would have already printed
            free(cd_flag);
        }
    } else { // no pipes, check for redirects and normal cmds
        ArrayList clean;
        make_list(&clean, list->size);
        char *input_file = NULL;
        char *output_file = NULL;
        
        for(int i = 0; args[i] != NULL; i++) {
            if(strcmp(args[i], "<") == 0) {
                if(args[i + 1] == NULL || strcmp(args[i + 1], ">") == 0 || strcmp(args[i + 1], "<") == 0 || strcmp(args[i + 1], "|") == 0) {
                    fprintf(stderr, "Error: did not specify input file\n");
                    free_list(&clean);
                    free(pipe_inds);
                    return 1;
                }
                input_file = args[i + 1];
                i++;
            } else if(strcmp(args[i], ">") == 0) {
                if(args[i + 1] == NULL || strcmp(args[i + 1], ">") == 0 || strcmp(args[i + 1], "<") == 0 || strcmp(args[i + 1], "|") == 0) {
                    fprintf(stderr, "Error: did not specify output file\n");
                    free_list(&clean);
                    free(pipe_inds);
                    return 1;
                }
                output_file = args[i + 1];
                i++;
            } else {
                list_append(&clean, args[i]);
            }
        }
        list_append(&clean, NULL);

        if(clean.size <= 1) {
            free_list(&clean);
            free(pipe_inds);
            return prev_status;
        }
        
        if (strcmp(clean.tokens[0], "die") == 0) {
            die_flag = 1;
        }
        if (strcmp(clean.tokens[0], "exit") == 0) {
            exit_flag = 1;
        }

        // handle cd in the parent
        if (strcmp(clean.tokens[0], "cd") == 0) {
            if(clean.tokens[1] == NULL || clean.tokens[2] != NULL) {
                fprintf(stderr, "Error: cd takes one argument\n");
                new_status = 1;
            } else if (chdir(clean.tokens[1]) != 0) {
                perror("cd");
                new_status = 1;
            } else {
                new_status = 0;
            }
        } else {
            new_status = forked_cmd(clean.tokens, input_file, output_file, devnull);
        }

        free_list(&clean);
    }

    free(pipe_inds);

    if(die_flag) {
        return STATUS_DIE;
    }
    if(exit_flag) {
        return STATUS_EXIT;
    }
    return new_status;
}

// parse through line to find tokens and return an ArrayList of the tokens
ArrayList parse_cmd(char *cmd) {
    ArrayList list;
    make_list(&list, 24);

    char *ptr = cmd;
    while(*ptr != '\0') {
        // get rid of leading whitespace if there is any
        while(isspace(*ptr)) {
            ptr++;
        }

        if(*ptr == '\0') {
            break;
        } else if(*ptr == '#') { // check if the line is a comment
            break;
        }

        list_append(&list, ptr);

        // find the end of the current token
        while(*ptr != '\0' && !isspace(*ptr)) {
            ptr++;
        }

        // separate current token from next token if not at end of line
        if(*ptr != '\0') {
            *ptr = '\0';
            ptr++;
        }
    }

    // end token list with null terminator for execv()
    list_append(&list, NULL);

    return list;
}

int main(int argc, char **argv) {
    int fd;
    int interactive_mode;
    int prev_status = 0;
    
    if(argc == 1) {
        fd = STDIN_FILENO;
    } else if(argc == 2){
        fd = open(argv[1], O_RDONLY);
        if(fd == -1) {
            perror("Error opening file");
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "Error: mysh takes at most 1 argument\n");
        return EXIT_FAILURE;
    }

    interactive_mode = isatty(fd);
    if(interactive_mode) {
        printf("Welcome to my shell!\n"); // welcome message if interactive
    }

    int devnull = (!interactive_mode && fd == STDIN_FILENO);
    char line_buf[MAX_LINE_LEN];
    int line_ind = 0;
    char char_buf;
    int run = 1;

    // input loop
    while(run) {
        if(interactive_mode) {
            printf("mysh> ");
            fflush(stdout);
        }
        
        line_ind = 0;

        // read one character at a time
        while(1) {
            int bytes = read(fd, &char_buf, 1);
            if(bytes == -1) {
                perror("read");
                return EXIT_FAILURE;
            }

            if(bytes == 0) { // EOF, exit shell
                if(line_ind > 0) {
                    line_buf[line_ind] = '\0';
                    ArrayList list = parse_cmd(line_buf);
                    prev_status = execute_cmd(&list, prev_status, devnull);
                    free_list(&list);
                }
                if(prev_status == STATUS_DIE) { // die was executed
                    return EXIT_FAILURE;
                }
                run = 0;
                break;
            } else if(char_buf == '\n') { // end of cmd
                line_buf[line_ind] = '\0';
                ArrayList list = parse_cmd(line_buf);
                prev_status = execute_cmd(&list, prev_status, devnull);
                free_list(&list);
                if(prev_status == STATUS_DIE) { // die was executed
                    return EXIT_FAILURE;
                } else if(prev_status == STATUS_EXIT) { // exit was executed
                    run = 0;
                }
                break;
            } else { // any other char
                if(line_ind >= MAX_LINE_LEN - 1) {
                    fprintf(stderr, "Error: maximum line length exceeded\n");
                    while(bytes > 0 && char_buf != '\n') {
                        bytes = read(fd, &char_buf, 1);
                    }
                    break;
                } else {
                    line_buf[line_ind] = char_buf;
                    line_ind++;
                }
            }
        }
    }

    if(interactive_mode) {
        printf("Exiting my shell.\n"); // goodbye message if interactive
    }

    // close file if it's not standard input
    if(fd != STDIN_FILENO) {
        close(fd);
    }

    return EXIT_SUCCESS;
}