all clean: mui.gen-$(PTF)-$(CFG).mak
	@nmake -nologo -f mui.gen-$(PTF)-$(CFG).mak $@

mui.gen-$(PTF)-$(CFG).mak: mui.mak.py langs.py
	@python mui.mak.py "CFG=$(CFG)" "PTF=$(PTF)" > mui.gen-$(PTF)-$(CFG).mak

langs.py: ../../apps/Tasks/src/TasksStrings.idl
	@lngs py -vi ../../apps/Tasks/src/TasksStrings.idl -o langs.py