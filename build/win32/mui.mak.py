import sys, os

print >>sys.stderr, os.getcwd()

LANGS=[
	"pl-PL pl 21 1 1250",
	"en-US en 9 1 1252"
]

PLT="Win32"
CFG="Release"

names = ["PLT", "CFG"]

for arg in sys.argv[1:]:
	s = arg.split('=', 1)
	if len(s) != 2: continue
	if s[0] not in names or not len(s[1]):
		continue
	globals()[s[0]] = s[1]


INTDIR="../../int/{}/{}/Tasks.exe/".format(PLT, CFG)
OUTDIR="../../bin/{}/{}/".format(PLT, CFG)
SRCDIR="../../apps/Tasks/src/"

def norm(p):
	return os.path.normpath(os.path.join(os.getcwd(), p))

print "all:",
for _lang in LANGS:
	lang = _lang.split(" ", 1)[0]
	print norm("{}{}/Tasks.exe.mui".format(OUTDIR, lang)),
print
print

print "clean:"
for _lang in LANGS:
	lang = _lang.split(" ", 1)[0]
	print "\t@echo [ --- ] " + lang
	print "\t@rm -fr " + norm("{}{}".format(OUTDIR, lang))
	print "\t@rm -f  " + norm("{}{}.mui.res".format(INTDIR, lang))
	print "\t@rm -f  " + norm("{}{}.res".format(INTDIR, lang))
	print "\t@rm -f  " + norm("{}{}.rc".format(INTDIR, lang))
print

__tmplt = norm("{}mui.tmplt.rc".format(SRCDIR))
__rcconf = norm("{}Tasks.rcconf".format(SRCDIR))
__exe = norm("{}Tasks.exe".format(OUTDIR))

for _lang in LANGS:
	lang, srclang, lang_id, sublang_id, codepage = _lang.split(" ")
	__dir = norm("{}{}".format(OUTDIR, lang))
	__rc = norm(("{}{}.rc").format(INTDIR, lang))
	__inlang = norm("{}locale/Tasks.{}".format(OUTDIR, srclang))
	__res = norm("{}{}.res".format(INTDIR, lang))
	__mui_res = norm("{}{}.mui.res".format(INTDIR, lang))
	__mui_exe = norm("{}{}/Tasks.exe.mui".format(OUTDIR, lang))

	print __dir + ":"
	print "\t@echo [ +++ ] " + __dir
	print "\t@mkdir " + __dir
	print
	print "{}: langs.py mui.rc.py {}".format(__rc, __inlang)
	print "\t@echo [ RC. ] " + __rc
	print '\t@python mui.rc.py "{}" "{}" "{}" {} {} {}'.format(__tmplt, __rc, __inlang, lang_id, sublang_id, codepage)
	print
	print "{}: {}".format(__mui_res, __rc)
	print "\t@echo [ MUI ] " + __mui_res
	print '\t@rc /nologo /i "{}" /fo "{}" /fm "{}" /g1 /q "{}" "{}"'.format(norm(SRCDIR), __res, __mui_res, __rcconf, __rc) # /v
	print
	print "{}: {} {} {}".format(__mui_exe, __dir, __mui_res, __exe)
	print "\t@echo [ EXE ] " + __mui_exe
	print '\t@link /machine:x86 /nologo /noentry /dll "/out:{}" "{}"'.format(__mui_exe, __mui_res) # /verbose
	print '\t@muirct -c "{}" -b "{}" -e "{}" 1>nul'.format(__exe, lang, __mui_exe)
	print
