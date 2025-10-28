NAME = webserv

CXX = clang++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g3
# CPPFLAGS are for C pre-processor flags (different from CXXFLAGS)
CPPFLAGS = -MMD -MP

SRCS = main.cpp Server.cpp Client.cpp HttpResponse.cpp SyscallError.cpp
HDRS = Server.hpp Client.hpp HttpResponse.hpp SyscallError.hpp structs_dev.hpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all clean fclean re val db format format-fix lint lint-fix check fix

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.d

fclean: clean
	rm -f $(NAME)

re: fclean all

val:
	valgrind ./webserv

# Formatting and linting section

# If compiledb is not installed do `pipx install compiledb`.
# compiledb is used to generate the compilation database (compile_commands.json)
# used by clang-tidy.

db:
	compiledb -n $(MAKE)

run: 
	make
	./webserv

format:
	clang-format -style=file -dry-run *.cpp *.hpp

format-fix:
	clang-format -style=file -i *.cpp *.hpp

lint: db
	clang-tidy -p=. -header-filter=.* *.cpp

lint-fix: db
	clang-tidy -p=. -header-filter=.* -fix *.cpp

check: format lint

# Careful, these rules will overwrite files.
# Make sure to use `make check` before committing irreversible changes.

fix: format-fix lint-fix

-include $(DEPS)
