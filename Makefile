include ./flags.mk

SRC=$(wildcard src/*.c) $(wildcard src/**/*.c)
SRC_MK=$(SRC:.c=.d)
OBJ=$(SRC:.c=.o)
OUT=compiler

BUILDFILES=$(OBJ) $(SRC_MK) $(OUT)

all: $(OUT)

clean:
	@echo "Cleaning buildfiles"
	@rm -f $(BUILDFILES)

%.o: %.c
	@echo "CC	$(shell basename $@)"
	@$(CC) -o $@ -c $< $(CPPFLAGS) $(CFLAGS)

$(OUT): $(OBJ)
	@echo "LD	$(shell basename $@)"
	@$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

.PHONY: all clean
