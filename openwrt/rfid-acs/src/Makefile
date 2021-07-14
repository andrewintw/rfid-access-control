
EXEC = rfid-acs
OBJS = rfid_reader.o  serial_linux.o

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(EXEC) *.o

