#include "Shell.h"

int main()
{

    signal(SIGCHLD, sigchld_handler);
    signal(SIGTSTP, sigtstp_handler);

    char* input_string = NULL; // Create an array to store user input. +2 for \0 and \n
    int enter_counter = 0;
    int add_variable_flag; // 1 if adding variable, 0 if executing command
    char working_directory[MAX_WORKING_DIRECTORY_CHARACTERS + 1];
    char* sentence; // Holding the *untouched* sentence
    char* trimmed_sentence; // Holding the *replaced* sentence (i.e. replaced $var in its value)
    unsigned long current_command_index;

    input_string = (char*) malloc(MAX_CHARACTERS + 2);
    if (input_string == NULL)
        exit_program("ERR");

    while (enter_counter < ENTER_PRESSED)
    {
        if (getcwd(working_directory, MAX_WORKING_DIRECTORY_CHARACTERS) == NULL)
        {
            perror("ERR");
            exit(EXIT_FAILURE);
        }


        // Getting a valid input
        while (1)
        {
            printf("#cmd:%d|#args:%d@%s>", number_of_commands, number_of_args, working_directory); // Print the shell prompt
            fgets(input_string, MAX_CHARACTERS + 2, stdin);

            // If entered more than MAX_ARGUMENTS, clear the input buffer
            if (input_string[strlen(input_string) - 1] != '\n')
            {
                while (getchar() != '\n'); // Clear buffer
                printf("ERR\n");
            }
            else
                break;
        }

        if (input_string[0] == '\n' && input_string[1] == '\0') // If user pressed enter
        {
            enter_counter++;
            continue;
        }

        enter_counter = 0;
        current_command_index = 0;
        sentence = trim_sentence(';', input_string); // Get first sentence until the semicolon

        while (sentence != NULL)
        {
            if (strchr(sentence, '\n') != NULL)
                sentence[strcspn(sentence,
                                 "\n")] = '\0'; // strcspn returns the first index of '\n' and then sentence[location] replaces it with a null terminator.

            trimmed_sentence = (char*) malloc(
                    strlen(sentence) + 1); // Holding the seperated split sentence between the semicolon

            if (trimmed_sentence == NULL)
                exit_program("ERR");

            strcpy(trimmed_sentence, sentence);

            add_variable_flag = 0;


            if (strchr(trimmed_sentence, '=') != NULL) // If user wants to save a variable
            {
                // check if the '=' is after an echo command

                char* echo_pointer = strstr(trimmed_sentence, "echo"); // Pointer to "echo"

                // If the pointer is located before the equal mark (meaning user wants to print and not to save the variable)
                if (echo_pointer == NULL || echo_pointer > strchr(trimmed_sentence, '='))
                {
                    add_variable(trimmed_sentence);
                    add_variable_flag = 1;
                }
            }

            // If variable was not added
            if (add_variable_flag == 0)
            {
                if (strchr(trimmed_sentence, '$') != NULL) // If user wants to use a variable
                {
                    int count = count_dollars(trimmed_sentence);
                    for (int i = 0; i < count; i++) // Replace all the variables in the input
                        replace_variable(&trimmed_sentence);
                }

                execute_command(trimmed_sentence);
            }

            free(trimmed_sentence);

            // Get the next command
            current_command_index += (strlen(sentence) + 1);
            free(sentence);
            sentence = trim_sentence(';', input_string + current_command_index);
        }
    }


    free_variables_memory();
    free(input_string);
    free(sentence);
    return 0;
}


/*
 Function counts how many pipes in a given string
 @param str - string to count the pipes
 @return the amount of pipes in the given string
*/
int get_pipes_amount(char* str)
{
    int count = 0;
    int is_quote_marks = 0;
    unsigned long length = strlen(str);
    for (int i = 0; str[i] != '\0' && i < length; i++)
    {
        if(str[i] == '"')
            is_quote_marks = !is_quote_marks;

        if (!is_quote_marks && str[i] == '|')
            count++;
    }

    return count;
}

void sigtstp_handler(int sig)
{
    last_suspended_pid = pid;
    kill (pid, SIGSTOP);
}

void sigchld_handler(int sig)
{
    // WNOHANG flag to make waitpid() unblocking
    waitpid(-1, NULL, WNOHANG);
}

/*
 Function to copy the given string from the beginning and until the given char.
 **[NOTICE] The function does not stop if the given char is inside quote marks.**
 **[NOTICE] - Function is using a dynamic memory allocation and user must free the returned variable when done - unless a NULL is returned**
 @param c - required char to stop at
 @param str - string to copy from
 @return the trimmed string.
*/
char* trim_sentence(char c, const char* str)
{
    if (str == NULL || str[0] == '\0')
        return NULL;

    int is_inside_quotes = 0;
    int i = 0;

    // Count str length until the first semicolon which is not inside quote marks
    for (; str[i] != '\0' && i < strlen(str); i++)
    {
        if (str[i] == '"')
            is_inside_quotes = !is_inside_quotes;

        if (is_inside_quotes == 0 && str[i] == c)
            break;
    }

    char* trimmed_sentence = (char*) malloc(i + 2);

    if (trimmed_sentence == NULL)
    {
        free_variables_memory();
        exit_program("ERR");
    }

    int j = 0;
    for (; j < i && str[j] != '\0'; j++)
        trimmed_sentence[j] = str[j];

    trimmed_sentence[j] = '\0';

    for (i = 0; i < j && str[i] != '\0'; i++)
        if (!isspace(str[i]))
            return trimmed_sentence;

    free(trimmed_sentence);
    return NULL;
}

// Function to call when malloc fails. Easier to maintain in case of changes in the print for example.
void exit_program(char* str)
{
    printf("%s\n", str);
    free_variables_memory();
    exit(EXIT_FAILURE);
}

/*
 Function to create a new variable node
 @param name - variable name
 @param value - variable value
 @return a pointer to the created variable
*/
variable* create_new_variable_node(char* name, char* value)
{
    variable* new_var = (variable*) malloc(sizeof(variable));

    if (!new_var)
        exit_program("ERR");

    new_var->name = (char*) malloc(strlen(name) + 1);
    new_var->value = (char*) malloc(strlen(value) + 1);
    new_var->next = NULL;

    if (!new_var->name || !new_var->value)
    {
        free(new_var->name);
        free(new_var->value);
        exit_program("ERR");
    }

    strcpy(new_var->name, name);
    strcpy(new_var->value, value);

    return new_var;
}


/*
 Function to free all the saved variables_list
*/
void free_variables_memory()
{
    variable* current = variables;
    variable* next;

    while (current != NULL)
    {
        next = current->next;
        free(current->name);
        free(current->value);
        free(current);
        current = next;
    }

    variables = NULL;
}


/*
 Function to count all arguments in a given string
 @param str - string to count the arguments
 @param regular_arguments - a variable to put the amount of counted arguments
 @param inside_quote_arguments - a variable to put the amount of counted inside-quote-marks arguments
 e.g. omri naor "live in jerusalem" - regular_arguments = 5, inside_quote_arguments = 3
*/
void count_arguments(const char* str, int* regular_arguments, int* inside_quote_arguments)
{
    int sum_arguments = 0; // To hold the total amount of arguments
    int sum_arguments_in_quote = 0; // To hold the total amount of arguments located inside quote marks
    int i = 0;
    int quotes_flag = 1; // 1 when need to count the next encountered word, else 0 (to avoid counting the same word multiple times)
    int arguments_flag = 1;
    int is_inside_quote = 0;

    while (str != NULL && str[i] != '\0')
    {
        // If found a quote mark
        if (str[i] == '"')
        {
            is_inside_quote = !is_inside_quote; // Change when we enter and leave the quote mark
            i++;
            continue;
        }

        // Finding an empty char means we finished to count the last word and are proceeding to find the next one that needs to be counted
        if (isspace(str[i]))
        {
            quotes_flag = 1;
            arguments_flag = 1;
        }

        // Found a new argument, regardless if inside quotes marks
        if (!isspace(str[i]) && arguments_flag == 1)
        {
            sum_arguments++;
            arguments_flag = 0;
        }

        // Found the next word inside the quote mark, count it and change quotes_flag to 0 to avoid counting the same word multiple times
        if (!isspace(str[i]) && quotes_flag == 1 && is_inside_quote)
        {
            sum_arguments_in_quote++;
            quotes_flag = 0;
        }

        i++;
    }

    *regular_arguments = sum_arguments;
    *inside_quote_arguments = sum_arguments_in_quote;
}


/*
 Function receives an array and an echo command, parses the command into arguments, puts the arguments inside the array, and checks all its different edge cases.
 **[NOTICE] - Function is using a dynamic memory allocation and user must free args[1] when done.**
 @param echo_ptr - pointer to the echo command and its arguments
 @param args - an array to parse the arguments into
 @return the amount of echo arguments
*/
int handle_echo_command(char* echo_ptr, char** args)
{
    char trimmed_arguments[strlen(echo_ptr) + 1];
    int inside_quote_arguments = 0; // To hold the amount of arguments inside quote marks
    int quote_marks_amount = 0; // To hold the total amount of quote marks inside the sentence
    int inside_quote_flag = 0;
    int echo_arguments_count = 0;

    count_arguments(echo_ptr, &echo_arguments_count, &inside_quote_arguments);
    echo_ptr += 4; // Skip the command string
    args[0] = "echo";

    // Skip leading white space
    while (*echo_ptr == ' ' || *echo_ptr == '\t' || *echo_ptr == '\n')
        echo_ptr++;

    // Remove the quote marks from the sentence and count the amount
    int i, j = 0;
    int last_char_space = 0;
    for (i = 0; echo_ptr[i] != '\0'; i++)
    {
        if (echo_ptr[i] == '"')
        {
            quote_marks_amount++;
            inside_quote_flag = !inside_quote_flag;
            continue;
        }

        if (!inside_quote_flag && (echo_ptr[i] == ' ' || echo_ptr[i] == '\t' || echo_ptr[i] == '\n'))
        {
            if (!last_char_space && echo_ptr[i+1] != '\0' && echo_ptr[i+1] != ' ')
            {
                trimmed_arguments[j++] = ' ';
                last_char_space = 1;
            }
        }
        else
        {
            trimmed_arguments[j++] = echo_ptr[i];
            last_char_space = 0;
        }
    }

    trimmed_arguments[j] = '\0';

    // Final arguments amount. Consider multiple arguments inside quote marks as a single argument
    echo_arguments_count = echo_arguments_count - abs(quote_marks_amount / 2 - inside_quote_arguments);

    args[1] = malloc(strlen(trimmed_arguments) + 1);
    if (args[1] == NULL)
        exit_program("ERR");

    strcpy(args[1], trimmed_arguments);
    args[2] = NULL;

    return echo_arguments_count;
}

/*
 Function to check if the given command is legal and may be executed
 @param args - array with a stored command and its arguments
 @param arguments_count - number of arguments for non-echo commands
 @param echo_arguments_count - number of arguments for echo commands
 @return 1 if legal, else return 0
*/
int is_legal_command(char** args, int arguments_count, int echo_arguments_count)
{
    // Entered space only
    if (args[0] == NULL)
        return 0;

    // .cd command is not allowed!
    if (strcmp(args[0], ".cd") == 0)
    {
        printf("cd not supported\n");
        return 0;
    }

    // More than MAX_ARGUMENTS
    if ((arguments_count > MAX_ARGUMENTS && (strcmp(args[0], "echo") != 0)) ||
        (echo_arguments_count > MAX_ARGUMENTS && (strcmp(args[0], "echo")) == 0))
    {
        printf("ERR\n");
        return 0;
    }

    return 1;
}


/*
 Function to execute shell commands.
 Stores the amount of used commands and arguments (legal commands only) in the global variables.
 @param command - command to be executed
*/
void execute_command(char* command)
{
    if (command == NULL || strcmp(command, " ") == 0 || strcmp(command, "") == 0)
        return;

    int pipes_amount = get_pipes_amount(command);
    char* args[strlen(command) + 1];
    int used_pipes = 0;

    // Save the original standard input file descriptor
    int original_stdin_fd = dup(STDIN_FILENO);

    while (used_pipes <= pipes_amount)
    {

        char* trimmed_command = trim_sentence('|', command);
        command = command + strlen(trimmed_command);

        if(*command == '|')
            command++;

        int arguments_count = 0; // To hold the amount of used arguments
        int echo_arguments_count = 0; // To hold the amount of used arguments for echo command
        int is_redirection = 0; // 1 when want to redirect the command's output into a file
        char* file_name = NULL;
        char* redirection_position = strchr(trimmed_command, '>');
        char* is_background_process = strchr(trimmed_command, '&');
        char* echo_ptr = strstr(trimmed_command, "echo");

        if (is_background_process != NULL)
            *is_background_process = '\0';

        if (redirection_position != NULL)
        {
            is_redirection = 1;
            file_name = remove_spaces(redirection_position + 1);
            *redirection_position = '\0';  // Remove the '>' from the command
        }

        // If the command is echo
        if (echo_ptr != NULL)
        {
            echo_arguments_count = handle_echo_command(echo_ptr, args);

            if (is_background_process != NULL)
                echo_arguments_count++;

            if (is_redirection)
                echo_arguments_count += 2;
        }

            // Any command except echo
        else
        {
            arguments_count = parse_arguments(trimmed_command, args);

            if (is_background_process != NULL)
                arguments_count++;

            if (is_redirection)
                arguments_count += 2;
        }


        // If the command is not legal (such as .cd or more than MAX_ARGUMENTS)
        if (!is_legal_command(args, arguments_count, echo_arguments_count))
        {
            free(trimmed_command);
            return;
        }

        if (strcmp(args[0], "bg") == 0)
        {
            if (last_suspended_pid != -1)
            {
                // Send the SIGCONT signal to the last suspended process
                kill(last_suspended_pid, SIGCONT);

                // Reset the last_suspended_pid variable
                last_suspended_pid = -1;
            }
            else
                printf("No suspended process\n");

            free(trimmed_command);
            number_of_args++;
            number_of_commands++;

            // Reset the standard input file descriptor to its original value
            dup2(original_stdin_fd, STDIN_FILENO);
            // Close the original stdin file descriptor, as it's no longer needed
            close(original_stdin_fd);
            return;
        }


        int pipe_fd[2];

        // Open pipe and check if failed
        if (pipe(pipe_fd) == -1)
        {
            perror("ERR");

            if (echo_ptr != NULL)
                free(args[1]);

            free(trimmed_command);
            free_variables_memory();
            exit(EXIT_FAILURE);
        }

        // Create a child process to execute the command
        pid = fork();

        // Fork failed
        if (pid < 0)
        {
            free_variables_memory();
            free(trimmed_command);
            if (echo_ptr != NULL)
                free(args[1]);

            perror("ERR");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) // Child process
        {
            signal(SIGTSTP, SIG_DFL);

            if (is_redirection)
            {
                // Open file to read and write and give read-write permissions to everyone (clear the file if already exists)
                int file_fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);

                // If failed to open the file
                if (file_fd < 0)
                {
                    perror("ERR");
                    free(trimmed_command);
                    // Reset the standard input file descriptor to its original value
                    dup2(original_stdin_fd, STDIN_FILENO);
                    // Close the original stdin file descriptor, as it's no longer needed
                    close(original_stdin_fd);
                    return;
                }

                // Replace child's STDOUT_FILENO with file_fd
                dup2(file_fd, STDOUT_FILENO);
                close(file_fd);
            }

                // If we haven't reached the last pipe -> send output inside the pipe
            else if (used_pipes < pipes_amount)
            {
                close(pipe_fd[0]);  // Close reading side
                dup2(pipe_fd[1], STDOUT_FILENO); // Redirect writing to pipe
            }

            execvp(args[0], args);

            if (echo_ptr != NULL)
                free(args[1]);

            free_variables_memory();
            free(trimmed_command);
            perror("ERR");
            exit(EXIT_FAILURE);
        }
        else // Parent process
        {
            int status = 0;

            if (is_background_process != NULL)
                kill(pid, SIGTSTP);


            else
                waitpid(pid, &status, WUNTRACED);


            // Close the unused write side of the pipe in the parent
            close(pipe_fd[1]);

            dup2(pipe_fd[0], STDIN_FILENO);  // Redirect reading from pipe
            close(pipe_fd[0]); // Close the original read side of the pipe, as it's no longer needed

            used_pipes++;

            if (echo_ptr != NULL)
                free(args[1]);

            // Child process failed, so return
            if (!WIFEXITED(status) || WEXITSTATUS(status) == EXIT_FAILURE)
            {
                free(trimmed_command);
                // Reset the standard input file descriptor to its original value
                dup2(original_stdin_fd, STDIN_FILENO);
                // Close the original stdin file descriptor, as it's no longer needed
                close(original_stdin_fd);
                return;
            }

            number_of_commands++; // Add to the total used valid commands

            // Add to the total used valid arguments
            if (echo_ptr != NULL)
                number_of_args += echo_arguments_count;
            else
                number_of_args += arguments_count;

            free(trimmed_command);
        }

    }

    // Reset the standard input file descriptor to its original value
    dup2(original_stdin_fd, STDIN_FILENO);

    // Close the original stdin file descriptor, as it's no longer needed
    close(original_stdin_fd);
}


/*
 Function to parse command arguments into a given array
 @param command - required command + its arguments to parse into the array
 @param args - array to store the command and the arguments
 @return the amount of arguments
*/
int parse_arguments(char* command, char* args[])
{
    int arguments_count = 0;
    // Parse the command into tokens separated by space
    while (command != NULL && *command != '\0')
    {
        // Skip leading white space
        while (*command == ' ' || *command == '\t' || *command == '\n')
            *command++ = '\0';

        // Reached the end
        if (*command == '\0')
            break;

        args[arguments_count] = command; // Store the command/arguments in args array
        arguments_count++; // Increment the arguments count

        // Find the end of the argument
        while (*command != '\0' && *command != ' ' && *command != '\t' && *command != '\n')
            command++;
    }

    args[arguments_count] = NULL;

    return arguments_count;
}


/*
 Function to store a variable and its value. If the variable already exists, function will update its value to the given one.
 @param details - string in the format of <Variable_Name>=<Variable_Value>
 @return 1 if added, 0 if updated and -1 if failed
*/
int add_variable(char* details)
{

    char* name = remove_spaces(details); // Get the whole string and remove the spaces
    char* equal_sign = strchr(name, '='); // Find the index of '=' in the string

    if (equal_sign == NULL) // '=' not found in the string
    {
        printf("Invalid input format. Please enter variable name and value separated by '='.\n");
        free(name);
        return -1;
    }

    *equal_sign = '\0'; // Replace '=' with null terminator to separate name and value
    char* value = remove_spaces(equal_sign + 1); // Get the value part of the string and remove the spaces

    if (update_variable(name, value) == 1) // Variable is already existing and updated with the new value
        return 0;


    // Create a new variable and add it to the list
    variable* new_var = create_new_variable_node(name, value);
    new_var->next = variables;
    variables = new_var;

    return 1;
}

/*
 Function to get a given variable's value
 @param name - the variable name
 @return the given variable's value. Space if the variable doesn't exist.
*/
char* get_variable_value(char* name)
{
    variable* temp = variables;
    while (temp != NULL)
    {
        if (strcmp(temp->name, name) == 0)
            return temp->value;

        temp = temp->next;
    }

    return " ";
}

/*
 Function to remove any leading and trailing spaces from a string
 @param str - string to remove the leading and trailing spaces from
 @return the same given string but without leading and trailing spaces
*/
char* remove_spaces(char* str)
{
    if (str == NULL)
        return NULL;

    int i, j = 0;
    unsigned long str_length = strlen(str);

    // Remove the leading spaces
    for (i = 0; i < str_length; i++)
        if (str[i] != ' ')
            break;  // Stop when non-space character is encountered

    // Copy remaining characters to the beginning of the string
    for (; i < str_length && str[i] != ' ' && str[i] != '\n'; i++)
        str[j++] = str[i];

    str[j] = '\0';  // Add null character at the end

    return str;
}

/*
 ** NOTICE: Function is using malloc and returns the allocated variable. User must free the variable when done (!) **
 Function to extract a variable name (starting with $) from a given string.
 @param str - string that contains a variable to extract
 @return string of the variable name. NULL if error occurred (string doesn't contain a $)
*/
char* extract_variable_name(const char* str)
{
    char* pos = strchr(str, '$'); // find position of $
    char* name = NULL;

    if (pos == NULL) // No '$' char located
        return NULL;

    int var_name_length = 1;
    while (pos[var_name_length] != ' ' && pos[var_name_length] != '\0') // Extract variable name's length
        var_name_length++;

    name = (char*) malloc(var_name_length);

    if (!name)
        exit_program("ERR");

    strncpy(name, pos + 1,
            var_name_length - 1); // Copy the variable name (starting at $+1 and ending after var_name_length)
    name[var_name_length - 1] = '\0';

    return name;
}

/*
 Function to replace a given variable, starting with $, with its respective value
 @param str - string that contains a variable to replace
*/
void replace_variable(char** str)
{
    char* name = extract_variable_name(*str);
    if (name == NULL) // Variable not found in the given string
        return;

    name[strcspn(name,
                 "\"")] = '\0'; // strcspn returns the first index of '\"' and then name[location] replaces it with a null terminator.

    char* value = get_variable_value(name);

    unsigned long value_length = strlen(value);
    unsigned long name_length = strlen(name);
    unsigned long original_str_length = strlen(*str);
    unsigned long new_size = original_str_length - name_length + value_length;


    char* replaced_str = realloc(*str, new_size + name_length);

    if (replaced_str == NULL)
        exit_program("ERR");

    unsigned long i = 0;
    // Get the location of the first $ char
    for (; replaced_str[i] != '$'; i++);

    // Shifts the string right to clear space for the new value
    memmove(replaced_str + i + value_length, replaced_str + i + name_length + 1, original_str_length - i - name_length);

    // Copy the new value into the string
    memcpy(replaced_str + i, value, value_length);

    replaced_str[new_size - 1] = '\0';
    *str = replaced_str;

    free(name);
}

/*
 Function updates a saved variable with a new required value
 @param name - name of the variable to update
 @param value - value of the variable to update
 @return 1 if variable found and updated, else return 0
*/
int update_variable(const char* name, const char* value)
{
    variable* temp = variables;

    while (temp != NULL && temp->name != NULL && temp->value != NULL)
    {
        if (strcmp(temp->name, name) == 0) // Variable found
        {
            // Reallocate memory for the new value
            char* new_value = (char*) realloc(temp->value, strlen(value) + 1);

            if (new_value == NULL)
                exit_program("ERR");


            // Update variable value and pointer
            strcpy(new_value, value);
            temp->value = new_value;

            return 1;
        }

        temp = temp->next;
    }

    return 0; // Variable not found
}

/*
 Function checks and counts how many the char "$" appears in the given strings
 @param str - string to check
 @return the amount of "$" in the given string
*/
int count_dollars(const char* str)
{
    int count = 0;
    unsigned long length = strlen(str);
    for (int i = 0; i < length; i++)
        if (str[i] == '$')
            count++;

    return count;
}