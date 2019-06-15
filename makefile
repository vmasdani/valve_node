CFLAGS=-Wall
CC=gcc
IDIR=-I./include
LIBS=-lwiringPi -lpthread

all: mqtt.o mqtt_pal.o cJSON.o main.c getip.o soft_i2c.o soft_lcd.o
	$(CC) $(OBJ) -o main main.c getip.o cJSON.o mqtt.o mqtt_pal.o soft_lcd.o soft_i2c.o $(IDIR) $(CFLAGS) $(LIBS)
	
mqtt.o: mqtt.c
	$(CC) -c mqtt.c $(IDIR) $(CFLAGS)

mqtt_pal.o: mqtt_pal.c
	$(CC) -c mqtt_pal.c $(IDIR) $(CFLAGS)

cJSON.o: cJSON.c
	$(CC) -c cJSON.c $(IDIR) $(CFLAGS)

getip.o: getip.c
	$(CC) -c getip.c $(IDIR) $(CFLAGS)

soft_i2c.o: soft_i2c.c
	$(CC) -c soft_i2c.c $(IDIR) $(CFLAGS)

soft_lcd.o: soft_lcd.c
	$(CC) -c soft_lcd.c $(IDIR) $(CFLAGS)

clean: 
	rm -rf *.o
