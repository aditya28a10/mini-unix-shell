CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
CPPFLAGS ?= -Iinclude -MMD -MP

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

TARGET = $(BIN_DIR)/mini-shell

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

.PHONY: all clean test run help dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(TARGET): $(OBJS) | dirs
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | dirs
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

-include $(DEPS)

run: all
	@./$(TARGET)

test: all
	@chmod +x tests/run_tests.sh
	@./tests/run_tests.sh

clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Cleaned build artifacts (obj/, bin/)."

help:
	@echo "Mini Unix Shell Build System"
	@echo "----------------------------"
	@echo "Available targets:"
	@echo "  all    : Compile and link the mini-shell binary (default)"
	@echo "  run    : Build and execute the mini-shell binary"
	@echo "  test   : Run automated test suite in tests/"
	@echo "  clean  : Remove all build outputs and object files"
	@echo "  help   : Display this help menu"
