###################################################################
# makefile通用包含文件
#
# 这文件是makefile公用代码，包含了大部分makefile要用到的公用代码
# 使用g++编译C++代码，暂不支持别的编译器
###################################################################

####以下是包含本文件前必须定义的变量####

  #BIN       变量定义输出文件的文件名
  #SRC_DIR   变量定义makefile要编译的文件所在跟目录，所有编译文件都在这个目录或各级子目录下
  #SRC_FILES 定义所有要编译的C++文件，文件路径都是相对SRC_DIR目录

  #嵌入本文件前必须先定义BIN变量
  ifndef BIN
    $(error "BIN" is not defined.)
  endif

  #嵌入本文件前必须先定义SRC_DIR变量
  ifndef SRC_DIR
    $(error "SRC_DIR" is not defined.)
  endif

  #嵌入本文件前必须先定义SRC_FILES变量
  ifndef SRC_FILES
    $(error "SRC_FILES" is not defined.)
  endif

  #SRC_FILES不能含有..
  ifeq ($(findstring ..,$(SRC_FILES)),..)
    $(error ".." in SRC_FILES is not allowed.)
  endif

####以下是包含本文件前的可选参数####

  #使用DEBUG编译（应该在make命令传入：make debug=1，或用export debug=1）
  #debug            ?=

  #编译输出类型，可以为EXEC（执行文件）、STATIC（静态链接库）
  BUILD_TYPE        ?=
  #另外，在包含本文件前或包含本文件后，给all增加依赖项可以增加编译的文件（但不要定义all的方法）

  #附加静态链接库，默认终极目标会自动产生对其依赖
  EXT_LNK_OBJS      ?=

  #自动收集的C++源文件（在gen目录下）
  GEN_FILES         ?=

  #支持的通用变量
  CXX               ?= g++
  CXXFLAGS          ?=
  CPPFLAGS          ?=
  AR                ?= ar
  ARFLAGS           ?=
  RM                ?= rm -f

####以下开始是本文件的主要逻辑实现部分####

CXXFLAGS += -std=c++11 -Wall

#编译输出路径
BIN_ROOT_DIR    := bin
BIN_RELEASE_DIR := $(BIN_ROOT_DIR)/release
BIN_DEBUG_DIR   := $(BIN_ROOT_DIR)/debug

#临时文件输出路径
TMP_ROOT_DIR    := obj
TMP_RELEASE_DIR := $(TMP_ROOT_DIR)/release
TMP_DEBUG_DIR   := $(TMP_ROOT_DIR)/debug

#自动收集文件路径
GEN_DIR         := $(TMP_ROOT_DIR)/gen

ifdef debug
  BIN_DIR := $(BIN_DEBUG_DIR)
  TMP_DIR := $(TMP_DEBUG_DIR)
  CXXFLAGS += -D_DEBUG -g -O0
else
  BIN_DIR := $(BIN_RELEASE_DIR)
  TMP_DIR := $(TMP_RELEASE_DIR)
  CXXFLAGS += -DNDEBUG -O3
endif

#所有编译中间文件
ALL_O := $(patsubst %,$(TMP_DIR)/src/%.o,$(SRC_FILES)) $(patsubst %,$(TMP_DIR)/gen/%.o,$(GEN_FILES))
ALL_D := $(patsubst %.o,%.d,$(ALL_O))

ifneq ($(MAKECMDGOALS),clean)

#定义默认终极目标$(BIN_DIR)/$(BIN)
#链接时分为静态库和运行文件两种，带.a的文件名为静态链接库

.PHONY : run all

all : $(BIN_DIR)/$(BIN)

run : all
	@$(BIN_DIR)/$(BIN)

BUILD_CMD :=

ifeq ($(BUILD_TYPE),)
  ifeq ($(findstring .a,$(BIN)),.a)
    BUILD_TYPE := STATIC
  else
    BUILD_TYPE := EXEC
  endif
endif

ifeq ($(BUILD_TYPE),EXEC)
  EXT_LNK_OBJS := $(filter %.o %.a,$(EXT_LNK_OBJS));
  BUILD_CMD = $(CXX) -o $@ $(LDFLAGS) $(LIBS) $^
endif

ifeq ($(BUILD_TYPE),STATIC)
  EXT_LNK_OBJS := $(filter %.o,$(EXT_LNK_OBJS));
  BUILD_CMD = $(AR) $(ARFLAGS) $@ $^
endif

ifeq ($(BUILD_CMD),)
  $(error Unvalid variable BUILD_TYPE=$(BUILD_TYPE))
endif

COMPILE_CMD = $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
DEPEND_CMD  = $(CXX) $(CXXFLAGS) $(CPPFLAGS) -MM -MT "$(TMP_DIR)/$*.o" -o $@ $<

$(BIN_DIR)/$(BIN) : $(ALL_O) $(EXT_LNK_OBJS)
	@mkdir -p $(@D)
	$(BUILD_CMD)
	@find . -name "*.lis" | xargs $(RM)
	@find . -name "tp??????" | xargs $(RM)

$(TMP_DIR)/src/%.o : $(SRC_DIR)/%
	@mkdir -p $(@D)
	$(COMPILE_CMD)

$(TMP_DIR)/src/%.d : $(SRC_DIR)/%
	@mkdir -p $(@D)
	$(DEPEND_CMD)

$(TMP_DIR)/gen/%.o : $(GEN_DIR)/%
	@mkdir -p $(@D)
	$(COMPILE_CMD)

$(TMP_DIR)/gen/%.d : $(GEN_DIR)/%
	@mkdir -p $(@D)
	$(DEPEND_CMD)

# 导入所有o文件对源代码的依赖关系
sinclude $(ALL_D)

# $(ALL_O) $(ALL_D) 都是中间文件，编译完成后不删除
.SECONDARY : $(ALL_O) $(ALL_D)

endif
#endif: ifneq ($(MAKECMDGOALS),clean)

.PHONY : clean
clean :
	$(RM) -r $(BIN_ROOT_DIR) $(TMP_ROOT_DIR)
	@find . -name "*.lis" | xargs $(RM)
	@find . -name "tp??????" | xargs $(RM)
