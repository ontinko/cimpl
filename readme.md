## Cimpl (WIP)

A stack-based bytecode VM interpreter for a small programming language with C-like (or Go-like, really) syntax

### Code example

```
# variable declaration
my_int : int = 10;
my_bool : bool = true;

# variable declaration with implicit types
explicit_int := 42;

# updating a variable

my_int = 13;
my_int = my_int + 1;
my_int += 1;
my_int++;

# variable reassignment is not allowed
# my_int := 128; does not run
# my_int : int = 256; does not run

# assigning a different type is also not allowed
# my_int = false; does not run

# creating a nested scope

{
    my_int := 0; # works because a new variable is being defined in new scope
}

# you can define functions and call functions

fn sum(a:int, b:int): int {
    return a + b;
}

three := sum(1, 2);

# you can also call functions recursivelly

fn fib(n:int): int {
    if n < 2 {
        return n;
    }
    return fib(n - 1) + fib(n + 2);
}

number := fib(8);

# and then print out the value with a line break at the end

println number; # syntax not final, will ass parentheses

# void functions don't require an explicit return type

fn do_something() {
    temp := 2 + 2;
}

```
