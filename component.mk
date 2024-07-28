COMPONENT_INCDIRS := src/include
COMPONENT_SRCDIRS := src

ifneq (,$(COMPONENT_RULE))

CONFIGDB_GEN_CMDLINE := $(PYTHON) $(COMPONENT_PATH)/tools/dbgen.py

COMPONENT_VARS := APP_CONFIGDB_DIR
APP_CONFIGDB_DIR ?= $(PROJECT_DIR)/out/ConfigDB
COMPONENT_INCDIRS += $(APP_CONFIGDB_DIR)

COMPONENT_VARS += CONFIGDB_SCHEMA
CONFIGDB_SCHEMA := $(wildcard *.cfgdb)

$(APP_CONFIGDB_DIR)/%.h: %.cfgdb
	$(CONFIGDB_GEN_CMDLINE) $< $(APP_CONFIGDB_DIR)

CONFIGDB_FILES := $(patsubst %.cfgdb,$(APP_CONFIGDB_DIR)/%.h,$(CONFIGDB_SCHEMA))
COMPONENT_PREREQUISITES := $(CONFIGDB_FILES)

.PHONY: configdb-clean
configdb-clean:
	$(Q) rm -f $(CONFIGDB_FILES)

clean: configdb-clean

endif
