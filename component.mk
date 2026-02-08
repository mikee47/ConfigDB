COMPONENT_DEPENDS := JsonStreamingParser

COMPONENT_INCDIRS := src/include
COMPONENT_SRCDIRS := src src/Json
COMPONENT_DOXYGEN_INPUT := src/include

ifneq (,$(COMPONENT_RULE))

CONFIGDB_GEN_CMDLINE := $(PYTHON) $(COMPONENT_PATH)/tools/dbgen.py

COMPONENT_VARS := APP_CONFIGDB_DIR
APP_CONFIGDB_DIR := $(PROJECT_DIR)/out/ConfigDB
COMPONENT_INCDIRS += $(APP_CONFIGDB_DIR)
COMPONENT_APPCODE := $(APP_CONFIGDB_DIR)

COMPONENT_VARS += CONFIGDB_SCHEMA
CONFIGDB_SCHEMA := $(wildcard *.cfgdb)

CONFIGDB_JSON := $(patsubst %.cfgdb,$(APP_CONFIGDB_DIR)/schema/%.json,$(CONFIGDB_SCHEMA))

##@ConfigDB

.PHONY: configdb-preprocess
configdb-preprocess: ##Pre-process .cfgdb into .json
	$(Q) $(CONFIGDB_GEN_CMDLINE) --preprocess --outdir $(APP_CONFIGDB_DIR) $(CONFIGDB_SCHEMA)

CONFIGDB_FILES := $(patsubst %.cfgdb,$(APP_CONFIGDB_DIR)/%.h,$(CONFIGDB_SCHEMA))
CONFIGDB_FILES := $(CONFIGDB_FILES) $(CONFIGDB_FILES:.h=.cpp)
COMPONENT_PREREQUISITES := configdb-preprocess $(CONFIGDB_FILES)

$(CONFIGDB_FILES): $(CONFIGDB_JSON)
	$(MAKE) configdb-build

.PHONY: configdb-build
configdb-build: $(CONFIGDB_SCHEMA) ##Parse schema and generate source code
	$(vecho) "CFGDB $^"
	$(Q) $(CONFIGDB_GEN_CMDLINE) --outdir $(APP_CONFIGDB_DIR) $^

.PHONY: configdb-rebuild
configdb-rebuild: configdb-clean configdb-build ##Force regeneration of source code

.PHONY: configdb-clean
configdb-clean: ##Remove generated files
	$(Q) rm -rf $(APP_CONFIGDB_DIR)/*

clean: configdb-clean

endif
