
# Compiler
CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2

# Find all nested include dirs named 'include'
INCLUDE_DIRS := include/
CXXFLAGS += $(addprefix -I, $(INCLUDE_DIRS)) 
# Libraries
LDLIBS := -pthread -lsqlite3


# Files
SRC := src/main.cpp
TEST_SRC := src/test.cpp 
RST_SRC := src/reset.cpp 

OBJ := build/main.o 
TEST_OBJ := build/test.o 
RST_OBJ := build/reset.o

OUT := build/main
TEST_OUT := build/test
RST_OUT := build/reset

.PHONY: all clean bear

out: $(OUT)

test: $(TEST_OUT)

reset: $(RST_OUT)

all: out test reset

# Link object file to create bina$(OUT): $(DAEMON_OBJ)
$(OUT): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

$(TEST_OUT): $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

$(RST_OUT): $(RST_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

# Compile source to object
build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ensure build directory exists
build:
	mkdir -p build

# Generate compile_commands.json using bear
bear:
	bear -- make clean all

clean:
	rm -rf build
	rm -f compile_commands.json
