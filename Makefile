# -- M A K E F I L E -----------------------------------------------------------
#  Author:  diroyer

# -- S E T T I N G S ----------------------------------------------------------

# set default target
.DEFAULT_GOAL := all

# use one shell for all commands
.ONESHELL:

# set shell program
override SHELL := $(shell which bash)

# set shell flags
# - c : run commands from command line
#  -e : exit immediately if a command exits with a non-zero status
#  -o : pipefail : return the exit status of the last command in the pipeline that failed

.SHELLFLAGS := -c -e -o pipefail -u

# set make flags
override MAKEFLAGS += --warn-undefined-variables --no-builtin-rules

# -- D I R E C T O R I E S ----------------------------------------------------

# source directory
override src_dir := $(CURDIR)/src

# include directory
override inc_dir := $(CURDIR)/inc


# -- T A R G E T S ------------------------------------------------------------

# project name
override project := ft_strace

# main executable
override name := $(project)

# compile command database
override cmddb := compile_commands.json

# -- S O U R C E S ------------------------------------------------------------

# get all source files
override srcs := $(shell find $(src_dir) -type f -name "*.c")

# object files
override objs := $(srcs:%.c=%.o)

# dependency files
override deps := $(objs:%.o=%.d)



# -- C O M P I L E R  S E T T I N G S -----------------------------------------

# compiler
override cc := $(shell which clang)

# compiler standard
override std := -std=gnu99 -m64

# compiler optimization
override opt := -O3

#debug
override dbg := 

def ?= VERBOSE DEBUG

override defines := $(addprefix -D, $(def))

# compiler flags
override cflags := $(std) $(opt) $(dbg) $(defines) -I$(inc_dir) \
					-Wall -Wextra -Werror -Wpedantic \
					-Wno-unused -Wno-unused-variable -Wno-unused-parameter

override ldflags :=

# dependency flags
override depflags = -MT $@ -MMD -MP -MF $*.d


# -- M A I N  T A R G E T S ---------------------------------------------------

all: $(name)

$(name): $(objs)
	$(cc) $^ -o $@ $(ldflags)

-include $(deps)
%.o : %.c Makefile
	$(cc) $(cflags) $(depflags) $(defines) -c $< -o $@

clean:
	@rm -rvf $(objs) $(deps) '.cache'

fclean: clean
	@rm -vf $(name)

re: fclean all


# -- P H O N Y  T A R G E T S -------------------------------------------------

.PHONY: all clean fclean re
