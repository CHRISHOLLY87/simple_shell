# simple shell
C program implementing a shell interface that accepts user commands and executes each command in a separate process. The shell program provides a command prompt, where the user inputs a line of command. The shell is responsible for executing the command.

## The program accepts the following commands
- ls: list files in the current directory
- cd: change directory
- cat: open file, as well as redirect file
- cp: copy file, as well as redirect file
- fg: foreground a background job
- jobs: list background jobs
- exit: quit the shell

## SIGNALS
- Ctrl<D>: terminate shell
- Ctrl<C> (SIGINT): kill a program running inside the shell
- Ctrl<Z> (SIGTSTP): ignore the signal