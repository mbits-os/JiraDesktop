import os, shutil, glob, winreg
from os import path

def mtime(f):
	try:
		stat = os.stat(f)
		return stat.st_mtime
	except:
		return 0

def unglob(src, files):
	out = []
	for f in files:
		if '*' in f:
			for o in glob.iglob(path.join(src, f)):
				out.append(o[len(src)+1:])
		else: out.append(f)
	return out

def copyFilesFlat(src, dst, files):
	for __f in files:
		f_s = __f
		f_d = __f
		if '|' in __f:
			f_d, f_s = __f.split('|', 1)
		s = path.join(src, f_s)
		d = path.join(dst, f_d)
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
			print(d)
			try: os.mkdir(os.path.dirname(d))
			except: pass
			shutil.copy(s, d)

# HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Microsoft SDKs\Windows
def SDKs():
	sdks = {}
	with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows") as key:
		index = 0
		while True:
			try:
				subkey = winreg.EnumKey(key, index)
				index += 1
				with winreg.OpenKey(key, subkey) as sub:
					sdks[subkey] = winreg.QueryValueEx(sub, "InstallationFolder")[0]
			except:
				break
	return sdks

def win_sdk():
	sdks = SDKs()
	sdk = None

	sdk_vers = list(sdks.keys())
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
		print('{}<Component Id="{}" Guid="{}">'.format("  " * depth, self.name, self.guid), file=out)
		depth += 1
		prefix = self.dir.replace(os.sep, '_').replace('-', '_')
		if len(prefix):
			prefix += '_'
		keypath = ' KeyPath="yes"'
		for file in self.files:
			__id = '{}{}'.format(prefix, file[0].replace('.', '_'))
			out.write('{}<File Id="{}" Name="{}" Source="{}" DiskId="{}"{} '.format("  " * depth, __id, file[0], file[1], self.id, keypath))
			if file[0] in links:
				print('>', file=out)
				for link in links[file[0]]:
					id = '{}_{}.Shortcut'.format(link[1], file[0].replace('.', '_'))
					print('{}  <Shortcut Id="{}" Directory="{}" Name="{}"'.format("  " * depth, id, link[1], link[0]), file=out)
					print('{}            WorkingDirectory="{}"'.format("  " * depth, self.dir_name), file=out)
					print('{}            Icon="{}" IconIndex="{}"'.format("  " * depth, link[2], link[3]), file=out)
					if len(link) > 4: # Display name
						print('{}            DisplayResourceDll="[#{}]" DisplayResourceId="{}"'.format("  " * depth, __id, link[4]), file=out)
					if len(link) > 5: # Infotip
						print('{}            DescriptionResourceDll="[#{}]" DescriptionResourceId="{}"'.format("  " * depth, __id, link[5]), file=out)
					print('{}            Advertise="yes" />'.format("  " * depth), file=out)
				print('{}</File>'.format("  " * depth), file=out)
			else:
				print('/>', file=out)
			keypath = ""
		depth -= 1
		print('{}</Component>'.format("  " * depth), file=out)

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
				comp_name = "{}.Component".format(self.dir.replace(os.sep, '_').replace('-', '_'))

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
			dir_id = dir_here.replace(os.sep, "_").replace('-', '_') + ".Directory"
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
			print("<DirectoryRef Id='{}'>".format(self.id), file=out)
		else:
			print("<Directory Id='{}' Name='{}'>".format(self.id, self.name), file=out)
		depth += 1
		for comp in self.comps:
			self.comps[comp].print_out(out, links, depth)
		for d in self.subs:
			self.subs[d].print_out(out, links, depth)
		depth -= 1
		out.write("  " * depth)
		if self.name is None:
			print("</DirectoryRef>", file=out)
		else:
			print("</Directory>", file=out)

	def print_fragment(self, out, links):
		print("""<?xml version="1.0" encoding="utf-8"?>
<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
  <Fragment>""", file=out)
		self.print_out(out, links, 2)
		print("""  </Fragment>
</Wix>""", file=out)

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
