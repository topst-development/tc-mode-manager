AUTOMAKE_OPTIONS = foreign subdir-objects
CC = @CC@
CPP = @CPP@
AM_CFLAGS = $(MODEPKG_CFLAGS) -I$(top_srcdir)/include
AM_CPPFLAGS = $(AM_CFLAGS)
LIBS = @LIBS@ $(MODEPKG_LIBS)
DEFS = @DEFS@
bin_PROGRAMS = TCModeManager
TCModeManager_SOURCES = src/DBusMsgDefNames.c \
						src/ModeDBusManager.c \
						src/ModeManager.cpp \
						src/ModeXMLParser.c \
						src/main.c

configdir = $(datadir)/mode
config_DATA = defaultmode.xml

