#!/usr/python

import os, sys, subprocess, _winreg

def call(*args):
	ret = subprocess.call(args)
	if ret: exit(ret)

def call_(*args):
	return subprocess.call(args)

def MSBuildPath():
	with _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\14.0") as key:
		return _winreg.QueryValueEx(key, "MSBuildToolsPath")[0]

no_push = True
if len(sys.argv) > 1:
	for arg in sys.argv[1:]:
		if arg == "--push": no_push = False
		if arg == "--no-push": no_push = True

if no_push:
	print "\n\n\n>> no push, when possible call 'git push --tags origin master' <<<\n\n\n"

print "[Updating the tree]"
call("git", "pull")
print "[Tag on 'master']"
call("python", "build_tag.py")
if not no_push:
	call("git", "push", "--tags", "origin", "master")

class chdir:
	def __init__(self, dir):
		self.stack = os.getcwd()
		self.dir = dir
	def __enter__(self):
		print "[Enter '%s']" % self.dir
		try: os.chdir(self.dir)
		except: pass
		return self
	def __exit__(self, eType, eValue, eTrace):
		print "[Exit '%s']" % self.dir
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
		call("msbuild", "/nologo", "/m", "/p:Configuration=Release", "JiraDesktop.sln")

print "[Packing]"
call("python", "pack.py")
print "[Done]"