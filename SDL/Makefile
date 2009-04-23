# Directories.
OBJ_DIR			:= obj/wii
LIB_DIR			:= lib
BIN_DIR			:= bin/wii
SDL_OBJ_DIR		:= $(OBJ_DIR)/sdl
SDL_SRC_DIR		:= src
TEST_OBJ_DIR	:= $(OBJ_DIR)/test
TEST_SRC_DIR	:= test

# Tools.
PIPE_TO_SED := 2>&1 | sed "s/:\([0-9]*\):/\(\1\) :/"

# Library source files.
SDL_SRCS	:= \
	$(wildcard $(SDL_SRC_DIR)/*.c) \
	$(wildcard $(SDL_SRC_DIR)/audio/*.c) \
	$(wildcard $(SDL_SRC_DIR)/audio/dummy/*.c) \
	$(wildcard $(SDL_SRC_DIR)/audio/wii/*.c) \
	$(wildcard $(SDL_SRC_DIR)/cdrom/*.c) \
	$(wildcard $(SDL_SRC_DIR)/cdrom/dummy/*.c) \
	$(wildcard $(SDL_SRC_DIR)/cpuinfo/*.c) \
	$(wildcard $(SDL_SRC_DIR)/events/*.c) \
	$(wildcard $(SDL_SRC_DIR)/file/*.c) \
	$(wildcard $(SDL_SRC_DIR)/file/wii/*.c) \
	$(wildcard $(SDL_SRC_DIR)/joystick/*.c) \
	$(wildcard $(SDL_SRC_DIR)/joystick/wii/*.c) \
	$(wildcard $(SDL_SRC_DIR)/loadso/dummy/*.c) \
	$(wildcard $(SDL_SRC_DIR)/main/wii/*.c) \
	$(wildcard $(SDL_SRC_DIR)/stdlib/*.c) \
	$(wildcard $(SDL_SRC_DIR)/thread/*.c) \
	$(wildcard $(SDL_SRC_DIR)/thread/wii/*.c) \
	$(wildcard $(SDL_SRC_DIR)/timer/*.c) \
	$(wildcard $(SDL_SRC_DIR)/timer/wii/*.c) \
	$(wildcard $(SDL_SRC_DIR)/video/*.c) \
	$(wildcard $(SDL_SRC_DIR)/video/dummy/*.c) \
	$(wildcard $(SDL_SRC_DIR)/video/wii/*.c)

# Library object files.
SDL_OBJS	:= $(subst $(SDL_SRC_DIR),$(SDL_OBJ_DIR),$(SDL_SRCS:.c=.o))

# Test source files.
# It can be useful to switch this variable around to select individual tests which are problematic.
TEST_SRCS	:= $(wildcard $(TEST_SRC_DIR)/*.c)
#TEST_SRCS	:= $(TEST_SRC_DIR)/testsprite.c

# Test object files.
TEST_OBJS	:= $(subst $(TEST_SRC_DIR)/,$(TEST_OBJ_DIR)/,$(TEST_SRCS:.c=.o))

# Test .DOL files.
TEST_DOLS	:= $(subst $(TEST_OBJ_DIR),$(BIN_DIR),$(TEST_OBJS:.o=.dol))

# What's a full build?
all: $(LIB_DIR)/libSDL.a install

# How to delete the intermediate files.
clean:
	@echo Cleaning $(TEST_DOLS) $(LIB_DIR)/libSDL.a $(SDL_OBJS) $(TEST_OBJS)
	@rm -f $(TEST_DOLS) $(LIB_DIR)/libSDL.a $(SDL_OBJS) $(TEST_OBJS)

# How to install to SD card ready for running.
install: $(TEST_DOLS)
	@-mkdir -p dols
	cp $(TEST_DOLS) dols

# How to build a library.
$(LIB_DIR)/libSDL.a: $(SDL_OBJS)
	@echo Archiving $@
	@-mkdir -p $(dir $@)
	powerpc-gekko-ar crs $@ $(SDL_OBJS)
	@echo ----

# How to build a DOL.
$(BIN_DIR)/%.dol: $(TEST_OBJ_DIR)/%.elf
	@echo Creating DOL $@
	@-mkdir -p $(dir $@)
	#powerpc-gekko-objcopy -O binary $< $@
	elf2dol $< $@
	@echo ----

# Compilation flags.
#COMMON_FLAGS	:= -g -O2 -mrvl -Wall -mcpu=750 -meabi -mhard-float $(MACHDEP) 
COMMON_FLAGS	:= -g -O2 -mrvl -Wall -mcpu=750 -meabi -mhard-float $(MACHDEP) 
INCLUDES		:= -Iinclude -I$(DEVKITPRO)/libogc/include
#DEFINES			:= -DGEKKO -DDEBUG_ERROR -DDEBUG_TIMERS -DDEBUG_THREADS -DDEBUG_BUILD -DDEBUG_CONVERT -DSDL_AUDIO_DRIVER_WII
DEFINES			:= -DGEKKO -DSDL_AUDIO_DRIVER_WII
CFLAGS			:= $(COMMON_FLAGS) $(INCLUDES) $(DEFINES)

# Test link flags.
LDFLAGS			:= $(COMMON_FLAGS) -L$(LIB_DIR) -L$(DEVKITPRO)/libogc/lib/wii -lSDL -lfat -lwiiuse -lbte -logc -lm 
#LDFLAGS			:= $(COMMON_FLAGS) -L$(LIB_DIR) -L$(DEVKITPRO)/libogc/lib/wii -lSDL -lfat -lwiiuse -lbte -logc -lm 
# -lsdcard

# How to link an ELF.
$(TEST_OBJ_DIR)/%.elf: $(TEST_OBJ_DIR)/%.o $(LIB_DIR)/libSDL.a
	@echo Linking $@
	@-mkdir -p $(dir $@)
	powerpc-gekko-g++ -o $@ $< $(LDFLAGS)
	#keep elf for debugging
	#cp $@ /tmp/elf 

# How to compile C file (Tests).
$(TEST_OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.c
	@echo Compiling $<
	@-mkdir -p $(dir $@)
	powerpc-gekko-gcc $(CFLAGS) -c $< -o $@ $(PIPE_TO_SsED)

# How to compile C file (SDL library).
$(SDL_OBJ_DIR)/%.o: $(SDL_SRC_DIR)/%.c
	@echo Compiling $<
	@-mkdir -p $(dir $@)
	powerpc-gekko-gcc $(CFLAGS) -c $< -o $@ $(PIPE_TO_SED)
