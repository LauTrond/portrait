BIN      := libportrait.a
SRC_DIR  := ../../src/sources
SRC_FILES:= \
    portrait/algorithm.cc \
    portrait/exception.cc \
    portrait/facedetect.cc \
    portrait/haarcascade.cc \
    portrait/processing.cc \
    snappy/snappy.cc \
    snappy/snappy-sinksource.cc \
    snappy/snappy-stubs-internal.cc \
    sybie/common/Arguments.cc \
    sybie/common/Event.cc \
    sybie/common/ManagedRes.cc \
    sybie/common/Streaming.cc \
    sybie/common/Text.cc \
    sybie/common/Time.cc \
    sybie/datain/Coding.cc \
    sybie/datain/DataItem.cc \
    sybie/datain/Load.cc \
    sybie/datain/Pool.cc \
    sybie/datain/Stream.cc
CXXFLAGS += \
    -I../../src/headers \
    -I../../src/headers/snappy \
    -I../../include \
    `pkg-config --cflags opencv`
LIBS += `pkg-config --libs opencv`

include build.mk
