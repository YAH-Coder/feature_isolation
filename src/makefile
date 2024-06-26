# Makefile feature isolation testing

CXX = g++
CXXFLAGS = -std=c++20 -Wall -pedantic 
INCDIR = ../include
SRCDIR = ../src
OBJDIR = ../obj

it: $(OBJDIR)/it2

$(OBJDIR)/it2: $(OBJDIR)/feature_analysis.o $(OBJDIR)/isolation.o
	$(CXX) $(CXXFLAGS) $(OBJDIR)/feature_analysis.o $(OBJDIR)/isolation.o -lz3 -o $(OBJDIR)/it2

$(OBJDIR)/feature_analysis.o: $(SRCDIR)/feature_analysis.cpp $(INCDIR)/feature_analysis.hpp
	$(CXX) $(CXXFLAGS) -c $(SRCDIR)/feature_analysis.cpp -o $(OBJDIR)/feature_analysis.o

$(OBJDIR)/isolation.o: $(SRCDIR)/isolation_test.cpp $(INCDIR)/isolation_new.hpp $(INCDIR)/spl_test.hpp
	$(CXX) $(CXXFLAGS) -c $(SRCDIR)/isolation_test.cpp -o $(OBJDIR)/isolation.o

clean:
	rm -f $(OBJDIR)/*

