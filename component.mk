COMPONENT_DEPENDS := JsonStreamingParser

COMPONENT_INCDIRS := src/include
COMPONENT_SRCDIRS := src src/Json
COMPONENT_DOXYGEN_INPUT := src/include

ifneq (,$(COMPONENT_RULE))

CONFIGDB_GEN_CMDLINE := $(PYTHON) $(COMPONENT_PATH)/tools/dbgen.py

COMPONENT_VARS := APP_CONFIGDB_DIR
APP_CONFIGDB_DIR := $(PROJECT_DIR)/$(OUT_BASE)/ConfigDB
COMPONENT_INCDIRS += $(APP_CONFIGDB_DIR)
COMPONENT_APPCODE := $(APP_CONFIGDB_DIR)

COMPONENT_VARS += CONFIGDB_SCHEMA
CONFIGDB_SCHEMA := $(wildcard *.cfgdb)

CONFIGDB_JSON := $(patsubst %.cfgdb,$(APP_CONFIGDB_DIR)/schema/%.json,$(CONFIGDB_SCHEMA))

.PHONY: configdb-parse
configdb-parse:
	$(Q) $(CONFIGDB_GEN_CMDLINE) --parse --outdir $(APP_CONFIGDB_DIR) $(CONFIGDB_SCHEMA)

CONFIGDB_FILES := $(patsubst %.cfgdb,$(APP_CONFIGDB_DIR)/%.h,$(CONFIGDB_SCHEMA))
CONFIGDB_FILES := $(CONFIGDB_FILES) $(CONFIGDB_FILES:.h=.cpp)
COMPONENT_PREREQUISITES := configdb-parse $(CONFIGDB_FILES)

$(CONFIGDB_FILES): $(CONFIGDB_JSON)
	$(MAKE) configdb-build

.PHONY: configdb-build
configdb-build: $(CONFIGDB_SCHEMA)
	$(vecho) "CFGDB $^"
	$(Q) $(CONFIGDB_GEN_CMDLINE) --outdir $(APP_CONFIGDB_DIR) $^

.PHONY: configdb-rebuild
configdb-rebuild: configdb-clean configdb-build

.PHONY: configdb-clean
configdb-clean:
	$(Q) rm -rf $(APP_CONFIGDB_DIR)/*

clean: configdb-clean

endif
