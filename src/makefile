# Makefile feature isolation development

CXX = g++
CXXFLAGS = -std=c++20 -Wall -pedantic
SRCDIR = ../src
INCDIR = ../include
OBJDIR = ../obj

# Further targets to be added in the line below.
all: isolation_test

test_isolation: $(OBJDIR)/it

$(OBJDIR)/it: $(OBJDIR)/feature_analysis.o $(OBJDIR)/isolation.o
	$(CXX) $(CXXFLAGS) -o $(OBJDIR)/it $(OBJDIR)/feature_analysis.o $(OBJDIR)/isolation.o -lminisat

$(OBJDIR)/feature_analysis.o: $(SRCDIR)/feature_analysis.cpp $(INCDIR)/feature_analysis.hpp
	$(CXX) $(CXXFLAGS) -o $(OBJDIR)/feature_analysis.o $(SRCDIR)/feature_analysis.cpp

$(OBJDIR)/isolation.o: $(SRCDIR)/isolation_test.cpp $(INCDIR)/isolation.hpp

clean:
	rm -f $(OBJDIR)/*



