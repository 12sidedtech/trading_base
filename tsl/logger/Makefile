OBJ = logger.o

TSL_PATH = ~/src/tsl
DEFINES=-D_GNU_SOURCE -D_TSL_DEBUG -DSYS_CACHE_LINE_LENGTH=64
OFLAGS = -g -O0
CFLAGS = -I. -I../../ -std=c99 $(DEFINES) $(OFLAGS)
LDFLAGS = $(TSL_PATH)/libtsl.a
TARGET = liblogger.a
AR = ar
ARFLAGS = rcs 

all: $(TARGET)

.c.o:
	$(CC) $(CFLAGS) -c $<

$(TARGET): $(OBJ)
	$(AR) $(ARFLAGS) $(TARGET) $(OBJ)
	# $(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	$(RM) $(OBJ)
	$(RM) $(TARGET)

.PHONY: clean all

