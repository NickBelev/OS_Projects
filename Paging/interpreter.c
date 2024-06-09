#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>

#include "shellmemory.h"
#include "shell.h"
#include "kernel.h"
#include "ready_queue.h"
#include "interpreter.h"

int MAX_ARGS_SIZE = 7;

char *error_msgs[] = {
    "no error",
    "file does not exist",
    "file could not be loaded",
    "no space left in shell memory",
    "ready queue is full",
    "scheduling policy error",
    "too many tokens",
    "too few tokens",
    "non-alphanumeric token",
    "unknown name",
    "cd",
    "mkdir"};

int handle_error(enum Error error_code)
{
    printf("Bad command: %s\n", error_msgs[error_code]);
    return error_code;
}

int help();
int quit();
int set(const char *var, const char *value);
int print(const char *var);
int run(const char *script);
int echo(const char *var);
int my_ls();
int my_mkdir(const char *dirname);
int my_touch(const char *filename);
int my_cd(const char *dirname);
int exec(const char *fname1, const char *fname2, const char *fname3); //, char* policy, bool background, bool mt);

// Interpret commands and their arguments
// the strings in command_args are malloced and must be freed after function call (if needed)
int interpreter(char *command_args[], int args_size)
{
    if (args_size < 1)
        return handle_error(TOO_FEW_TOKENS);
    if (args_size > MAX_ARGS_SIZE)
        return handle_error(TOO_MANY_TOKENS);

    for (int i = 0; i < args_size; i++)
    { // strip spaces, newline, etc.
        command_args[i][strcspn(command_args[i], "\r\n")] = '\0';
    }

    if (strcmp(command_args[0], "help") == 0)
    { // help
        if (args_size > 1)
            return handle_error(TOO_MANY_TOKENS);
        return help();
    }
    else if (strcmp(command_args[0], "quit") == 0 || strcmp(command_args[0], "exit") == 0)
    { // quit
        if (args_size > 1)
            return handle_error(TOO_MANY_TOKENS);
        // TODO
        return quit();
    }
    else if (strcmp(command_args[0], "set") == 0)
    { // set
        if (args_size < 3)
            return handle_error(TOO_FEW_TOKENS);
        int total_len = 0;
        for (int i = 2; i < args_size; i++)
        {
            total_len += strlen(command_args[i]) + 1;
        }
        char *value = (char *)calloc(1, total_len);
        char spaceChar = ' ';
        for (int i = 2; i < args_size; i++)
        {
            strncat(value, command_args[i], strlen(command_args[i]));
            if (i < args_size - 1)
            {
                strncat(value, &spaceChar, 1);
            }
        }
        int errCode = set(command_args[1], value);
        free(value);
        return errCode;
    }
    else if (strcmp(command_args[0], "print") == 0)
    { // print
        if (args_size < 2)
            return handle_error(TOO_FEW_TOKENS);
        if (args_size > 2)
            return handle_error(TOO_MANY_TOKENS);
        return print(command_args[1]);
    }
    else if (strcmp(command_args[0], "run") == 0)
    { // run
        if (args_size < 2)
            return handle_error(TOO_FEW_TOKENS);
        if (args_size > 2)
            return handle_error(TOO_MANY_TOKENS);
        return run(command_args[1]);
    }
    else if (strcmp(command_args[0], "echo") == 0)
    { // echo
        if (args_size > 2)
            return handle_error(TOO_MANY_TOKENS);
        return echo(command_args[1]);
    }
    else if (strcmp(command_args[0], "my_ls") == 0)
    { // ls
        if (args_size > 1)
            return handle_error(TOO_MANY_TOKENS);
        return my_ls(command_args[0]);
    }
    else if (strcmp(command_args[0], "my_mkdir") == 0)
    {
        if (args_size > 2)
            return handle_error(TOO_MANY_TOKENS);
        return my_mkdir(command_args[1]);
    }
    else if (strcmp(command_args[0], "my_touch") == 0)
    {
        if (args_size > 2)
            return handle_error(TOO_MANY_TOKENS);
        return my_touch(command_args[1]);
    }
    else if (strcmp(command_args[0], "my_cd") == 0)
    {
        if (args_size > 2)
            return handle_error(TOO_MANY_TOKENS);
        return my_cd(command_args[1]);
    }
    else if (strcmp(command_args[0], "exec") == 0)
    {
        if (args_size <= 1)
            return handle_error(TOO_FEW_TOKENS);
        if (args_size > 5)
            return handle_error(TOO_MANY_TOKENS);
        if (args_size == 2)
            return exec(command_args[1], NULL, NULL);
        else if (args_size == 3)
            return exec(command_args[1], command_args[2], NULL);
        else if (args_size == 4)
            return exec(command_args[1], command_args[2], command_args[3]);
    }
    else if (strcmp(command_args[0], "resetmem") == 0)
    {
        if (args_size != 1)
            return handle_error(TOO_MANY_TOKENS);
        
        varmemory_free();
        varmemory_init();
        return 0;
    }

    return handle_error(BAD_COMMAND);
}

int help()
{

    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
run SCRIPT.TXT		Executes the file SCRIPT.TXT\n ";
    printf("%s\n", help_string);
    return 0;
}

int quit()
{
    printf("%s\n", "Bye!");
    ready_queue_destory();
    system("rm -r ./backing");
    exit(0);
}

int set(const char *var, const char *value)
{
    char *link = "=";
    char buffer[1000];
    strcpy(buffer, var);
    strcat(buffer, link);
    strcat(buffer, value);
    mem_set_value(var, value);
    return 0;
}

int print(const char *var)
{
    char *value = mem_get_value(var);
    if (value == NULL)
    {
        // value = "\n";
        printf("\n");
        return 0;
    }
    else
    {
        printf("%s\n", value);
        free(value);
    }
    return 0;
}

int echo(const char *var)
{
    if (var[0] == '$')
        print(&var[1]);
    else
        printf("%s\n", var);
    return 0;
}

int my_ls()
{
    int errCode = system("ls | sort");
    return errCode;
}

// there is a possible memory leak here with mem_get_value call
int my_mkdir(const char *dirname)
{
    const char *dir = dirname;
    if (dirname[0] == '$')
    {
        char *value = mem_get_value(&dirname[1]);
        if (value == NULL || strchr(value, ' ') != NULL)
        {
            return handle_error(ERROR_MKDIR);
        }
        dir = value;
    }
    int namelen = strlen(dir);
    char *command = (char *)calloc(1, 7 + namelen);
    strncat(command, "mkdir ", 7);
    strncat(command, dir, namelen);
    int errCode = system(command);
    free(command);
    return errCode;
}

int my_touch(const char *filename)
{
    int namelen = strlen(filename);
    char *command = (char *)calloc(1, 7 + namelen);
    strncat(command, "touch ", 7);
    strncat(command, filename, namelen);
    int errCode = system(command);
    free(command);
    return errCode;
}

int my_cd(const char *dirname)
{
    struct stat info;
    if (stat(dirname, &info) == 0 && S_ISDIR(info.st_mode))
    {
        // the object with dirname must exist and is a directory
        int errCode = chdir(dirname);
        return errCode;
    }
    return handle_error(ERROR_CD);
}

int run(const char *script)
{
    // errCode 11: bad command file does not exist
    int errCode = 0;

    // load script into shell
    errCode = process_initialize(script);
    if (errCode == 11)
    {
        return handle_error(errCode);
    }

    // run with FCFS
    //  schedule_by_policy("FCFS"); //, false);
    schedule_by_policy("RR");

    // clears pages
    free_framestore();
    return errCode;
}

int exec(const char *fname1, const char *fname2, const char *fname3)
{
    int error_code = 0;
    if (fname1 != NULL)
    {
        error_code = process_initialize(fname1);
        if (error_code != 0)
        {
            return handle_error(error_code);
        }
    }
    if (fname2 != NULL)
    {
        error_code = process_initialize(fname2);
        if (error_code != 0)
        {
            return handle_error(error_code);
        }
    }
    if (fname3 != NULL)
    {
        error_code = process_initialize(fname3);
        if (error_code != 0)
        {
            return handle_error(error_code);
        }
    }
    error_code = schedule_by_policy("RR");
    if (error_code != 0)
    {
        return handle_error(error_code);
    }

    // clears pages
    free_framestore();

    return error_code;
}
