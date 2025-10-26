NAME = webserv

CXX = clang++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g3 -MMD -MP

SRCS = main.cpp Epoll.cpp Server.cpp Socket.cpp Client.cpp
HDRS = Epoll.hpp Server.hpp Socket.hpp Client.hpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all clean fclean re val db format format-fix lint lint-fix

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) -o $(NAME) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.d

fclean: clean
	rm -f $(NAME)

re: fclean all

val:
	valgrind ./webserv

db:
	compiledb -n $(MAKE)

format:
	clang-format --style=file --dry-run *.cpp *.hpp

format-fix:
	clang-format --style=file -i *.cpp *.hpp

lint:
	clang-tidy -p=. -header-filter=.* *.cpp

lint-fix:
	clang-tidy -p=. -header-filter=.* -fix *.cpp

check: format lint

fix: format-fix lint-fix

-include $(DEPS)
