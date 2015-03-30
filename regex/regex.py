#!/usr/bin/env python2

import sys


class MyRegex:

    def __init__(self, regex, k=0):
        self.index = k
        self.quant = ''
        if len(regex) == 1:
            self.char = regex
            return
        self.char = None
        self.expr = [[]]
        i = 0
        parencount = 0
        substart = None
        while i < len(regex):
            k += 1
            if regex[i] == '(':
                if parencount == 0:
                    substart = i+1
                parencount += 1
            elif regex[i] == ')':
                parencount -= 1
                if parencount == 0:
                    self.expr[-1].append(MyRegex(regex[substart:i], k))
            elif parencount == 0:
                if regex[i] == '|':
                    self.expr.append([])
                elif regex[i] == '?':
                    self.expr[-1][-1].q()
                elif regex[i] == '*':
                    self.expr[-1][-1].a()
                elif regex[i] == '+':
                    self.expr[-1][-1].p()
                else:
                    self.expr[-1].append(MyRegex(regex[i], k))
            i += 1

    def __str__(self):
        if self.char:
            retstr = self.char
            if self.quant:
                retstr += self.quant
            return retstr
        retstr = "("
        for sub in self.expr:
            for subsub in sub:
                retstr += "(" + str(subsub) + ")"
            retstr += "|"
        retstr = retstr[:-1] + ")" + self.quant
        return retstr.strip()

    def q(self):
        self.quant = '?'

    def a(self):
        self.quant = '*'

    def p(self):
        self.quant = '+'

    def alt_match(self, teststr, k=None, indices=[]):
        print k
        if self.char:
            if k != None:
                return self.char == teststr[k]
            for i in xrange(len(teststr)):
                if self.char == teststr[i]:
                    return True
            return False
        if k == None:
            k = 0
        l = len(teststr)
        while k < l:
            if 0 not in indices:
                indices.append(0)
            for sub in self.expr:
                if self.index in indices:
                    if self.quant == '' or self.quant == '?':
                        indices.remove(self.index)
                    matchnext = False
                    for subsub in sub:
                        if matchnext:
                            if subsub.alt_match(teststr, k, indices):
                                indices.append(subsub.index)
                            if subsub.quant != '*' and subsub.quant != '?':
                                matchnext = False
                        elif subsub.index in indices:
                            matchnext = True
                            if subsub.quant == '' or subsub.quant == '?' or not subsub.alt_match(teststr, k, indices):
                                indices.remove(subsub.index)
                    if matchnext:
                        return True
                    if sub[0].alt_match(teststr, k, indices):
                        indices.append(sub[0].index)
            k += 1
        for sub in self.expr:
            if sub[-1].index in indices:
                return True
        return False

    def match(self, teststr,  k=None, indices=None):
        next_indices = []
        if k == None:
            for i in xrange(len(teststr)):
                next_indices += self.match(teststr, i)
            return next_indices
        if indices == None:
            indices = [k]
        nomatch = True
        if self.char:
            for i in indices:
                if i == len(teststr):
                    continue
                j = i
                while self.char == teststr[j]:
                    j += 1
                    next_indices.append(j)
                    if j == len(teststr) or self.quant == '':
                        break
        else:
            for sub in self.expr:
                tmp_indices = indices
                for subsub in sub:
                    tmp_indices = subsub.match(teststr, k, tmp_indices)
                next_indices += tmp_indices
        if self.quant == '*' or self.quant == '?':
            for i in indices:
                if i not in next_indices:
                    next_indices.append(i)
        return next_indices

myr = MyRegex(sys.argv[1])
print myr
if len(sys.argv) > 2:
    print myr.match(sys.argv[2])
    print myr.alt_match(sys.argv[2])
