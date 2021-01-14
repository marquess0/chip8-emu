main: main.c
	gcc main.c -o main -lsdl2 -lsdl2_image -L../common/lib -I../common/include