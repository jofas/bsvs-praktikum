build: server client

server: main.o server.o buffer.o sensors.o grovepi.o
	gcc -o bin/server main.o server.o buffer.o sensors.o grovepi.o -pthread

main.o: src/server/main.c
	gcc -c src/server/main.c

server.o: src/server/server.c
	gcc -c src/server/server.c

buffer.o: src/server/buffer.c
	gcc -c src/server/buffer.c

sensors.o: src/sensors.c
	gcc -c src/sensors.c

grovepi.o: src/grovepi.c
	gcc -c src/grovepi.c

client: src/client/main.c
	gcc -o bin/client src/client/main.c
