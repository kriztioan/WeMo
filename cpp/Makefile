CPP_FILES:=$(wildcard *.cpp)
OBJ_FILES:=$(patsubst %.cpp,%.o,$(CPP_FILES))
DEP_FILES:=deps.d
PROGS:=wemod
CPPFLAGS:=-flto -O2 -MMD -MF $(DEP_FILES)
LIBS:=-lssl -lcrypto -lz

all: $(PROGS)

-include $(DEP_FILES)

$(PROGS): $(OBJ_FILES)
	$(CXX) -o $(PROGS) $(OBJ_FILES) $(CPPFLAGS) $(LIBS)

%.o: %.cpp
	$(CXX) -c $< $(CPPFLAGS)

clean:
	$(RM) $(DEP_FILES) $(OBJ_FILES) $(PROGS)
