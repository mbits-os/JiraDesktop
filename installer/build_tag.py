#!/usr/bin/python

import os
from subprocess import call, check_output
VERSION = check_output([ "python", "version.py", "--in", "../apps/Tasks/src/version.h", "-b", "PROGRAM_VERSION_BUILD", "!SEMANTIC"]).strip()
TAG = check_output([ "python", "version.py", "--in", "../apps/Tasks/src/version.h", "!NIGHTLY"]).strip()

call(["git","add","../apps/Tasks/src/version.h"])
call(["git","commit","-m","Build for version %s" % VERSION])
call(["git","tag",TAG])