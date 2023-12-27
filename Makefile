include libcont.mk

SRCS_OBJS := $(patsubst %.c,$(OBJS_DIR)/%.o,$(SRCS))

$(OBJS_DIR)/%.o:$(SRCS_DIR)/%.c
	@mkdir -vp $(dir $@)
	$(CC) \
		$(CFLAGS) \
		-MMD \
		-MP \
		-o $@ \
		-c $< \
		-I $(INCS_DIR)

all: $(NAME)

-include  $(SRCS_OBJS:.o=.d)

$(NAME): $(SRCS_OBJS)
	$(CC) \
		$^ \
		$(CFLAGS) \
		-I $(INCS_DIR) \
		-dynamiclib \
		-o $(NAME) 

specs: all
	$(CC) \
		$(CFLAGS) \
		$(CFLAGS_DBG) \
		unit-tests/tests/array_basic.c \
		unit-tests/tests/array_usage.c \
		unit-tests/tests/dynstr_basic.c \
		unit-tests/srcs/unit_tests.c \
		unit-tests/srcs/main.c \
		-I srcs \
		-I unit-tests/srcs \
		-o prog \
		-L. \
		libcont.dylib \


g: CFLAGS += $(CFLAGS_DBG)
g: all

clean:
	rm -rf *.dSYM
	rm -rf $(OBJS_DIR)

fclean: clean
	rm -rf $(NAME)
	rm -rf prog

re: fclean all

.PHONY	: all clean g spec fclean re 