# Makefile feature isolation testing

CXX = g++
CXXFLAGS = -std=c++20 -Wall -pedantic -O3
INCDIR = ../include
SRCDIR = ../src
OBJDIR = ../obj

it: $(OBJDIR)/feature_analysis.o $(OBJDIR)/isolation.o
	$(CXX) $(CXXFLAGS) $(OBJDIR)/feature_analysis.o $(OBJDIR)/isolation.o -lz3 -o $(OBJDIR)/feature_isolation

$(OBJDIR)/feature_analysis.o: $(SRCDIR)/feature_analysis.cpp $(INCDIR)/feature_analysis.hpp
	$(CXX) $(CXXFLAGS) -c $(SRCDIR)/feature_analysis.cpp -o $(OBJDIR)/feature_analysis.o

$(OBJDIR)/isolation.o: $(SRCDIR)/feature_isolation.cpp $(INCDIR)/isolation.hpp $(INCDIR)/spl.hpp
	$(CXX) $(CXXFLAGS) -c $(SRCDIR)/feature_isolation.cpp -o $(OBJDIR)/isolation.o

clean:
	rm -f $(OBJDIR)/*
