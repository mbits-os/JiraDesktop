import sys, os, glob, zipfile

ROOT = 'apps/Tasks/Release/'
FILES = [
	'tasks.exe',
	'locale/Tasks.*'
]

def paths():
	for FILE in FILES:
		for name in glob.iglob(os.path.join(ROOT, FILE)):
			yield (name, os.path.join('bin', os.path.relpath(name, ROOT)))


with zipfile.ZipFile(sys.argv[1], 'w') as zip:
	for src, dst in paths(): zip.write(src, dst)
	zip.write('msbuild.log')
