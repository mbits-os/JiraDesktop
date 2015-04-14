#!/usr/python

import os
from subprocess import call, check_output
VERSION = check_output([ "python", "version.py", "../apps/Tasks/src/version.h", "!SEMANTIC", "PROGRAM_VERSION_BUILD"]).strip()
TAG = check_output([ "python", "version.py", "../apps/Tasks/src/version.h", "!NIGHTLY"]).strip()

call(["git","add","../apps/Tasks/src/version.h"])
call(["git","commit","-m","Build for version %s" % VERSION])
call(["git","tag",TAG])