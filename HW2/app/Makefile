SOURCES = $(shell find . -type f -iname '*.c')
TARGET = HW2_20141542.out
all: $(TARGET)

$(TARGET): $(SOURCES)
	arm-none-linux-gnueabi-gcc -static $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)
