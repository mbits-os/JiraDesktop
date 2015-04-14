#!/usr/python

import os, sys, subprocess, _winreg, argparse, tempfile

out = tempfile.NamedTemporaryFile(prefix='tasks-tmp-', suffix='-win32.log', dir='.', delete=False)

def present(args):
	global out
	out.write("$ %s\n" % " ".join(args))
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
		"!SEMANTIC"]).strip()
	return ver.strip()

def Stability():
	ver = subprocess.check_output([ "python", "version.py", "../apps/Tasks/src/version.h", "PROGRAM_VERSION_STABILITY"])
	return ver.strip()

def Branch():
	try:
		var = subprocess.check_output([ "git", "rev-parse", "--abbrev-ref", "HEAD"]).strip()
		return var
	except subprocess.CalledProcessError, e: return ""

STABILITY = Stability()

POLICY_TAG_AND_PUSH = 0
POLICY_TAG = 1
POLICY_REUSE = 2

class PushAction(argparse.Action):
	def __call__(self, parser, namespace, values, option_string=None):
		setattr(namespace, self.dest, POLICY_REUSE)
		setattr(namespace, "tag", values)

parser = argparse.ArgumentParser(description='Prepare automated build of JiraDesktop.', usage='%(prog)s [options]')
group = parser.add_mutually_exclusive_group()
group.add_argument("-n, --no-push", dest="push", action="store_const", const=POLICY_TAG, default=POLICY_TAG_AND_PUSH,
					help="Bumps build number on current master and builds the solution")
group.add_argument("-p, --push", dest="push", action="store_const", const=POLICY_TAG_AND_PUSH, default=POLICY_TAG_AND_PUSH,
					help="Like --push, but pushes the version tag to the origin")
group.add_argument("-u, --use", dest="push", metavar="TAG", nargs="?", action=PushAction, default=POLICY_TAG_AND_PUSH,
					help="Builds solution from pre-existing tag. If not TAG is given, one is calculated from 'version.h'")
parser.add_argument("--dry-run", action="store_true", default=False,
					help="If present, will only print out the steps that would be otherwise taken")

args = parser.parse_args()

policy = args.push
tag = None

if policy == POLICY_REUSE:
	tag = args.tag

if policy == POLICY_TAG:
	print >>out, "[WARNING]\n>> no push, when possible call 'git push --tags origin master' <<"
	
class Step:
	def __init__(self, title, callable, *args):
		self.title = title
		self.callable = callable
		self.args = args
	def __call__(self, pos, max):
		print "Step %s/%s %s" % (pos, max, self.title)
		print >>out, "\n[%s]" % self.title
		self.callable(*self.args)

def tag_master():
	if Branch() == "master":
		call("git", "pull", "--rebase")
	else:
		call("git", "checkout", "master")
	call("python", "build_tag.py")

	VERSION = Version()
	TAG = "v" + VERSION
	print >>out, "Tagged as '%s'" % TAG

def switch_log():
	global out, VERSION, STABILITY
	LOGFILE = "tasks-%s%s-win32.log" % (VERSION, STABILITY)
	previous = out.name
	out.flush()
	out.close()
	if os.path.exists(LOGFILE):
		os.remove(LOGFILE)
	os.rename(previous, LOGFILE)
	out = open(LOGFILE, "a+b")
	out.seek(os.SEEK_END)

class pushd:
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
	
def build_sln():
	switch_log() # by now, we know the version to use
	with pushd(".."):
		call_("rm", "-rf", "int")
		call_("rm", "-rf", "bin")
		with pushd("build/win32"):
			msbuild = MSBuildPath()
			if msbuild:
				os.environ["PATH"] += os.pathsep + msbuild
			path = "..\\..\\installer\\tasks-%s%s-win32-msbuild.log" % (VERSION, STABILITY)
			call_direct("msbuild", "/nologo",
				"/flp:LogFile=" + path,
				"/noconlog", "/m", "/p:Configuration=Release",
				"JiraDesktop.sln")

def nothing(): pass

prog = []
prog.append(Step("Updating the tree", call, "git", "fetch", "--tags", "-v"))

if policy == POLICY_REUSE:
	VERSION = Version()
	if tag is not None: TAG = tag
	else: TAG = "v" + VERSION

	prog.append(Step("Using '%s'" % TAG, call, "git", "checkout", TAG))
else:
	prog.append(Step("Tagging 'master'", tag_master))

if policy == POLICY_TAG_AND_PUSH:
	prog.append(Step("Pushing to 'origin'", call, "git", "push", "--tags", "origin", "master"))

prog.append(Step("Getting dependencies", call, "python", "copy_res.py"))
prog.append(Step("Building 'Release:Win32'", build_sln))
prog.append(Step("Packing", call, "python", "pack.py"))
prog.append(Step("Done", nothing))

max = len(prog)
i = 1

if args.dry_run:
	for step in prog:
		if len(step.args):
			print "%s/%s" % (i, max), step.title, "(%s)" % " ".join([str(arg) for arg in step.args])
		else:
			print "%s/%s" % (i, max), step.title
		i += 1
else:
	for step in prog:
		step(i, max)
		i += 1

out.close()
