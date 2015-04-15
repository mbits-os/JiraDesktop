#!/usr/python

import os, sys, re, argparse
from os import path
from subprocess import call

macros = {
	"!SEMANTIC" : "{PROGRAM_VERSION_MAJOR}.{PROGRAM_VERSION_MINOR}.{PROGRAM_VERSION_PATCH}{PROGRAM_VERSION_STABILITY}+{PROGRAM_VERSION_BUILD}",
	"!NIGHTLY" : "v{PROGRAM_VERSION_MAJOR}.{PROGRAM_VERSION_MINOR}.{PROGRAM_VERSION_PATCH}/{PROGRAM_VERSION_BUILD}"
}

parser = argparse.ArgumentParser(description='Extract version info from version.h', usage='%(prog)s --in FILE [-b VALUE | -n VALUE] [-m | FORMAT]')
parser.add_argument("--in", dest="fname", metavar="FILE", required=True, help="selects the filename to use")
group = parser.add_mutually_exclusive_group()
group.add_argument("-b", "--bump", metavar="VALUE", help="if present, will bump the value, and write it back to the header")
group.add_argument("-n", "--next", metavar="VALUE", help="if present, will bump the value, but will not write it back to the file")
parser.add_argument("-m", "--macros", action="store_true", help="prints known macros and their values")
parser.add_argument("format", metavar="FORMAT", nargs='?', help="either Python format or !MACRO. For format see https://docs.python.org/2/library/string.html#format-string-syntax")

args = parser.parse_args()
if not (args.macros or args.format):
	parser.error('one of -m/--marcos or FORMAT is required')

if args.macros and args.format:
	parser.error('argument -m/--macros: not allowed with argument FORMAT')

if args.macros and args.bump:
	parser.error('argument -m/--macros: not allowed with argument -b/--bump')

bump = args.bump
if not bump: bump = args.next

lines = []
with open(args.fname) as f:
	lines = f.readlines()

save = False
vars = {}
for i in range(len(lines)):
	m = re.match("^\#\s*define\s+([^ ]+)\s+(.*)$", lines[i])
	if m:
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
				save = args.bump
		vars[fld] = val

if args.macros:
	out   = []
	sizes = [0, 0, 0]
	keys  = macros.keys()
	keys.sort()

	header = ("NAME", "OUTPUT", "VALUE")
	for i in range(len(sizes)):
		sizes[i] = len(header[i])

	for key in keys:
		tup = (key, macros[key].format(**vars), macros[key])
		for i in range(len(sizes)):
			if len(tup[i]) > sizes[i]:
				sizes[i] = len(tup[i])
		out.append(tup)

	fmt = "|"
	for i in range(len(sizes)):
		fmt += " {%s:<%s} |" % (i, sizes[i])

	line = "+"
	for i in range(len(sizes)):
		line += "-" * sizes[i] + "--+"

	print line
	print fmt.format(*header)
	print line
	for tup in out:
		print fmt.format(*tup)
	print line
	exit(0)

if save:
	with open(args.fname, "w") as f:
		f.write("".join(lines))

fields = args.format
if fields in macros:
	fields = macros[fields]

sys.stdout.write(fields.format(**vars))
