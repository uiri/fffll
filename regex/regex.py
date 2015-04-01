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

    def valid_starts(self):
        if self.char:
            return [self.char]
        valid = []
        for sub in self.expr:
            valid.append(sub[0].valid_starts())
        return valid

    def match(self, teststr, k=None, indices=None):
        if indices == None:
            indices = []
        if self.char:
            if k == None:
                for c in teststr:
                    if c == self.char:
                        indices.append(self.index)
                return indices
            if self.char == teststr[k]:
                return [self.index]
            return []
        if k == None:
            l = len(teststr)
            k = 0
            while k < l:
                indices = self.match(teststr, k, indices)
                k += 1
            return indices
        rem_zero = False
        if 0 not in indices:
            rem_zero = True
            indices.append(0)
        for sub in self.expr:
            matchnext = False
            prev_index = None
            for subsub in sub:
                if matchnext:
                    rem = False
                    if subsub.index not in indices:
                        rem = True
                        indices.append(subsub.index)
                    indices += subsub.match(teststr, k, indices)
                    if rem and subsub.index in indices:
                        indices.remove(subsub.index)
                    if subsub.quant != '*' and subsub.quant != '?':
                        matchnext = False
                elif subsub.index in indices:
                    matchnext = True
                    prev_index = subsub.index
                    if subsub.quant == '' or subsub.quant == '?' or [] == subsub.match(teststr, k, indices):
                        indices.remove(subsub.index)
            if matchnext:
                if self.index in indices:
                    indices.remove(self.index)
                if prev_index:
                    indices.append(prev_index)
                return indices
            if self.index in indices and sub[0].match(teststr, k, indices):
                indices.append(sub[0].index)
        if self.index in indices:
            indices.remove(self.index)
        if rem_zero and 0 in indices:
            indices.remove(0)
        return indices

tests = [
    ("Single char not present", "a", "dshdlhjkldfh", False),
    ("Single char present start", "a", "adshdlhjkldfh", True),
    ("Single char present middle", "a", "dshdlhajkldfh", True),
    ("Single char present end", "a", "dshdlhjkldfha", True),
    ("String not present", "abc", "dshdlhjkldfh", False),
    ("String present start", "abc", "abcdshdlhjkldfh", True),
    ("String present middle", "abc", "dshdlhabcjkldfh", True),
    ("String present end", "abc", "dshdlhjkldfhabc", True),
    ("String optional char not present", "ab?c", "dshdlhjkldfh", False),
    ("String optional char fully present start", "ab?c", "abcdshdlhjkldfh", True),
    ("String optional char fully present middle", "ab?c", "dshdlhabcjkldfh", True),
    ("String optional char fully present end", "ab?c", "dshdlhjkldfhabc", True),
    ("String optional char partly present start", "ab?c", "acdshdlhjkldfh", True),
    ("String optional char partly present middle", "ab?c", "dshdlhacjkldfh", True),
    ("String optional char partly present end", "ab?c", "dshdlhjkldfhac", True),
    ("String optional repetition not present", "ab*c", "dshdlhjkldfh", False),
    ("String optional repetition repeatedly present start", "ab*c", "abbcdshdlhjkldfh", True),
    ("String optional repetition repeatedly present middle", "ab*c", "dshdlhabbcjkldfh", True),
    ("String optional repetition repeatedly present end", "ab*c", "dshdlhjkldfhabbc", True),
    ("String optional repetition singly present start", "ab*c", "abcdshdlhjkldfh", True),
    ("String optional repetition singly present middle", "ab*c", "dshdlhabcjkldfh", True),
    ("String optional repetition singly present end", "ab*c", "dshdlhjkldfhabc", True),
    ("String optional repetition partly present start", "ab*c", "acdshdlhjkldfh", True),
    ("String optional repetition partly present middle", "ab*c", "dshdlhacjkldfh", True),
    ("String optional repetition partly present end", "ab*c", "dshdlhjkldfhac", True),
    ("String required repetition not present", "ab+c", "dshdlhjkldfh", False),
    ("String required repetition repeatedly present start", "ab+c", "abbcdshdlhjkldfh", True),
    ("String required repetition repeatedly present middle", "ab+c", "dshdlhabbcjkldfh", True),
    ("String required repetition repeatedly present end", "ab+c", "dshdlhjkldfhabbc", True),
    ("String required repetition singly present start", "ab+c", "abcdshdlhjkldfh", True),
    ("String required repetition singly present middle", "ab+c", "dshdlhabcjkldfh", True),
    ("String required repetition singly present end", "ab+c", "dshdlhjkldfhabc", True),
    ("String required repetition partly present start", "ab+c", "acdshdlhjkldfh", False),
    ("String required repetition partly present middle", "ab+c", "dshdlhacjkldfh", False),
    ("String required repetition partly present end", "ab+c", "dshdlhjkldfhac", False),
    ("OR Single char neither present", "a|b", "dshdlhjkldfh", False),
    ("OR Single char first present start", "a|b", "adshdlhjkldfh", True),
    ("OR Single char first present middle", "a|b", "dshdlhajkldfh", True),
    ("OR Single char first present end", "a|b", "dshdlhjkldfha", True),
    ("OR Single char second present start", "a|b", "bdshdlhjkldfh", True),
    ("OR Single char second present middle", "a|b", "dshdlhbjkldfh", True),
    ("OR Single char second present end", "a|b", "dshdlhjkldfhb", True),
    ("OR String neither present", "abc|def", "dshdlhjkldfh", False),
    ("OR String first present start", "abc|def", "abcdshdlhjkldfh", True),
    ("OR String first present middle", "abc|def", "dshdlhabcjkldfh", True),
    ("OR String first present end", "abc|def", "dshdlhjkldfhabc", True),
    ("OR String second present start", "abc|def", "defshdlhjkldfh", True),
    ("OR String second present middle", "abc|def", "dshdlhdefjkldfh", True),
    ("OR String second present end", "abc|def", "dshdlhjkldfhdef", True),
    ("String OR char neither present", "a(b|d)c", "dshdlhjkldfh", False),
    ("String OR char first present start", "a(b|d)c", "abcdshdlhjkldfh", True),
    ("String OR char first present middle", "a(b|d)c", "dshdlhabcjkldfh", True),
    ("String OR char first present end", "a(b|d)c", "dshdlhjkldfhabc", True),
    ("String OR char second present start", "a(b|d)c", "adcdshdlhjkldfh", True),
    ("String OR char second present middle", "a(b|d)c", "dshdlhadcjkldfh", True),
    ("String OR char second present end", "a(b|d)c", "dshdlhjkldfhadc", True),
    ("String OR optional char not at all present", "a(b|d)?c", "dshdlhjkldfh", False),
    ("String OR optional char first present start", "a(b|d)?c", "abcdshdlhjkldfh", True),
    ("String OR optional char first present middle", "a(b|d)?c", "dshdlhabcjkldfh", True),
    ("String OR optional char first present end", "a(b|d)?c", "dshdlhjkldfhabc", True),
    ("String OR optional char second present start", "a(b|d)?c", "adcdshdlhjkldfh", True),
    ("String OR optional char second present middle", "a(b|d)?c", "dshdlhadcjkldfh", True),
    ("String OR optional char second present end", "a(b|d)?c", "dshdlhjkldfhadc", True),
    ("String OR optional char partly present start", "a(b|d)?c", "acdshdlhjkldfh", True),
    ("String OR optional char partly present middle", "a(b|d)?c", "dshdlhacjkldfh", True),
    ("String OR optional char partly present end", "a(b|d)?c", "dshdlhjkldfhac", True),
    ("String OR optional char repetition not at all present", "a(b|d)*c", "dshdlhjkldfh", False),
    ("String OR optional char repetition first repeatedly present start", "a(b|d)*c", "abbcdshdlhjkldfh", True),
    ("String OR optional char repetition first repeatedly present middle", "a(b|d)*c", "dshdlhabbcjkldfh", True),
    ("String OR optional char repetition first repeatedly present end", "a(b|d)*c", "dshdlhjkldfhabbc", True),
    ("String OR optional char repetition second repeatedly present start", "a(b|d)*c", "addcdshdlhjkldfh", True),
    ("String OR optional char repetition second repeatedly present middle", "a(b|d)*c", "dshdlhaddcjkldfh", True),
    ("String OR optional char repetition second repeatedly present end", "a(b|d)*c", "dshdlhjkldfhaddc", True),
    ("String OR optional char repetition first singly present start", "a(b|d)*c", "abcdshdlhjkldfh", True),
    ("String OR optional char repetition first singly present middle", "a(b|d)*c", "dshdlhabcjkldfh", True),
    ("String OR optional char repetition first singly present end", "a(b|d)*c", "dshdlhjkldfhabc", True),
    ("String OR optional char repetition second singly present start", "a(b|d)*c", "adcdshdlhjkldfh", True),
    ("String OR optional char repetition second singly present middle", "a(b|d)*c", "dshdlhadcjkldfh", True),
    ("String OR optional char repetition second singly present end", "a(b|d)*c", "dshdlhjkldfhadc", True),
    ("String OR optional char repetition partly present start", "a(b|d)*c", "acdshdlhjkldfh", True),
    ("String OR optional char repetition partly present middle", "a(b|d)*c", "dshdlhacjkldfh", True),
    ("String OR optional char repetition partly present end", "a(b|d)*c", "dshdlhjkldfhac", True),
    ("String OR required char repetition not at all present", "a(b|d)+c", "dshdlhjkldfh", False),
    ("String OR required char repetition first repeatedly present start", "a(b|d)+c", "abbcdshdlhjkldfh", True),
    ("String OR required char repetition first repeatedly present middle", "a(b|d)+c", "dshdlhabbcjkldfh", True),
    ("String OR required char repetition first repeatedly present end", "a(b|d)+c", "dshdlhjkldfhabbc", True),
    ("String OR required char repetition second repeatedly present start", "a(b|d)+c", "addcdshdlhjkldfh", True),
    ("String OR required char repetition second repeatedly present middle", "a(b|d)+c", "dshdlhaddcjkldfh", True),
    ("String OR required char repetition second repeatedly present end", "a(b|d)+c", "dshdlhjkldfhaddc", True),
    ("String OR required char repetition first singly present start", "a(b|d)+c", "abcdshdlhjkldfh", True),
    ("String OR required char repetition first singly present middle", "a(b|d)+c", "dshdlhabcjkldfh", True),
    ("String OR required char repetition first singly present end", "a(b|d)+c", "dshdlhjkldfhabc", True),
    ("String OR required char repetition second singly present start", "a(b|d)+c", "adcdshdlhjkldfh", True),
    ("String OR required char repetition second singly present middle", "a(b|d)+c", "dshdlhadcjkldfh", True),
    ("String OR required char repetition second singly present end", "a(b|d)+c", "dshdlhjkldfhadc", True),
    ("String OR required char repetition partly present start", "a(b|d)+c", "acdshdlhjkldfh", False),
    ("String OR required char repetition partly present middle", "a(b|d)+c", "dshdlhacjkldfh", False),
    ("String OR required char repetition partly present end", "a(b|d)+c", "dshdlhjkldfhac", False),
    ("String OR string neither present", "a(bcd|efg)h", "dshdlhjkldfh", False),
    ("String OR string first present start", "a(bcd|efg)h", "abcdhshdlhjkldfh", True),
    ("String OR string first present middle", "a(bcd|efg)h", "dshdlhabdhcjkldfh", True),
    ("String OR string first present end", "a(bcd|efg)h", "dshdlhjkldfhabcdh", True),
    ("String OR string second present start", "a(bcd|efg)h", "aefghdshdlhjkldfh", True),
    ("String OR string second present middle", "a(bcd|efg)h", "dshdlhaefghjkldfh", True),
    ("String OR string second present end", "a(bcd|efg)h", "dshdlhjkldfhaefgh", True),
    ("String OR optional string not at all present", "a(bcd|efg)?h", "dshdlhjkldfh", False),
    ("String OR optional string first present start", "a(bcd|efg)?h", "abcdhshdlhjkldfh", True),
    ("String OR optional string first present middle", "a(bcd|efg)?h", "dshdlhabcdhjkldfh", True),
    ("String OR optional string first present end", "a(bcd|efg)?h", "dshdlhjkldfhabcdh", True),
    ("String OR optional string second present start", "a(bcd|efg)?h", "aefghshdlhjkldfh", True),
    ("String OR optional string second present middle", "a(bcd|efg)?h", "dshdlhaefghjkldfh", True),
    ("String OR optional string second present end", "a(bcd|efg)?h", "dshdlhjkldfhaefgh", True),
    ("String OR optional string partly present start", "a(bcd|efg)?h", "ahdshdlhjkldfh", True),
    ("String OR optional string partly present middle", "a(bcd|efg)?h", "dshdlhahjkldfh", True),
    ("String OR optional string partly present end", "a(bcd|efg)?h", "dshdlhjkldfhah", True),
    ("String OR optional string repetition not at all present", "a(bcd|efg)*h", "dshdlhjkldfh", False),
    ("String OR optional string repetition first repeatedly present start", "a(bcd|efg)*h", "abcdbcdhshdlhjkldfh", True),
    ("String OR optional string repetition first repeatedly present middle", "a(bcd|efg)*h", "dshdlhabcdbcdhjkldfh", True),
    ("String OR optional string repetition first repeatedly present end", "a(bcd|efg)*h", "dshdlhjkldfhabcdbcdh", True),
    ("String OR optional string repetition second repeatedly present start", "a(bcd|efg)*h", "aefgefghdshdlhjkldfh", True),
    ("String OR optional string repetition second repeatedly present middle", "a(bcd|efg)*h", "dshdlhaefgefghjkldfh", True),
    ("String OR optional string repetition second repeatedly present end", "a(bcd|efg)*h", "dshdlhjkldfhaefgefgh", True),
    ("String OR optional string repetition first singly present start", "a(bcd|efg)*h", "abcdhshdlhjkldfh", True),
    ("String OR optional string repetition first singly present middle", "a(bcd|efg)*h", "dshdlhabcdhjkldfh", True),
    ("String OR optional string repetition first singly present end", "a(bcd|efg)*h", "dshdlhjkldfhabcdh", True),
    ("String OR optional string repetition second singly present start", "a(bcd|efg)*h", "aefghdshdlhjkldfh", True),
    ("String OR optional string repetition second singly present middle", "a(bcd|efg)*h", "dshdlhaefghjkldfh", True),
    ("String OR optional string repetition second singly present end", "a(bcd|efg)*h", "dshdlhjkldfhaefgh", True),
    ("String OR optional string repetition partly present start", "a(bcd|efg)*h", "ahdshdlhjkldfh", True),
    ("String OR optional string repetition partly present middle", "a(bcd|efg)*h", "dshdlhahjkldfh", True),
    ("String OR optional string repetition partly present end", "a(bcd|efg)*h", "dshdlhjkldfhah", True),
    ("String OR required string repetition not at all present", "a(bcd|efg)+h", "dshdlhjkldfh", False),
    ("String OR required string repetition first repeatedly present start", "a(bcd|efg)+h", "abcdbcdhdshdlhjkldfh", True),
    ("String OR required string repetition first repeatedly present middle", "a(bcd|efg)+h", "dshdlhabcdbcdhjkldfh", True),
    ("String OR required string repetition first repeatedly present end", "a(bcd|efg)+h", "dshdlhjkldfhabcdbcdh", True),
    ("String OR required string repetition second repeatedly present start", "a(bcd|efg)+h", "aefgefghdshdlhjkldfh", True),
    ("String OR required string repetition second repeatedly present middle", "a(bcd|efg)+h", "dshdlhaefgefghjkldfh", True),
    ("String OR required string repetition second repeatedly present end", "a(bcd|efg)+h", "dshdlhjkldfhaefgefgh", True),
    ("String OR required string repetition first singly present start", "a(bcd|efg)+h", "abcdhshdlhjkldfh", True),
    ("String OR required string repetition first singly present middle", "a(bcd|efg)+h", "dshdlhabcdhjkldfh", True),
    ("String OR required string repetition first singly present end", "a(bcd|efg)+h", "dshdlhjkldfhabcdh", True),
    ("String OR required string repetition second singly present start", "a(bcd|efg)+h", "aefghdshdlhjkldfh", True),
    ("String OR required string repetition second singly present middle", "a(bcd|efg)+h", "dshdlhaefghjkldfh", True),
    ("String OR required string repetition second singly present end", "a(bcd|efg)+h", "dshdlhjkldfhaefgh", True),
    ("String OR required string repetition partly present start", "a(bcd|efg)+h", "ahdshdlhjkldfh", False),
    ("String OR required string repetition partly present middle", "a(bcd|efg)+h", "dshdlhahjkldfh", False),
    ("String OR required string repetition partly present end", "a(bcd|efg)+h", "dshdlhjkldfhah", False),
    ]
for (test_name, test_re, test_str, test_bool) in tests:
    myr = MyRegex(test_re)
    match_res = myr.match(test_str)
    if bool(match_res) != test_bool:
        print "FAILING:", test_name, test_re, test_str, match_res
