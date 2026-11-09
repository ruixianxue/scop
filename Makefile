# ============================================================
#  scop Makefile
# ============================================================

NAME     = scop

CXX      = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++17
LDFLAGS  = -lglfw -lGL

SRC_DIR  = src
OBJ_DIR  = obj

SRCS     = $(SRC_DIR)/main.cpp
OBJS     = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Default rule: build the program
all: $(NAME)

# Link all object files into the final program
$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME) $(LDFLAGS)

# Compile each .cpp file into a .o file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Remove object files
clean:
	rm -rf $(OBJ_DIR)

# Remove object files and the program
fclean: clean
	rm -f $(NAME)

# Rebuild everything from scratch
re: fclean all

.PHONY: all clean fclean re
