# Visual Stupid Makefile 1628872598@qq.com
# Layout: [include/]  src/
# All .c and Makefile in src, .h can be in include. No other sub-directories.
# Project related: CPPFLAGS
# .cpp, CXX, CXXFLAGS for C++

OUT      = main.out
SRCS     = $(wildcard *.c)
OBJS     = $(patsubst %.c,%.o,$(SRCS))
CC       = gcc
CXX      = g++
CFLAGS   = -std=c99 -Wall -W
CXXFLAGS = -std=c++0x -Wall -W
CPPFLAGS = -I ../include
LDFLAGS  =

CFLAGS   += -g
#CFLAGS  += -DNDEBUG
CFLAGS   += -D_POSIX_SOURCE -D_BSD_SOURCE
LDFLAGS  += -lpthread

$(OUT): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

include $(SRCS:.c=.d)

%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

.PHONY  : clean
clean:
	rm *.d *.d.* $(OBJS) $(OUT)

