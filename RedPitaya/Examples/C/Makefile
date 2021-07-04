MODEL ?= Z10

CFLAGS  = -g -std=gnu11 -Wall
CFLAGS += -I/opt/redpitaya/include
CFLAGS += -O3
LDFLAGS = -L/opt/redpitaya/lib
LDLIBS =  -lpthread -lrp -lm

ifeq ($(MODEL),Z20_250_12)
INCLUDE += -I/opt/redpitaya/include/api250-12
LDLIBS += -lrp-gpio -lrp-i2c
endif

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=)

all: $(OBJS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	$(RM) *.o
	$(RM) $(OBJS)
