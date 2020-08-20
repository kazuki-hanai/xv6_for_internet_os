PROGNAME = 9psv
P9_DIR = $U/$(PROGNAME)
P9_SRCS = \
	$(P9_DIR)/server.c \
	$(P9_DIR)/util.c \
	$(P9_DIR)/fid.c \
	$(P9_DIR)/file.c \
	$(P9_DIR)/qid.c \
	$(P9_DIR)/req.c \
	$(P9_DIR)/version.c \
	$(P9_DIR)/error.c \
	$(P9_DIR)/attach.c \
	$(P9_DIR)/walk.c \
	$(P9_DIR)/open.c \
	$(P9_DIR)/read.c \
	$(P9_DIR)/write.c \
	$(P9_DIR)/clunk.c \
	$(P9_DIR)/remove.c \
	$(P9_DIR)/stat.c \

P9_OBJS = $(addprefix $(BUILD_DIR)/, $(P9_SRCS:.c=.o))
P9_DEPS = $(P9_OBJS:.o=.d)

-include $(P9_DEPS)

_$(PROGNAME): $(P9_OBJS) $(ULIBOBJS)
	@mkdir -p $(BUILD_DIR)/$U/$(PROGNAME)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $(BUILD_DIR)/$U/$@ $^
