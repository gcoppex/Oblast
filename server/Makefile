CC = g++
CFLAGS  = -Os -Wall
LD = g++
MKDIR_P = mkdir -p

SRC = ./src
BIN = ./bin
BLD = ./bld

TARGET = oblast

all: $(TARGET)

$(TARGET): $(BLD)/$(TARGET).o
	$(MKDIR_P) $(BIN)
	$(LD) $(CFLAGS) -o $(BIN)/$(TARGET) $(BLD)/$(TARGET).o

$(BLD)/$(TARGET).o: $(SRC)/$(TARGET).cpp
	$(MKDIR_P) $(BLD)
	$(CC) $(CFLAGS) -c $(SRC)/$(TARGET).cpp -o $(BLD)/$(TARGET).o

clean:
	$(RM) $(TARGET)
