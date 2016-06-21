CC = gcc

all: mysql_pack_shall
	@echo "All"

mysql_pack_shall: mysql_pack_shall.o
	@echo $@
	$(CC) -o $@ $^

mysql_pack_shall.o: mysql_pack_shall.c

install:
	@sudo mv mysql_pack_shall /bin/
clean:
	@rm -rf mysql_pack_shall *.o
