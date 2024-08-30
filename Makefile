#
# Basic KallistiOS skeleton / test program
# Copyright (C) 2001-2004 Megan Potter
# Copyright (C) 2024 Falco Girgis
#   

# Put the filename of the output binary here
TARGET = ddnet-dc.elf

SOURCE_DIRS  := \
	src \
	src/base \
	src/engine/client \
	src/engine/shared \
	src/engine/external/json-parser \
	src/engine/external/wavpack \
	src/engine/external/pnglite \
	src/game \
	src/game/generated \
	src/game/client \
	src/game/client/components \
	src/game/editor

CPP_FILES    := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.cpp))
C_FILES      := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
OBJS         := $(C_FILES:%.c=%.o) $(CPP_FILES:%.cpp=%.o)

CFLAGS     := -Isrc -I$(KOS_PORTS)/include/zlib -I$(KOS_PORTS)/include/opus -I$(KOS_PORTS)/include/opusfile
LIBS       := -lKGL -lopusfile -lopus -logg -lz
#CXXFLAGS     := -IlibADXplay/LibADX
#LIBS         := -lADX

# Optional path to a directory of resources to bundle within your ELF binary.
# Its contents are accessible via the "/rd/" virtual directory at runtime.
#KOS_ROMDISK_DIR = romdisk
#OBJS           += romdisk.o

# Main rule which forces our ELF binary to be built
all: rm-elf $(TARGET)

# Include the common KOS Makefile rules and configuration
include $(KOS_BASE)/Makefile.rules

# Cleans the binary ELF file plus the intermediate .o files
clean: rm-elf
	-rm -f $(OBJS)

# Removes the binary ELF file
rm-elf:
	-rm -f $(TARGET)

# Invokes the compiler to build the target from our object files
$(TARGET): $(OBJS)
	kos-c++ -o $(TARGET) $(OBJS) $(LIBS)

# Attempts to run the target using the configured loader application
run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

# Creates a distributable/release ELF which strips away its debug symbols
dist: $(TARGET)
	$(KOS_STRIP) $(TARGET)

cdi: $(TARGET) cd_root
	mkdcdisc -e $(TARGET) -d cd_root -o $(TARGET).cdi

cd_root:
	mkdir cd_root

src/game/generated:
	@[ -d $@ ] || mkdir -p $@
	python datasrc/compile.py network_source > $@/protocol.cpp
	python datasrc/compile.py network_header > $@/protocol.h
	python datasrc/compile.py client_content_source > $@/client_data.cpp
	python datasrc/compile.py client_content_header > $@/client_data.h
	python scripts/cmd5.py src/engine/shared/protocol.h $@/protocol.h src/game/tuning.h src/game/gamecore.cpp $@/protocol.h > $@/nethash.cpp
