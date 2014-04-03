#!/usr/bin/env python2
"""FFFLL - Python implementation of a Fun Functioning Functional Little Language
     Copyright (C) 2013 W Pearson

     This program is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.
     """

import ply.lex as lex
import ply.yacc as yacc
import sys, copy

class ArgList(object):

    def __init__(self, args):
        if args == None:
            self.args = args
            self.count = 0
        else:
            self.args = args
            self.count = len(args)

    def __getitem__(self, key):
        return self.args[key]

    def __str__(self):
        return str(self.args)
    
    def evaluate(self):
        if self.count == 0:
            return []
        evaluated = []
        for arg in self.args:
            if isinstance(arg, BlockVal):
                evaluated.append(arg)
            else:
                evaluated.append(arg.evaluate())
        return evaluated

class Value(object):

    def __init__(self, valtype, value):
        self.valtype = valtype
        self.value = value

    def evaluate(self):
        return self.value

    def __str__(self):
        return str(self.value)

class StdFileVal(Value):

    def __init__(self, value):
        super(StdFileVal, self).__init__("f", value)
    
class FileVal(Value):

    def __init__(self, value):
        f = open(value)
        refcount[f] = 1
        super(FileVal, self).__init__("f", f)
    
class FuncVal(Value):

    def __init__(self, name, arglist):
        super(FuncVal, self).__init__("c", name)
        self.arglist = arglist
        self.name = name
        
    def evaluate(self):
        return funcdefs[self.name].evaluate(self.arglist)
        
class ListVal(Value):

    def __init__(self, value):
        super(ListVal, self).__init__("l", value)
                    
class StrVal(Value):

    def __init__(self, value):
        value = value[1:-1]
        super(StrVal, self).__init__("s", value)

class NumVal(Value):

    def __init__(self, value):
        super(NumVal, self).__init__("i", value)
        
class Variable(object):

    def __init__(self, name):
        self.name = name
        
    def evaluate(self):
        try:
            return varlist[-1][self.name]
        except KeyError:
            None

class BlockVal(Value):

    def __init__(self, statements):
        super(BlockVal, self).__init__("d", statements)

    def evaluate(self):
        for statement in self.value:
            s = statement.evaluate()
        return s

class BoolExpr(Value):

    def __init__(self, expr):
        self.stack = []
        self.stack.append(expr)
        self.neg = False
        super(BoolExpr, self).__init__("b", self.stack)

    def evaluate(self):
        if len(self.stack) == 1:
            r = bool(self.stack[0].evaluate())
        else:
            i = 1
            r = self.stack[0].evaluate()
            while i < len(self.stack):
                if self.stack[i] == '<':
                    i += 1
                    r = r < self.stack[i].evaluate()
                if self.stack[i] == '>':
                    i += 1
                    r = r > self.stack[i].evaluate()
                if self.stack[i] == '=':
                    i += 1
                    r = r == self.stack[i].evaluate()
                if self.stack[i] == '|':
                    i += 1
                    r = r or self.stack[i].evaluate()                    
                if self.stack[i] == '&':
                    i += 1
                    r = r and self.stack[i].evaluate()
                i += 1
        if self.neg:
            return not r
        return r
    
class FuncDef(object):

    def __init__(self, arglist, block):
        self.arglist = arglist
        self.block = block

    def evaluate(self, arglist):
        self.scope(arglist)
        arglist = arglist.evaluate()
        r = self.block.evaluate()
        self.descope()
        return r
        
    def scope(self, arglist):
        var = copy.copy(basevarlist)
        for i in xrange(arglist.count):
            var[self.arglist[i].name] = arglist[i].evaluate()
            try:
                if var[self.arglist[i].name] in refcount:
                    refcount[var[self.arglist[i].name]] += 1
            except TypeError:
                pass
        varlist.append(var)
        
    def descope(self):
        for var in varlist[-1]:
            try:
                if varlist[-1][var] in refcount:
                    refceount[varlist[-1][var]] -= 1
                    if refcount[varlist[-1][var]] == 0:
                        varlist[-1][var].close()
            except TypeError:
                pass
        varlist.pop()

class DefDef(FuncDef):

    def __init__(self):
        self.arglist = ["name", "arglist", "block"]

    def evaluate(self, arglist):
        self.scope(arglist)
        r = varlist[-1]["name"].value.upper()
        funcdefs[varlist[-1]["name"].value.upper()] = FuncDef(ArgList(varlist[-1]["arglist"].evaluate()), varlist[-1]["block"])
        self.descope()
        return r

    def scope(self, arglist):
        var = {}
        for i in xrange(arglist.count):
            var[self.arglist[i]] = arglist[i]
        varlist.append(var)
        
class SetDef(FuncDef):

    def __init__(self):
        pass

    def evaluate(self, arglist):
        varlist[-1][arglist[0].name] = arglist[1].evaluate()
        return varlist[-1][arglist[0].name]
        
class WriteDef(FuncDef):

    def __init__(self):
        pass

    def evaluate(self, arglist):
        arglist = arglist.evaluate()
        if not isinstance(arglist[0], file):
            return ""
        r =""
        for i in xrange(len(arglist[1:])):
            if isinstance(arglist[i+1], float):
                if arglist[i+1]%1 == 0:
                    arglist[i+1] = int(arglist[i+1])
            if isinstance(arglist[i+1], list):
                r += "["
                for arg in arglist[i+1]:
                    if isinstance(arg, StrVal):
                        r += '"' + str(arg) + '"'
                    else:
                        r += str(arg)
                    r += ", "
                r = r[:-2] + " ]"
            else:
                r += str(arglist[i+1])
        if arglist[0] == sys.stdout and r[-1] != '\n':
            r += '\n'
        arglist[0].write(r)
        return r

class ReadDef(FuncDef):

    def __init__(self):
        pass

    def evaluate(self, arglist):
        readfile = arglist[0].evaluate()
        if not isinstance(readfile, file):
            return ""
        r = readfile.readline().strip()
        return r
    
class IfDef(FuncDef):

    def __init__(self):
        pass

    def evaluate(self, arglist):
        arglist = arglist.evaluate()
        if len(arglist) != 2 and len(arglist) != 3:
            return None
        if arglist[0]:
            return arglist[1].evaluate()
        if len(arglist) == 3:
            return arglist[2].evaluate()
        return None

class WhileDef(FuncDef):

    def __init__(self):
        pass

    def evaluate(self, arglist):
        if arglist.count != 2:
            return None
        a = None
        while arglist[0].evaluate():
            a = arglist[1].evaluate()
        return a
    
class AddDef(FuncDef):

    def __init__(self):
        pass

    def evaluate(self, arglist):
        arglist = arglist.evaluate()
        s = arglist[0]
        for i in xrange(len(arglist[1:])):
            s += arglist[i+1]
        return s
         
class MulDef(FuncDef):

    def __init__(self):
        pass

    def evaluate(self, arglist):
        arglist = arglist.evaluate()
        s = arglist[0]
        for i in xrange(len(arglist[1:])):
            s *= arglist[i+1]
        return s

class RetDef(FuncDef):

    def __init__(self):
        pass

    def evaluate(self, arglist):
        return arglist[0].evaluate()
    
if len(sys.argv) > 1:
    with open(sys.argv[1]) as progfile:
        progread = progfile.read()
    progread = progread.replace(") {", "),{")
    progread = progread.replace("} {", "},{")
    progread = progread.replace("If", "IF")
    progread = progread.replace("IF (", "IF((")
    progread = progread.replace("While", "WHILE")
    progread = progread.replace("WHILE (", "WHILE((")
    progread = progread.replace("WHILE !", "WHILE(!")
    progread = progread.replace("Def", "DEF")
    progread = progread.replace("DEF ", "DEF(\"")
    progread = progread.replace("::", "\",:")
    progread = progread.replace("}\n", "})\n")
else:
    progread = ''
    
tokens = ('NUM', 'STR', 'FUNC', 'VAR')

t_STR = '"([^"]+)?"'

literals = ":;()[]{}=!<>|&,/*"

t_ignore_MULTILINECOMMENT = r'/\*[^\u04]*\*/'
t_ignore = ' \t'

def t_NUM(t):
    '-?\d+(\.\d+)?'
    t.value = float(t.value)
    return t

def t_FUNC(t):
    '[A-Z]([A-Za-z0-9]+)?'
    t.value = t.value.upper()
    return t

def t_VAR(t):
    '[a-z]([A-Za-z0-9]+)?'
    t.value = t.value.lower()
    return t

def t_newline(t):
    r'\n+'
    t.lexer.lineno += t.value.count("\n")

def t_error(t):
    print "Illegal character %s" % t.value[0]
    t.lexer.skip(1)

lexer = lex.lex()
lexer.input(progread)

def p_statementlist(p):
    '''statementlist : statementlist funcall
                     | funcall'''
    if len(p) > 2:
        p[1].append(p[2])
        p[0] = p[1]
    else:
        p[0] = [ p[1] ]

def p_funcall(p):
    '''funcall : FUNC arglist'''
    p[0] = FuncVal(p[1], p[2])

def p_arglist(p):
    '''arglist : "(" list ")"
               | "(" ")"'''
    if len(p) > 3:
        p[0] = ArgList(p[2])
    else:
        p[0] = ArgList(None)
    
def p_list(p):
    '''list : list "," value
            | value'''
    if len(p) > 2:
        p[1].append(p[3])
        p[0] = p[1]
    else:
        p[0] = [ p[1] ]

def p_boolexpr(p):
    '''boolexpr : "!" "(" bexpr ")"
                | "(" bexpr ")"'''
    if p[1] == "!":
        p[3].neg = True
        p[0] = p[3]
    else:
        p[0] = p[2]
    
def p_bexpr(p):
    '''bexpr : compexpr "&" compexpr
             | compexpr "|" compexpr
             | compexpr'''
    if len(p) > 2:
        p[1].stack.append(p[2])
        for s in p[3].stack:
            p[1].stack.append(s)
        p[0] = p[1]
    else:
        p[0] = p[1]

def p_compexpr(p):
    '''compexpr : value ">" value
                | value "<" value
                | value "=" value
                | value'''
    p[0] = BoolExpr(p[1])
    if len(p) > 2:
        p[0].stack.append(p[2])
        p[0].stack.append(p[3])
        
def p_value_str(p):
    '''value : STR'''
    p[0] = StrVal(p[1])

def p_value_num(p):
    '''value : NUM'''
    p[0] = NumVal(p[1])

def p_value_var(p):
    '''value : VAR'''
    p[0] = Variable(p[1])

def p_value_funcall(p):
    '''value : funcall'''
    p[0] = p[1]

def p_value_list(p):
    '''value : "[" list "]"
             | "[" "]"'''
    if len(p) > 3:
        p[0] = ListVal(p[2])
    else:
        p[0] = ListVal([])
    
def p_value_block(p):
    '''value : "{" statementlist "}"'''
    p[0] = BlockVal(p[2])

def p_value_bexpr(p):
    '''value : boolexpr'''
    p[0] = p[1]
    
funcdefs = {"DEF": DefDef(), "SET": SetDef(), "WRITE": WriteDef(), "READ": ReadDef(), "IF": IfDef(), "WHILE": WhileDef(), "ADD": AddDef(), "MUL": MulDef(), "RETURN": RetDef()}
basevarlist = {"stdout": sys.stdout, "stdin": sys.stdin, "stderr": sys.stderr}
varlist = []
varlist.append(copy.copy(basevarlist))
refcount = {}

parser = yacc.yacc()
funcs = parser.parse(progread)
for func in funcs:
    func.evaluate()
