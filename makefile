CFLAGS=-Wall
CC=gcc
IDIR=-I./include
LIBS=-lwiringPi -lpthread

all: mqtt.o mqtt_pal.o cJSON.o main.c
	$(CC) $(OBJ) -o main main.c cJSON.o mqtt.o mqtt_pal.o $(IDIR) $(CFLAGS) $(LIBS)
	
mqtt.o: mqtt.c
	$(CC) -c mqtt.c $(IDIR) $(CFLAGS)

mqtt_pal.o: mqtt_pal.c
	$(CC) -c mqtt_pal.c $(IDIR) $(CFLAGS)

cJSON.o: cJSON.c
	$(CC) -c cJSON.c $(IDIR) $(CFLAGS)

clean: 
	rm -rf *.o
