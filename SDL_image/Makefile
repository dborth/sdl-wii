# Directories.
OBJ_DIR			:= obj
LIB_DIR			:= lib
BIN_DIR			:= bin
SRC_DIR			:= src
INCLUDE_DIR		:= include
TEST_OBJ_DIR		:= $(OBJ_DIR)/test
TEST_SRC_DIR		:= src
INSTALL_HEADER_DIR	?= $(DEVKITPRO)/libogc/include/
INSTALL_LIB_DIR		?= $(DEVKITPRO)/libogc/lib/wii

# Tools.
PIPE_TO_SED := 2>&1 | sed "s/:\([0-9]*\):/\(\1\) :/"

# Library source files.
SRCS	:= $(filter-out $(SRC_DIR)/IMG_ImageIO.c $(SRC_DIR)/showimage.c, $(wildcard $(SRC_DIR)/*.c))

# Library object files.
OBJS	:= $(subst $(SRC_DIR),$(OBJ_DIR),$(SRCS:.c=.o))

# Test source files.
# It can be useful to switch this variable around to select individual tests which are problematic.
TEST_SRCS	:= $(TEST_SRC_DIR)/showimage.c

# Test object files.
TEST_OBJS	:= $(subst $(TEST_SRC_DIR)/,$(TEST_OBJ_DIR)/,$(TEST_SRCS:.c=.o))

# Test .DOL files.
TEST_DOLS	:= $(subst $(TEST_OBJ_DIR),$(BIN_DIR),$(TEST_OBJS:.o=.dol))

# What's a full build?
all: $(LIB_DIR)/libSDL_image.a $(INCLUDE_DIR)/SDL_image.h install

# How to delete the intermediate files.
clean:
	@echo Cleaning $(LIB_DIR)/libSDL_image.a $(OBJ_DIR)
	@rm -f $(LIB_DIR)/libSDL_image.a $(OBJS) $(TEST_OBJS)
	
lib-install:
	@mkdir -p $(INSTALL_HEADER_DIR) $(INSTALL_LIB_DIR)
	@mkdir -p $(INSTALL_HEADER_DIR)/SDL
	@cp -frv $(LIB_DIR)/*.* $(INSTALL_LIB_DIR)
	@cp -frv include/*.* $(INSTALL_HEADER_DIR)/SDL

# How to install to SD card ready for running.
install: $(TEST_DOLS)
	@-mkdir -p dols
	cp $(TEST_DOLS) dols

# How to build a library.
$(LIB_DIR)/libSDL_image.a: $(OBJS)
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

# How to copy the header file
$(INCLUDE_DIR)/SDL_image.h: $(SRC_DIR)/SDL_image.h
	@echo Copy $(SRC_DIR)/SDL_image.h to $(INCLUDE_DIR)
	@-mkdir -p $(dir $@)
	@cp $(SRC_DIR)/SDL_image.h $(INCLUDE_DIR)

# Compilation flags.
COMMON_FLAGS	:= -g -O2 -mrvl -Wall $(MACHDEP)
INCLUDES		:= -Iinclude -I$(DEVKITPRO)/libogc/include -I$(DEVKITPRO)/libogc/include/SDL -I$(DEVKITPRO)/libogc/include/jpeg
# -I$(DEVKITPRO)/libpng/include
DEFINES			:= -DGEKKO -DDEBUG_ERROR -DDEBUG_TIMERS -DDEBUG_THREADS -DDEBUG_BUILD -DDEBUG_CONVERT -DLOAD_BMP -DLOAD_JPG -DLOAD_GIF -DLOAD_LBM -DLOAD_PCX -DLOAD_PNM -DLOAD_TGA -DLOAD_XCF -DLOAD_XPM -DLOAD_XV -DLOAD_PNG
# 
# -DLOAD_TIFF
CFLAGS			:= $(COMMON_FLAGS) $(INCLUDES) $(DEFINES)

# Test link flags.
LDFLAGS			:= $(COMMON_FLAGS) -L$(LIB_DIR) -L$(DEVKITPRO)/libSDL/lib -L$(DEVKITPRO)/libpng/lib -L$(DEVKITPRO)/libogc/lib/wii -L$(DEVKITPRO)/libjpeg/lib  -lSDL_image -lSDL -lpng -ljpeg -lfat -lwiiuse -lbte -logc -lm  -lz
# -ltiff

# How to link an ELF.
$(TEST_OBJ_DIR)/%.elf: $(TEST_OBJ_DIR)/%.o $(LIB_DIR)/libSDL_image.a $(DEVKITPRO)/libSDL/lib/libSDL.a 
	@echo Linking $@
	@-mkdir -p $(dir $@)
	powerpc-gekko-g++ -o $@ $< $(LDFLAGS)
	#keep elf for debugging
	#cp $@ /tmp/elf 

# How to compile C file (Tests).
$(TEST_OBJ_DIR)/%.o: $(TEST_SRC_DIR)/showimage.c
	@echo Compiling $<
	@-mkdir -p $(dir $@)
	powerpc-gekko-gcc $(CFLAGS) -c $< -o $@ $(PIPE_TO_SED)

# How to compile C file (SDL library).
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo Compiling $<
	@-mkdir -p $(dir $@)
	@powerpc-gekko-gcc $(CFLAGS) -c $< -o $@ $(PIPE_TO_SED)
