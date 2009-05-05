# Directories.
OBJ_DIR			:= obj
LIB_DIR			:= lib
BIN_DIR			:= bin
SRC_DIR			:= src
INCLUDE_DIR		:= include
INSTALL_HEADER_DIR	?= $(DEVKITPRO)/libogc/include
INSTALL_LIB_DIR		?= $(DEVKITPRO)/libogc/lib/wii

# Tools.
PIPE_TO_SED := 2>&1 | sed "s/:\([0-9]*\):/\(\1\) :/"

# Library source files.
SRCS	:= $(wildcard $(SRC_DIR)/*.c)

# Library object files.
OBJS	:= $(subst $(SRC_DIR),$(OBJ_DIR),$(SRCS:.c=.o))

# What's a full build?
all: $(LIB_DIR)/libSDL_net.a

# How to delete the intermediate files.
clean:
	@echo Cleaning $(LIB_DIR)/libSDL_net.a $(OBJ_DIR)
	@rm -f $(LIB_DIR)/libSDL_net.a $(OBJS) $(TEST_OBJS)
	
install:
	
lib-install:
	@mkdir -p $(INSTALL_HEADER_DIR) $(INSTALL_LIB_DIR)
	@mkdir -p $(INSTALL_HEADER_DIR)/SDL
	@cp -frv $(LIB_DIR)/*.* $(INSTALL_LIB_DIR)
	@cp -frv include/*.* $(INSTALL_HEADER_DIR)/SDL

# How to build a library.
$(LIB_DIR)/libSDL_net.a: $(OBJS)
	@echo Archiving $@
	@-mkdir -p $(dir $@)
	@powerpc-gekko-ar crs $@ $(OBJS)
	@echo ----

# How to copy a header file
$(INCLUDE_DIR)/SDL_net.h: $(SRC_DIR)/SDL_net.h
	@echo Copy $(SRC_DIR)/SDL_net.h to $(INCLUDE_DIR)
	@-mkdir -p $(dir $@)
	@cp $(SRC_DIR)/SDL_net.h $(INCLUDE_DIR)

# Compilation flags.
COMMON_FLAGS	:= -g -O2 -mrvl -Wall $(MACHDEP)
INCLUDES		:= -Iinclude -I$(DEVKITPRO)/libogc/include -I$(DEVKITPRO)/libogc/include/SDL
# -include sys/wait.h 
# -include sys/syslimits.h

DEFINES			:= -DGEKKO

CFLAGS			:= $(COMMON_FLAGS) $(INCLUDES) $(DEFINES)

# Test link flags.
LDFLAGS			:= $(COMMON_FLAGS) -L$(LIB_DIR) -L$(DEVKITPRO)/libogc/lib/wii -lSDL -lfat -lwiiuse -lbte -logc -lm

# How to compile C file (SDL library).
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo Compiling $<
	@-mkdir -p $(dir $@)
	@powerpc-gekko-gcc $(CFLAGS) -c $< -o $@ $(PIPE_TO_SED)
