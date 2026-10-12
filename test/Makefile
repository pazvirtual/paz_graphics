CXXVER := 17
MINMACOSVER := 11.0

ifeq ($(OS), Windows_NT)
    OSPRETTY := Windows
    ifeq ($(MSYSTEM), UCRT64)
        CC := gcc
        CXX := g++
        LIBPATH := /ucrt64/lib
    else ifeq ($(MSYSTEM), CLANG64)
        CC := clang
        CXX := clang++
        LIBPATH := /clang64/lib
    else
        $(error Unsupported Windows environment)
    endif
else
    ifeq ($(shell uname -s), Darwin)
        OSPRETTY := macOS
        CC := clang
        CXX := clang++
    else
        OSPRETTY := Linux
        CC := gcc
        CXX := g++
    endif
    LIBPATH := /usr/local/lib
endif
CXXFLAGS := -std=c++$(CXXVER) -O3 -Wall -Wextra -Wno-missing-braces \
    -Wold-style-cast
ifeq ($(OSPRETTY), macOS)
    CXXFLAGS += -mmacosx-version-min=$(MINMACOSVER) -Wunguarded-availability
else ifeq ($(OSPRETTY), Windows)
    CXXFLAGS += -mwindows
else
    CXXFLAGS += -pthread
endif
ifeq ($(CC), clang)
    CXXFLAGS += -Wno-string-plus-int
endif
CXXFLAGS += -I..
LDLIBS := ../libpazgraphics.a
ifeq ($(OSPRETTY), macOS)
    LDLIBS += -framework MetalKit -framework Metal -framework Cocoa -framework \
        IOKit
else ifeq ($(OSPRETTY), Windows)
    LDLIBS += -ld3d11 -ldxgi -ld3dcompiler -ldxguid
    ifeq ($(CC), gcc)
        LDLIBS += -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic
    else
        LDLIBS += -Wl,-Bstatic -lc++ -Wl,-Bdynamic
    endif
else
    # Try a few locations for GLFW.
    ifneq (, $(wildcard $(LIBPATH)/libglfw3.a))
        LDLIBS += $(LIBPATH)/libglfw3.a
    else ifneq (, $(wildcard /usr/lib/libglfw3.a))
        LDLIBS += /usr/lib/libglfw3.a
    else ifneq (, $(wildcard /usr/lib64/libglfw3.a))
        LDLIBS += /usr/lib64/libglfw3.a
    else ifneq (, $(wildcard /usr/lib/x86_64-linux-gnu/libglfw3.a))
        LDLIBS += /usr/lib/x86_64-linux-gnu/libglfw3.a
    else
        $(error Could not find "libglfw3.a".)
    endif
    LDLIBS += -lGL -lX11 -ldl
endif

SRC := $(wildcard *.cpp)
EXE := $(SRC:.cpp=)

print-% : ; @echo "$* = $($*)"

.PHONY: $(SRC)

all: $(EXE)

ifeq ($(OSPRETTY), macOS)
%: %.cpp
	$(CXX) -arch arm64 -o $@_arm64 $< $(CXXFLAGS) $(INCL) $(LDFLAGS) \
	    $(LDLIBS)
	$(CXX) -arch x86_64 -o $@_x86_64 $< $(CXXFLAGS) $(INCL) $(LDFLAGS) \
	    $(LDLIBS)
	lipo -create -output $@ $@_arm64 $@_x86_64
endif

ifeq ($(OSPRETTY), macOS)
clean:
	$(RM) $(EXE) $(foreach n,$(EXE),$n_arm64 $n_x86_64)
else
clean:
	$(RM) $(EXE)
endif
