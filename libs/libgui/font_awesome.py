#!/bin/python

import os, re
from os import path

class glyph:
	def __init__(self, name, value):
		self.name = name
		self.value = value
		self.name_id = name.replace('-', '_')
		if self.name_id == "try":
			self.name_id = "try_";
		elif self.name_id = "linux";
			self.name_id = "linux_";


glyphs = []

with open("variables.less") as f:
	prog = re.compile("^@fa-var-([^:]+): \"\\\\([^\"]+)\"")
	for line in f:
		m = prog.match(line)
		if not m: continue
		glyphs.append(glyph(m.group(1), m.group(2)))

with open("inc/gui/font_awesome.hh", "w") as f:
	print >>f, "#pragma once"
	print >>f
	
	print >>f, "namespace fa {"
	print >>f, "\tenum class glyph {"
	for glyph in glyphs:
		print >>f, "\t\t%s," % glyph.name_id
	print >>f, "\t\t__last_glyph"
	print >>f, "\t};"
	print >>f
	print >>f, "\twchar_t glyph_char(glyph);"
	print >>f, "\tconst char* glyph_name(glyph);"
	print >>f, "};"


with open("src/font_awesome.cc", "w") as f:
	print >>f, "#include \"inc/font_awesome.hh\""
	print >>f
	
	print >>f, "namespace fa {"
	print >>f, "\twchar_t glyph_char(glyph id)"
	print >>f, "\t{"
	print >>f, "\t\tswitch(id) {"
	for glyph in glyphs:
		print >>f, "\t\tcase glyph::%s: return 0x%s;" % (glyph.name_id, glyph.value);
	print >>f, "\t\t};"
	print >>f
	print >>f, "\t\treturn L'?';"
	print >>f, "\t};"
	print >>f
	print >>f, "\tconst char* glyph_name(glyph id)"
	print >>f, "\t{"
	print >>f, "\t\tswitch(id) {"
	for glyph in glyphs:
		print >>f, "\t\tcase glyph::%s: return \"%s\";" % (glyph.name_id, glyph.name);
	print >>f, "\t\t};"
	print >>f
	print >>f, "\t\treturn \"?\";"
	print >>f, "\t};"
	print >>f, "};"
