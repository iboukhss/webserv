CXX = clang++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS = src/main.cpp src/Socket.cpp src/Server.cpp src/Client.cpp
OBJS = $(SRCS:.cpp=.o)
NAME = webserv

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f src/*.o

fclean: clean
	rm -f webserv

re: fclean all

.PHONY: all clean fclean re
