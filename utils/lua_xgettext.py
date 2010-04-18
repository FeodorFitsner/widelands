#!/usr/bin/env python
# encoding: utf-8

from collections import defaultdict
import re

from confgettext import head


class Lua_GetText(object):
    _SIMPLE_STRING = re.compile(
        r'''_\s*
        (?P<paren>[(])?\s*        #  opening parenthesis?
        (?P<all_text>(
            ((?P<quote>["'])|(?P<doublep>\[\[))  #  opening string mark?
            (?P<text>.*?(?<!\\))
            (?(quote)(?P=quote)|\]\])\s*
            (?(paren)(?P<concat>\.\.\s*))?
        )+)
        (?(paren)\)) # if opening parenthese was found, find closing one
        ''', re.M | re.DOTALL | re.VERBOSE
    )
    _CONCATENATION = re.compile(
        r'''(['"]|(\]\]))\s*\.\.\s*(['"]|\[\[)?  # a " .. ' continuation
        ''', re.M | re.DOTALL | re.VERBOSE
    )
    def __init__(self):
        self.findings = defaultdict(list)

    def parse(self, contents, filename):
        for m in self._SIMPLE_STRING.finditer(contents):
            if m.group("concat"):
                text = m.group("all_text").strip()
                text = self._CONCATENATION.subn("", text)[0]
                if text.startswith('[['): text = text[2:]
                elif text[0] in '\'"': text = text[1:]
                if text.endswith(']]'): text = text[:-2]
                elif text[-1] in '\'"': text = text[:-1]
                start = m.start('paren') or m.start("all_text")
            else:
                text = m.group("text")
                start = m.start('text')

            # Lua uses the same escaping as python. Let's use this to our
            # advantage.
            text = eval('str("""%s""")' % text)

            self.findings[text].append(
                (filename, contents[:start].count('\n') + 1)
            )

    @property
    def found_something_to_translate(self):
        return len(self.findings.keys()) > 0

    def merge(self, other_findings):
        for key in other_findings:
            self.findings[key].extend(other_findings[key])

    def __str__(self):
        s = head

        for string in sorted(self.findings.keys()):
            occurences = self.findings[string]
            occurences.sort( lambda o1,o2: cmp(o1[0], o2[0])) # Sort by filename

            for filename, lineno in occurences:
                s += "#: %s:%i\n" % (filename, lineno)
            string = string.replace('\\', '\\\\').replace('"', '\\"')
            if not s.count('\n'):
                s += 'msgid "%s"\n' % string
            else:
                s += 'msgid ""\n'
                lines = string.splitlines()
                s += ''.join('"%s\\n"\n' % l for l in lines[:-1])
                s += '"%s"\n' % lines[-1]
            s += 'msgstr ""\n\n'

        return s

def gettext(text, filename):
    t = Lua_GetText()
    t.parse(text, filename)

    return str(t)


if __name__ == '__main__':
    import sys
    t = Lua_GetText()

    for fn in sys.argv[1:]:
        t.parse(open(fn).read(), fn)

    print t