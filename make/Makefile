BIN      := potrait
SRC_DIR  := ../src/sources
SRC_FILES:= main.cc
CXXFLAGS += `pkg-config --cflags opencv` -I../src/headers -I../include
LIBS     += `pkg-config --libs opencv`

OPENCV_PREFIX := $(shell pkg-config --variable=prefix opencv)
OPENCV_HAAR_DIR := $(OPENCV_PREFIX)/Share/OpenCV/haarcascades
DATAIN_FILES := $(OPENCV_HAAR_DIR)/haarcascade_frontalface_alt.xml \
                $(OPENCV_HAAR_DIR)/haarcascade_eye_tree_eyeglasses.xml

COMMON_DIR?=../../common
include $(COMMON_DIR)/datain/make/link.mk
include $(COMMON_DIR)/lib/make/link.mk
include $(COMMON_DIR)/mk/build.mk
