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
CXXVER := 14
CXXFLAGS := -std=c++$(CXXVER) -Ofast -Wall -Wextra -Wno-missing-braces -Wold-style-cast
ifeq ($(OSPRETTY), macOS)
    CXXFLAGS += -mmacosx-version-min=10.11 -Wunguarded-availability
else
    ifeq ($(OSPRETTY), Windows)
        CXXFLAGS += -Wno-deprecated-copy -static -mwindows
    endif
    CXXFLAGS += -pthread
endif
CXXFLAGS += -I..
LDLIBS := ../libpazgraphics.a
ifeq ($(OSPRETTY), macOS)
    LDLIBS += -framework MetalKit -framework Metal -framework Cocoa -framework IOKit
else
    LDLIBS += $(LIBPATH)/libglfw3.a
    ifeq ($(OSPRETTY), Linux)
        LDLIBS += -lGL -lX11 -ldl
    else
        LDLIBS += -ld3d11 -ldxgi -ld3dcompiler -ldxguid
    endif
endif

SRC := $(wildcard *.cpp)
EXE := $(SRC:.cpp=)

.PHONY: $(SRC)

all: $(EXE)

clean:
	$(RM) $(EXE)

print-% : ; @echo $* = $($*)
