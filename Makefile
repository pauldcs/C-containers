include libcont.mk
include specs.mk

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

specs: g
	$(CC) \
		unit-tests/main.c \
		unit-tests/srcs/unit_tests.c \
		-I unit-tests/srcs \
		$(CFLAGS) \
		$(CFLAGS_DBG) \
		$(TEST_MAIN) \
		$(SPECS_SRCS) \
		-I $(INCS_DIR) \
		-o tester \
		-L. \
		$(NAME) \


g: CFLAGS += $(CFLAGS_DBG)
g: all

clean:
	rm -rf *.dSYM
	rm -rf $(OBJS_DIR)

fclean: clean
	rm -rf $(NAME)
	rm -rf prog
	rm -rf tester

re: fclean all

.PHONY	: all clean g specs fclean re 