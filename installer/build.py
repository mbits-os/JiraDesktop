#!/usr/python

import os, sys, subprocess, _winreg

class buffer:
	def __init__(self):   self.content = ""
	def write(self, str): self.content += str
	def flush(self):      pass
		
out = buffer()

def present(args):
	global out, LOGFILE
	out.write("$_ ")
	out.write(" ".join(args))
	out.write("\n")
	out.flush()

def call(*args):
	present(args)
	ret = subprocess.call(args, stdout=out, stderr=out)
	if ret: exit(ret)
	out.flush()

def call_simple(*args):
	present(args)
	try: out.write(subprocess.check_output(args, stderr=subprocess.STDOUT))
	except subprocess.CalledProcessError, e: exit(e.returncode)

def call_direct(*args):
	present(args)
	ret = subprocess.call(args)
	if ret: exit(ret)

def call_(*args):
	return subprocess.call(args)

def MSBuildPath():
	with _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\14.0") as key:
		return _winreg.QueryValueEx(key, "MSBuildToolsPath")[0]

def Version():
	ver = subprocess.check_output([ "python", "version.py", "../apps/Tasks/src/version.h",
		"PROGRAM_VERSION_MAJOR,PROGRAM_VERSION_MINOR,PROGRAM_VERSION_PATCH,PROGRAM_VERSION_BUILD"])
	return ver.strip()

def Stability():
	ver = subprocess.check_output([ "python", "version.py", "../apps/Tasks/src/version.h", "PROGRAM_VERSION_STABILITY"])
	return ver.strip()

use_tag = False
no_push = True
if len(sys.argv) > 1:
	for arg in sys.argv[1:]:
		if arg == "--push": no_push = False
		if arg == "--no-push": no_push = True
		if arg == "--use-tag": use_tag = True
		if arg == "--tag": use_tag = False

if no_push:
	arg = " --tags"
	if use_tag:
		arg = ""
	print >>out, "[WARNING]\n>> no push, when possible call 'git push%s origin master' <<" % arg

print "Step 1/5 Updating the tree"
print >>out, "\n[Updating the tree]"

call_simple("git", "pull", "--tags", "origin", "master:master")

if use_tag:
	VERSION = Version()
	TAG = "v" + VERSION

	print "Step 2/5 Using '%s'" % TAG
	print >>out, "\n[Using '%s']" % TAG

	call_simple("git", "checkout", TAG)
else:
	print "Step 2/5 Tagging 'master'"
	print >>out, "\n[Tagging 'master']"

	call_simple("git", "checkout", "master")
	call_simple("python", "build_tag.py")

	VERSION = Version()
	TAG = "v" + VERSION
	print >>out, "Tagged as '%s'" % TAG

STABILITY = Stability()
LOGFILE = "tasks-%s%s-win32.log" % (VERSION, STABILITY)
f = open(LOGFILE, "w")
f.write(out.content)
out = f
out.flush()

if not no_push:
	call("git", "push", "--tags", "origin", "master")

class chdir:
	def __init__(self, dir):
		self.stack = os.getcwd()
		self.dir = dir
	def __enter__(self):
		try: os.chdir(self.dir)
		except: pass
		return self
	def __exit__(self, eType, eValue, eTrace):
		try: os.chdir(self.stack)
		except: pass
		return False

with chdir(".."):
	call_("rm", "-rf", "int")
	call_("rm", "-rf", "bin")
	with chdir("build/win32"):
		msbuild = MSBuildPath()
		if msbuild:
			os.environ["PATH"] += os.pathsep + msbuild
		print "Step 3/5 Building 'Release:Win32'"
		print >>out, "\n[Building 'Release:Win32']"
		path = "..\\..\\installer\\tasks-%s%s-win32-msbuild.log" % (VERSION, STABILITY)
		call_direct("msbuild", "/nologo",
			"/flp:LogFile=" + path,
			"/noconlog", "/m", "/p:Configuration=Release",
			"JiraDesktop.sln")

print "Step 4/5 Packing"
print >>out, "\n[Packing]"
call("python", "pack.py")
print "Step 5/5 Done"
print >>out, "\n[Done]"
f.close()
