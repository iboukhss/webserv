NAME = webserv

CXX = clang++
CXXFLAGS = -Wall -Werror -Wextra -std=c++98 -g3

SRCS = main.cpp Epoll.cpp Server.cpp Socket.cpp Client.cpp
OBJS = $(SRCS:.cpp=.o)
HDRS = Epoll.hpp Server.hpp Socket.hpp Client.hpp

.PHONY: all clean fclean re val db lint

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f *.o

fclean: clean
	rm -f $(NAME)

re: fclean all

val:
	valgrind ./webserv

db:
	compiledb -n $(MAKE)

lint:
	run-clang-tidy -p=. -header-filter=.* -use-color
