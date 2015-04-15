#!/usr/python

from libbuild import *
import os, sys, _winreg, argparse, tempfile

out = tempfile.NamedTemporaryFile(prefix='tasks-tmp-', suffix='-win32.log', dir='.', delete=False)

def MSBuildPath():
	with _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\14.0") as key:
		return _winreg.QueryValueEx(key, "MSBuildToolsPath")[0]

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
					help="Like --no-push, but pushes the version tag to the origin")
group.add_argument("-u, --use", dest="push", metavar="TAG", nargs="?", action=PushAction, default=POLICY_TAG_AND_PUSH,
					help="Builds solution from pre-existing tag. If not TAG is given, one is calculated from 'version.h'")
parser.add_argument("--dry-run", action="store_true", default=False,
					help="If present, will only print out the steps that would be otherwise taken")

args = parser.parse_args()

policy = args.push
tag = None

if policy == POLICY_REUSE:
	tag = args.tag

if policy == POLICY_TAG and not args.dry_run:
	print >>out, "[WARNING]\n>> no push, when possible call 'git push --tags origin master' <<"

def tag_master(out):
	global VERSION, TAG
	if Branch() == "master":
		call("git", "pull", "--rebase")
	else:
		call("git", "checkout", "master")
	call("python", "build_tag.py")

	VERSION = Version()
	TAG = Tag()
	print >>out, "Tagged as '%s'" % TAG

def switch_log(unused):
	global out, VERSION
	LOGFILE = "tasks-%s-win32.log" % VERSION
	previous = out.name
	out.flush()
	out.close()
	if os.path.exists(LOGFILE):
		os.remove(LOGFILE)
	os.rename(previous, LOGFILE)
	out = open(LOGFILE, "a+b")
	out.seek(os.SEEK_END)

def build_sln(out):
	global VERSION
	call(out, "python", "msbuild.py", "tasks-%s-win32-msbuild.log" % VERSION)

def nothing(out): pass

prog = Steps()
prog.step("Updating the tree", call, "git", "fetch", "--tags", "-v")

if policy == POLICY_REUSE:
	VERSION = Version()
	if tag is not None: TAG = tag
	else: TAG = Tag()

	prog.step("Using '%s'" % TAG, call, "git", "checkout", TAG)
else:
	prog.step("Tagging 'master'", tag_master)

if policy == POLICY_TAG_AND_PUSH:
	prog.step("Pushing to 'origin'", call, "git", "push", "--tags", "origin", "master")

# by now, we know the version to use
prog.hidden(switch_log) \
	.step("Getting dependencies", call, "python", "copy_res.py") \
	.step("Building 'Release:Win32'", build_sln) \
	.step("Packing", call, "python", "pack.py") \
	.step("Done", nothing)

prog.run(out, args.dry_run)

out.close()
