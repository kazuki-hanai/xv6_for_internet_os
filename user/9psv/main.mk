PROGNAME = 9psv
P9_DIR = $U/$(PROGNAME)
P9_SRCS = \
	$(P9_DIR)/server.c \
	$(P9_DIR)/utils/string.c \
	$(P9_DIR)/utils/fcall.c \
	$(P9_DIR)/utils/util.c \
	$(P9_DIR)/fid.c \
	$(P9_DIR)/file.c \
	$(P9_DIR)/qid.c \
	$(P9_DIR)/req.c \
	$(P9_DIR)/parse.c \
	$(P9_DIR)/compose.c \
	$(P9_DIR)/fcalls/version.c \
	$(P9_DIR)/fcalls/attach.c \
	$(P9_DIR)/fcalls/error.c \
	$(P9_DIR)/fcalls/flush.c \
	$(P9_DIR)/fcalls/walk.c \
	$(P9_DIR)/fcalls/open.c \
	$(P9_DIR)/fcalls/create.c \
	$(P9_DIR)/fcalls/read.c \
	$(P9_DIR)/fcalls/write.c \
	$(P9_DIR)/fcalls/clunk.c \
	$(P9_DIR)/fcalls/remove.c \
	$(P9_DIR)/fcalls/stat.c \

P9_OBJS = $(addprefix $(BUILD_DIR)/, $(P9_SRCS:.c=.o))
P9_DEPS = $(P9_OBJS:.o=.d)

-include $(P9_DEPS)

_$(PROGNAME): $(P9_OBJS) $(ULIBOBJS)
	@mkdir -p $(BUILD_DIR)/$U/$(PROGNAME)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $(BUILD_DIR)/$U/$@ $^
