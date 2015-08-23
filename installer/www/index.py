#!/usr/bin/python

import os, sys, argparse, json, markdown, urllib, re

parser = argparse.ArgumentParser(description='Creates index.html for the upload')
parser.add_argument('-d', dest='dir', required=True, help='target directory')
parser.add_argument('-t', dest='type', required=True, help='type of index (root, builds, versions, version, single, update, all)')
parser.add_argument('-#', dest='id', help='number of the build. required for the type "update"')

args = parser.parse_args()

def sizeOf(path):
	try:
		size = os.stat(path).st_size
		ratio = 1
		suffix = "B"

		if size >= 1024:
			ratio *= 1024
			suffix = "KiB"
			if size >= 1024 * 1204:
				ratio *= 1024
				suffix = "MiB"
				if size >= 1024 * 1204 * 1024:
					ratio *= 1024
					suffix = "GiB"

		if ratio == 1:
			return "%s B" % size
		else:
			size *= 10
			size /= ratio
			size = int(size)
			return "%s.%s %s" % (int(size / 10), size % 10, suffix)
	except: return ""

exts = {
	"bin" : [".exe", ".msi"],
	"zip" : [".zip", ".7z", ".tar", ".tgz", ".gz"],
	"txt" : [".txt", ".log", ".nfo"]
}

def iconClass(fname):
	ext = os.path.splitext(fname)[1]
	for klass in exts:
		if ext in exts[klass]:
			return klass
	return "file"

class NullVcs:
	def commitLink(self, commit, text) : return text
	def tagLink(self, tag, text) : return text
	def additionalShortLinks(self, build) : return []
	def additionalLinks(self, build) : return []
	def codeLink(self, build, file, line, text): return text

class GithubVcs:
	def __init__(self, project):
		self.uri = "https://github.com/" + urllib.quote(project)

	def commitLink(self, commit, text):
		return '<a href="%s/commit/%s" rel="nofollow">%s</a>' % (self.uri, urllib.quote(commit), text)

	def tagLink(self, tag, text):
		return '<a href="%s/tree/%s" rel="nofollow">%s</a>' % (self.uri, urllib.quote(tag), text)

	def additionalShortLinks(self, build):
		if "tag" not in build:
			return []
		tag = build["tag"]
		out = []
		out.append('<a href="%s/releases/tag/%s" rel="nofollow"><span class="icon icon-github"></span>see on github</a>' % (self.uri, urllib.quote(tag, '')))
		return out

	def additionalLinks(self, build):
		if "tag" not in build:
			return []
		tag = build["tag"]
		out = []
		for ext in ["zip", "tar.gz"]:
			out.append('<a href="%s/archive/%s.%s" rel="nofollow"><span class="left light icon icon-github"></span>Source code (%s)</a>' % (self.uri, urllib.quote(tag), urllib.quote(ext), ext))
		return out

	def codeLink(self, build, file, line, text):
		if 'tag' not in build:
			return text
		return '<a href="%s/blob/%s/%s#L%s">%s</a>' % (self.uri, build['tag'], file, line, text)

def public_build(build):
	return build is not None and 'public' in build and build['public']

def version_json(dir, json):
	build = {}
	build['vcs'] = NullVcs()
	build['name'] = json['name']
	if 'tag' in json: build['tag'] = json['tag']
	if 'commit' in json: build['commit'] = json['commit']
	if 'stability' in json: build['stability'] = json['stability']
	if 'public' in json: build['public'] = json['public']
	if 'github' in json: build['vcs'] = GithubVcs(json['github'])
	# TODO: elif 'bitbucket' in json: ...

	if 'tag' in build and build['name'] == build['tag']:
		m = re.match('v([0-9]+)\.([0-9]+)\.([0-9]+)/([0-9]+)', build['name'])
		if m:
			name = '%s.%s.%s' % (m.group(1), m.group(2), m.group(3))
			if 'stability' in build:
				stability = build['stability']
				if len(stability):
					name += '-%s' % '.'.join(stability.split(' '))
			if not public_build(build):
				name += '+%s' % m.group(4)
			build['name'] = name

	if 'stability' in build:
		stability = build['stability'].lower()
		if stability == 'rc' or stability[:3] == 'rc ':
			build['banner'] = 'candidate'
		else:
			build['banner'] = 'pre-release'

	packages = {}
	for pkg in json['files']:
		bin = {}
		logs = {}
		for platform in pkg['platforms']:
			if platform == 'no-arch':
				base = '{package}-{version}'.format(**pkg)
			else:
				base = '{package}-{version}-{0}'.format(platform, **pkg)

			platform = pkg['platforms'][platform]
			if 'archive' in platform:
				archkey = 'archive'
			elif 'arch' in platform:
				archkey = 'arch'
			else:
				archkey = None

			if archkey:
				for arch in platform[archkey]:
					fname = '%s.%s' % (base, arch)
					if os.path.exists(os.path.join(dir, fname)):
						bin[fname] = os.path.splitext(fname)[1][1:]
			for log in platform['logs']:
				if log == '':
					fname = '%s.log' % base
					name = 'log'
				else:
					fname = '%s-%s.log' % (base, log)
					name = '%s.log' % log
				if os.path.exists(os.path.join(dir, fname)):
					logs[fname] = name
		out = {
			'version' : pkg['version'],
			'binaries': bin,
			'logs': logs
		}
		packages[pkg['package']] = out
	build['files'] = packages
	return build

def version(dir):
	try:
		with open(os.path.join(dir, 'version.json')) as f:
			doc = json.load(f)
			return version_json(dir, doc)
	except: return None

def page_commit(build):
	if build is None or "commit" not in build: return ""
	commit = build["commit"].split(',')
	name = commit[0]
	commit = "".join(commit)
	text = '<span class="icon icon-commit"></span>%s' % name
	return build["vcs"].commitLink(commit, text)

def page_tag(build):
	if build is None or "tag" not in build: return ""
	text = '<span class="icon icon-tag"></span>%s' % build["tag"]
	return build["vcs"].tagLink(build["tag"], text)

def page_header(out, title, updir = True, links = []):
	print >>out, '<html>'
	print >>out, '<head>'
	print >>out, '<title>%s</title>' % title
	print >>out, '<meta name="viewport" content="width=device-width, initial-scale=1">'
	print >>out, '<link rel="stylesheet" type="text/css" href="/ui/pages.css" />'
	print >>out, '</head>'
	print >>out, '<body>'
	print >>out, '<div class="contents">'
	print >>out, '<h1>%s</h1>' % title
	if len(links):
		print >>out, '<ul class="page-links">'
		for link in links:
			print >>out, '<li>%s</li>' % link
		print >>out, '</ul>'
	if updir:
		print >>out, '<p class="parent-dir"><a href="../"><span class="icon icon-dir-up"></span>parent directory</a></p>'

def page_footer(out):
	print >>out, '</div>'
	print >>out, '</body>'
	print >>out, '</html>'

def page_compilation(out, build, title, issues):
	if not len(issues): return
	print >>out, '<h4>%s</h4>' % title
	print >>out, '<ul class="issues">'
	for issue in issues:
		text = '<span title="%s, line %s">%s</span>' % (issue[0], issue[1], issue[2])
		text = build['vcs'].codeLink(build, issue[0], issue[1], text)
		print >>out, '<li>%s</li>' % text
	print >>out, '</ul>'

def page_msbuild(out, dir, build):
	if build is None: return
	warnings = []
	errors = []
	for pkg in build['files']:
		for log in build['files'][pkg]['logs']:
			if not log.endswith('-msbuild.log'):
				continue
			with open(os.path.join(dir, log)) as f:
				for line in f:
					m = re.match('\s+\.\..\.\..([^(]+)\(([0-9]+)\):\s+(\S+)\s+(.*)\[[^]]+\]', line.rstrip())
					if m:
						if m.group(3) == 'warning':
							warnings.append((m.group(1).replace('\\', '/'), int(m.group(2)), m.group(4)))
						elif m.group(3) == 'error':
							errors.append((m.group(1).replace('\\', '/'), int(m.group(2)), m.group(4)))

	if len(warnings) or len(errors):
		print >>out, '<h2 class="issues">Build issues</h2>'
	page_compilation(out, build, 'Errors', errors)
	page_compilation(out, build, 'Warnings', warnings)

def page_entry(out, link, name, latest, hints = [], phony = None):
	phony_class = ""
	if phony is not None: phony_class = " phony"
	out.write('<li class="entry%s"><a href="%s/">' % (phony_class, urllib.quote(link)))
	out.write(name)
	out.write('</a>')
	if latest:
		out.write(' <span class="latest">%s</span>' % latest)
	if len(hints):
		print >>out, '<ul class="hints">'
		for hint in hints:
			if len(hint) > 2:
				out.write('<li')
				attrs = hint[2]
				for attr in attrs:
					out.write(' %s="%s"' % (attr, attrs[attr]))
				out.write('>')
			else: out.write('<li>')
			if hint[0]:
				out.write('<span class="title light">%s</span>' % hint[0])
			if len(hint) > 1 and len(hint[1]):
				print >>out, '<ul class="short-hints">'
				for short in hint[1]:
					print >>out, '<li>%s</li>' % short
				print >>out, '</ul>'
			print >>out, '</li>'
		print >>out, '</ul>'
	if phony is not None:
		print >>out, '<div class="reason"><b>Warning:</b> %s</div>' % phony
	print >>out, '</li>'

def page_files(link, refs):
	fnames = sorted(refs.keys())
	files = []
	for fname in fnames:
		files.append('<a href="%s/%s" rel="nofollow"><span class="left light icon icon-%s"></span>%s</a>' % (urllib.quote(link), urllib.quote(fname), iconClass(fname), refs[fname]))
	return files

def page_packages(out, dir, link, build, latest):
	if build is None:
		name = "build %s" % link
		if latest:
			latest = name
			name = "latest"
		page_entry(out, link, name, latest, [], "Release description is missing")
		return

	name = build["name"]
	if latest:
		latest = name
		name = "latest"
	pkgs = build["files"]
	keys = sorted(pkgs.keys())
	hints = []

	commit = page_commit(build)
	tag = page_tag(build)

	links = []
	if 'banner' in build:
		banner = build['banner']
		links.append('<span class="%s">%s</span>' % (banner, banner))
	if commit: links.append(commit)
	if tag: links.append(tag)

	links += build['vcs'].additionalShortLinks(build)

	if len(links):
		hints.append([None, links, {'class':'page-hints'}])

	if os.path.exists(os.path.join(dir, link, 'release-notes.txt')):
		hints.append(['see', ['<a href="%s/#notes"><i>release notes</i></a>' % urllib.quote(link)]])
	for key in keys:
		pkg = pkgs[key]
		bins = pkg['binaries']
		logs = pkg['logs']
		links = page_files(link, bins) + page_files(link, logs)
		hints.append([key + ' files:', links])
	if len(keys) == 1:
		hints[len(hints) - 1][0] = 'files:'

	page_entry(out, link, name, latest, hints)

def page_public_entry(out, dir, link, build):
	name = link
	latest = build["name"]
	pkgs = build["files"]
	keys = sorted(pkgs.keys())
	hints = []

	commit = page_commit(build)
	tag = page_tag(build)

	links = []
	if commit: links.append(commit)
	if tag: links.append(tag)

	links += build['vcs'].additionalShortLinks(build)

	if len(links):
		hints.append([None, links, {'class':'page-hints'}])

	page_entry(out, link, name, latest, hints)

def page_version(out, dir, link, latest):
	subs = os.listdir(dir)
	subs.sort(reverse=True)

	if latest:
		build_latest = version(dir)['files']
		name = "latest"
	else:
		build_latest = version(os.path.join(dir, "latest"))['files']
		name = link
	tmp = [build_latest[key]['version'] for key in build_latest]
	build_latest = None
	if len(tmp):
		tmp = tmp[0].split('+', 1)
		if len(tmp) > 1:
			build_latest = tmp[1]

	builds = []

	for sub in subs:
		test = os.path.join(dir, sub)
		if not os.path.isdir(test) : continue
		if sub == "latest": continue
		icon = 'dir-o'
		if sub == build_latest: icon = 'dir'
		build = '<a href="%s/%s"><span class="icon icon-%s"></span>' % (urllib.quote(link), urllib.quote(sub), icon)
		if sub == build_latest: build += '<strong>'
		build += sub
		if sub == build_latest: build += '</strong>'
		build += '</a>'
		builds.append(build)

	page_entry(out, link, name, latest, [["builds:", builds]])

def page_root_link(out, link, name, description):
	page_entry(out, link, name, False, [[description]]);

def page_build(out, title, dir):
	page_header(out, title)

	latest = version(os.path.join(dir, "latest"))['files']
	tmp = [latest[key]['version'] for key in latest]
	latest = None
	if len(tmp):
		tmp = tmp[0].split('+', 1)
		if len(tmp) > 1:
			latest = tmp[1]

	subs = os.listdir(dir)
	subs.sort(reverse=True)

	print >>out, '<ul class="subdirs">'

	for sub in subs:
		test = os.path.join(dir, sub)
		if not os.path.isdir(test) : continue
		#if sub == "latest": continue
		page_packages(out, dir, sub, version(test), sub == "latest")

	# print latest
	print >>out, '</ul>'
	page_footer(out)

def page_dls(dir, files):
	files = sorted(files)
	out = []
	for fname in files:
		out.append('<a href="%s" rel="nofollow"><small class="right light">%s</small><span class="left light icon icon-%s"></span>%s</a>' % (urllib.quote(fname), sizeOf(os.path.join(dir, fname)), iconClass(fname), fname))
	return out

def page_prev_notes(dir):
	full = os.path.abspath(dir)
	parent, here = os.path.split(full)
	try:
		if here != str(int(here)):
			return None
	except:
		return None
	here = int(here)
	for i in range(1, here):
		test1 = os.path.join(parent, str(here - i), 'release-notes.txt')
		test2 = os.path.join(parent, str(here - i), 'version.json')
		if os.path.exists(test1) and os.path.exists(test2): return test1

def issues_in(lines):
	issues = {}
	for line in lines:
		m = re.match('\s*\*\s+\[([A-Z]+)-([0-9]+)\]', line.rstrip())
		if m: issues[(m.group(1), m.group(2))] = 1
	return sorted(issues.keys())

def apply_diff(lines, path):
	new = issues_in(lines)
	old = []
	
	with open(path) as f:
		old = issues_in(f.readlines())

	added = []
	for issue in new:
		if issue not in old:
			added.append(issue)

	text = []
	for line in lines:
		m = re.match('(\s*\*\s+)(\[([A-Z]+)-([0-9]+)\]\s+.*)', line.rstrip())
		if m and (m.group(3), m.group(4)) in added:
			text.append('%s<ins>%s</ins>' % (m.group(1), m.group(2)))
		else:
			text.append(line.rstrip())

	return text

def index_single(out, dir):
	build = version(dir)
	commit = page_commit(build)
	tag = page_tag(build)

	page_links = []
	if build is not None and 'banner' in build:
		banner = build['banner']
		page_links.append('<span class="%s">%s</span>' % (banner, banner))
	if commit: page_links.append(commit)
	if tag: page_links.append(tag)

	if build is not None:
		page_links += build['vcs'].additionalShortLinks(build)
		build_name = build["name"]
	else:
		build_name = "build %s" % os.path.basename(dir)
	page_header(out, build_name, True, page_links)

	if build is not None:
		try:
			with open(os.path.join(dir, 'release-notes.txt')) as r:
				lines = r.readlines()
				prev = page_prev_notes(dir)
				if prev is not None:
					lines = apply_diff(lines, prev)
				else:
					lines = [line.rstrip() for line in lines]
				print >>out, '<h2><span id="notes">Release notes</span></h2>'
				print >>out, markdown.markdown("\n".join(lines))
		except: pass

	if not public_build(build):
		page_msbuild(out, dir, build)

	bins = []
	logs = []
	links = []
	if build is None:
		print >>out, '<h2 class="phony">Warning</h2>'
		print >>out, '<p class="phony reason">No downloadable files were found. This build misses package description file. Uploader should probably re-upload the package again.</p>'
	else:
		for name in build["files"]:
			pkg = build["files"][name]
			bins += pkg['binaries'].keys()
			logs += pkg['logs'].keys()
		links = page_dls(dir, bins) + page_dls(dir, logs) + build["vcs"].additionalLinks(build)
	if len(links):
		print >>out, '<h2><span id="downloads">Downloads</span></h2>'
		print >>out, '<ul class="files">'
	for link in links:
		print >>out, '<li>%s</li>' % link
	if len(links):
		print >>out, '</ul>'

	page_footer(out)

def index_builds(out, dir):
	page_build(out, 'all nightlies', dir)

def index_version(out, dir):
	page_build(out, 'nightlies for %s' % os.path.split(dir)[1], dir)

def index_versions(out, dir):
	page_header(out, 'versions')

	latest = None
	latest_path = os.path.join(dir, "latest")
	if os.path.isdir(latest_path):
		latest = version(latest_path)['files']
		tmp = [latest[key]['version'] for key in latest]
		latest = None
		if len(tmp):
			latest = tmp[0].split('+', 1)[0].split('-', 1)[0]

	subs = os.listdir(dir)
	subs.sort(reverse=True)

	print >>out, '<ul class="subdirs">'

	for sub in subs:
		test = os.path.join(dir, sub)
		if not os.path.isdir(test) : continue
		if sub == "latest":
			page_packages(out, test, sub, version(test), True)
		else:
			page_version(out, test, sub, False)

	# print latest
	print >>out, '</ul>'
	page_footer(out)

def index_public(out, dir):
	page_header(out, 'releases')
	subs = os.listdir(dir)
	subs.sort(reverse=True)

	print >>out, '<ul class="subdirs">'

	for sub in subs:
		test = os.path.join(dir, sub)
		if not os.path.isdir(test) : continue
		page_public_entry(out, dir, sub, version(test))

	print >>out, '</ul>'
	page_footer(out)

def index_root(out, dir):
	title = 'Downloads'
	try:
		with open(os.path.join(dir, "name.json")) as f:
			title = json.load(f) + " builds"
	except: pass
	page_header(out, title, False)
	print >>out, '<ul class="subdirs">'

	subs = [
		('latest', 'latest', 'latest build available'),
		('builds', 'all nightlies', 'list of all builds for every version'),
		('versions', 'versions', 'breakup of builds per package versions'),
		('releases', 'releases', 'nightly builds to be released to the public')
	]
	for sub in subs:
		item_dir = os.path.join(dir, sub[0])
		if not os.path.exists(item_dir):
			continue
		if sub[0] == 'latest':
			page_packages(out, item_dir, sub[0], version(item_dir), True)
		else:
			page_root_link(out, *sub)

	print >>out, '</ul>'
	page_footer(out)

def call(dir, func, sub_func = None):
	print dir
	with open(os.path.join(dir, "index.html"), "w+b") as out:
		func(out, dir)

	if sub_func is None: return

	subs = os.listdir(dir)

	for sub in subs:
		test = os.path.join(dir, sub)
		if not os.path.isdir(test) : continue
		if os.path.islink(test) : continue
		if sub == "latest": continue
		print test
		with open(os.path.join(dir, sub, "index.html"), "w+b") as out:
			sub_func(out, test)

if args.type == 'all':
	call(args.dir, index_root)
	call(os.path.join(args.dir, 'builds'), index_builds, index_single)
	call(os.path.join(args.dir, 'versions'), index_versions, index_version)
	if os.path.exists(os.path.join(args.dir, 'releases')):
		call(os.path.join(args.dir, 'releases'), index_public, index_single)
	exit(0)

if args.type == 'update':
	if not args.id:
		parser.error('argument -# is required')
	call(args.dir, index_root)
	call(os.path.join(args.dir, 'builds'), index_builds)
	call(os.path.join(args.dir, 'builds', args.id), index_single)
	call(os.path.join(args.dir, 'versions'), index_versions)
	pkgs = version(os.path.join(args.dir, 'builds', args.id))['files']
	versions = [pkgs[pkg]['version'] for pkg in pkgs]
	if len(versions):
		ver = versions[0].split('+', 1)[0].split('-', 1)[0]
		call(os.path.join(args.dir, 'versions', ver), index_version)
	exit(0)

func = 'index_' + args.type
if func not in globals():
	print >>sys.stderr, 'Type unknown: ' + args.type
	exit(1)
func = globals()[func]

with open(os.path.join(args.dir, "index.html"), "w+b") as out:
	func(out, args.dir)
