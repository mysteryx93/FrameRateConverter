# Linux build of the native FrameRateConverter plugin
# (AviSynth+ and VapourSynth).
#
#   make
#   make avs
#   make vs

CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17 -fPIC -Wall

SRC   := Src
BUILD := build

AVS_CFLAGS := $(shell pkg-config --cflags avisynth)

# Do not add Src/Avisynth to the include path: quoted includes from
# those .cpp files already find siblings, and <avisynth.h> must resolve
# to the system AviSynth+ headers rather than the bundled Windows copy.
INCLUDES := \
	-I$(SRC)/Common \
	-I$(SRC)/Environments \
	-I$(SRC)/VapourSynth

COMMON_SRCS := \
	$(SRC)/Common/StripeMaskBase.cpp \
	$(SRC)/Common/ContinuousMaskBase.cpp \
	$(SRC)/Common/ConvertFpsLimitBase.cpp \
	$(SRC)/Common/merge.cpp \
	$(SRC)/Environments/instrset_detect.cpp

AVX2_SRC := $(SRC)/Common/merge_avx2.cpp
AVX2_OBJ := $(BUILD)/merge_avx2.o

AVS_SRCS := \
	$(COMMON_SRCS) \
	$(SRC)/Avisynth/StripeMaskAvs.cpp \
	$(SRC)/Avisynth/ContinuousMaskAvs.cpp \
	$(SRC)/Avisynth/ConvertFpsLimitAvs.cpp \
	$(SRC)/Avisynth/conditional.cpp \
	$(SRC)/Avisynth/conditional_functions.cpp \
	$(SRC)/Avisynth/InitAvs.cpp

VS_SRCS := \
	$(COMMON_SRCS) \
	$(SRC)/VapourSynth/StripeMaskVpy.cpp \
	$(SRC)/VapourSynth/ContinuousMaskVpy.cpp \
	$(SRC)/VapourSynth/ConvertFpsLimitVpy.cpp \
	$(SRC)/VapourSynth/InitVpy.cpp

AVS_SO := $(BUILD)/avisynth/libframerateconverter.so
VS_SO  := $(BUILD)/vapoursynth/libframerateconverter.so

.PHONY: all avs vs clean

all: avs vs

avs: $(AVS_SO)
vs: $(VS_SO)

$(AVX2_OBJ): $(AVX2_SRC)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -mavx2 -mfma $(INCLUDES) -c -o $@ $<

VERSION_H := $(SRC)/Common/FrcVersion.h

$(AVS_SO): $(AVS_SRCS) $(AVX2_OBJ) $(VERSION_H)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AVS_CFLAGS) $(INCLUDES) -shared -o $@ $(AVS_SRCS) $(AVX2_OBJ)

$(VS_SO): $(VS_SRCS) $(AVX2_OBJ) $(VERSION_H)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -shared -o $@ $(VS_SRCS) $(AVX2_OBJ)

clean:
	rm -rf $(BUILD)
