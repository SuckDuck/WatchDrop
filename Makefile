PREFIX = /usr/local

all: main.c
	gcc -o watchdrop \
	-I ./nativefiledialog-extended/src/include \
	-L ./nativefiledialog-extended/build/src \
	main.c \
	-lnfd \
	-lglib-2.0 \
	-lgtk-3 \
	-lgdk-3 \
	-lgobject-2.0

install: all
	mkdir -p ${PREFIX}/bin
	cp -f watchdrop ${PREFIX}/bin
	chmod u=rwx,g=rwx,o=rx ${PREFIX}/bin/watchdrop

uninstall:
	rm -f ${PREFIX}/bin/watchdrop