##
# Android platform definitions, includes, flags..
#

# add any Android dependend includes
# SDK_INCLUDES +=
# INCLUDES     +=   $(SDK_INCLUDES)

##
# if warning supression or other Android platform dependend flags are necessary,
# add them here
#
# DEFINES      += -Wno-unused-but-set-variable

##
# setup rootfs
#
# SDK_ROOTFS   :=   $(INSTALL_DIR)


ifeq ($(V),1)
$(info --- Android ENV ---)
$(info PROFILE=$(PROFILE))
$(info INSTALL_DIR=$(INSTALL_DIR))
$(info TARGET_FS=$(TARGET_FS))
$(info --- Plume ENV ---)
$(info TARGET=$(TARGET))
$(info PLATFORM=$(PLATFORM))
$(info VENDOR=$(VENDOR))
$(info INCLUDES=$(INCLUDES))
$(info DEFINES=$(DEFINES))
$(info SDK_ROOTFS=$(SDK_ROOTFS))
$(info -----------------)
endif
