PROG:=wemod
CPP_FILES:=$(wildcard *.cpp)
OBJ_FILES:=$(patsubst %.cpp,%.o,$(CPP_FILES))

$(PROG): $(OBJ_FILES)
	c++ -o wemod $(OBJ_FILES)

%.o: %.cpp
	c++ -c $<

clean:
	rm -f *.o wemod