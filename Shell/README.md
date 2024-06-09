# COMP310_W2024_A1

Students:
Theodor Semerdzhiev (261118892)
Nick Belev (261076111)

## Features


1. **If Statements:**
   - Bonus feature implemented.
   - Syntax: `if identifier (== or !=) identifier then command else command fi`
   - Executes the first command if the condition is true, otherwise executes the second command.
   - The commands can be any arbitrary command, including other if statements or for loops.


2. **For Loops: Our own command**
   - Syntax: `for [number] do command done`
   - Executes the specified command the given number of times, the number can also be a variable.
   - The command can be any arbitrary command, including other if statements or for loops.
   - number HAS to be number, we use atoi for the conversion, so if the number is really large then it might cause some weird behaviour. If its a not a number, then you should get a error message.

## Usage Examples

- For Loop Example:
  ```sh
  for 3 do echo hello done
  ```
  ```sh
  set x 123
  for $x do echo hello done
  ```
    
Have Fun!!