[INTRODUCTION]
E (working on the name) is a dynamically typed, simple scripting language.
It is a language targeted at integration with game engines, offering
great interopability with C/C++ (other languages will be worked on).

It currently has 4 basic types: int, float, string and list.


[PRINTING]
Values can be printed to the console using one of the two builtin functions provided.
print(): prints all variables provided, in order, to the console.
println(): Similar to print, but prints a newline after printing the arguments.


[]


[VARIABLE DECLERATION]
Variables in the script can be declared using the 'let' keyword.
For example, to declare a variable PI, we can use the syntax:
let PI; // Initializes PI to VOID
At first, all variables have the value of VOID. However, variables can be initialized to another value.
Let's initialize our PI variable to 3.
let PI = 3;

Notice we won't change the value of PI across the program. In this case, we can mark the variable as 'const' (constant!)
To do this, we have three, albeit similar methods.
let const PI = 3;
const let PI = 3;
and
const PI = 3;
Note that if you mark a variable as const, you *must* initialize it, and can not change its value later.


[BYTECODE STARTUP]
On starting the script, the VM initializes all global (and namespaced/qualified) variables.
After this, it jumps to the main function.
The program must have a main function defined.
main Can not accept any arguments.

fn main()


[BYTECODE FORMAT]
Compiled binary file structure is:
MAGIC : u32
bytesMemNeeded : u32 // << The number of bytes of memory required to load the whole program
                         Explanation below
nLiterals : u32
Literals : e_var[nLiterals]{
   Type : e_vartype
   value : e_varval
 For strings:
   Type : e_vartype
   Len : u32
   String : u8[Len]
}

nFunctions : u32
Functions = e_function[]{
  code_size : u32
  nargs : u32
  name_hash : u32
  arg_slots = u32[nargs]
  function_code : u8[code_size]
}

bytecodeSize : u32
bytecodeStream : u8[bytecodeSize]

We use a single allocation for the entire program to ensure the code remains fast.
Thus, we rely on the compiler to provide us with the bytes of memory required to initialize
the program. The value provided is a conservative estimate, and some bytes may be lost.
Also, strings are ref counted. So We have to point the refc
to the allocation (+offset) and initialize their refcounters to 1.


[C INTEROPABILITY | CALLING C FUNCTIONS FROM SCRIPT]
C functions can be called from the script using the e_extern_function entry
in e_exec_info. The table defines a list of functions that can be called,
using their hashes.

A namespaced (external) function can be defined from C simply by hashing the full name.
So, for instance, to define a function 'sin' in the math namespace externally:
Create an external function entry with the name: "math::sin". This function is
fully visible to the script.

External functions are checked BEFORE script functions. This way, one can override
a user defined function easily.
The actual order of function calls is: builtin > external > local.

However, you will need to be careful not to override a function of the
script unintentionally. Choose a clear and concise name. Make sure it
does not or will not appear in the script.