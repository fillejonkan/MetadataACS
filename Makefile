PROG	= MetadataACS
SRCS	= main.c debug.c metadata_pair.c camera/camera.c overlay.c acs.c
OBJS    = $(SRCS:.c=.o)



PROGS	= $(PROG)

PKGS = gio-2.0 glib-2.0 cairo axparameter axevent
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS)) -DGETTEXT_PACKAGE=\"libexif-12\" -DLOCALEDIR=\"\"
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))
LDFLAGS  += -s -laxoverlay -laxevent -laxparameter -laxhttp -lvdo -pthread

all: $(PROG) $(OBJS)

$(PROG): $(OBJS)
	$(CC) $^ $(CFLAGS) $(LIBS) $(LDFLAGS) -lm $(LDLIBS) -o $@
	$(STRIP) $@

clean:
	rm -f $(PROG) $(OBJS)
