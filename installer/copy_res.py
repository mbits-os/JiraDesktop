#!/usr/bin/python

import os, shutil
from os import path

resources = "../../JiraDesktop.files/res"

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
			print(d)
			shutil.copy(s, d)

for root, dirs, files in os.walk(resources):
	here = root[len(resources):]
	if len(here): here = here[1:]
	here = path.join(".", here)
	if not mtime(here): os.makedirs(here)
	copyFilesFlat(root, here, files)
