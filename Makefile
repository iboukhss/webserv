# Executable name
NAME      = webserv

# Directories
SRC_DIR   = src
BUILD_DIR = build
OBJ_DIR   = $(BUILD_DIR)/obj
BIN_DIR   = $(BUILD_DIR)/bin

# Main target
TARGET    = $(BIN_DIR)/$(NAME)

# Source tree (keep in alphabetical order)
SRCS      = $(addprefix $(SRC_DIR)/, \
            config/server_config.hpp \
            core/client.cpp \
            core/client.hpp \
            core/server.cpp \
            core/server.hpp \
            http/http_request.hpp \
            http/http_response.cpp \
            http/http_response.hpp \
            router/router.cpp \
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

# Regular cleaning targets
PHONY += clean fclean re
clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(BIN_DIR)

re: fclean all

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

check: format lint

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
