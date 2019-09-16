
SOURCE = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SOURCE))
EXE = $(patsubst %.c,%,$(SOURCE))

all: clean $(EXE)

$(EXE): $(OBJS)
	@$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f *.o $(EXE)
