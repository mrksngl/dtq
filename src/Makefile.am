
SUFFIXES = .c .h .y .l

AM_YFLAGS = -d

# binary programs
bin_PROGRAMS=dtq
# sources of that program
dtq_SOURCES=dtq.c dtq.h parser.h parser.c dtq-bison.y dtq-flex.l query.c query.h

BUILT_SOURCES = dtq-bison.h

clean-local:
	-rm -f dtq-bison.c dtq-bison.h dtq-flex.c
