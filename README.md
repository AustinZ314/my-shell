Austin Zhang asz43
Amanda Lei al1458

## Implementation Notes:
- The nested loops in `main()` read in one byte at a time using `read()` and pass input to `parse_cmd()` one line at a time
- `parse_cmd()` takes the line and separates it into tokens
- `execute_cmd()` goes through the tokens to process pipes, redirects, and commands
- An ArrayList struct is used to store the tokens/arguments and resizes to handle arbitrary amounts of tokens
- Commands (including built-in commands, except for `cd` only when it's executed as an individual command not in a pipeline) are run in a child process using `fork()` so that they can participate in redirects and pipes
- Commands with `exit`, `die`, and `cd` are flagged so that their behavior is also executed in the parent process (executing them only in a child process wouldn't affect the actual shell)
- `mysh` has an interactive mode and a batch mode, determined through the use of isatty() based on the source of input
- `execv()` is used to execute programs since it can take in a variable number of arguments and it works well with the ArrayList

### Description of Testing Files
All of the files we use for testing are in the `tests/` directory.
- 

#### Test 1
Requirement: mysh runs in interactive mode when the input comes from the terminal and prints a welcome message, a goodbye message, and a prompt.  
Detection Method: If mysh is run without any arguments, then it should print the welcome message and display the prompt. When exiting normally, it should also print the goodbye message.  
Test: Start mysh in interactive mode by running `./mysh`. Then terminate the program by running the `exit` command. Check that the welcome, goodbye, and prompt messages appear in the terminal.  

#### Test 2
Requirement: mysh runs in batch mode when it's given a file as an argument or when it receives input from a pipe.
Detection Method: When mysh is run with a file argument, the commands in the file should run but no prompt should be printed. 
Description: Run mysh with a file argument that contains a few valid commands. 
Test:  

#### Test 3
Requirement:
Detection Method:
Description:
Test:  

