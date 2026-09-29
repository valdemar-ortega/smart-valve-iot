# Host unit tests for the hardware-independent firmware code.
#   make -C firmware/test               build and run
#   make -C firmware/test SANITIZE=1    same, with ASan + UBSan
#   make -C firmware/test clean

CC      ?= gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Werror -O0 -g -I../main
ifeq ($(SANITIZE),1)
CFLAGS  += -fsanitize=address,undefined -fno-sanitize-recover=all
endif
SRCS    := test_pressure_math.c ../main/pressure_math.c
BIN     := test_pressure_math

.PHONY: all clean
all: $(BIN)
	./$(BIN)

$(BIN): $(SRCS) ../main/pressure_math.h
	$(CC) $(CFLAGS) $(SRCS) -o $@

clean:
	rm -f $(BIN)
