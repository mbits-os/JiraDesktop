#!/usr/bin/python

import os, sys, urlparse, subprocess, json

def check_output(*args):
	try: return subprocess.check_output(args).strip()
	except subprocess.CalledProcessError, e:
		print e
		exit(e.returncode)
		
tag = sys.argv[1]

long = check_output("git", "rev-parse", "--verify", "refs/tags/" + tag)
short = check_output("git", "rev-parse", "--short", "--verify", "refs/tags/" + tag)
commit = "%s,%s" % (short, long[len(short):])

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
	url = urlparse.urlparse(pull)
	if url.netloc == 'github.com':
		github = url.path[1:]
if github and github.endswith(".git"):
	github = github[0:len(github)-4]

print tag
print commit
print github

build = {
	"name" : tag,
	"tag" : tag,
	"commit" : commit,
	"files": [{
		"package": "tasks",
		"version": version(short),
		"platforms": {
			"win32": { "archive": ["zip", "msi"], "logs": ["", "msbuild"] }
		}
	}]
}

if github: build["github"] = github

json.dump(build, sys.stdout, indent=4)
