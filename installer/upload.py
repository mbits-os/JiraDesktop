#!/usr/bin/python

import os, sys, json, subprocess, argparse, tempfile
from StringIO import StringIO

parser = argparse.ArgumentParser(description='Uploads the build to repo', fromfile_prefix_chars='@')
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
rename = {}

for package in packages:
	relnote = "{package}-{version}-notes.txt".format(**package)
	if os.path.exists(relnote):
		if 'release-notes.txt' not in rename: rename['release-notes.txt'] = []
		rename['release-notes.txt'].append(relnote)

	version = "{package}-{version}-version.json".format(**package)
	if os.path.exists(version):
		if 'version.json' not in rename: rename['version.json'] = []
		rename['version.json'].append(version)

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

dirs = ["%s/builds/%s" % (dest, build), "%s/releases/%s" % (dest, version), "%s/ui" % dest]
cmd = ["mkdir", "-p"] + dirs

print "$", " ".join(cmd)
remote(*cmd)

scp = ["scp"] + files + ["%s:%s/builds/%s/" % (server, dest, build)]
print ">", " ".join(scp)
call(*scp)

for dst in rename:
	if not len(rename[dst]): continue
	scp = ["scp", rename[dst][0], "%s:%s/builds/%s/%s" % (server, dest, build, dst)]
	print ">", " ".join(scp)
	call(*scp)

scp = ["scp", "www/ui/index.html", "www/ui/pages.css", "%s:%s/ui/" % (server, dest)]
print ">", " ".join(scp)
call(*scp)

scp = ["scp", "www/index.py", "%s:~/" % server]
print ">", " ".join(scp)
call(*scp)

commands = []
for link in links:
	commands.append(["cd", "%s/%s;" % (dest, link[0]), "rm", "-f", "%s;" % link[1], "ln", "-s", link[2], link[1]])
commands.append(["python", "~/index.py", "-d", dest, "-t", "update", "-#", build])
for cmd in commands:
	print "$", " ".join(cmd)
	remote(*cmd)
