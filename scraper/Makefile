
MYCXX = /usr/local/msdk-4.3.6-mips-EL-2.6.34-0.9.30.3_hwfpu/bin/mipsel-linux-g++
MYAR = ar
MYSTRIP = /usr/local/msdk-4.3.6-mips-EL-2.6.34-0.9.30.3_hwfpu/bin/mipsel-linux-strip
MYRANLIB = ranlib
TEST = jb

MYDEFS          =
rm              = /bin/rm -f
cp              = /bin/cp -f
CC              = $(MYCXX)
CXX		= $(MYCXX)
AR              = $(MYAR) cr
RANLIB          = $(MYRANLIB)
LIBNAME         = libresult.a

WARNING = -Wall

CFLAGS= $(INCLUDES) $(DEFINES) -DSYS_UNIX=1 $(MYDEFS) -g


SRCS		= result.cpp
OBJS		= result.o

TMDB		+= tmdb
TOREC		+= torec
CSFD		+= csfd

BINARIES  = $(TMDB) $(TOREC) $(CSFD)

SVNDEV = -D'SVN_REV="$(shell svnversion -n .)"'
#CFLAGS+=-I../lib_file/tinyxml/ 
CFLAGS+=-I../../libtinyxml-read-only/tinyxml -static $(SVNDEV)
#CFLAGS+=-std=c++0x
LDFLAGS=-L../../libtinyxml-read-only/lib
#LDFLAGS=-L../lib_file/lib/

.cpp.o:
	$(SHOW_COMMAND)$(CC) $(CFLAGS) $(DEBUGFLAG) $(WARNING) -c $*.cpp

all: $(BINARIES)

$(LIBNAME) : $(OBJS)
	$(SHOW_COMMAND)$(rm) $@
	$(SHOW_COMMAND)$(AR) $@ $(OBJS)
	$(SHOW_COMMAND)$(RANLIB) $@

$(TMDB): $(LIBNAME) $(TMDB).cpp
	$(SHOW_COMMAND)$(CC) $(LDFLAGS) $(CFLAGS) -o $(TMDB) tmdb.cpp -L. -lresult -ltinyxml $(FLAGS)
	$(MYSTRIP) $(TMDB)
	cp tmdb ../plugin

$(CSFD): $(LIBNAME) $(CSFD).cpp
	$(SHOW_COMMAND)$(CC) $(LDFLAGS) $(CFLAGS) -o $(CSFD) $(CSFD).cpp -L. -lresult -ltinyxml $(FLAGS)
	$(MYSTRIP) $(CSFD)
	cp $(CSFD) ../plugin

$(TOREC): $(LIBNAME) $(TOREC).cpp
	$(SHOW_COMMAND)$(CC) $(LDFLAGS) $(CFLAGS) -o $(TOREC) $(TOREC).cpp -L. -lresult -ltinyxml
	$(MYSTRIP) $(TOREC)
	cp $(TOREC) ../plugin

clean:
	$(SHOW_COMMAND)$(rm) $(OBJS) $(LIBNAME) *.o core *~ $(BINARIES) 

