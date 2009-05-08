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
SRCS	:= $(wildcard $(SRC_DIR)/*.c)

# Library object files.
OBJS	:= $(subst $(SRC_DIR),$(OBJ_DIR),$(SRCS:.c=.o))

# What's a full build?
all: $(LIB_DIR)/libSDL_gfx.a headers

# How to delete the intermediate files.
clean:
	@echo Cleaning $(LIB_DIR)/libSDL_gfx.a $(OBJ_DIR)
	@rm -f $(LIB_DIR)/libSDL_gfx.a $(OBJS) $(TEST_OBJS)

# How to build a library.
$(LIB_DIR)/libSDL_gfx.a: $(OBJS)
	@echo Archiving $@
	@-mkdir -p $(dir $@)
	@powerpc-gekko-ar crs $@ $(OBJS)
	@echo ----
	
install:
	@mkdir -p $(INSTALL_HEADER_DIR) $(INSTALL_LIB_DIR)
	@mkdir -p $(INSTALL_HEADER_DIR)/SDL
	@cp -frv $(LIB_DIR)/*.* $(INSTALL_LIB_DIR)
	@cp -frv include/*.* $(INSTALL_HEADER_DIR)/SDL

# How to copy the header file
headers: 
	@echo Copy headers to $(INCLUDE_DIR)
	@-mkdir -p $(INCLUDE_DIR)
	@cp $(wildcard $(SRC_DIR)/*.h) $(INCLUDE_DIR)

# Compilation flags.
COMMON_FLAGS	:= -g -O2 -mrvl -Wall $(MACHDEP)
INCLUDES		:= -Iinclude -I$(DEVKITPRO)/libogc/include -I$(DEVKITPRO)/libogc/include/SDL
DEFINES			:= -DGEKKO -DDEBUG_ERROR -DDEBUG_TIMERS -DDEBUG_THREADS -DDEBUG_BUILD -DDEBUG_CONVERT
CFLAGS			:= $(COMMON_FLAGS) $(INCLUDES) $(DEFINES)

# How to compile C file (SDL library).
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo Compiling $<
	@-mkdir -p $(dir $@)
	@powerpc-gekko-gcc $(CFLAGS) -c $< -o $@ $(PIPE_TO_SED)
