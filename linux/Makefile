
SOURCE = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SOURCE))
EXE = rfid_reader

CFLAGS += -Wall -Wno-unused-variable -Wno-unused-but-set-variable
#CFLAGS += -DDEBUG

all: clean $(EXE)

$(EXE): $(OBJS)
	@$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f *.o $(EXE) tags
