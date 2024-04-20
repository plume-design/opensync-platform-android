###############################################################################
#
# Android manager
#
###############################################################################
UNIT_DISABLE := $(if $(CONFIG_MANAGER_ANDROIDM),n,y)

UNIT_NAME := androidm

UNIT_TYPE := BIN

UNIT_SRC    := src/androidm_main.c
UNIT_SRC    += src/androidm_ovsdb.c
UNIT_SRC    += src/androidm_event.c

ifeq ($(CONFIG_PERIPHERAL_DEVICE_OVSDB),y)
UNIT_SRC    += src/androidm_peripheral.c
endif

ifeq ($(CONFIG_ANDROIDM_APP_REPORT),y)
UNIT_SRC    += src/androidm_app_usage.c
endif

ifeq ($(CONFIG_ANDROIDM_STREAMING_REPORT),y)
UNIT_SRC    += src/androidm_streaming.c
endif

UNIT_CFLAGS := -I$(UNIT_PATH)/inc
UNIT_CFLAGS += -Iplatform/android/src/lib/osandroid/inc

UNIT_LDFLAGS += -lprotobuf-c
UNIT_LDFLAGS += -ljansson
UNIT_LDFLAGS += -lev

UNIT_EXPORT_CFLAGS := $(UNIT_CFLAGS)
UNIT_EXPORT_LDFLAGS := $(UNIT_LDFLAGS)

UNIT_DEPS := src/lib/ovsdb
UNIT_DEPS += src/lib/schema
UNIT_DEPS += src/lib/common
UNIT_DEPS += src/lib/json_util
UNIT_DEPS += src/lib/module
UNIT_DEPS += platform/android/src/lib/osandroid

UNIT_DEPS_CFLAGS += src/lib/ovsdb
UNIT_DEPS_CFLAGS += src/lib/module
