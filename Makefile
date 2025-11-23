################################################################################
# Project setup

NAME      = webserv

SRC_DIR   = src
BUILD_DIR = build
OBJ_DIR   = $(BUILD_DIR)/obj
BIN_DIR   = $(BUILD_DIR)/bin

TARGET    = $(BIN_DIR)/$(NAME)

# Source tree (keep in alphabetical order)
SRCS      = $(addprefix $(SRC_DIR)/, \
            config/server_config.cpp \
            config/server_config.hpp \
            core/client.cpp \
            core/client.hpp \
            core/server.cpp \
            core/server.hpp \
            core/server_defaults.hpp \
            core/signals.cpp \
            core/signals.hpp \
            handler/static_file_handler.cpp \
            handler/static_file_handler.hpp \
            handler/handler.cpp \
            handler/handler.hpp \
            http/http_request.hpp \
            http/http_response.cpp \
            http/http_response.hpp \
            router/router.cpp \
            router/router.hpp \
            util/str_split.cpp \
            util/string.hpp \
            util/syscall_error.cpp \
            util/syscall_error.hpp \
            main.cpp \
)

# Separate .cpp and .hpp files
CPPS      = $(filter %.cpp,$(SRCS))
HPPS      = $(filter %.hpp,$(SRCS))

# Objects and dependencies
OBJS      = $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%,$(CPPS:.cpp=.o))
DEPS      = $(OBJS:.o=.d)

# Compiler settings
CXX       = clang++
CXXFLAGS  = -Wall -Wextra -Werror -std=c++98 -g3
CPPFLAGS  = -I$(SRC_DIR) -MMD -MP

# For utest.h
CPPFLAGS  += -Ithird_party/utest

################################################################################
# Default build

# Default target
PHONY += all run val
all: $(TARGET)

run: all
	./$(TARGET)

val: all
	valgrind ./$(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OBJS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

# Default cleaning targets
PHONY += clean fclean re
clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(BIN_DIR)

re: fclean all

################################################################################
# Unit tests

TEST_NAME    = run_tests

TEST_SRC_DIR = tests
TEST_OBJ_DIR = $(BUILD_DIR)/obj_tests

TEST_TARGET  = $(BIN_DIR)/$(TEST_NAME)

# All tests sources (keep in alphabetical order)
TEST_SRCS    = $(addprefix $(TEST_SRC_DIR)/, \
               http_response_unittest.cpp \
               router_unittest.cpp \
               static_file_handler_unittest.cpp \
               str_split_unittest.cpp \
               main.cpp \
)

TEST_CPPS    = $(filter %.cpp,$(TEST_SRCS))

TEST_OBJS    = $(patsubst $(TEST_SRC_DIR)/%,$(TEST_OBJ_DIR)/%,$(TEST_CPPS:.cpp=.o))
TEST_DEPS    = $(TEST_OBJS:.o=.d)

DEPS         += $(TEST_DEPS)

# Test runner target
PHONY += test
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS) $(filter-out $(OBJ_DIR)/main.o,$(OBJS))
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@

$(TEST_OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

# Tests cleaning target
PHONY += test-clean
test-clean:
	rm -rf $(TEST_OBJ_DIR) $(TEST_TARGET)

################################################################################
# Formatting and linting section

# If compiledb is not installed do `pipx install compiledb`.
# compiledb is used to generate the compilation database (compile_commands.json)
# used by clang-tidy.

PHONY += db format lint check
db:
	compiledb -n $(MAKE)

format:
	clang-format -style=file --dry-run -Werror $(CPPS) $(HPPS)

lint: db
	clang-tidy -p=. --header-filter=.* --warnings-as-errors=* $(CPPS)

check: format lint test

# Careful, these targets will overwrite files.
# Make sure to use `make check` before committing irreversible changes.
PHONY += format-fix lint-fix fix
format-fix:
	clang-format -style=file -i $(CPPS) $(HPPS)

lint-fix: db
	clang-tidy -p=. --header-filter=.* -fix $(CPPS)

fix: format-fix lint-fix



# Include dependencies
-include $(DEPS)

.PHONY: $(PHONY)
