# Setup
#
NAME = leccy

CPP = c++
CPPFLAGS = -Wall -Wextra -Werror -Os -std=c++11 -flto #-g
CPPLIBS = -lcurl

RM = rm -f

# Sources
#
SRC = $(wildcard *.cpp)
INC = $(wildcard *.hpp)
OBJ = $(SRC:.cpp=.o)

# Rules
#
.PHONY : all
all : $(NAME)

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@ $(CPPLIBS)

$(NAME) : $(OBJ)
	$(CPP) $(CPPFLAGS) $^ -o $@ $(CPPLIBS)

.PHONY : clean
clean :
	$(RM) $(OBJ)

.PHONY : fclean
fclean : clean
	$(RM) $(NAME)

.PHONY : re
re : fclean all
