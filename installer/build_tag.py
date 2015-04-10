#!/usr/python

import os
from subprocess import call, check_output
ver = check_output([ "python", "version.py", "../apps/Tasks/src/version.h",
	"PROGRAM_VERSION_MAJOR,PROGRAM_VERSION_MINOR,PROGRAM_VERSION_PATCH,PROGRAM_VERSION_BUILD",
	"PROGRAM_VERSION_BUILD"])
VERSION = ver.strip()

call(["git","add","../apps/Tasks/src/version.h"])
call(["git","commit","-m","Build for version %s" % VERSION])
call(["git","tag","v%s" % VERSION])