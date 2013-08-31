CXX=g++
CXXFLAGS=-Wall -g -std=c++11 -I../include 
SOURCES=Test.cpp main.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=cglbtest
LDFLAGS= -lluajit-5.1 

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	
