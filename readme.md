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
```

### TODO

-   functions: you can define and call a function, as in the parser will understand it, but the bytecode compiler and the VM won't.
-   strings: for now there are only `bool` and `int` datatypes
-   `print()`: would be really nice to be able to output to the stdout
