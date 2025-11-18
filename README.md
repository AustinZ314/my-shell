Austin Zhang asz43
Amanda Lei al1458

## Implementation Notes:


## Test Plan
Our test suite verifies the correctness of our implementation of mysh, confirming that it meets all the requirements that are included in the project writeup.

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

