# C Project: Mini Linux Shell

## About

A typical Linux Shell created from scratch. This shell supports many commands such as:

- Semicolon split: user may split different commands in a single input by using a semicolon character. The shell will split and execute each command seperated by a semicolon.

- Typical commands such as echo, sleep, cat, expr, bg and so on.

- Saving variables by using the <var_name>=<var_value> template.

- Redirecting output into files by using the <output> > <file_name> template.

- Redirecting one command's output to another command's input using pipes |

- Using CTRL + Z to suspend the running process.

- Using a '&' character to run a command in the background.


## Structure

- `main.c` - Main function definition. Manages the user's input and edits the input according to the required command.

## The Algorithm

- The program starts by displaying the shell prompt. The prompt includes the number of legal commands and arguments that have been used so far.

- The algorithm checks the given input and handles it accordingly:

  - Save variables 
  - Execute commands
  - Make a process run in the background
  - Suspend a process
  - Redirect output into a pipe or file

Lastly, the algorithm counts and displays the new number of legal commands and arguments that have been used.

- The main function uses a while loop and constantly asks the user for his input. To exit the program, the user needs to press enter three times in row (empty input).


## Remarks:

- Arguments for Echo command written inside quote marks will be treated as a single argument and printed in the exactly the same way. This also means any character written inside quote marks as a part of the echo command will NOT be treated as a special character and will be printed to the screen. 
- User may not use more than ten arguments for each command (the command is also counted as an argument).
- cd command is not supported and may not be used.

## Examples


![Example](https://user-images.githubusercontent.com/106623821/235354286-924912b4-0c5b-45d0-b8d3-764a976fb7b7.png)





