###############################################################################
#
# Android unit override for target library
#
###############################################################################

# Common target library sources
UNIT_SRC := $(TARGET_COMMON_SRC)

# Platform specific target library sources
UNIT_SRC_PLATFORM := $(OVERRIDE_DIR)
UNIT_SRC_TOP += $(OVERRIDE_DIR)/target_stats.c
UNIT_SRC_TOP += $(OVERRIDE_DIR)/target_init.c
UNIT_SRC_TOP += $(OVERRIDE_DIR)/target_radio.c
UNIT_SRC_TOP += $(OVERRIDE_DIR)/target_logpull.c
UNIT_SRC_TOP += $(OVERRIDE_DIR)/target_util.c

UNIT_CFLAGS += -I$(OVERRIDE_DIR)
UNIT_CFLAGS += -I$(OVERRIDE_DIR)/inc

UNIT_EXPORT_LDFLAGS := -lm

UNIT_DEPS_CFLAGS += src/lib/ovsdb
UNIT_DEPS_CFLAGS += platform/android/src/lib/osandroid
