# Use `make CONFIG=release` or `make CONFIG=debug` to switch between build
# configurations
CONFIG ?= debug

# Compiler settings
CXX = clang++
CXXFLAGS = -std=c++98 -Wall -Wextra -Werror
CPPFLAGS = -Isrc -Ithird_party/utest -MMD -MP

# Build specific options
ifeq ($(CONFIG),debug)
  CXXFLAGS += -g3 -fsanitize=address,undefined
  CPPFLAGS += -DDEBUG
else ifeq ($(CONFIG),release)
  CXXFLAGS += -O2
  CPPFLAGS += -DNDEBUG
endif

# Targets
target = build/$(CONFIG)/bin/webserv
test_target = build/$(CONFIG)/bin/run_tests

# Main sources (keep in alphabetical order)
srcs = \
  src/config/server_config.cpp \
  src/config/server_config.hpp \
  src/core/client.cpp \
  src/core/client.hpp \
  src/core/server.cpp \
  src/core/server.hpp \
  src/core/server_defaults.hpp \
  src/core/signals.cpp \
  src/core/signals.hpp \
  src/handler/delete_handler.cpp \
  src/handler/delete_handler.hpp \
  src/handler/error_handler.cpp \
  src/handler/error_handler.hpp \
  src/handler/handler.cpp \
  src/handler/handler.hpp \
  src/handler/static_file_handler.cpp \
  src/handler/static_file_handler.hpp \
  src/handler/upload_handler.cpp \
  src/handler/upload_handler.hpp \
  src/http/http_parser.cpp \
  src/http/http_parser.hpp \
  src/http/http_request.cpp \
  src/http/http_request.hpp \
  src/http/http_response.cpp \
  src/http/http_response.hpp \
  src/http/http_version.cpp \
  src/http/http_version.hpp \
  src/router/router.cpp \
  src/router/router.hpp \
  src/util/log_message.cpp \
  src/util/log_message.hpp \
  src/util/str_split.cpp \
  src/util/str_trim.cpp \
  src/util/string.hpp \
  src/util/syscall_error.cpp \
  src/util/syscall_error.hpp \
  src/main.cpp \

cpps = $(filter %.cpp,$(srcs))
hpps = $(filter %.hpp,$(srcs))

objs = $(patsubst src/%.cpp,build/$(CONFIG)/obj/%.o,$(cpps))
deps = $(objs:.o=.d)

# Test sources (keep in alphabetical order)
test_srcs = \
  tests/delete_handler_unittest.cpp \
  tests/upload_handler_unittest.cpp \
  tests/error_handler_unittest.cpp \
  tests/http_parser_unittest.cpp \
  tests/http_response_unittest.cpp \
  tests/router_unittest.cpp \
  tests/static_file_handler_unittest.cpp \
  tests/str_split_unittest.cpp \
  tests/main.cpp \

test_cpps = $(filter %.cpp,$(test_srcs))

test_objs = $(patsubst tests/%.cpp,build/$(CONFIG)/obj_tests/%.o,$(test_cpps))
test_deps = $(test_objs:.o=.d)

all_cpps = $(cpps) $(test_cpps)
all_hpps = $(hpps)
all_deps = $(deps) $(test_deps)

# Build main target
PHONY += all run
all: $(target)

run: all
	./$(target)

$(target): $(objs)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(objs) -o $@

build/$(CONFIG)/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

# Build test target
PHONY += test
test: $(test_target)
	@echo "Running unit tests..."
	./$(test_target)

$(test_target): $(test_objs) $(filter-out build/$(CONFIG)/obj/main.o,$(objs))
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@

build/$(CONFIG)/obj_tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

# Cleanup rules
PHONY += clean fclean re
clean:
	rm -rf build/$(CONFIG)

fclean: clean
	rm -rf build

re: fclean all

# Formatting and linting rules
PHONY += db format lint check
db:
	compiledb -n $(MAKE)

format:
	clang-format --style=file --dry-run --Werror $(all_cpps) $(all_hpps)

lint: db
	clang-tidy -p=. --header-filter=src/ --warnings-as-errors=* $(all_cpps)

# Rule used by GitHub CI
check: format lint test

-include $(all_deps)

.PHONY: $(PHONY)
