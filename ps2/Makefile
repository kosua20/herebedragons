ifndef SOURCE_PATH
SOURCE_PATH = ./
endif

EE_BIN = dragon.elf
EE_OBJS = src/transform.o src/Pad.o src/Object.o src/Skybox.o src/Scene.o src/dragon.o

EE_LIBS = -lc -ldraw -lgraph -lmath3d -lpacket2 -ldma -lpatches -ldebug -lpad

GLOBAL_DEFINES =                      \
	-I$(SOURCE_PATH)                  \
	-Dnullptr=NULL                    \
	-DLOG_PRINTF_ADD_MESSAGE_PREFIX=1 \
	-DUSE_CUSTOM_ASSERT=1

# ---------------------------------------------------------

EE_CXXFLAGS +=                \
	-O3                       \
	-funroll-loops            \
	-fexpensive-optimizations \
	-fno-exceptions           \
	-fno-rtti                 \
	-Wall                     \
	-Wformat=2                \
	-Wmissing-braces          \
	-Wparentheses             \
	-Wpointer-arith           \
	-Wreturn-type             \
	-Wsequence-point          \
	-Wswitch                  \
	-Wuninitialized           \
	-Wunknown-pragmas         \
	-Wwrite-strings           \
	$(GLOBAL_DEFINES)


all: $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)


include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
