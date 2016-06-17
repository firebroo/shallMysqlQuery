all:
	gcc mysql_pack_shall.c -O2 -o mysql_pack_shall
clean:
	rm -rf mysql_pack_shall
