#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

#define MAX_LINE_LEN = 2048 // POSIX max line length

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
    // but im only calling this function at the end of the line basically right? so its not necessary?
}

// parse through line to find tokens and then call function to execute command
// return exit status to keep track of most recent status
int parse_cmd(char *cmd, int prev_status) {
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
            p++;
        }

        // separate current token from next token if not at end of line
        if(*ptr != '\0') {
            *ptr = '\0';
            p++;
        }
    }

    // end token list with null terminator for execv()
    list_append(&list, NULL);

    // no cmd to execute, don't change prev_status
    if(list.size <= 1) {
        free_list(&list);
        return prev_status;
    }

    // JUST FOR DEBUG
    printf("Tokens found (%d): \n", list.size - 1);
    for (int i = 0; i < list.size - 1; i++) {
        printf("  token[%d]: '%s'\n", i, list.tokens[i]);
    }
    printf("----\n");

    int curr_status = 0;
    // add logic to execute the cmd

    free_list(&list);
    return new_status;
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

    char line_buf[MAX_LINE_LEN];
    int line_ind = 0;
    char char_buf;
    int run = 1;

    // input loop
    while(run) {
        if(interactive_mode) {
            printf("mysh> ");
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
                    prev_status = parse_cmd(line_buf);
                }
                run = 0;
                break;
            } else if(char_buf == '\n') { // end of cmd
                line_buf[line_ind] = '\0';
                prev_status = parse_cmd(line_buf);
                break;
            } else { // any other char
                if(line_ind >= buf_len - 1) {
                    // line length exceeded, do something (error or just go to next line)
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