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

A fffll program is made up of a series of function calls. Each function call consists of a name, followed by an opening parenthesis followed by a list of values separated by commas followed by a closing parenthesis. The name must designate a value that is a function. Values have the following types:

### Function  
A function is defined by a key-value list which will be used to initialize a new scope from its argument list followed by a statement list. Functions create anew level of scope.  
### Regular Expression  
A regular expression is a / followed by a PCRE-compatible RegEx followed by another /  
### String  
Strings start and end with ". There is no escaping that is needed for strings. Two strings that are beside each other will be joined with a " (ie: "This contains """ -> This contains ")  
### Bytes  
Bytes may be inserted in between two strings to have an arbitrary character inserted into a string. It contains of an @ followed by two hexadecimal digits from 0 to f.  
### Boolean Expression  
A boolean expression is made up of an opening parenthesis, optionally preceded by a !, followed by one or more comparison expressions, optionally surrounded by parentheses, joined together using & or | followed by a closing parenthesis. A comparison expression is a value followed by an operator which is one of =, <, >, ? or ~. If the operator is ~, the second value must be a regular expression. = tests for equality. < tests for the first value being less than the second. > tests for the first value being greater than the second. ? tests that the first value is the type of the second value. If the both values are key-value lists, all the named keys of the second list must be valid keys of the first value.  

### I/O Stream
An I/O stream is a value that is returned by the builtin open function.  

### Real Number
A real number consists of one or more decimal digits, optionally preceded by a - to designate negativity, optionally followed by a . and one or more decimal digits to designate a fractional part.

### Key-Value List
A key-value list starts with an opening square bracket, is followed by a comma delimited list of values. Each value is optionally preceded by a name to designate the key followed by a :. A key-value list is ended with a closing square bracket. If a value does not have a named key, its key is a whole number. Keys start at 0 and are incremented by one for each value.  

### Statement List
A statement list starts with an opening curly brace followed by a series of functional calls followed by a closing curly braces.  

