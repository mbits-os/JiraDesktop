#!/usr/bin/python

import os, sys, argparse, json, markdown

parser = argparse.ArgumentParser(description='Creates index.html for the upload')
parser.add_argument('-d', dest='dir', required=True, help='taregt directory')
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

def packages(dir):
	with open(os.path.join(dir, 'version.json')) as f:
		doc = json.load(f)
		packages = {}
		for pkg in doc:
			bin = {}
			logs = {}
			for platform in pkg["platforms"]:
				if platform == "no-arch":
					base = "{package}-{version}".format(**pkg)
				else:
					base = "{package}-{version}-{0}".format(platform, **pkg)

				platform = pkg["platforms"][platform]
				if "archive" in platform:
					archkey = "archive"
				elif "arch" in platform:
					archkey = "arch"
				else:
					archkey = None

				if archkey:
					for arch in platform[archkey]:
						fname = "%s.%s" % (base, arch)
						if os.path.exists(os.path.join(dir, fname)):
							bin[fname] = os.path.splitext(fname)[1][1:]
				for log in platform["logs"]:
					if log == "":
						fname = "%s.log" % base
						name = "log"
					else:
						fname = "%s-%s.log" % (base, log)
						name = "%s.log" % log
					if os.path.exists(os.path.join(dir, fname)):
						logs[fname] = name
			out = {
				'version' : pkg['version'],
				'binaries': bin,
				'logs': logs
			}
			packages[pkg['package']] = out
		return packages

def page_header(out, title, updir = True):
	print >>out, '<title>%s</title>' % title
	print >>out, '<link rel="stylesheet" type="text/css" href="/ui/pages.css" />'
	print >>out, '<div class="contents">'
	print >>out, '<h1>%s</h1>' % title
	if updir:
		print >>out, '<p class="parent-dir"><a href="../"><span class="icon icon-dir-up"></span>Parent Directory</a></p>'

def page_footer(out):
	print >>out, '</div>'

def page_entry(out, link, name, latest, hints = []):
	out.write('<li class="entry"><a href="%s/">' % link)
	if latest:
		out.write('<strong>')
	out.write(name)
	if latest:
		out.write('</strong>')
	out.write('</a>')
	if latest:
		out.write(' <span class="latest">[latest]</span>')
	if len(hints):
		print >>out, '<ul class="hints">'
		for hint in hints:
			out.write('<li>')
			if hint[0]:
				out.write('<span class="title light">%s</span>' % hint[0])
			if len(hint) > 1 and len(hint[1]):
				print >>out, '<ul class="short-hints">'
				for short in hint[1]:
					print >>out, '<li>%s</li>' % short
				print >>out, '</ul>'
			print >>out, '</li>'
		print >>out, '</ul>'
	print >>out, '</li>'

def page_files(link, refs):
	fnames = sorted(refs.keys())
	files = []
	for fname in fnames:
		files.append('<a href="%s/%s"><span class="left light icon icon-%s"></span>%s</a>' % (link, fname, iconClass(fname), refs[fname]))
	return files

def page_packages(out, dir, link, pkgs, latest):
	keys = sorted(pkgs.keys())
	hints = []
	if os.path.exists(os.path.join(dir, link, 'release-notes.txt')):
		hints.append(['see', ['<a href="%s/#notes"><strong>release notes</strong></a>' % link]])
	for key in keys:
		pkg = pkgs[key]
		bins = pkg['binaries']
		logs = pkg['logs']
		links = page_files(link, bins) + page_files(link, logs)
		hints.append([key + ':', links])

	page_entry(out, link, ', '.join([pkgs[pkg]['version'] for pkg in pkgs]), latest, hints)

def page_version(out, dir, link, latest):
	subs = os.listdir(dir)
	subs.sort(reverse=True)

	build_latest = packages(os.path.join(dir, "latest"))
	tmp = [build_latest[key]['version'] for key in build_latest]
	build_latest = None
	if len(tmp):
		build_latest = tmp[0].split('+', 1)[1]

	builds = []

	for sub in subs:
		test = os.path.join(dir, sub)
		if not os.path.isdir(test) : continue
		if sub == "latest": continue
		icon = 'dir-o'
		if sub == build_latest: icon = 'dir'
		build = '<a href="%s/%s"><span class="icon icon-%s"></span>' % (link, sub, icon)
		if sub == build_latest: build += '<strong>'
		build += sub
		if sub == build_latest: build += '</strong>'
		builds.append(build)

	page_entry(out, link, link, latest, [["builds:", builds]])

def page_root_link(out, link, name, description):
	page_entry(out, link, name, False, [[description]]);

def page_build(out, title, dir):
	page_header(out, title)

	latest = packages(os.path.join(dir, "latest"))
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
		if sub == "latest": continue
		page_packages(out, dir, sub, packages(test), latest == sub)

	# print latest
	print >>out, '</ul>'
	page_footer(out)

def index_single(out, dir):
	with open(os.path.join(dir, 'version.json')) as f:
		doc = json.load(f)
		versions = {}
		for package in doc:
			version = package['version']
			name = package['package']
			versions[version] = 1
			# print name, version
		versions = versions.keys()
		versions.sort()
		page_header(out, ', '.join(versions))
		try:
			with open(os.path.join(dir, 'release-notes.txt')) as r:
				print >>out, '<h2><span id="notes">Release notes</span></h2>'
				print >>out, markdown.markdown("".join(r.readlines()))
		except: pass
		
		files = []
		
		for package in doc:
			for platform in package["platforms"]:
				if platform == "no-arch":
					base = "{package}-{version}".format(**package)
				else:
					base = "{package}-{version}-{0}".format(platform, **package)
				platform = package["platforms"][platform]
				if "archive" in platform:
					archkey = "archive"
				elif "arch" in platform:
					archkey = "arch"
				else:
					archkey = None
				if archkey:
					for arch in platform[archkey]:
						fname = "%s.%s" % (base, arch)
						if os.path.exists(os.path.join(dir, fname)):
							files.append(fname)
				for log in platform["logs"]:
					if log == "":
						fname = "%s.log" % base
					else:
						fname = "%s-%s.log" % (base, log)
					if os.path.exists(os.path.join(dir, fname)):
						files.append(fname)

		if len(files):
			print >>out, '<h2><span id="downloads">Downloads</span></h2>'
			print >>out, '<ul class="files">'

		for fname in files:
			print >>out, '<li><a href="%s"><small class="right light">%s</small><span class="left light icon icon-%s"></span>%s</a></li>' % (fname, sizeOf(os.path.join(dir, fname)), iconClass(fname), fname)

		if len(files):
			print >>out, '</ul>'
		page_footer(out)

def index_builds(out, dir):
	page_build(out, 'Builds', dir)

def index_version(out, dir):
	page_build(out, 'Releases for %s' % os.path.split(dir)[1], dir)

def index_versions(out, dir):
	page_header(out, 'Releases')

	latest = packages(os.path.join(dir, "latest"))
	tmp = [latest[key]['version'] for key in latest]
	latest = None
	if len(tmp):
		latest = tmp[0].split('+', 1)[0]
	
	subs = os.listdir(dir)
	subs.sort(reverse=True)
	
	print >>out, '<ul class="subdirs">'

	for sub in subs:
		test = os.path.join(dir, sub)
		if not os.path.isdir(test) : continue
		if sub == "latest": continue
		page_version(out, test, sub, latest == sub)

	# print latest
	print >>out, '</ul>'
	page_footer(out)
	
def index_root(out, dir):
	page_header(out, 'Downloads', False)
	print >>out, '<ul class="subdirs">'

	subs = [
		('builds', 'Builds', 'list of all builds for every version'),
		('releases', 'Releases', 'list of all versions'),
		('latest', 'Latest', 'latest build available')
	]
	for sub in subs:
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
	call(os.path.join(args.dir, 'releases'), index_versions, index_version)
	exit(0)

if args.type == 'update':
	if not args.id:
		parser.error('argument -# is required')
	call(os.path.join(args.dir, 'builds'), index_builds)
	call(os.path.join(args.dir, 'builds', args.id), index_single)
	call(os.path.join(args.dir, 'releases'), index_versions)
	pkgs = packages(os.path.join(args.dir, 'builds', args.id))
	versions = [pkgs[pkg]['version'] for pkg in pkgs]
	if len(versions):
		version = versions[0].split('+', 1)[0]
		call(os.path.join(args.dir, 'releases', version), index_version)
	exit(0)

func = 'index_' + args.type
if func not in globals():
	print >>sys.stderr, 'Type unknown: ' + args.type
	exit(1)
func = globals()[func]

with open(os.path.join(args.dir, "index.html"), "w+b") as out:
	func(out, args.dir)
