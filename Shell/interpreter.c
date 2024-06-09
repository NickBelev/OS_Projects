#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <sys/stat.h> // these could be useful?
#include "shellmemory.h"
#include "shell.h"

int MAX_ARGS_SIZE = 7;

int badcommand(){
	printf("%s\n", "Unknown Command");
	return 1;
}

/**
 * DESCRIPTION:
 * Helper command for printing nicer, more precise error messages
*/
int badcommand_custom(const char *s){
	printf("%s%s\n", "Bad command: ", s);
	return 1;
}

/**
 * DESCRIPTION:
 * Helper command for printing error message
*/
int error_message(const char *s) {
    printf("%s\n", s);
    return 1;
}

// For run command only
int badcommandFileDoesNotExist(){
	printf("%s\n", "Bad command: File not found");
	return 3;
}


/* Forward declarations */
int help();
int quit();
int set(char* var, char* value);
int print(char* var);
int run(char* script);
int badcommandFileDoesNotExist();
int set_template_string(char *input, char *command_args[], int args_size);
int if_statement(char* command_args[], int args_size);
int for_statement(char* command_args[], int args_size);
int is_alphanumeric(const char *str);
int is_numeric(const char *str);

// This is default size for all of our char arrays
// this is way more than we need, to be sure, we did 
#define DEFAULT_BUFFER_SIZE 1000

// Interpret commands and their arguments
int interpreter(char* command_args[], int args_size){

	int i;

    if(args_size == 0) return badcommand();
	if (args_size < 1 || args_size > MAX_ARGS_SIZE) {
        if(
            strcmp(command_args[0], "set") != 0 && 
            strcmp(command_args[0], "if") != 0 &&
            strcmp(command_args[0], "for") != 0) {
    		return badcommand();
        }
	}


	for ( i=0; i<args_size; i++){ //strip spaces new line etc
		command_args[i][strcspn(command_args[i], "\r\n")] = 0;
	}

	if (strcmp(command_args[0], "help")==0){
	    //help
	    if (args_size != 1) return badcommand();
	    return help();
	
	} else if (strcmp(command_args[0], "quit")==0) {
		//quit
		if (args_size != 1) return badcommand();
		return quit();

	} else if (strcmp(command_args[0], "set")==0) {
		//set
		if (args_size < 3 || args_size > 7) return badcommand_custom("set");	

        // 1000 is chosen, which is overkill for this assignment, but just to be safe
        char val[DEFAULT_BUFFER_SIZE] = {'\0'};

        for(int i=2; i < args_size; i++) {

            strcat(val, command_args[i]);
            if(i < args_size -1)
                strcat(val, " ");
        }
        
		return set(command_args[1], val);
	
	} else if (strcmp(command_args[0], "print")==0) {
		if (args_size != 2) return badcommand();
		return print(command_args[1]);
	
	} else if (strcmp(command_args[0], "run")==0) {
		if (args_size != 2) return badcommand();
		return run(command_args[1]);
	
    } else if(strcmp(command_args[0], "echo") == 0) {
        if(args_size != 2) 
            return badcommand_custom("echo");
        //handles edge case for echo $
        // where you reference a variable without providing a name
        if(strlen(&command_args[1][1]) == 0 && command_args[1][0] == '$') {
            return badcommand_custom("echo");
        }
        char input[DEFAULT_BUFFER_SIZE] = {'\0'};
        // if its a variable reference
        if(command_args[1][0] == '$') {
            char * var = mem_get_value(&command_args[1][1]);
            if(var) strcat(input, var);
        
        // normal string
        } else {
            strcat(input, command_args[1]);
        }

        printf("%s\n", input);
        return 0;
    
    } else if(strcmp(command_args[0], "my_ls") == 0) {
        if(args_size != 1) {
            return badcommand();
        }
        
        fflush(stdout);
        return system("ls");

    } else if(strcmp(command_args[0], "my_mkdir") == 0) {
        if(args_size != 2) {
            return badcommand();
        }
        int return_code = mkdir(command_args[1], 0777);
        return return_code == 0? return_code: badcommand_custom("my_mkdir");

    } else if(strcmp(command_args[0], "my_touch") == 0) {
        if(args_size != 2) {
            return badcommand();
        }
        // constructs command and runs it
        char cmd[DEFAULT_BUFFER_SIZE] = {'\0'};
        strcat(cmd, "touch ");
        strcat(cmd, command_args[1]);
        fflush(stdout);
        return system(cmd);


    } else if(strcmp(command_args[0], "my_cd") == 0) {
        if(args_size != 2) {
            return badcommand();
        }

        int return_code = chdir(command_args[1]);
        return return_code? badcommand_custom("my_cd"): return_code;


    } else if(strcmp(command_args[0], "my_cat") == 0) {
        if(args_size != 2) {
            return badcommand();
        }

        // constructs command and runs it
        char cmd[DEFAULT_BUFFER_SIZE] = {'\0'};
        strcat(cmd, "cat ");
        strcat(cmd, command_args[1]);
        fflush(stdout);
        
        int return_code = system(cmd);
        return return_code? badcommand_custom("my_cat"): return_code;

    } else if(strcmp(command_args[0], "if") == 0) {
        return if_statement(command_args, args_size);
    } else if(strcmp(command_args[0], "for") == 0) {
        return for_statement(command_args, args_size);
	} else return badcommand();
}

int help(){
	char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
run SCRIPT.TXT		Executes the file SCRIPT.TXT\n ";
	printf("%s\n", help_string);
    fflush(stdout);
	return 0;
}

// deletes backing folder and exists with code 0
int quit(){
	printf("%s\n", "Bye!");
    system("rm -r ./backing");
	exit(0);
}

int set(char* var, char* value){
	char *link = "=";
	char buffer[1000];
	strcpy(buffer, var);
	strcat(buffer, link);
	strcat(buffer, value);

	mem_set_value(var, value);
	return 0;
}

int print(char* var){
    char *str = mem_get_value(var);
    if(!str)
        printf("Variable Does not exists\n");
    else 
        printf("%s\n", str); 
	return 0;
}

int run(char* script){
	int errCode = 0;
	char line[1000];
	FILE *p = fopen(script,"rt");  // the program is in a file

	if(p == NULL){
		return badcommandFileDoesNotExist();
	}

	fgets(line,999,p);
	while(1){
		errCode = parseInput(line);	// which calls interpreter()
		memset(line, 0, sizeof(line));

		if(feof(p)){
			break;
		}
		fgets(line,999,p);
	}

    fclose(p);

	return errCode;
}

/**
 * DESCRIPTION:
 * Takes all strings in command_args and concatenates them into the input array.
 * If a argument as a $, then its fetched as a variable from memory
*/
int set_template_string(char *input, char *command_args[], int args_size) {
    for(int i=1; i < args_size; i++) {
        if(command_args[i][0] == '$') {
            char* val = mem_get_value(&command_args[i][1]);
            strcat(input, val);
        } else {
            strcat(input, command_args[i]);
        }
        if(i < args_size -1) 
            strcat(input, " ");
    }
    return 0;
}

/**
 * DESCRIPTION:
 * This function is used by the if and for commands to fetch the length of a command body.
*/
int get_next_token(
    char* command_args[], 
    int args_size, 
    int index, 
    const char* terminator[], 
    int terminator_length) {

    int count = 0;
    int depth = 0;

    for(int i = index; i < args_size; i++) {
        for(int j=0; j < terminator_length; j++) {
            if(depth == 0 && strcmp(terminator[j], command_args[i]) == 0) {
                return count;
            }
        }   

        if(strcmp(command_args[i], "for") == 0 || strcmp(command_args[i], "if") == 0)
            depth++;
        
        if(strcmp(command_args[i], "done") == 0 || strcmp(command_args[i], "fi") == 0)
            depth--;

        count++;
    }
    return depth > 0? -1: count;
}

char *preprocess_ident(char *ident) {
    if(ident[0] == '$') {
        char *var = mem_get_value(&ident[1]);
        if(!var) return "\0";
        return var;
    }
    return ident;
}

/**
 * DESCRIPTION:
 * Wether this operation is accepcted by if command
*/
int isValidOp(const char *op) {
    return (strcmp(op, "!=") == 0) || (strcmp(op, "==") == 0);
}

/**
 * DESCRIPTION:
 * Parses if statement and then recursively runs it
 * 
 * PARAMS:
 * command_args: list of arguments 
*/
int if_statement(char* command_args[], int args_size) {
    int index = 0;
    // makes sure if is the first token
    if(strcmp(command_args[index++], "if") != 0) 
        return badcommand_custom("if");
    if(args_size < 6) 
        return badcommand_custom("if, must have atleast 6 arguments");
    
    const char *else_[] = {"else"};
    const char *fi_[] = {"fi"};

    // We only really need like a 100 and something characters
    // so we added extra to be sure
    char lhs[200] = {'\0'};
    char rhs[200] = {'\0'};

    char *buffer[args_size];


    // gets left identifier 
    strcat(lhs, preprocess_ident(command_args[index++]));
    
    // gets operator
    char *op = command_args[index++];
    if(!isValidOp(op)) return error_message("missing operator");

    // Gets right identifier
    strcat(rhs, preprocess_ident(command_args[index++]));

    
    // makes sure then is included
    if(strcmp(command_args[index++], "then") != 0) {
        return error_message("missing 'then'");
    }

    // gets length of the if body
    int body_length = get_next_token(command_args, args_size, index, else_, 1);
    if(body_length == -1) 
        return error_message("Empty if clause");
    
    // sets arguments for body
    char *body[body_length];
    for(int i=0; i < body_length; i++) {
        body[i] = command_args[index+i];
    }
    index += body_length;
    
    if(index >= args_size) return error_message("Empty if clause");

    if(strcmp(command_args[index++], "else") != 0) return error_message("Empty if clause'");
    
    // gets length of the if body
    int else_body_length = get_next_token(command_args, args_size, index, fi_, 1);
    char *else_body[else_body_length];
    for(int i=0; i < else_body_length; i++) 
        else_body[i] = command_args[index+i];

    // expects end of if statment token
    index += else_body_length;
    if(index >= args_size) return badcommand_custom("Empty if clause");
    if(strcmp(command_args[index], "fi") != 0) {
        return badcommand_custom("Empty if clause'");
    }

    if(strcmp(op, "==") == 0) {
        return strcmp(lhs, rhs) == 0? 
        interpreter(body, body_length): 
        interpreter(else_body, else_body_length);
    } else if(strcmp(op, "!=") == 0) {
        return 
        strcmp(lhs, rhs) != 0? 
        interpreter(body, body_length): 
        interpreter(else_body, else_body_length);
    } else {
        return badcommand_custom("if");
    }

    return 0;
}

/**
 * DESCRIPTION:
 * Parses for syntax and then recursively runs the command 
*/
int for_statement(char* command_args[], int args_size) {
    int index=0;
    if(args_size < 4) 
        return badcommand_custom("for, expects at least 4 tokens");

    if(strcmp(command_args[index++], "for") != 0)
        return badcommand_custom("for");
    
    const char *terminator[] = {"done"};

    // gets expression info
    char *iteration_count = preprocess_ident(command_args[index++]);
    if(!is_numeric(iteration_count)) return badcommand_custom("for, expected numeric");

    if(strcmp(command_args[index++], "do") != 0) return badcommand_custom("for, expected 'do'"); 

    int count = atoi(iteration_count);

    // parses while body
    int while_body_length = get_next_token(command_args, args_size, index, terminator, 1);
    if(while_body_length == -1) return badcommand_custom("for, expected 'done' token");
    char* while_body[while_body_length];
    for(int i=0; i < while_body_length; i++) {
        while_body[i] = command_args[index+i];
    }
    index+= while_body_length;
    if(strcmp(command_args[index++], "done") != 0) return badcommand_custom("for, expected 'done' token");
    if(index < args_size) return badcommand_custom("for, end of statement expected");

    for(int i=0; i < count ; i++) {
        interpreter(while_body, while_body_length);
    }

    return 0;
}

/**
 * DESCRIPTION:
 * Checks if a string is numeric (i.e number)
*/
int is_numeric(const char *str) {
    while(*str) {
        if(!(*str >= '0' && *str <= '9')) {
            return 0;
        }
        str++;
    }
    return 1;
}

/**
 * DESCRIPTION:
 * Checks if a string is a alphanumeric
*/
int is_alphanumeric(const char *str) {
    while (*str) {
        if (!((*str >= '0' && *str <= '9') || (*str >= 'A' && *str <= 'Z') || (*str >= 'a' && *str <= 'z'))) {
            //Uses the numeric values of ASCII characters to check if the character is alphanumeric
            return 0; // Not alphanumeric
        }
        str++; // Move to next character
    }
    return 1; // Is alphanumeric, positive to mimic boolean in if-statement
}
