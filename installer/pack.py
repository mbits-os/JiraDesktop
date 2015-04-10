#!/usr/python

import os, shutil, glob, zipfile, _winreg
from os import path
from subprocess import call, check_output

MSVCRT = False

WiX = "c:\\Program Files (x86)\\WiX Toolset v3.9\\bin"

ROOT = ".."
PLATFORM = "Win32"
CONFIG = "Release"
VERSION = "1.0.2a"

try:
	ver = check_output([ "python", "version.py", "../apps/Tasks/src/version.h",
		"PROGRAM_VERSION_MAJOR,PROGRAM_VERSION_MINOR,PROGRAM_VERSION_PATCH,PROGRAM_VERSION_BUILD,PROGRAM_VERSION_STABILITY"])
	VERSION = ver.strip()
except:
	pass

ZIPNAME = "tasks-%s-%s.zip" % (VERSION, PLATFORM.lower())
MSINAME = "tasks-%s-%s.msi" % (VERSION, PLATFORM.lower())
WXS = "Tasks.wxs"
WIXOBJ = "Tasks.wixobj"

ARTIFACTS = "artifacts"
RES = "res"

artifactFiles = [ "Tasks.exe" ]
resFiles = [ "LICENSE", "apps/Tasks/res/Tasks.ico" ]

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
		call([ "signtool.exe", "sign", "/a", "/t", "http://timestamp.verisign.com/scripts/timstamp.dll", "-d", "Tasks %s" % VERSION, path.join(ARTIFACTS, f) ])

print ZIPNAME

try:
	os.unlink(ZIPNAME)
except:
	pass

with zipfile.ZipFile(ZIPNAME, 'w') as zip:
	for key in storage:
		dir = storage[key]
		for f in glob.iglob(path.join(key, "*")):
			dest = f[len(key)+1:]
			if dir != "":
				dest = path.join(dir, dest)
			zip.write(f, dest)

msvcrtDef=[]
if MSVCRT: msvcrtDef = ['-dMSVCRT']

call(["candle", "-nologo", WXS] + msvcrtDef)
call(["light", "-nologo", "-sice:ICE07", "-sice:ICE60", "-ext", "WixUIExtension", WIXOBJ, "-out", MSINAME])
call(["signtool", "sign", "/a", "/t", "http://timestamp.verisign.com/scripts/timstamp.dll", "-d", "Tasks %s Installer" % VERSION, MSINAME])
