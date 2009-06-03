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
INSTALL_HEADER_DIR	?= $(DEVKITPRO)/libogc/include
INSTALL_LIB_DIR		?= $(DEVKITPRO)/libogc/lib/wii

# Tools.
PIPE_TO_SED := 2>&1 | sed "s/:\([0-9]*\):/\(\1\) :/"

# Library source files.
SRCS	:= $(filter-out $(SRC_DIR)/glfont.c $(SRC_DIR)/showfont.c, $(wildcard $(SRC_DIR)/*.c))

# Library object files.
OBJS	:= $(subst $(SRC_DIR),$(OBJ_DIR),$(SRCS:.c=.o))

# Test source files.
# It can be useful to switch this variable around to select individual tests which are problematic.
TEST_SRCS	:= $(TEST_SRC_DIR)/showfont.c

# Test object files.
TEST_OBJS	:= $(subst $(TEST_SRC_DIR)/,$(TEST_OBJ_DIR)/,$(TEST_SRCS:.c=.o))

# Test .DOL files.
TEST_DOLS	:= $(subst $(TEST_OBJ_DIR),$(BIN_DIR),$(TEST_OBJS:.o=.dol))

# What's a full build?
all: $(LIB_DIR)/libSDL_ttf.a $(INCLUDE_DIR)/SDL_ttf.h

# How to delete the intermediate files.
clean:
	@echo Cleaning $(LIB_DIR)/libSDL_ttf.a $(OBJ_DIR)
	@rm -f $(LIB_DIR)/libSDL_ttf.a $(OBJS) $(TEST_OBJS)

# How to install to SD card ready for running.
test-install: $(TEST_DOLS)
	@-mkdir -p dols
	cp $(TEST_DOLS) dols
	
install:
	@mkdir -p $(INSTALL_HEADER_DIR) $(INSTALL_LIB_DIR)
	@mkdir -p $(INSTALL_HEADER_DIR)/SDL
	@cp -frv $(LIB_DIR)/*.* $(INSTALL_LIB_DIR)
	@cp -frv include/*.* $(INSTALL_HEADER_DIR)/SDL

# How to build a library.
$(LIB_DIR)/libSDL_ttf.a: $(OBJS)
	@echo Archiving $@
	@-mkdir -p $(dir $@)
	@powerpc-gekko-ar crs $@ $(OBJS)
	@echo ----

# How to build a DOL.
$(BIN_DIR)/%.dol: $(TEST_OBJ_DIR)/%.elf
	@echo Creating DOL $@
	@-mkdir -p $(dir $@)
	#powerpc-gekko-objcopy -O binary $< $@
	elf2dol $< $@
	@echo ----

# How to copy a header file.
$(INCLUDE_DIR)/SDL_ttf.h: $(SRC_DIR)/SDL_ttf.h
	@echo Copy $(SRC_DIR)/SDL_ttf.h to $(INCLUDE_DIR)
	@-mkdir -p $(dir $@)
	@cp $(SRC_DIR)/SDL_ttf.h $(INCLUDE_DIR)

# Compilation flags.
COMMON_FLAGS	:= -g -O3 -mrvl -Wall $(MACHDEP)
INCLUDES		:= -Iinclude -I$(DEVKITPRO)/libogc/include -I$(DEVKITPRO)/libogc/include/SDL -I$(DEVKITPRO)/libogc/include/freetype
DEFINES			:= -DGEKKO -DDEBUG_ERROR -DDEBUG_TIMERS -DDEBUG_THREADS -DDEBUG_BUILD -DDEBUG_CONVERT 
CFLAGS			:= $(COMMON_FLAGS) $(INCLUDES) $(DEFINES)

# Test link flags.
LDFLAGS			:= $(COMMON_FLAGS) -L$(LIB_DIR) -L$(DEVKITPRO)/libogc/lib/wii -lSDL_ttf -lSDL -lfreetype -lfat -lwiiuse -lbte -logc -lm

# How to link an ELF.
$(TEST_OBJ_DIR)/%.elf: $(TEST_OBJ_DIR)/%.o $(LIB_DIR)/libSDL_ttf.a $(DEVKITPRO)/libSDL/lib/libSDL.a
	@echo Linking $@
	@-mkdir -p $(dir $@)
	powerpc-gekko-g++ -o $@ $< $(LDFLAGS)
	#keep elf for debugging
	#cp $@ /tmp/elf

# How to compile C file (Tests).
$(TEST_OBJ_DIR)/%.o: $(TEST_SRC_DIR)/showfont.c
	@echo Compiling $<
	@-mkdir -p $(dir $@)
	powerpc-gekko-gcc $(CFLAGS) -c $< -o $@ $(PIPE_TO_SED)

# How to compile C file (SDL library).
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo Compiling $<
	@-mkdir -p $(dir $@)
	@powerpc-gekko-gcc $(CFLAGS) -c $< -o $@ $(PIPE_TO_SED)
