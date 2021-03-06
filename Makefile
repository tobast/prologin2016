# -*- Makefile -*-

lib_TARGETS = champion

# Tu peux rajouter des fichiers sources, headers, ou changer
# des flags de compilation.
champion-srcs = prologin.cc \
	naive.cpp \
	greedy.cpp \
	overlay.cpp \
	stats.cpp
champion-dists = naive.h \
	greedy.cpp \
	overlay.h \
	greedy.h \
	stats.h \
	constants.h \
	README
champion-cxxflags = -ggdb3 -Wall -Wextra -std=c++11

# Evite de toucher a ce qui suit
champion-dists += prologin.hh
STECHEC_LANG=cxx
include ../includes/rules.mk
