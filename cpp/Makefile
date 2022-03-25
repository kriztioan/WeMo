PROG:=wemod
LIBS:=-lssl -lcrypto
CPP_FILES:=$(wildcard *.cpp)
OBJ_FILES:=$(patsubst %.cpp,%.o,$(CPP_FILES))

$(PROG): $(OBJ_FILES)
	$(CXX) -o $(PROG) $(OBJ_FILES) $(LIBS)

%.o: %.cpp
	$(CXX) -c $<

clean:
	$(RM) *.o $(PROG)
