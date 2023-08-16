PROJNAME := PAZ_Graphics
LIBNAME := $(shell tr '[:upper:]' '[:lower:]' <<< $(shell sed 's/_//g' <<< $(PROJNAME)))
ifeq ($(OS), Windows_NT)
    LIBPATH := /mingw64/lib
    INCLPATH := /mingw64/include
    OSPRETTY := Windows
else
    ifeq ($(shell uname -s), Darwin)
        LIBPATH := /usr/local/lib
        OSPRETTY := macOS
    else
        LIBPATH := /usr/local/lib64
        OSPRETTY := Linux
    endif
    INCLPATH := /usr/local/include
endif
CXXVER := 14
OPTIM := fast
CFLAGS := -O$(OPTIM) -Wall -Wextra -Wno-missing-braces
ifeq ($(OSPRETTY), macOS)
    CFLAGS += -mmacosx-version-min=10.10
else
    ifeq ($(OSPRETTY), Windows)
        CFLAGS += -Wno-cast-function-type
    endif
endif
CXXFLAGS := -std=c++$(CXXVER) $(CFLAGS)
ifeq ($(OSPRETTY), Windows)
    CXXFLAGS += -Wno-deprecated-copy
endif
ARFLAGS := -rcs

CSRC := $(wildcard *.c) $(wildcard *.cpp)
CSRC := $(filter-out $(wildcard *.save.*), $(CSRC))
OBJCSRC := $(wildcard *.mm)
OBJCSRC := $(filter-out $(wildcard *.save.*), $(OBJCSRC))
OBJ := $(patsubst %.c, %.o, $(patsubst %.cpp, %.c, $(CSRC)))
ifeq ($(OSPRETTY), macOS)
    OBJ += $(OBJCSRC:%.mm=%.o)
endif

REINSTALLHEADER := $(shell cmp -s $(PROJNAME) $(INCLPATH)/$(PROJNAME); echo $$?)

default: $(OBJ)
	$(RM) lib$(LIBNAME).a
	ar $(ARFLAGS) lib$(LIBNAME).a $^
	make -C examples

ifneq ($(REINSTALLHEADER), 0)
install:
	cp $(PROJNAME) $(INCLPATH)/
	cp lib$(LIBNAME).a $(LIBPATH)/
else
install:
	cp lib$(LIBNAME).a $(LIBPATH)/
endif

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
	make -C examples clean

print-% : ; @echo $* = $($*)
