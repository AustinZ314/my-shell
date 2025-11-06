#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#define BASE_LINE_LEN = 128

void execute(char *line) {

}

int main(int argc, char **argv) {
    int fd;
    int interactive_mode;
    
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

    int buf_len = BASE_LINE_LEN;
    char line_buf[buf_len];
    int line_ind = 0;
    char c;
    int run = 1;

    // input loop
    while(run) {
        if(interactive_mode) {
            printf("mysh> ");
        }

        // read one character at a time
        while(1) {
            int bytes = read(fd, &c, 1);

            if(bytes == 0) { // EOF, exit shell
                if(line_ind > 0) {
                    line_buf[line_ind] = '\0';
                    execute(line_buf);
                }
                run = 0;
                break;
            } else if(bytes == -1) {
                perror("read");
                return EXIT_FAILURE;
            } else if(c == '\n') { // end of cmd
                line_buf[line_ind] = '\0';
                execute(line_buf);
                break;
            } else { // any other char
                if(line_ind >= buf_len - 1) {
                    // increase buffer size
                } else {
                    line_buf[line_ind] = c;
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