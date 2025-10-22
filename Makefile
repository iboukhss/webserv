NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Werror -Wextra -std=c++98 -g

SRCS = main.cpp Epoll.cpp Server.cpp
OBJS = $(SRCS:.cpp=.o)
HDRS = Epoll.hpp Server.hpp

.PHONY: all clean fclean re

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:	
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean $(NAME)

val:
	valgrind ./webserv
