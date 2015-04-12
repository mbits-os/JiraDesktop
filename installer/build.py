#!/usr/python

import os, sys, subprocess, _winreg, argparse

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
	except subprocess.CalledProcessError, e:
		if e.output is not None:
			out.write(e.output)
		print out.content
		print e
		exit(e.returncode)

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

def Branch():
	try:
		var = subprocess.check_output([ "git", "rev-parse", "--abbrev-ref", "HEAD"]).strip()
		return var
	except subprocess.CalledProcessError, e: return ""

POLICY_TAG_AND_PUSH = 0
POLICY_TAG = 1
POLICY_REUSE = 2

class PushAction(argparse.Action):
	def __call__(self, parser, namespace, values, option_string=None):
		setattr(namespace, self.dest, POLICY_REUSE)
		setattr(namespace, "tag", values)

parser = argparse.ArgumentParser(description='Prepare automated build of JiraDesktop.', usage='%(prog)s [options]')
group = parser.add_mutually_exclusive_group()
group.add_argument("--no-push", dest="push", action="store_const", const=POLICY_TAG, default=POLICY_TAG_AND_PUSH,
					help="Bumps build number on current master and builds the solution")
group.add_argument("--push", dest="push", action="store_const", const=POLICY_TAG_AND_PUSH, default=POLICY_TAG_AND_PUSH,
					help="Like --push, but pushes the version tag to the origin")
group.add_argument("--use", dest="push", metavar="TAG", nargs="?", action=PushAction, default=POLICY_TAG_AND_PUSH,
					help="Builds solution from pre-existing tag. If not TAG is given, one is calculated from 'version.h'")

args = parser.parse_args()

policy = args.push
tag = None

if policy == POLICY_REUSE:
	tag = args.tag

if policy == POLICY_TAG:
	print >>out, "[WARNING]\n>> no push, when possible call 'git push --tags origin master' <<" % arg

print "Step 1/5 Updating the tree"
print >>out, "\n[Updating the tree]"

call_simple("git", "fetch", "--tags", "-v")

if policy == POLICY_REUSE:
	VERSION = Version()
	if tag is not None: TAG = tag
	else: TAG = "v" + VERSION

	print "Step 2/5 Using '%s'" % TAG
	print >>out, "\n[Using '%s']" % TAG

	call_simple("git", "checkout", TAG)
else:
	print "Step 2/5 Tagging 'master'"
	print >>out, "\n[Tagging 'master']"

	if Branch() == "master":
		call_simple("git", "pull", "--rebase")
	else:
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

if policy == POLICY_TAG_AND_PUSH:
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
