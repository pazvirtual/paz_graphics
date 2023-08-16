PROJNAME := PAZ_Graphics
LIBNAME := $(shell echo $(PROJNAME) | sed 's/_//g' | tr '[:upper:]' '[:lower:]')
ifeq ($(OS), Windows_NT)
    LIBPATH := /mingw64/lib
    INCLPATH := /mingw64/include
    OSPRETTY := Windows
else
    ifeq ($(shell uname -s), Darwin)
        OSPRETTY := macOS
    else
        OSPRETTY := Linux
    endif
    LIBPATH := /usr/local/lib
    INCLPATH := /usr/local/include
endif
CXXVER := 14
OPTIM := 3
ZIPNAME := $(PROJNAME)-$(OSPRETTY)
CFLAGS := -O$(OPTIM) -Wall -Wextra -Wno-missing-braces
ifeq ($(OSPRETTY), macOS)
    CFLAGS += -mmacosx-version-min=10.11 -Wunguarded-availability -Wno-string-plus-int
else
    ifeq ($(OSPRETTY), Windows)
        CFLAGS += -Wno-cast-function-type
    endif
endif
CXXFLAGS := -std=c++$(CXXVER) $(CFLAGS) -Wold-style-cast
ifeq ($(OSPRETTY), Windows)
    CXXFLAGS += -Wno-deprecated-copy
endif
ARFLAGS := -rcs

CSRC := $(wildcard *.c) $(wildcard *.cpp)
ifeq ($(OSPRETTY), macOS)
    EXCL := gl_core_4_1.c $(patsubst %_macos.mm, %.cpp, $(wildcard *_macos.mm)) $(wildcard *_linux.cpp) $(wildcard *_windows.cpp)
else
    ifeq ($(OSPRETTY), Windows)
        EXCL := gl_core_4_1.c $(wildcard *_linux.cpp)
    else
        EXCL := $(wildcard *_windows.cpp)
    endif
endif
CSRC := $(filter-out $(EXCL), $(CSRC))
OBJCSRC := $(wildcard *.mm)
OBJ := $(patsubst %.c, %.o, $(patsubst %.cpp, %.c, $(CSRC)))
ifeq ($(OSPRETTY), macOS)
    OBJ += $(OBJCSRC:%.mm=%.o)
endif

REINSTALLHEADER := $(shell cmp -s $(PROJNAME) $(INCLPATH)/$(PROJNAME); echo $$?)
REINSTALLLIB := $(shell cmp -s lib$(LIBNAME).a $(LIBPATH)/lib$(LIBNAME).a; echo $$?)

print-% : ; @echo $* = $($*)

.PHONY: test
default: test
	$(MAKE) -C examples

lib$(LIBNAME).a: $(OBJ)
	$(RM) lib$(LIBNAME).a
	ar $(ARFLAGS) lib$(LIBNAME).a $^

ifneq ($(REINSTALLHEADER), 0)
    ifneq ($(REINSTALLLIB), 0)
install: $(PROJNAME) lib$(LIBNAME).a
	cp $(PROJNAME) $(INCLPATH)/
	cp lib$(LIBNAME).a $(LIBPATH)/
    else
install: $(PROJNAME) lib$(LIBNAME).a
	cp $(PROJNAME) $(INCLPATH)/
    endif
else
    ifneq ($(REINSTALLLIB), 0)
install: $(PROJNAME) lib$(LIBNAME).a
	cp lib$(LIBNAME).a $(LIBPATH)/
    else
install: $(PROJNAME) lib$(LIBNAME).a
    endif
endif

test: lib$(LIBNAME).a
	$(MAKE) -C test
	test/test

analyze: $(OBJCSRC)
	$(foreach n, $(OBJCSRC), clang++ --analyze $(n) $(CXXFLAGS) && $(RM) $(n:%.mm=%.plist);)

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

%.o: %.mm
	$(CC) -c -o $@ $< $(CXXFLAGS)

clean:
	$(RM) $(OBJ) lib$(LIBNAME).a
	$(MAKE) -C test clean
	$(MAKE) -C examples clean

zip: $(PROJNAME) lib$(LIBNAME).a
	zip -j $(ZIPNAME).zip $(PROJNAME) lib$(LIBNAME).a
