import sys, langs as lng, strings

with open(sys.argv[1], "rb") as infile:
	text = "".join(infile.readlines())

toks = text.split("$(")
prog = [(None, toks[0])]
toks = toks[1:]
names = {}

for tok in toks:
	name, text = tok.split(')', 1)
	prog.append((name, text))
	names[name] = ""

full_lang = (int(sys.argv[5]) << 10) | int(sys.argv[4])
names["CODEPAGE"] = sys.argv[6]
names["SUBLANG_ID"] = sys.argv[5]
names["LANG_ID"] = sys.argv[4]
names["LANG_HEX"] = hex(full_lang).split("0x", 1)[1]
while len(names["LANG_HEX"]) < 4: names["LANG_HEX"] = "0" + names["LANG_HEX"]

tr = strings.open(sys.argv[3])
app_name = tr.get(lng.LNG_APP_NAME)
infotip = '"%s"' % tr.get(lng.LNG_APP_INFOTIP).replace('"', '""').replace("%1", app_name).replace("%2", '" PROGRAM_VERSION_STRING PROGRAM_VERSION_STABILITY "')
names["LNG_APP_DESCRIPTION"] = '"%s"' % tr.get(lng.LNG_APP_DESCRIPTION).replace('"', '""')
names["LNG_APP_LINK_INFOTIP"] = infotip.replace('%3"', '" VERSION_STRINGIFY(PROGRAM_VERSION_BUILD)').replace("%3", '" VERSION_STRINGIFY(PROGRAM_VERSION_BUILD) "')
names["LNG_APP_NAME"] = '"%s"' % app_name.replace('"', '""')

with open(sys.argv[2], "wb") as out:
	for p in prog:
		if p[0] is not None:
			out.write(names[p[0]])
		out.write(p[1])
