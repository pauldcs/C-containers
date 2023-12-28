NAME          := libcont.dylib
CC            := gcc
SRCS_DIR      := srcs
OBJS_DIR      := .objs
BUILD_DIR     := build
INCS_DIR      := srcs

CFLAGS   := \
	-Wall     \
	-Wextra   \
	-Werror   \
	-pedantic \
#	-O3

# export ASAN_OPTIONS="log_path=sanitizer.log"
# export ASAN_OPTIONS="detect_leaks=1"

CFLAGS_DBG              := \
	-g3                      \
	-O0                      \
	-fsanitize=address       \
	-fsanitize=undefined     \
	-fno-omit-frame-pointer  \
	-fstack-protector-strong \
	-fno-optimize-sibling-calls 

SRCS := \
	array.c \
	dynstr.c 
