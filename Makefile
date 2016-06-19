all:
	gcc mysql_pack_shall.c -O2 -o mysql_pack_shall
install:
	sudo mv mysql_pack_shall /bin/
clean:
	rm -rf mysql_pack_shall
