#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

# Directories.
OBJ_DIR			:= obj
LIB_DIR			:= lib
BIN_DIR			:= bin
SRC_DIR			:= src
INCLUDE_DIR		:= include
TEST_OBJ_DIR		:= $(OBJ_DIR)/test
TEST_SRC_DIR		:= src
INSTALL_HEADER_DIR	?= $(PORTLIBS_PATH)/wii/include
INSTALL_LIB_DIR		?= $(PORTLIBS_PATH)/wii/lib

# Tools.
PIPE_TO_SED := 2>&1 | sed "s/:\([0-9]*\):/\(\1\) :/"

# Library source files.
SRCS	:= $(filter-out $(SRC_DIR)/playwave.c $(SRC_DIR)/playmus.c $(SRC_DIR)/music_cmd.c, $(wildcard $(SRC_DIR)/*.c)) $(wildcard $(SRC_DIR)/mikmod/*.c) $(wildcard $(SRC_DIR)/timidity/*.c) 
# $(wildcard $(SRC_DIR)/native_midi/*.c)

# Library object files.
OBJS	:= $(subst $(SRC_DIR),$(OBJ_DIR),$(SRCS:.c=.o))

# Test source files.
# It can be useful to switch this variable around to select individual tests which are problematic.
TEST_SRCS	:= $(TEST_SRC_DIR)/playwave.c $(TEST_SRC_DIR)/playmus.c

# Test object files.
TEST_OBJS	:= $(subst $(TEST_SRC_DIR)/,$(TEST_OBJ_DIR)/,$(TEST_SRCS:.c=.o))

# Test .DOL files.
TEST_DOLS	:= $(subst $(TEST_OBJ_DIR),$(BIN_DIR),$(TEST_OBJS:.o=.dol))

# What's a full build?
all: $(LIB_DIR)/libSDL_mixer.a $(INCLUDE_DIR)/SDL_mixer.h

# How to install to SD card ready for running.
test-install: $(TEST_DOLS)
	@-mkdir -p dols
	cp $(TEST_DOLS) dols

# How to delete the intermediate files.
clean:
	@echo Cleaning $(LIB_DIR)/libSDL_mixer.a $(OBJ_DIR)
	@rm -f $(LIB_DIR)/libSDL_mixer.a $(OBJS) $(TEST_OBJS)
	
install: all
	@mkdir -p $(INSTALL_HEADER_DIR) $(INSTALL_LIB_DIR)
	@mkdir -p $(INSTALL_HEADER_DIR)/SDL
	@cp -frv $(LIB_DIR)/*.* $(INSTALL_LIB_DIR)
	@cp -frv include/*.* $(INSTALL_HEADER_DIR)/SDL

# How to build a library.
$(LIB_DIR)/libSDL_mixer.a: $(OBJS)
	@echo Archiving $@
	@-mkdir -p $(dir $@)
	@powerpc-eabi-ar crs $@ $(OBJS)
	@echo ----
	
# How to build a DOL.
$(BIN_DIR)/%.dol: $(TEST_OBJ_DIR)/%.elf
	@echo Creating DOL $@
	@-mkdir -p $(dir $@)
	elf2dol $< $@
	@echo ----

# How to copy a header file
$(INCLUDE_DIR)/SDL_mixer.h: $(SRC_DIR)/SDL_mixer.h
	@echo Copy $(SRC_DIR)/SDL_mixer.h to $(INCLUDE_DIR)
	@-mkdir -p $(dir $@)
	@cp $(SRC_DIR)/SDL_mixer.h $(INCLUDE_DIR)

# Compilation flags.
COMMON_FLAGS	:= -g -O3 -mrvl -Wall $(MACHDEP)
INCLUDES		:= -Iinclude -I$(LIBOGC_INC) -I$(PORTLIBS_PATH)/wii/include/SDL -I$(PORTLIBS_PATH)/wii/include/mpeg -I../smpeg/include -Isrc/mikmod -Isrc/timidity -I../SDL/include
# -include sys/wait.h 
# -include sys/syslimits.h
# -Isrc/native_midi
DEFINES			:= -DGEKKO -DDEBUG_ERROR -DDEBUG_TIMERS -DDEBUG_THREADS -DDEBUG_BUILD -DDEBUG_CONVERT -DWAV_MUSIC -DMOD_MUSIC -DMID_MUSIC -DUSE_TIMIDITY_MIDI -DMP3_MUSIC -DOGG_USE_TREMOR -DOGG_MUSIC
# -DUSE_NATIVE_MIDI
# -DOGG_MUSIC 
# 
#   -DCMD_MUSIC
CFLAGS			:= $(foreach dir,$(PORTLIBS),-L$(dir)/lib) $(foreach dir,$(PORTLIBS),-I$(dir)/include) $(COMMON_FLAGS) $(INCLUDES) $(DEFINES)

# Test link flags.
LDFLAGS			:= $(COMMON_FLAGS) -L$(LIB_DIR) -L$(LIBOGC_LIB) -L$(PORTLIBS_PATH)/wii/lib -lSDL_mixer -lsmpeg -lSDL -lfat -lwiiuse -lbte -logc -lm -lvorbisidec

# How to link an ELF.
$(TEST_OBJ_DIR)/%.elf: $(TEST_OBJ_DIR)/%.o $(LIB_DIR)/libSDL_mixer.a $(DEVKITPRO)/libSDL/lib/libSDL.a
	@echo Linking $@
	@-mkdir -p $(dir $@)
	powerpc-eabi-g++ -o $@ $< $(LDFLAGS)
	#keep elf for debugging
	#cp $@ /tmp/elf

# How to compile C file (Tests).
$(TEST_OBJ_DIR)/%.o: $(TEST_SRC_DIR)/playmus.c $(TEST_SRC_DIR)/playwave.c
	@echo Compiling $<
	@-mkdir -p $(dir $@)
	powerpc-eabi-gcc $(CFLAGS) -c $< -o $@ $(PIPE_TO_SED)

# How to compile C file (SDL library).
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo Compiling $<
	@-mkdir -p $(dir $@)
	powerpc-eabi-gcc $(CFLAGS) -c $< -o $@ $(PIPE_TO_SED)
