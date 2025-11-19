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

## Testing Plan
Our test suite focuses on confirming the correctness and robustness of each part of mysh. Our tests check whether mysh produces the correct output, prints out errors, and terminates in appropriate scenarios.

### Description of Testing Files
All of the files we use for testing are in the `tests/` directory.
- `batch.txt` Contains simple commands (echo, pwd) to verify that mysh correctly runs in batch mode and does not print interactive prompts.
- `pipe.txt` Tests pipelines and redirection. Includes a pipeline (echo ... | tr ...) and redirect commands to ensure mysh correctly handles |, <, and >.
- `cond.txt` Tests conditional execution using and and or. The file contains commands that deliberately succeed or fail to confirm that mysh runs the appropriate follow-up commands.
- `which.txt` Tests the which built-in command, including valid paths (e.g., ls), built-in commands (e.g., exit), and nonexistent commands to verify correct error handling.
- `cd.txt` Tests the cd built-in command to check that it fails in the correct cases and changes the current working directory of the parent process.
- `comment.txt` Tests whether tokens in comments are correctly ignored by mysh
- `syntax.txt` Contains commands that include syntax errors to confirm that mysh fails in these cases.

#### Test 1
Requirement: mysh runs in interactive mode when the input comes from the terminal and prints a welcome message, a goodbye message, and a prompt.  
Detection Method: If mysh is run without any arguments, then it should print the welcome message and display the prompt. When exiting normally, it should also print the goodbye message.  
Test: Start mysh in interactive mode by running `./mysh`. Then execute a command like `echo hello` to confirm that it takes input from the terminal. Then terminate the program by running the `exit` command. Check that the welcome, goodbye, and prompt messages appear in the terminal.  

#### Test 2
Requirement: mysh runs in batch mode when it is not receiving input from the terminal.  
Detection Method: When mysh is run with a file argument, the commands in the file should run but no prompt should be printed.  
Test: Run `./mysh tests/batch.txt`, which contains a few commands for mysh to execute. Output contains hello and a valid path from pwd without printing any interactive prompts and exits normally. There should be no welcome or goodbye message.  

#### Test 3
Requirement: Pipelines `|` work by properly arranging the standard input and output for each process, and redirections `>` / `<` work properly by specifying an input or output file.  
Detection Method: When mysh executes a line containing a pipe, the command following the pipe should use the preceding command's output as its input. When mysh executes a line containing a redirect, it should appropriately take input or send output to the corresponding file, depending on which redirect is present.  
Test: Run `./mysh tests/pipe.txt` with the test file pipe.txt. The first line of output should be PIPELINE-TEST, meaning that `tr` used the output of `echo` as its input. The second line should be hello, meaning that mysh correctly set the output file of `echo hello` and set the input file of `cat`. The third line should be the current working directory, meaning that the output of `which ls` was piped into `pwd` (which ignores it). This also shows that built-in commands can be used with pipes and redirects.  

#### Test 4
Requirement: mysh should handle the conditionals `and` and `or` properly.  
Detection Method: When `and` or `or` is the first token, then the execution status of the previous command determines whether the current command is executed or not. Run commands that deliberately fail or succeed, followed by commands starting with `and` or `or`.  
Test: Run `./mysh tests/cond.txt`, which first executes a command that fails, followed by an `or`. This should print ran-or because the previous command failed. Then it executes a command that succeeds, followed by an `and`. ran-and should print because the previous command succeeded. The next `or` should not execute because it follows a successful command, but the last `and` should execute because the second `or` was ignored.  

#### Test 5
Requirement: `which` prints nothing and fails if it is given the wrong number of arguments, or the name of a built-in, or if the program is not found.  
Detection Method: `which` can be run for several cases: with a real path, a built-in command, no arguments, or a nonexistent command. The output can then be compared to the expected output.  
Test: Run `./mysh tests/which.txt`. `which ls` should print a real path like `/bin/ls`, `which exit` should print nothing (built-in), `which fakecommand` should print nothing (command not found), and `which` should also print nothing (wrong number of arguments).  

#### Test 6
Requirement: `pwd` outputs the correct current working directory.  
Detection Method: Running `./mysh` from a known directory and executing only `pwd` should output the expected directory.  
Test: Run `./mysh` from the directory that mysh is located in. Then execute the command `pwd`. The output should be the same directory that mysh is located in.  

#### Test 7
Requirement: `cd` changes the current working directory of the parent process and fails if more than one argument is given or if it is unable to change the directory.  
Detection Method: After mysh executes `cd`, `pwd` can be used to confirm whether the current working directory was changed or not.  
Test: Run `./mysh tests/cd.txt`. The path to the original working directory should be printed, followed by the same path but with `/tests` appended. The last two calls to `cd` should fail due to too many arguments and not finding the directory, respectively.  

#### Test 8
Requirement: `exit` should cause mysh to terminate successfully after executing the job involving `exit`.  
Detection Method: Running the test mentioned in the writeup (a command such as foo | exit) should terminate mysh successfully only after foo finishes executing. `echo $?` can then be used to print the exit status of the last program.  
Test: Run `./mysh` and then enter the command `mkdir tests/before_exit | exit`. The directory `before_exit` should still be made. To confirm that mysh exited successfully, enter `echo $?` in the terminal (not mysh) to check that the most recent exit status was 0, meaning success.  

#### Test 9
Requirement: `die` should cause mysh to terminate with failure and the arguments to die should be printed out, separated by spaces.  
Detection Method: Running `die` with several arguments should print all of those arguments to output. `echo $?` can then be used to check that mysh terminated with failure.  
Test: Run `./mysh` and then execute `die 1 two 3 four`. The arguments `1 two 3 four` should be printed out. Then run `echo $?` in the terminal and check that the output is 1, meaning failure.  

#### Test 10
Requirement: Trying to execute a command that includes a slash `/` should be interpreted by mysh as a path to an executable file.  
Detection Method: `which` can be used to find the path to an executable. Then the executable can be run with and without the path (with and without slashes). If the command with the slash is interpreted as a path and not as a different command, then each execution should give the same output.  
Test: Run `./mysh` and execute `which ls`. Then run a command containing just the output of `which ls`, and run `ls`. These two should output the same result.  

#### Test 11
Requirement: Tokens after comments should not be executed and a command containing only a comment should be ignored.  
Detection Method: If the first token in a line is the character `#`, then mysh should ignore that line and print nothing. If `#` appears in the middle of a line, then commands before the `#` should be executed and any tokens after should be ignored.  
Test: Run `./mysh tests/comment.txt`. The only output should be the word before. The first two commands should be ignored entirely, and `echo before # echo after` should not execute `echo after`.  

#### Test 12
Requirement: Commands containing syntax errors should fail.  
Detection Method: If a command is deliberately run with a syntax error, such as two redirects or pipes in a row with no other token in between, it will print out an error and fail.  
Test: Run `./mysh tests/syntax.txt`. All three commands in the file should fail and print an error, since they each contain a syntax error. After each failed command, mysh should go to the next newline and continue executing commands.  

#### Test 13
Requirement: mysh should always terminate successfully unless it executes the `die` command or it cannot open its argument.  
Detection Method: mysh can be run three times: once where it terminates using `exit`, once where it terminates with `die`, and once where the given argument is invalid. `echo $?` can be used between each time mysh is run to check the exit status of each call.  
Test: Run `./mysh` and enter `exit`. Then run `echo $?`, which should output 0 for success. Run `./mysh` again and enter `die`. Running `echo $?` should output 1 for failure. Run `./mysh fake_file.txt`. Running `echo $?` should still output 1, for failure.  
