#!/usr/bin/python

import sys, os, subprocess, winreg

def MSBuildPath():
	with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\14.0") as key:
		return winreg.QueryValueEx(key, "MSBuildToolsPath")[0]

def call(*args):
	return subprocess.call(args)

msbuild = MSBuildPath()
if msbuild:
	sys.stdout.write("-- MSBuild: {}\n".format(msbuild))
	os.environ["PATH"] += os.pathsep + msbuild

exit(call("msbuild", "/nologo", "/flp:LogFile=msbuild.log", "/clp:Verbosity=minimal", "/m", "/p:Configuration=Release", "JiraDesktop.sln"))
