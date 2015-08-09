#!/usr/python

import os, zipfile, libpack as lib
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

WXS_SOURCES = "Tasks.sources.wxs"
WIXOBJ_SOURCES = "Tasks.sources.wixobj"

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

sdk = lib.win_sdk()
if sdk is not None:
	os.environ["PATH"] += os.pathsep + sdk

os.environ["PATH"] += os.pathsep + WiX

if not path.exists(ARTIFACTS) : os.makedirs(ARTIFACTS)
if not path.exists(RES) : os.makedirs(RES)

lib.copyFilesFlat(path.join(ROOT, "bin", PLATFORM, CONFIG), ARTIFACTS, artifactFiles)
lib.copyFilesFlat(ROOT, RES, resFiles)

# sign
if sdk is not None:
	for f in artifactFiles:
		call([ "signtool.exe", "sign", "/a", "/t", "http://timestamp.verisign.com/scripts/timstamp.dll", "-d", "Tasks %s" % SIGNVER, path.join(ARTIFACTS, f) ])

contents = lib.file_list(storage)

print ZIPNAME

try:
	os.unlink(ZIPNAME)
except:
	pass

with zipfile.ZipFile(ZIPNAME, 'w') as zip:
	for source, dest in contents:
		zip.write(source, dest)

cabs = [
	[ARTIFACTS, RES, "fonts"],
	["libcurl"],
	["msvcrt"]
]

comps = {
	(1, "") : ("Main.Component", "0BF923DF-8E07-40F6-B937-02B2FF523DC0"),
	(2, "") : ("Curl.Component", "C40018C7-B3EA-4B2F-8D1F-307F01324CD7"),
	(3, "") : ("MSVCRT.Component", "6D42C511-CF7F-4480-A5FE-5FC3A0C3FF5D"),
	(1, "fonts") : (None, "7D0F0F45-4967-4E2D-B903-421763CF3CFF"),
	(1, "locale") : (None, "32BD8778-F9E8-4613-9291-5518711547C2")
}

links = {
	"Tasks.exe": [
		("Tasks", "ProgramMenuFolder", "Tasks.ico", 0),
		("Tasks", "DesktopFolder", "Tasks.ico", 0)
	]
}

with open(WXS_SOURCES, 'w') as wsx:
	lib.msi_fragment(cabs, comps, contents).print_fragment(wsx, links)

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
call(["candle", "-nologo", WXS_SOURCES])
call(["light", "-nologo", "-sice:ICE07", "-sice:ICE60", "-sice:ICE61", "-ext", "WixUIExtension", WIXOBJ, WIXOBJ_SOURCES, "-out", MSINAME])
call(["signtool", "sign", "/a", "/t", "http://timestamp.verisign.com/scripts/timstamp.dll", "-d", "Tasks %s Installer" % SIGNVER, MSINAME])
