#!/usr/python

import os, shutil, glob, _winreg
from os import path

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
		if path.isdir(s):
			for root, dirs, files in os.walk(s):
				try:
					os.makedirs(d)
				except:
					pass
				copyFilesFlat(s, d, dirs + files)
				return
		s_time = mtime(s)
		d_time = mtime(d)
		if s_time > d_time:
			print d
			shutil.copy(s, d)

# HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Microsoft SDKs\Windows
def SDKs():
	sdks = {}
	with _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows") as key:
		index = 0
		while True:
			try:
				subkey = _winreg.EnumKey(key, index)
				index += 1
				with _winreg.OpenKey(key, subkey) as sub:
					sdks[subkey] = _winreg.QueryValueEx(sub, "InstallationFolder")[0]
			except:
				break
	return sdks

def win_sdk():
	sdks = SDKs()
	sdk = None

	sdk_vers = sdks.keys()
	sdk_vers.sort()
	for ver in sdk_vers:
		signtool = path.join(sdks[ver], "Bin", "signtool.exe")
		if path.exists(signtool):
			sdk = path.join(sdks[ver], "Bin")
	return sdk

def file_list(storage):
	ret = []

	for key in storage:
		dir = storage[key]
		for f in glob.iglob(path.join(key, "*")):
			if path.isdir(f):
				for root, dirs, files in os.walk(f):
					for p in files:
						s = path.join(root, p)
						dest = path.join(root[len(key)+1:], p)
						if dir != "":
							dest = path.join(dir, dest)
						ret.append((s, dest))
			else:
				dest = f[len(key)+1:]
				if dir != "":
					dest = path.join(dir, dest)
				ret.append((f, dest))
	return ret

class component:
	def __init__(self, cab_id, dir, dir_name, name, guid):
		self.id = cab_id
		self.dir = dir
		self.dir_name = dir_name
		self.name = name
		self.guid = guid
		self.files = []

	def append(self, file, source):
		self.files.append((file, source))

	def print_out(self, out, links, depth = 1):
		print >>out, '{}<Component Id="{}" Guid="{}">'.format("  " * depth, self.name, self.guid)
		depth += 1
		prefix = self.dir.replace(os.sep, '_')
		if len(prefix):
			prefix += '_'
		keypath = ' KeyPath="yes"'
		for file in self.files:
			out.write('{}<File Id="{}{}" Name="{}" Source="{}" DiskId="{}"{} '.format("  " * depth, prefix, file[0].replace('.', '_'), file[0], file[1], self.id, keypath))
			if file[0] in links:
				print >>out, '>'
				for link in links[file[0]]:
					id = '{}_{}.Shortcut'.format(link[1], file[0].replace('.', '_'))
					print >>out, '{}  <Shortcut Id="{}" Directory="{}" Name="{}"'.format("  " * depth, id, link[1], link[0])
					print >>out, '{}            WorkingDirectory="{}"'.format("  " * depth, self.dir_name)
					print >>out, '{}            Icon="{}" IconIndex="{}"'.format("  " * depth, link[2], link[3])
					print >>out, '{}            Advertise="yes" />'.format("  " * depth)
				print >>out, '{}</File>'.format("  " * depth)
			else:
				print >>out, '/>'
			keypath = ""
		depth -= 1
		print >>out, '{}</Component>'.format("  " * depth)

class directory:
	def __init__(self, id, dir, name = None):
		self.id = id
		self.dir = dir
		self.name = name
		self.comps = {}
		self.subs = {}

	def append_here(self, cabId, file, source, comp_tmpl):
		if cabId not in self.comps:
			comp_id = (cabId, self.dir)

			comp_name = None
			comp_guid = 'PUT-GUID-HERE'
			if comp_id in comp_tmpl:
				comp_name, comp_guid = comp_tmpl[comp_id]
				
			if comp_name is None:
				comp_name = "{}.Component".format(self.dir.replace(os.sep, '_'))

			self.comps[cabId] = component(cabId, self.dir, self.id, comp_name, comp_guid)
		self.comps[cabId].append(file, source)

	def append_deep(self, deep, cabId, file, source, comp_tmpl):
		if len(deep) == 0:
			self.append_here(cabId, file, source, comp_tmpl)
			return
		here = deep[0]
		deep = deep[1:]
		if here not in self.subs:
			dir_here = path.join(self.dir, here)
			if self.dir == "": dir_here = here
			dir_id = dir_here.replace(os.sep, "_") + ".Directory"
			self.subs[here] = directory(dir_id, path.join(self.dir, here), here)
		self.subs[here].append_deep(deep, cabId, file, source, comp_tmpl)

	def append(self, cabId, file, source, comp_tmpl):
		file_path = path.dirname(file)
		file = path.basename(file)

		if file_path == "":
			self.append_here(cabId, file, source, comp_tmpl)
			return

		self.append_deep(file_path.split(os.sep), cabId, file, source, comp_tmpl)

	def print_out(self, out, links, depth = 1):
		out.write("  " * depth)
		if self.name is None:
			print >>out, "<DirectoryRef Id='{}'>".format(self.id)
		else:
			print >>out, "<Directory Id='{}' Name='{}'>".format(self.id, self.name)
		depth += 1
		for comp in self.comps:
			self.comps[comp].print_out(out, links, depth)
		for d in self.subs:
			self.subs[d].print_out(out, links, depth)
		depth -= 1
		out.write("  " * depth)
		if self.name is None:
			print >>out, "</DirectoryRef>"
		else:
			print >>out, "</Directory>"

	def print_fragment(self, out, links):
		print >>out, """<?xml version="1.0" encoding="utf-8"?>
<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
  <Fragment>"""
		self.print_out(out, links, 2)
		print >>out, """  </Fragment>
</Wix>"""

def dir_cab(cabs, name):
	for i in range(len(cabs)):
		if name in cabs[i]:
			return i + 1
	return 0

def dir_from(file_path):
	pair = file_path.split(os.sep, 1)
	if len(pair) == 1: return ""
	return pair[0]

def msi_fragment(cabs, comps, list):
	ret = directory("INSTALLDIR", "")

	for source, dest in list:
		cab = dir_cab(cabs, dir_from(source))
		ret.append(cab, dest, source, comps)
	return ret
