CC=LD_LIBRARY_PATH=$(TOOLCHAIN_BASE)/lib $(TOOLCHAIN_BASE)/bin/arm-brcm-linux-uclibcgnueabi-g++
CFLAGS=-I$(TOOLCHAIN_BASE)/usr/include -O3 -Wall
OBJ=connection.o iptables.o logger.o main.o resolver.o server.o stream.o
LIBS=-lpthread -static-libstdc++

all: proxy

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

proxy: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f $(OBJ) proxy

deploy: proxy
	scp proxy hosts autorun.sh admin@192.168.1.1:/mnt/sda1
