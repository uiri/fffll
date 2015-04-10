#!/usr/bin/env python2

import sys
from collections import OrderedDict

class MyRegex:

    def __init__(self, regex, k=0):
        self.index = k
        self.quant = ''
        if len(regex) == 1:
            self.char = regex
            self.findex = k
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
                    k = self.expr[-1][-1].findex
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
                    k = self.expr[-1][-1].findex
            i += 1
        self.findex = k

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

    def state_machine(self, recur=False):
        sm = OrderedDict()
        if self.char:
            sm[self.index] = {}
            sm[self.index][self.char] = [-1]
            if recur:
                return sm, [-1]
            else:
                return sm
        endindices = []
        for sub in self.expr:
            indices = [self.index]
            for subsub in sub:
                subm, subind = subsub.state_machine(True)
                for i in xrange(len(subind)):
                    if subind[i] == -1:
                        subind[i] = subsub.index
                _k, v = subm.popitem(last=False)
                for (k, a) in v.iteritems():
                    for i in xrange(len(a)):
                        if v[k][i] == -1:
                            v[k][i] = subsub.index
                for i in indices:
                    try:
                        sm[i].update(v)
                    except KeyError:
                        sm[i] = v
                    if subsub.quant == '+' or subsub.quant == '*':
                        try:
                            sm[i][''].append(i)
                        except KeyError:
                            sm[i][''] = [i]
                    if subsub.quant == '?' or subsub.quant == '*':
                        try:
                            sm[i][''] += subind
                        except KeyError:
                            sm[i][''] = subind
                sm.update(subm)
                indices = subind
            endindices += indices
        if recur:
            return sm, endindices
        else:
            for i in endindices:
                try:
                    sm[i].update({'': [-1]})
                except KeyError:
                    sm[i] = {'': [-1]}
            return sm

    def match(self, teststr, k=None, indices=None):
        if indices == None:
            indices = [0]
        sm = self.state_machine()
        if k == None:
            l = len(teststr)
            k = 0
        while len(indices):
            next_indices = []
            # try:
            #     print teststr[k], indices, sm
            # except IndexError:
            #     print indices, sm
            next_indices = []
            for i in indices:
                if i == -1:
                    return True
                try:
                    for key in sm[i].keys():
                        if teststr[k] == key:
                            next_indices += sm[i][key]
                except IndexError:
                    pass
                try:
                    next_indices += sm[i]['']
                except KeyError:
                    pass
            if k < l:
                next_indices.append(0)
            elif indices == next_indices:
                break
            indices = next_indices
            k += 1
        return False

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
    ("String OR string first present middle", "a(bcd|efg)h", "dshdlhabcdhcjkldfh", True),
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
    if match_res != test_bool:
        print "FAILING:", test_name, test_re, test_str, match_res
MyRegex("a(bcd|efg)*hi?jk").state_machine()
