EXE	=	main

MAIN = main.cc

CC	=	g++ 

COPTS	=	-fPIC -DLINUX -Wall 
#COPTS	=	-g -fPIC -DLINUX -Wall 

#DEBUG_LEVEL = -DDEBUG

DEBUG_LEVEL = -DNODEBUG

FLAGS	=	-Wall -s
#FLAGS	=	-Wall

DEPLIBS	=       -L. -l CAENVME -l ncurses -lc -lm -l CAENComm -lpthread

GTKFLAGS = `pkg-config --cflags gtk+-2.0` 

GTKLIBS =`pkg-config --libs gtk+-2.0`

PACKAGE_LIBS = -L/lib -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lm -lpangocairo-1.0 -lpango-1.0 -lcairo 
#-lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0

PACKAGE_CFLAGS = -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/gtk-2.0 -I/usr/lib/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/freetype2 -I/usr/include/libpng12  

LIBS	=	-L./fadc/lib -lfadc_analizer

INCLUDEDIR =	-I. -I./fadc/include

OBJS	=       v1720.o  interface.o support.o callback.o

INCLUDES =	CAENVMElib.h CAENVMEtypes.h CAENVMEoslib.h CAENComm.h

ROOT_LIBS = `root-config --libs`
ROOT_FLAGS = `root-config --cflags`
#########################################################################

all	:	$(EXE)

clean	:
		/bin/rm -f $(OBJS) $(EXE)

$(EXE)	:	$(OBJS) $(MAIN)
		/bin/rm -f $(EXE)
		rootcint -f mydict.cxx -c MyLinkDef.h
		$(CXX) $(INCLUDEDIR) -fPIC -O3 -c mydict.cxx $(ROOT_FLAGS) -o mydict.o
		$(CC) $(FLAGS) $(INCLUDEDIR) $(PACKAGE_LIBS) $(DEBUG_LEVEL) $(PACKAGE_CFLAGS) -o $(EXE) $(OBJS) $(DEPLIBS) $(MAIN) mydict.o $(LIBS)  $(GTKFLAGS) $(GTKLIBS) $(ROOT_FLAGS) $(ROOT_LIBS)

$(OBJS)	:	$(INCLUDES) Makefile

%.o	:	%.c
		$(CC) $(COPTS) $(PACKAGE_CFLAGS) $(GTKFLAGS) $(DEBUG_LEVEL) $(INCLUDEDIR) -c -o $@ $< $(ROOT_FLAGS)

