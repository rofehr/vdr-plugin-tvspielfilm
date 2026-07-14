#
# Makefile for the vdr-tvspielfilm plugin
#

PLUGIN = tvspielfilm

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION' tvspielfilm.c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The C++ compiler and options:

CXX      ?= g++
CXXFLAGS ?= -g -O2 -Wall -Wextra -Wno-parentheses -Wunused-parameter -Werror=overloaded-virtual -Wno-clobbered -std=c++17

### The directory environment:


LIBDIR = $(call PKGCFG,libdir)

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

### The version number of VDR's plugin API:

APIVERSION = $(shell sed -n 'y/ATV/atv/; s/^\s*#\s*define\s*APIVERSION\s*"\?\([^"]*\)"\?\s*$$/\1/p' $(VDRDIR)/config.h)

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### Includes and Defines (add further entries here):

INCLUDES += -I$(VDRDIR)/include
INCLUDES += $(shell pkg-config --cflags libcurl)

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

LIBS += $(shell pkg-config --libs libcurl)

### The object files (add further files here):

OBJS = $(PLUGIN).o rssfetcher.o menu.o setup.o

### Implicit rules:

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(CXXFLAGS) $(DEFINES) $(INCLUDES) tvspielfilm.c $(OBJS:.o=.cpp) > $@

-include $(DEPFILE)

### Targets:

all: libvdr-$(PLUGIN).so

tvspielfilm.o: tvspielfilm.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

libvdr-$(PLUGIN).so: tvspielfilm.o $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) \
	    -shared \
	    -Wl,-soname=$@.$(APIVERSION) \
	    -o $@ tvspielfilm.o $(filter-out tvspielfilm.o,$(OBJS)) \
	    $(LIBS)
		
install-lib: libvdr-$(PLUGIN).so
	install -d $(DESTDIR)$(LIBDIR)
	install -m755 libvdr-$(PLUGIN).so \
		$(DESTDIR)$(LIBDIR)/libvdr-$(PLUGIN).so.$(APIVERSION)
		
install: install-lib

dist: $(I18Npo) clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(OBJS) tvspielfilm.o $(DEPFILE) *.so *.tgz core* *~
