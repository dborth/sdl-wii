#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	libsmpeg
BUILD		:=	obj
SOURCES		:=	src src/video src/audio
DATA		:=	data  
INCLUDES	:=	src -I$(LIBOGC_INC)/SDL
SRC_DIR			:= src
LIB_DIR			:= lib
INCLUDE_DIR		:= include
INSTALL_HEADER_DIR	?= $(LIBOGC_INC)
INSTALL_LIB_DIR		?= $(LIBOGC_LIB)

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

CFLAGS	= -g -O2 -Wall $(MACHDEP) $(INCLUDE) -DDISABLE_VIDEO_CALLBACK_THREAD -DNOCONTROLS
CXXFLAGS	=	$(CFLAGS) -DDISABLE_VIDEO_CALLBACK_THREAD -DNOCONTROLS

LDFLAGS	=	-g $(MACHDEP) -Wl,-Map,$(notdir $@).map

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:=	-lSDL -lwiiuse -lbte -lfat -logc -lm

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(LIBOGC_LIB)/SDL

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(LIB_DIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(filter-out $(dir)/glmovie.c $(dir)/glmovie-tile.c $(dir)/gtv.c $(dir)/plaympeg.c,$(wildcard $(dir)/*.c))))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))
HEADERS		:=	$(foreach dir,$(SOURCES),$(notdir $(filter-out $(dir)/glmovie.h $(dir)/gtv.h,$(wildcard $(dir)/*.h))))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) \
					$(sFILES:.s=.o) $(SFILES:.S=.o)

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES), -iquote $(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD) \
					-I$(LIBOGC_INC) -I$(LIBOGC_INC)/SDL

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
					-L$(LIBOGC_LIB)

export OUTPUT	:=	$(CURDIR)/$(LIB_DIR)/$(TARGET)

.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@-mkdir -p $(LIB_DIR)
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
	@-mkdir -p $(INCLUDE_DIR)
	@cp $(wildcard $(SRC_DIR)/*.h) $(INCLUDE_DIR)

#---------------------------------------------------------------------------------
clean:
	@rm -fr $(BUILD) $(OUTPUT).a
	
install:
	@mkdir -p $(INSTALL_HEADER_DIR) $(INSTALL_LIB_DIR)
	@mkdir -p $(INSTALL_HEADER_DIR)/mpeg
	@cp -frv $(LIB_DIR)/*.* $(INSTALL_LIB_DIR)
	@cp -frv include/*.* $(INSTALL_HEADER_DIR)/mpeg

#---------------------------------------------------------------------------------
run:
	wiiload $(TARGET).dol


#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
#$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).a: $(OFILES)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
