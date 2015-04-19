#!/usr/bin/python

import os, sys, json, subprocess, argparse, tempfile
from StringIO import StringIO

parser = argparse.ArgumentParser(description='Uploads the build to repo')
parser.add_argument("-d", dest="dir", default="~/www", help="root directory")
parser.add_argument("-s", dest="server", required=True, help="machine name")

args   = parser.parse_args()
server = args.server
dest   = args.dir
if dest[len(dest)-1] == '/': dest = dest[0:len(dest)-1]

def call(*args):
	ret = subprocess.call(args)
	if ret: exit(ret)
	
def remote(*args):
	global server
	args = ["ssh", server] + list(args)
	call(*args)

def check_output(*args):
	try: return subprocess.check_output(args).strip()
	except subprocess.CalledProcessError, e:
		print e
		exit(e.returncode)

def Version():
	return check_output("python", "version.py", "--in", "../apps/Tasks/src/version.h", "!SEMANTIC")

def Build():
	return check_output("python", "version.py", "--in", "../apps/Tasks/src/version.h", "{PROGRAM_VERSION_BUILD}")

def ShortVersion():
	return check_output("python", "version.py", "--in", "../apps/Tasks/src/version.h",
		"{PROGRAM_VERSION_MAJOR}.{PROGRAM_VERSION_MINOR}.{PROGRAM_VERSION_PATCH}{PROGRAM_VERSION_STABILITY}")

def JiraVersion():
	return check_output("python", "version.py", "--in", "../apps/Tasks/src/version.h",
		"{PROGRAM_VERSION_MAJOR}.{PROGRAM_VERSION_MINOR}")


packages = [{
	"package": "tasks",
	"platforms": {
		"win32": {
			"archive": ["zip", "msi"],
			"logs": ["", "msbuild"]
		}
	},
	"version": Version(),
}]

files = []
relnotes = []

if os.path.exists("notes.ans"):
	cmd = ["notes.py", "@notes.ans", "-v" + JiraVersion(), "-otasks-%s-notes.txt" % Version()]
	print "$", " ".join(cmd)
	call("python", *cmd)

for package in packages:
	relnote = "{package}-{version}-notes.txt".format(**package)
	if os.path.exists(relnote):
		relnotes.append(relnote)
	for platform in package["platforms"]:
		if platform == "no-arch":
			base = "{package}-{version}".format(**package)
		else:
			base = "{package}-{version}-{0}".format(platform, **package)
		platform = package["platforms"][platform]
		for arch in platform["archive"]:
			files.append("%s.%s" % (base, arch))
		for log in platform["logs"]:
			if log == "":
				files.append("%s.log" % base)
			else:
				files.append("%s-%s.log" % (base, log))

files.sort()
ftmp = []
for file in files:
	if not os.path.exists(file):
		continue
	ftmp.append(file)
files = ftmp

version = ShortVersion()
build = Build()

links = [
	("builds", "latest", build),
	("releases/%s" % version, build, "../../builds/%s" % build),
	("releases/%s" % version, "latest", build),
	("releases", "latest", "%s/%s" % (version, build))
]

dirs = ["%s/builds/%s" % (dest, build), "%s/releases/%s" % (dest, version)]

commands = []
commands.append(["mkdir", "-p"] + dirs)
for link in links:
	commands.append(["cd", "%s/%s;" % (dest, link[0]), "rm", "-f", "%s;" % link[1], "ln", "-s", link[2], link[1]])
for cmd in commands:
	print "$", " ".join(cmd)
	remote(*cmd)

scp = ["scp"] + files + ["%s:%s/builds/%s/" % (server, dest, build)]
print "$", " ".join(scp)
call(*scp)

cat = ["cat", ">%s/builds/%s/version.json" % (dest, build)]
with tempfile.TemporaryFile() as tmp:
	print "$", " ".join(cat)

	json.dump(packages, tmp, sort_keys=True)
	tmp.seek(0)

	ret = subprocess.call(["ssh", server] + cat, stdin=tmp)
	if ret: exit(ret)

if len(relnotes):
	cat = ["cat", ">%s/builds/%s/release-notes.txt" % (dest, build)]
	print "$", " ".join(cat)
	if len(relnotes) == 1:
		with open(relnotes[0]) as notef:
			ret = subprocess.call(["ssh", server] + cat, stdin=notef)
			if ret: exit(ret)
	else:
		with tempfile.TemporaryFile() as tmp:
			for fname in relnotes:
				print >>tmp, "###", fname
				with open(fname) as notef:
					tmp.writelines(notef.readlines())

			tmp.seek(0)

			ret = subprocess.call(["ssh", server] + cat, stdin=tmp)
			if ret: exit(ret)
