# fffll

Fun Functioning Functional Little Language (abbreviated fffll, pronounced 'fell')

by W Pearson

## Compile and Install

The reference implementation is written in C. The initial rough draft was written in python; it no longer conforms to the spec. Grab the tarball to get the C implementation. This C implementation depends on curl and pcre; it will not compile if either of these are missing. If you are sure that these are installed then you should be able to compile and install the tarball in the usual way:

```sh
./configure
make
sudo make install
```

## Language Spec

### Lexical Analysis

In the following situations, anything goes, with the exception of the termination sequence for said situation:

Characters situated between / and /; These are interpreted as regular expressions. / is not allowed inside of a regular expression.

Characters situated between " and "; If the next character after the closing " is a ", it is interpreted as intending to insert a " into the string and the string does not close. If the next character after the closing " is a @, the next two characters must be hexadecimal digits. The byte which these two characters represent is inserted at the end of the string. The string will continue if a " is the next character after the two hexadecimal digits. Hexadecimal digits are 0 to 9, a to f and A to F.

Characters situated between --* and *--. These are interpreted as multiline comments. *-- is not allowed inside of a multiline comment.

Characters situated between -- and a newline character, 0x0A. These are interpreted as single line comments. 0x0A is not allowed inside of a single line comment.

Charcters situated between a % and a newline character. These are interpreted as a comma separated list of imports. Each import designates the filename excluding the ff file extension to be imported. This filename will be used to namespace functions and variables defined in the imported fffll file. Imports are relative to the directory in which the importer resides or to the global FFFLL_DIR environment variable.

Outside of these situations, whitespace characters, that is 0x20, 0x09 and 0x0A, are ignored and only operators and delimiters, names and numbers are allowed:

Names may be described with the following regular expression: [_A-Za-z][_A-Za-z0-9]*

Real numbers may be described with the following regular expression: -?[0-9]+(\.[0-9]+)?

Numbers which are used to index key-value lists must NOT have a fractional part.

The following characters are taken as operators and delimiters:


,  .  :  (  )  [  ]  {  }  <  >  =  &  |  !  ?  ~


#### Automatic Comma Insertion

When inside of an argument list, commas may be automatically inserted where none is present. This will occur when a string, a boolean expression, a statementlist or a key-value list is followed by a statementlist, a key-value list that is not an argument list or another string.

### Types

The Basic types have already been described: Real Numbers, Strings and Bytes. Bytes are raw bytes which are inserted into a String using the @-notation. Value types are composed using operators and delimiters with the basic types. A value is anything which has a type.

#### I/O Stream

An I/O Stream is a standard I/O stream or a value that is returned by the `open` builtin and interacted with using the `write` and `read` builtins. I/O Streams include files and HTTP request-response pairs. The standard I/O streams are provided without the need to call the `open` builtin and are as follows: `stdin`, `stdout` and `stderr`.

#### Key-Value List

An argument list is a special key-value list that is delimited using ( and ). Normal key-value lists are delimited using [ and ]. Key-value lists consist of comma separated key-value parts. A key-value part consists of an optional key component which is a name followed by a ':' and a required value part which is either a name or a value of any type. A value without a name key will be assigned the lowest non-negative whole number not yet assigned as its key. Thus, the first value without a name key has the numeric key 0.

##### Ranges

There are special key-value list portions, referred to as ranges, which consist of a collection of number values that are number keyed. Ranges are created using the double dot syntax. The double dot syntax takes a start number, an optional numeric increment and an end number. Each value in the range is assigned its own number key.

It creates all the values starting at the start number, going up by the increment each time and ending before the end number. Two dots go between the start number and the increment and two dots go between the increment and the end number. If an increment is not provided, it defaults to 1 and only two dots go between the start number and the end number. A number may be either a Real Number or a name that will be evaluated as a Real Number when needed.

##### Indexing

A number or name that is a valid key in a list may be used to access its corresponding value in the list (this is referred to as indexing the list) by taking the name designating the list, appending a dot to it and then appending the valid key after that. Alternatively, a name which refers to a string representation of a named key or the Real Number representation may be used to index a list by its value by encasing it in '[' and ']'.

Thus, if x refers to 0 and list refers to a list of at least one element, list.0 and list.[x] may both be used to index the list's first element. Similarly, if list has a named key that is test and x refers to "test" then list.test and list.[x] are equivalent and access the value mapped to the key test.

When indexing a list which contains one or more Ranges, each value that the range expands to gets its own number key. It is not a good idea to name key a range; the start value of the range may end up with a name key or a numbered key. The desired behaviour is likely to be achieved by name keying a list that contains the range.

#### Boolean Expression

A boolean expression optionally starts with a ! and is delimited by ( and ) and conists of one or more comparison expressions. If there is more than one comparison expression in a boolean expression they will be joined using the | (or) or the & (and) logical operators. If a boolean expression starts with a !, the expression inside of the ( and ) will be evaluated and then negated.

A comparison expression may or may not be delimited by ( and ). Comparison expressions consist of two values joined using one of the comparison operators: = < > ? ~.

=, < and > are the equality, less than and greater than operators respectively.

? is the typeof operator and will evaluate to true if both values have the same type -- if both values are key-value lists, all named keys in the second value must be present in the first value or the comparison expression will evaluate to false.

The ~ operator takes a value before it and a regular expression after it. This is one of the only contexts in which regular expressions are permitted.

Two comparison expressions joined by a | will be evaluated to true if one of them is true. Two comparison expressions joined by a & will be evaluated to true if both of them are true. When evaluating a boolean expression, all comparison expressions are evaluated and then the results are reduced using their joining operator going from left to right. For example, where c is a comparison expression, (c|c&c|c) will be evaluated as (((c|c)&c)|c).

#### Statementlist

A statementlist is delimited by { } and consists of a series of function calls. A function call consists of a functor, which designates the function to be called, and an argument list, which designates the values to be passed as arguments to the function. A functor is either a name which designates the function to be called, a value inside of a key-value list that is accessed, a function call or the definition of the function to be called. If a name does not designate a function, a function call does not return a function or the accessed value in a key-value list is not a function, an error will be raised related to an improper functor.

#### Function

A function definition is composed of a key-value list which represents its arguments and a statementlist. Each function has its own level of scope. The return value of a function is the return value of the last function call in its statementlist.

### Execution

#### Programs

A fffll program consists of a series of function calls, comments and imports. Imports cause another fffll file to be executed in the middle of the execution of the file doing the import. A function call is as defined in the definition of a statement list. Comments are ignored during execution. Each function call in a program will be executed in the order in which it appears in the program.

#### Errors

It is possible for a program to call the `die` builtin in order to stop execution immediately with an error message. One may place a block of code that may die as the first argument to `save` and the first name-keyed argument to save will be used to save the program from dying; the name of the name-keyed argument will be used as the name of the error message.

If the saving block changes the value of the name referring to the error, the error is considered "saved". Otherwise, the error was not "saved" and will be propagated to the next save block up, if any exist.

#### Constants

A name may refer to any value. A name which starts with an underscore may be `set` only once. Any subsequent attempts to `set` it will fail; the name will still point to its old value. All builtins, in addition to their variable names, are available through their constant names. It is not possible to assign constants which have the same name as a builtin.

### Builtins

There are a number of functions which are available to fffll programs without any imports. In addition to these builtins, the standard I/O streams (`stdin`, `stdout` and `stderr`) are also available without any calls to the `open` builtin and follow the same pattern of having a variable name and a constant name. The constant name of any builtin is its variable name preceded by an underscore.

The following functions are builtins:

* `add`
* `cat`
* `die`
* `for`
* `head`
* `if`
* `len`
* `mul`
* `open`
* `push`
* `rcp`
* `read`
* `save`
* `set`
* `tail`
* `tok`
* `write`
