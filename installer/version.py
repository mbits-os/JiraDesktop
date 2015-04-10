#!/usr/python

import os, sys, re
from os import path
from subprocess import call

bump = None

if len(sys.argv) < 3:
	print "version.py <file> <fields> (<field-to-bump>)"
	exit(1)

fname = sys.argv[1]
fields = sys.argv[2].split(',')

if len(sys.argv) > 3:
	bump = sys.argv[3]

lines = []
with open(fname) as f:
	lines = f.readlines()

save = False
vars = {}
for i in range(len(lines)):
	m = re.match("^\#\s*define\s+([^ ]+)\s+(.*)$", lines[i])
	if m and m.group(1) in fields:
		fld = m.group(1)
		val = m.group(2)
		rest = val
		if len(rest) and rest[:1] == '"':
			n = re.match('"([^"]+)"', rest)
			if not n: continue
			val = n.group(1)
		else:
			n = re.match('([0-9]+)', rest)
			if not n: continue
			val = int(n.group(1))
			if bump is not None and fld == bump:
				val = val + 1
				split = re.match("^(\#\s*define\s+[^ ]+\s+)([0-9]+)", lines[i])
				newLine = "%s%s%s" % (split.group(1), val, lines[i][len(split.group(0)):])
				lines[i] = newLine
				save = True
		vars[fld] = val

if save:
	with open(fname, "w") as f:
		f.write("".join(lines))

first = True
for key in fields:
	if key in vars:
		val = vars[key]
		if isinstance(val, int):
			if first: first = False
			else: sys.stdout.write(".")
			sys.stdout.write("%s" % val)
		else: sys.stdout.write(val)
print