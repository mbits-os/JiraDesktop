#!/usr/bin/python

import os, argparse, json

parser = argparse.ArgumentParser(description='Creates index.html for the upload')
parser.add_argument('-d', dest='dir', required=True, help='taregt directory')
parser.add_argument('-f', dest='field', required=True, help='name of the field to test')
parser.add_argument('-v', dest='value', required=True, help='value to set, if missing')

args = parser.parse_args()

def read(dir):
	with open(os.path.join(dir, 'version.json')) as f:
		return json.load(f)

def write(dir, obj):
	with open(os.path.join(dir, 'version.json'), 'w+b') as f:
		json.dump(obj, f, separators=(',',':'))


dir = os.path.join(args.dir, 'builds')

subs = os.listdir(dir)
for sub in subs:
	subdir = os.path.join(dir, sub)
	if not os.path.isdir(subdir) : continue
	if subdir == "latest": continue
	obj = read(subdir)
	if args.field not in obj:
		print subdir
		obj[args.field] = args.value
		write(subdir, obj)
