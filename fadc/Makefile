# Makefile for the fadc object

INCDIR := -I$(JLAB_SOFTWARE)/banks/src -I$(EVIO)/src/libsrc -I$(EVIO)/src/libsrc++ -I./include
ROOTINC := $(shell root-config --cflags)
ROOTLIB := $(shell root-config --glibs)
LIBDIR := -L$(EVIO)/lib  -L$(BANKS)/lib -L./lib
LIBS := -lbanks -levioxx -levio -lfadc_analizer -lQtSql -lQtXml 
CXX := g++

LINKER_FLAG := -Wl,-rpath,$(JLAB_SOFTWARE)/fadc/lib


lib:
	$(CXX) $(INCDIR) -fPIC -O3 -c src/EnergyFunctions.cc -o lib/EnergyFunctions.o 
	$(CXX) $(INCDIR) -fPIC -O3 -c src/TimeFunctions.cc -o lib/TimeFunctions.o 	
	$(CXX) $(INCDIR) -fPIC -O3 -c src/fadc.cc -o lib/fadc.o
	$(CXX) -shared -o lib/libfadc_analizer.so lib/EnergyFunctions.o lib/TimeFunctions.o lib/fadc.o
install_lib:
#	install lib/libfadc_analizer.so /usr/lib
#	/sbin/ldconfig
#	rm lib/*.so
#	rm lib/*.o
#	ln -s /usr/lib/libfadc_analizer.so lib/libfadc_analizer.so

	/sbin/ldconfig
	rm lib/*.o



exe:	lib	
	rootcint -f src/mydict.cxx -c include/main.h include/MyLinkDef.h
	$(CXX) $(INCDIR) -fPIC -O3 -c src/mydict.cxx $(ROOTINC) -o lib/mydict.o
	$(CXX) $(LINKER_FLAG) -o bin/ana_fadc src/main.cc $(ROOTINC) $(QTFLAGS) $(ROOTLIB) $(INCDIR) $(LIBDIR) $(LIBS) $(QTLIBS) lib/mydict.o


clean:
	rm lib/*
	rm bin/ana_fadc




#	g++ -c EnergyFunctions.cc
#	g++ -c fadc.cc
#	g++ -o main main.cc fadc.o EnergyFunctions.o
