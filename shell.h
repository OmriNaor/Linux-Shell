#ifndef LINUX_SHELL_SHELL_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_CHARACTERS 510
#define MAX_ARGUMENTS 10
#define ENTER_PRESSED 3
#define MAX_WORKING_DIRECTORY_CHARACTERS 510

typedef struct variable variable;

struct variable
{
    char* name;
    char* value;
    variable* next;
};

void execute_command(char*);

int add_variable(char*);

variable* create_new_variable_node(char*, char*);

char* get_variable_value(char*);

char* extract_variable_name(const char*);

int update_variable(const char*, const char*);

void replace_variable(char**);

char* remove_spaces(char*);

int count_dollars(const char*);

void free_variables_memory();

void count_arguments(const char*, int*, int*);

void exit_program(char*);

int handle_echo_command(char*, char**);

int is_legal_command(char**, int, int);

char* trim_sentence(char, const char*);

void sigchld_handler(int);

void sigtstp_handler(int);

int parse_arguments(char*, char* []);

int get_pipes_amount(char*);

pid_t last_suspended_pid = -1;
pid_t pid;
int number_of_commands = 0; // Total number of executed commands
int number_of_args = 0; // Total number of executed arguments
variable* variables = NULL;

#define LINUX_SHELL_SHELL_H

#endif
