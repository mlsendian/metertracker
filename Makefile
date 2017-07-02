all:
	gcc -o ledreader -lwiringPi -lhiredis ledreader.c
