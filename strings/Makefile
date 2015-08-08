LNGS=pl en

FILE=TasksStrings
IDL=../apps/Tasks/src/$(FILE).idl
TARGET=../locale
EMAIL=Marcin Zdun <mzdun@midnightbits.com>

POS=$(addsuffix /$(FILE).po,$(LNGS))
MOS=$(addsuffix /$(FILE).mo,$(LNGS))
TARGETS=$(addprefix $(TARGET)/Tasks.,$(LNGS))

all: help

help:
	@echo Targets: pot langs
	@echo pot - updates $(FILE).pot from $(FILE).idl
	@echo po - updates $(FILE).po files
	@echo langs - updates final files from $(FILE).po

pot: $(FILE).pot
langs: $(TARGETS)
clean:
	@rm -f $(MOS) $(TARGETS) $(FILE).pot
po: $(LNGS)
$(LNGS): $(FILE).pot
	@echo $(addsuffix /$(FILE).po,$@)
	@msgmerge "$(addsuffix /$(FILE).po,$@)" "$<" -o "$(addsuffix /$(FILE).po,$@)"

$(FILE).pot: $(IDL)
	@echo $@
	@lngs pot -i "$<" -o "$@" -c "$(EMAIL)" -a "$(EMAIL)"

$(TARGETS): $(MOS) $(IDL)
	@echo $@
	@mkdir -p "$(TARGET)"
	@lngs make -o "$@" -m "$(addprefix $(subst .,,$(suffix $@)),/$(FILE).mo)" -l attr_language.txt -i "$(IDL)"

$(MOS): $(POS)
	@echo $@
	@msgfmt "$(addsuffix .po,$(basename $@))" -o "$@"

.PHONY: all clean help pot langs po $(LNGS)
