CC = i586-poky-linux-gcc
ARCH = x86
CROSS_COMPILE = i586-poky-linux-


IOT_HOME = /opt/iot-devkit/1.7.3/sysroots

PATH := $(PATH):$(IOT_HOME)/x86_64-pokysdk-linux/usr/bin/i586-poky-linux

SROOT=$(IOT_HOME)/i586-poky-linux


obj-m = test.o

all:
		i586-poky-linux-gcc -pthread -Wall -o test.o test.c --sysroot=$(SROOT)
clean:
		rm -f *.o
		#.SILENT: *.o

