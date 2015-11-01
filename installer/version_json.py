#!/usr/bin/python

import os, sys, argparse, urllib.parse, subprocess, json

parser = argparse.ArgumentParser(description='version.json generator')
parser.add_argument('-t', dest='tag', required=True, help='chooses tag to generate version.json for')
parser.add_argument('-o', dest='fname', help='selects the output file; defaults to stdout')

args = parser.parse_args()

def check_output(*args):
	try: return subprocess.check_output(args).strip()
	except subprocess.CalledProcessError as e:
		print(e)
		exit(e.returncode)
		
tag = args.tag

long_commit = check_output("git", "rev-parse", "--verify", "refs/tags/" + tag)
short_commit = check_output("git", "rev-parse", "--short", "--verify", "refs/tags/" + tag)
commit = "%s,%s" % (short_commit, long_commit[len(short_commit):])

def version(sha):
	name = "version-%s.h" % sha
	contents = check_output("git", "show", "%s:apps/Tasks/src/version.h" % sha)
	with open(name, "w+b") as f:
		f.write(contents)
	out = check_output("python", "version.py", "--in", name, "!SEMANTIC")
	os.remove(name)
	return out

pull = check_output("git", "ls-remote", "--get-url", "origin")
github = None
if 'git@github.com:' == pull[0:15]:
	github = pull[15:]
else:
	url = urllib.parse.urlparse(pull)
	if url.netloc == 'github.com':
		github = url.path[1:]
if github and github.endswith(".git"):
	github = github[0:len(github)-4]

ver = version(short)
build = {
	"name" : tag,
	"tag" : tag,
	"commit" : commit,
	"files": [{
		"package": "tasks",
		"version": ver,
		"platforms": {
			"win32": { "archive": ["zip", "msi"], "logs": ["", "msbuild"] }
		}
	}]
}

if github: build["github"] = github

ver = ver.split('+', 1)[0].split('-', 1)
if len(ver) > 1:
	ver = ver[1]
	stability = ' '.join(ver.split('.', 1))
	if len(stability): build["stability"] = stability

if args.fname:
	with open(args.fname, "w+b") as out:
		json.dump(build, out, separators=(',',':'))
else:
	json.dump(build, sys.stdout, separators=(',',':'))
