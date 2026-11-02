SDK_DIR ?= $(abspath ../../../bk_avdk_smp)

$(if $(shell cp -vfr sdk_patch/* $(SDK_DIR)/ 2>&1),,$(error "Failed to copy sdk_patch files"))

PROJECT_MAKE_FILE := $(SDK_DIR)/tools/build_tools/build_files/project_main.mk
ifeq ($(wildcard $(PROJECT_MAKE_FILE)),)
    $(error "$(PROJECT_MAKE_FILE) not exist, please check sdk directory.")
endif
include $(PROJECT_MAKE_FILE)
