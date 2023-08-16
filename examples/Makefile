CXXVER := 17
MINMACOSVER := 10.11

ifeq ($(OS), Windows_NT)
    LIBPATH := /mingw64/lib
    OSPRETTY := Windows
else
    ifeq ($(shell uname -s), Darwin)
        OSPRETTY := macOS
    else
        OSPRETTY := Linux
    endif
    LIBPATH := /usr/local/lib
endif
CXXFLAGS := -std=c++$(CXXVER) -O3 -Wall -Wextra -Wno-missing-braces -Wold-style-cast
ifeq ($(OSPRETTY), macOS)
    CXXFLAGS += -mmacosx-version-min=$(MINMACOSVER) -Wunguarded-availability -Wno-string-plus-int
else
    ifeq ($(OSPRETTY), Windows)
        CXXFLAGS += -Wno-deprecated-copy -static -mwindows
    endif
    CXXFLAGS += -pthread
endif
CXXFLAGS += -I..
LDLIBS := ../libpazgraphics.a $(LIBPATH)/libpazio.a
ifeq ($(OSPRETTY), macOS)
    LDLIBS += -framework MetalKit -framework Metal -framework Cocoa -framework IOKit
else
    ifeq ($(OSPRETTY), Linux)
        LDLIBS += $(LIBPATH)/libglfw3.a -lGL -lX11 -ldl
    else
        LDLIBS += $(LIBPATH)/libpthread.a -ld3d11 -ldxgi -ld3dcompiler -ldxguid
        LDFLAGS += -static-libgcc -static-libstdc++
    endif
endif

SRC := $(wildcard *.cpp)
EXE := $(SRC:.cpp=)

print-% : ; @echo $* = $($*)

.PHONY: $(SRC)

all: $(EXE)

ifeq ($(OSPRETTY), macOS)
%: %.cpp
	$(CXX) -arch arm64 -o $@_arm64 $< $(CXXFLAGS) $(INCL) $(LDFLAGS) $(LDLIBS)
	$(CXX) -arch x86_64 -o $@_x86_64 $< $(CXXFLAGS) $(INCL) $(LDFLAGS) $(LDLIBS)
	lipo -create -output $@ $@_arm64 $@_x86_64
endif

clean:
	$(RM) $(EXE)
