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
- `batch.txt` Contains simple commands (echo, pwd) to verify that mysh correctly runs in batch mode and does not print interactive prompts.
- `pipe.txt` Tests pipelines and redirection. Includes a pipeline (echo ... | tr ...) and redirect commands to ensure mysh correctly handles |, <, and >.
- `cond.txt` Tests conditional execution using and and or. The file contains commands that deliberately succeed or fail to confirm that mysh runs the appropriate follow-up commands.
- `which.txt` Tests the which built-in, including valid paths (e.g., ls), built-in commands (e.g., exit), and nonexistent commands to verify correct error handling.

#### Test 1
Requirement: mysh runs in interactive mode when the input comes from the terminal and prints a welcome message, a goodbye message, and a prompt.  
Description: If mysh is run without any arguments, then it should print the welcome message and display the prompt. When exiting normally, it should also print the goodbye message.  
Test: Start mysh in interactive mode by running `./mysh`. Then terminate the program by running the `exit` command. Check that the welcome, goodbye, and prompt messages appear in the terminal.  

#### Test 2
Requirement: mysh runs in batch mode when it's given a file as an argument or when it receives input from a pipe.
Description: When mysh is run with a file argument, the commands in the file should run but no prompt should be printed. 
Test: Run `./mysh tests/batch.txt`. Output contains hello and a valid path from pwd without printing any interactive prompts and exits normally.

#### Test 3
Requirement: basic pipeline `|` and redirection `>` / `<` work.
Description: When running with test file pipe.txt, should see first line prints PIPELINE-TEST and then hello.
Test: Run `./mysh tests/pipe.txt`. Pipeline outputs as above and file created with correct contents.

#### Test 4
Requirement: mysh should handle conditionals as described in the project instructions
Description: Conditional outputs present in correct places according to input file.
Test: Run `./mysh tests/cond.txt`. Should print ran-or printed because previous command failed, and then ran-and printed because previous command succeeded.

#### Test 5
Requirement: `which` prints nothing and fails if it is given the wrong number of arguments, or the name of a built-in,
or if the program is not found
Description: which ls prints a real path like /bin/ls. which exit prints nothing. fakecommand prints an error (command not found) and shell continues.
Test: Run `./mysh tests/which.txt`. Ensure that which returns expected path for system commands and unknown command is gracefully handled.