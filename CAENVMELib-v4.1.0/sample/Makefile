TARGET = CAENVMEDemo
CFLAGS = -O2 -g
LDFLAGS = -lm -lCAENVME -lncurses

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = CAENVMEDemoMain.o CAENVMEDemoVme.o console.o CaenVmeVSL.o

%.o: %.c
	$(CC) $(CFLAGS) -I../include -L. -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -I../include -L. -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
