###############################################################################
#
# OpenSync Android API library
#
###############################################################################
UNIT_NAME := osandroid

UNIT_DISABLE := $(if $(CONFIG_MANAGER_ANDROIDM),n,y)

UNIT_TYPE := LIB

UNIT_SRC += src/osandroid_streaming.c
UNIT_SRC += src/osandroid_app_usage.c
UNIT_SRC += src/osandroid_ipc.c

UNIT_CFLAGS := -I$(UNIT_PATH)/inc
UNIT_LDFLAGS += -lzmq

UNIT_EXPORT_CFLAGS := $(UNIT_CFLAGS)
UNIT_EXPORT_LDFLAGS := $(UNIT_LDFLAGS)

UNIT_DEPS_CFLAGS += src/lib/ovsdb

UNIT_DEPS := src/lib/const
UNIT_DEPS += src/lib/log
UNIT_DEPS += src/lib/common
