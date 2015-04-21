#!/usr/bin/python

import os, sys, urlparse, urllib, httplib, json, argparse

parser = argparse.ArgumentParser(description='Creates relase notes from JIRA version info', fromfile_prefix_chars='@', usage='%(prog)s [options]')
parser.add_argument('-s', dest='jira', required=True, help='service URL')
parser.add_argument('-p', dest='project', required=True, help='project key or identifier')
parser.add_argument('-v', dest='version', required=True, help='version name or key')
parser.add_argument('-k', dest='types', help='comma separated list of issue types for the Known Issues section')
parser.add_argument('-o', dest='output', help='file to write the notes to (defaults to stdout)')
parser.add_argument('-a', dest='auth', help='uses this value as basic authentication')

args   = parser.parse_args()

def write_section(out, header, section):
	print >>out, '####', header
	if len(section): print >>out
	for item in section:
		print >>out, '* [%s] %s' % item

def write_info(out, info):
	keys = info.keys()
	keys.sort()
	first = True
	for key in keys:
		if key == '?': continue
		if first: first = False
		else: print >>out
		write_section(out, key, info[key])

	if '?' in keys:
		if first: first = False
		else: print >>out
		write_section(out, 'Known issues', info['?'])

def write_out(fname, info):
	if fname:
		with open(args.output, 'w+b') as f: write_info(f, info)
	else:
		write_info(sys.stdout, info)

baseUrl = urlparse.urlparse(args.jira)

scheme = baseUrl.scheme
if not scheme: scheme = 'http'

netloc = baseUrl.netloc
if not netloc: raise RuntimeError('No host in URL ' + args.jira + '. Did you forget to use "//"?')

if scheme == 'http':
	conn = httplib.HTTPConnection(netloc)
elif scheme == 'https':
	conn = httplib.HTTPSConnection(netloc)
else:
	raise RuntimeError('Unknown URL scheme: ' + scheme)

headers = {}
if args.auth:
	headers['Authorization'] = 'Basic ' + args.auth
	
def get_json(api, query = {}):
	global args, headers
	query = urllib.urlencode(query)
	if query != '': query = '?' + query
	res = urlparse.urlsplit(urlparse.urljoin(args.jira, api)).path + query
	conn.request('GET', res, None, headers)
	resp = conn.getresponse()
	return json.load(resp)

vers = get_json('rest/api/2/project/%s/versions' % args.project)
versions = {}
for ver in vers:
	versions[ver['name']] = ver['id']

if args.version not in versions:
	keys = versions.keys()
	keys.sort();
	raise RuntimeError('Unknown project version: ' + args.version + '. Known versions are: ' + ', '.join(keys))
	
issues = get_json('rest/api/2/search', {'jql': 'fixVersion=%s' % versions[args.version], 'fields': 'issuetype,resolution,summary'})["issues"]

known = []
if args.types:
	known = args.types.split(',')

info = {}

for issue in issues:
	key = issue['key']
	issue = issue['fields']
	issuetype = issue['issuetype']['name']
	summary = issue['summary']
	resolution = issue['resolution']
	
	if resolution:
		if issuetype not in info:
			info[issuetype] = []
		info[issuetype].append((key, summary))
	elif issuetype in known:
		if '?' not in info:
			info['?'] = []
		info['?'].append((key, summary))

write_out(args.output, info)