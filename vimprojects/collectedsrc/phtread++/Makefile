#
# Copyright (c) 2004-2008 Darron M Broad
# All rights reserved.
#
# Licensed under the terms of the BSD license, see file LICENSE
# for details.
#
# $Id: Makefile,v 1.4 2004/06/09 02:19:52 darron Exp $
# 

CXX= g++
CXXFLAGS= -O2 -Wall -g -fexceptions

#-fno-rtti

CXXINC=
CXXLIB= -pthread

OBJECT= testapp.o testthread.o
HEADER= testapp.h thread.h testthread.h
TARGET= testapp

BINDIR= /usr/local/bin

BSDINSTALL= /usr/bin/install

all: $(TARGET)

$(TARGET):$(OBJECT)
	$(CXX) $(CXXLIB) $(OBJECT) -o $(TARGET)

$(OBJECT):$(HEADER)

install: all
	$(BSDINSTALL) -d -g0 -m0755 -o0 $(BINDIR)
	$(BSDINSTALL) -g0 -m0755 -o0 $(TARGET) $(BINDIR)

clean:
	rm -f *.o *~ .*~ $(TARGET) $(TARGET).core core

.c.o:
	$(CXX) $(CXXINC) $(CXXFLAGS) -o $@ -c $<

%.o: %.cpp
	$(CXX) $(CXXINC) $(CXXFLAGS) -o $@ -c $<
