#!/usr/python

import os, shutil, glob, zipfile, _winreg
from os import path
from subprocess import call, check_output

MSVCRT = False

WiX = "c:\\Program Files (x86)\\WiX Toolset v3.10\\bin"
VERSION_H = "../apps/Tasks/src/version.h"

ROOT = ".."
PLATFORM = "Win32"
CONFIG = "Release"
VERSION = None

VERSION_MAJOR = None
VERSION_MINOR = None
VERSION_PATCH = None
VERSION_BUILD = None
COMPANY = None
PACKAGE = None

try:
	VERSION = check_output([ "python", "version.py", "--in", VERSION_H, "!SEMANTIC"]).strip()

	VERSION_MAJOR = check_output([ "python", "version.py", "--in", VERSION_H, "{PROGRAM_VERSION_MAJOR}"]).strip()
	VERSION_MINOR = check_output([ "python", "version.py", "--in", VERSION_H, "{PROGRAM_VERSION_MINOR}"]).strip()
	VERSION_PATCH = check_output([ "python", "version.py", "--in", VERSION_H, "{PROGRAM_VERSION_PATCH}"]).strip()
	VERSION_BUILD = check_output([ "python", "version.py", "--in", VERSION_H, "{PROGRAM_VERSION_BUILD}"]).strip()
	COMPANY = check_output([ "python", "version.py", "--in", VERSION_H, "{PROGRAM_COPYRIGHT_HOLDER}"]).strip()
	PACKAGE = check_output([ "python", "version.py", "--in", VERSION_H, "{PROGRAM_NAME}"]).strip()

	SIGNVER = "%s.%s.%s" % (VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)
except:
	pass

ZIPNAME = "tasks-%s-%s.zip" % (VERSION, PLATFORM.lower())
MSINAME = "tasks-%s-%s.msi" % (VERSION, PLATFORM.lower())
WXS = "Tasks.wxs"
WIXOBJ = "Tasks.wixobj"

ARTIFACTS = "artifacts"
RES = "res"

artifactFiles = [ "Tasks.exe" ]
resFiles = [ "LICENSE", "apps/Tasks/res/Tasks.ico", "locale" ]

storage = {
	ARTIFACTS: "",
	RES: "",
	"fonts": "fonts",
	"libcurl" : ""
}

if MSVCRT: storage["msvcrt"] = ""

def mtime(f):
	try:
		stat = os.stat(f)
		return stat.st_mtime
	except:
		return 0

def copyFilesFlat(src, dst, files):
	for f in files:
		s = path.join(src, f)
		d = path.join(dst, path.basename(f))
		if os.path.isdir(s):
			for root, dirs, files in os.walk(s):
				try:
					os.makedirs(d)
				except:
					pass
				copyFilesFlat(s, d, dirs + files)
				return
		s_time = mtime(s)
		d_time = mtime(d)
		if s_time > d_time:
			print d
			shutil.copy(s, d)

# HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Microsoft SDKs\Windows
def SDKs():
	sdks = {}
	with _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows") as key:
		index = 0
		while True:
			try:
				subkey = _winreg.EnumKey(key, index)
				index += 1
				with _winreg.OpenKey(key, subkey) as sub:
					sdks[subkey] = _winreg.QueryValueEx(sub, "InstallationFolder")[0]
			except:
				break
	return sdks

sdks = SDKs()
sdk = None

sdk_vers = sdks.keys()
sdk_vers.sort()
for ver in sdk_vers:
	signtool = path.join(sdks[ver], "Bin", "signtool.exe")
	if path.exists(signtool):
		sdk = path.join(sdks[ver], "Bin")

if sdk is not None:
	os.environ["PATH"] += os.pathsep + sdk

os.environ["PATH"] += os.pathsep + WiX

if not path.exists(ARTIFACTS) : os.makedirs(ARTIFACTS)
if not path.exists(RES) : os.makedirs(RES)

copyFilesFlat(path.join(ROOT, "bin", PLATFORM, CONFIG), ARTIFACTS, artifactFiles)
copyFilesFlat(ROOT, RES, resFiles)

# sign
if sdk is not None:
	for f in artifactFiles:
		call([ "signtool.exe", "sign", "/a", "/t", "http://timestamp.verisign.com/scripts/timstamp.dll", "-d", "Tasks %s" % SIGNVER, path.join(ARTIFACTS, f) ])

print ZIPNAME

try:
	os.unlink(ZIPNAME)
except:
	pass

with zipfile.ZipFile(ZIPNAME, 'w') as zip:
	for key in storage:
		dir = storage[key]
		for f in glob.iglob(path.join(key, "*")):
			if os.path.isdir(f):
				for root, dirs, files in os.walk(f):
					for p in files:
						s = os.path.join(root, p)
						dest = os.path.join(root[len(key)+1:], p)
						if dir != "":
							dest = path.join(dir, dest)
						zip.write(s, dest)
			else:
				dest = f[len(key)+1:]
				if dir != "":
					dest = path.join(dir, dest)
				zip.write(f, dest)

additional=[]
if MSVCRT: additional += ['-dMSVCRT']

if VERSION_MAJOR is not None: additional += ['-dVERSION_MAJOR=%s'%VERSION_MAJOR]
if VERSION_MINOR is not None: additional += ['-dVERSION_MINOR=%s'%VERSION_MINOR]
if VERSION_PATCH is not None: additional += ['-dVERSION_PATCH=%s'%VERSION_PATCH]
if VERSION_BUILD is not None: additional += ['-dVERSION_BUILD=%s'%VERSION_BUILD]
if COMPANY is not None:
	if " " in COMPANY: additional += ['-dCOMPANY="%s"'%COMPANY]
	else: additional += ['-dCOMPANY=%s'%COMPANY]
if PACKAGE is not None:
	if " " in PACKAGE: additional += ['-dPACKAGE="%s"'%PACKAGE]
	else: additional += ['-dPACKAGE=%s'%PACKAGE]

call(["candle", "-nologo", WXS] + additional)
call(["light", "-nologo", "-sice:ICE07", "-sice:ICE60", "-sice:ICE61", "-ext", "WixUIExtension", WIXOBJ, "-out", MSINAME])
call(["signtool", "sign", "/a", "/t", "http://timestamp.verisign.com/scripts/timstamp.dll", "-d", "Tasks %s Installer" % SIGNVER, MSINAME])
