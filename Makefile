TARGET = sniffer

SRCS = common.c mysql_parse.c sniffer.c hashtable.c

OBJS = $(SRCS:.c=.o)

INSTALLDIR = /bin/

CC = cc
CFLAGS = -O2

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

install:
	mv $(TARGET) $(INSTALLDIR)

clean:
	rm -rf $(TARGET) $(OBJS)

depclean:
	sudo rm -rf $(TARGET) $(OBJS) $(INSTALLDIR)$(TARGET)


%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $<
