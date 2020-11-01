EXE = npshell np_simple np_single_proc
OBJ_DIR = obj

SOURCES = $(filter-out $(wildcard src/np*.cpp), $(wildcard src/*.cpp))

OBJS = $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))

OBJS_NPSHELL = $(OBJS) $(OBJ_DIR)/npshell.o
OBJS_NP_SIMPLE = $(OBJS) $(OBJ_DIR)/np_simple.o
OBJS_NP_SINGLE_PROC = $(OBJS) $(OBJ_DIR)/np_single_proc.o

CXXFLAGS = -std=c++17 -I./include -Wall -O2

LIBS =

$(OBJ_DIR)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: create_object_directory $(EXE)
	@echo Compile Success

create_object_directory:
	mkdir -p $(OBJ_DIR)

npshell: $(OBJS_NPSHELL)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

np_simple: $(OBJS_NP_SIMPLE)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

np_single_proc: $(OBJS_NP_SINGLE_PROC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf $(EXE) $(OBJ_DIR)
