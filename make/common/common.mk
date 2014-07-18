ifndef INCLUDE_LIB_PORTRAIT_LINK_MK
INCLUDE_LIB_PORTRAIT_LINK_MK:=1

PORTRAIT_DIR ?= ../..
ifneq ($(PORTRAIT_DIR), $(wildcard $(PORTRAIT_DIR)))
    $(error Cannot find portrait at $(PORTRAIT_DIR), define PORTRAIT_DIR first.)
endif

PORTRAIT_LIB_DIR := $(PORTRAIT_DIR)/make/common

PORTRAIT_LIBA_DEBUG := \
  bin/debug/libportrait.a
PORTRAIT_CASSCADE_DEBUG := \
  obj/debug/src/portrait/haarcascade.cc.o
PORTRAIT_LIBA_RELEASE := \
  bin/release/libportrait.a
PORTRAIT_CASSCADE_RELEASE := \
  obj/release/src/portrait/haarcascade.cc.o

ifdef debug
  PORTRAIT_LIBA:=$(PORTRAIT_LIBA_DEBUG)
  PORTRAIT_CASSCADE:=$(PORTRAIT_CASSCADE_DEBUG)
else
  PORTRAIT_LIBA:=$(PORTRAIT_LIBA_RELEASE)
  PORTRAIT_CASSCADE:=$(PORTRAIT_CASSCADE_RELEASE)
endif

CXXFLAGS     += `pkg-config --cflags opencv` -I$(PORTRAIT_DIR)/include
LIBS         += `pkg-config --libs opencv`
EXT_LNK_OBJS += $(PORTRAIT_LIB_DIR)/$(PORTRAIT_LIBA) \
                $(PORTRAIT_LIB_DIR)/$(PORTRAIT_CASSCADE)

ifneq ($(MAKECMDGOALS),clean)

#定义all空依赖避免覆盖build.mk中的默认终极目标
all :

FORCE:

define PORTRAIT_MAKE_RULE
$(PORTRAIT_LIB_DIR)/$(1) : \
  $(if $(shell $(MAKE) -C $(PORTRAIT_LIB_DIR) print=1 $(2)),FORCE)
	@$(MAKE) -C $(PORTRAIT_LIB_DIR) $(1) $(2)
endef

$(eval $(call PORTRAIT_MAKE_RULE,$(PORTRAIT_LIBA_DEBUG),debug=1))
$(eval $(call PORTRAIT_MAKE_RULE,$(PORTRAIT_CASSCADE_DEBUG),debug=1))
$(eval $(call PORTRAIT_MAKE_RULE,$(PORTRAIT_LIBA_RELEASE),))
$(eval $(call PORTRAIT_MAKE_RULE,$(PORTRAIT_CASSCADE_RELEASE),))

endif

include $(PORTRAIT_LIB_DIR)/build.mk

endif #ifndef INCLUDE_LIB_PORTRAIT_LINK_MK
