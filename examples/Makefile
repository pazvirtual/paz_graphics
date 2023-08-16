ifeq ($(OS), Windows_NT)
    LIBPATH := /mingw64/lib
    OSPRETTY := Windows
else
    ifeq ($(shell uname -s), Darwin)
        LIBPATH := /usr/local/lib
        OSPRETTY := macOS
    else
        LIBPATH := /usr/local/lib64
        OSPRETTY := Linux
    endif
endif
CXXVER := 14
CXXFLAGS := -std=c++$(CXXVER) -Ofast -Wall -Wextra -Wno-missing-braces
ifeq ($(OSPRETTY), macOS)
    CXXFLAGS += -mmacosx-version-min=10.10
else
    ifeq ($(OSPRETTY), Windows)
        CXXFLAGS += -Wno-deprecated-copy -static -mwindows
    endif
    CXXFLAGS += -pthread
endif
INCL := -I..
LIBS := ../libpazgraphics.a $(LIBPATH)/libpazio.a
ifeq ($(OSPRETTY), macOS)
    LIBS += -framework MetalKit -framework Metal -framework Cocoa
else
    LIBS += $(LIBPATH)/libglfw3.a -lstdc++fs
    ifeq ($(OSPRETTY), Linux)
        LIBS += -lGL -lX11 -ldl
    else
        LIBS += -lgdi32 -lopengl32
    endif
endif

SRC := $(wildcard *.cpp)
OBJ := $(SRC:.cpp=.o)
EXE := $(OBJ:.o=)

.PHONY: $(SRC)

all: $(EXE)

%: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCL) $(LIBS)

clean:
	$(RM) $(OBJ) $(EXE)

print-% : ; @echo $* = $($*)
