LIBUSB_INCLUDE_PATH=/usr/include/libusb-1.0
#GCC_LIB_OPTIONS=-fpic
##GCC_OPTIONS=-g -O2 -Wall
GCC_OPTIONS=-g3 -Wall
all:	libfcd.a fcd testfcd semtool

# hidraw version:

libfcd.a:	libfcd.h libfcd.c hidapi.h hid.c Makefile
	gcc $(GCC_OPTIONS) -c  -I$(LIBUSB_INCLUDE_PATH) -o libfcd.o libfcd.c
	gcc $(GCC_OPTIONS) -c  -I$(LIBUSB_INCLUDE_PATH) -o hid.o hid.c
	ar rcs libfcd.a libfcd.o hid.o

fcd: libfcd.a fcd.c Makefile
	gcc $(GCC_OPTIONS) -o fcd fcd.c  -L. -lfcd -ludev

#
# libusb version:

# libfcd.a:	libfcd.h libfcd.c hidapi.h hid-libusb.c Makefile
# 	gcc $(GCC_OPTIONS) -c  -I$(LIBUSB_INCLUDE_PATH) -o libfcd.o libfcd.c  -lusb-1.0
# 	gcc $(GCC_OPTIONS) -c  -I$(LIBUSB_INCLUDE_PATH) -o hid-libusb.o hid-libusb.c -lusb-1.0
# 	ar rcs libfcd.a libfcd.o hid-libusb.o

# fcd: libfcd.a fcd.c Makefile
# 	gcc $(GCC_OPTIONS) -o fcd fcd.c -I$(LIBUSB_INCLUDE_PATH) -I. -L. -lfcd -lusb-1.0 -lpthread -ludev

testfcd: libfcd.a testfcd.c Makefile
	gcc $(GCC_OPTIONS) -o testfcd testfcd.c -I$(LIBUSB_INCLUDE_PATH) -I. -L. -lfcd -ludev

semtool: semtool.c Makefile
	gcc $(GCC_OPTIONS) -o semtool semtool.c  -I. -L. -lpthread

install: fcd semtool
	su -c 'cp fcd /usr/bin'
	su -c 'cp semtool /usr/bin;\
	rm -f /usr/bin/semcreate /usr/bin/semdelete /usr/bin/sempost /usr/bin/semwait;\
	ln -s /usr/bin/semtool /usr/bin/semcreate;\
	ln -s /usr/bin/semtool /usr/bin/semdelete;\
	ln -s /usr/bin/semtool /usr/bin/sempost;\
	ln -s /usr/bin/semtool /usr/bin/semwait;'
