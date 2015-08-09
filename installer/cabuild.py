#!/usr/python

import os, sys, subprocess, _winreg

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

def MSBuildPath():
	with _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\14.0") as key:
		return _winreg.QueryValueEx(key, "MSBuildToolsPath")[0]

def call(*args):
	return subprocess.call(args)

msbuild = MSBuildPath()
if msbuild:
	os.environ["PATH"] += os.pathsep + msbuild

cwd = os.getcwd()
with pushd("custom"):
	call("rm", "-rf", "int")
	call("rm", "-rf", "bin")
	with pushd("build"):
		exit(call("msbuild", "/nologo", "/clp:Verbosity=minimal", "/m", "/p:Configuration=Release", "TasksCustomActions.sln"))
