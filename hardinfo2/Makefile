GTK_LIBS = -lpthread -lgthread-2.0 -lrt -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfreetype -lfontconfig -lgobject-2.0 -lglib-2.0  
GTK_CFLAGS = -pthread -I/usr/include/gtk-2.0 -I/usr/lib/x86_64-linux-gnu/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/pango-1.0 -I/usr/include/gio-unix-2.0/ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/libpng12  
SOUP_LIBS = 
SOUP_CFLAGS = 
PACKAGE = hardinfo2
ARCHOPTS = 
LIBDIR = /usr/lib64

CCFLAGS = -fPIC -pipe -Wall -g 
CFLAGS = $(GTK_CFLAGS) $(SOUP_CFLAGS) -I.
CC = gcc $(ARCHOPTS) -g
CCSLOW = gcc -O0 -g

# ----------------------------------------------------------------------------

OBJECTS = hardinfo.o util.o iconcache.o loadgraph.o  \
		stock.o callbacks.o expr.o report.o binreloc.o \
		vendor.o socket.o syncmanager.o
BENCHMARK_OBJECTS = fbench.o sha1.o blowfish.o md5.o nqueens.o fftbench.o

MODULES = computer.so devices.so benchmark.so network.so

all:	$(OBJECTS) $(MODULES)
	$(CC) $(CCFLAGS) -o hardinfo -Wl,-export-dynamic $(OBJECTS) $(GTK_LIBS) \
		$(SOUP_LIBS)

md5.o:
	$(CCSLOW) $(CCFLAGS) $(CFLAGS) -c md5.c -o $@

blowfish.o:
	$(CCSLOW) $(CCFLAGS) $(CFLAGS) -c blowfish.c -o $@

sha1.o:
	$(CCSLOW) $(CCFLAGS) $(CFLAGS) -c sha1.c -o $@

fbench.o:
	$(CCSLOW) $(CCFLAGS) $(CFLAGS) -c fbench.c -o $@

fftbench.o:
	$(CCSLOW) $(CCFLAGS) $(CFLAGS) -c fftbench.c -o $@

nqueens.o:
	$(CCSLOW) $(CCFLAGS) $(CFLAGS) -c nqueens.c -o $@

benchmark.so:	benchmark.c
	make $(BENCHMARK_OBJECTS)
	$(CCSLOW) $(CCFLAGS) $(CFLAGS) -o $@ -shared $< $(BENCHMARK_OBJECTS) \
		$(GTK_FLAGS) $(GTK_LIBS)
	ln -sf ../$@ modules

%.so:	%.c
	$(CC) $(CCFLAGS) $(CFLAGS) -o $@ -shared $< $(GTK_FLAGS) $(GTK_LIBS)
	ln -sf ../$@ modules
	
clean:
	rm -rf .xvpics pixmaps/.xvpics *.o *.so hardinfo modules/*.so report
	find . -name \*~ -exec rm -v {} \;
	find . -name x86 -type l -exec rm -v {} \;

dist-clean:	clean
	rm -rf Makefile debian/hardinfo/ config.h arch/this

package:	dist-clean
	@echo "Creating tar.gz..."
	cd .. && tar czf $(PACKAGE).tar.gz $(PACKAGE)/* && cd $(PACKAGE)
	@echo "Creating tar.bz2..."
	cd .. && tar cjf $(PACKAGE).tar.bz2 $(PACKAGE)/* && cd $(PACKAGE)

deb:	dist-clean
	@echo "Creating deb..."
	dpkg-buildpackage -rfakeroot -k${USER}

install:	all
	rm -rf ${DESTDIR}${LIBDIR}/hardinfo/modules ${DESTDIR}/usr/share/hardinfo/pixmaps

	mkdir -p ${DESTDIR}/usr/bin
	mkdir -p ${DESTDIR}/usr/local
	mkdir -p ${DESTDIR}/usr/share/applications
	mkdir -p ${DESTDIR}${LIBDIR}/hardinfo/modules
	mkdir -p ${DESTDIR}/usr/share/hardinfo/pixmaps

	cp hardinfo.desktop ${DESTDIR}/usr/share/applications

	cp hardinfo ${DESTDIR}/usr/bin/hardinfo

	cp -Lr modules/*.so ${DESTDIR}${LIBDIR}/hardinfo/modules

	cp -Lr pixmaps/* ${DESTDIR}/usr/share/hardinfo/pixmaps

	cp benchmark.conf ${DESTDIR}/usr/share/hardinfo
	cp benchmark.data ${DESTDIR}/usr/share/hardinfo

	chmod 755 ${DESTDIR}/usr/bin/hardinfo

installer:
	makepackage
	
