COMPONENT_DEPENDS := \
	ConfigDB \
	LittleFS

COMPONENT_VARS += ENABLE_MALLOC_COUNT
ENABLE_MALLOC_COUNT ?= 1

ifeq ($(ENABLE_MALLOC_COUNT),1)
COMPONENT_DEPENDS	+= malloc_count
COMPONENT_CXXFLAGS	+= -DENABLE_MALLOC_COUNT=1
endif

HWCONFIG := basic-config
