##
## EPITECH PROJECT, 2025
## zappy_gui
## File description:
## Makefile for 3D GUI with Raylib
##

CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -Iinclude -I/usr/local/include -g -O2

LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Si raylib est installé ailleurs, ajuster ces paths
# CXXFLAGS += -I/path/to/raylib/include
# LDFLAGS += -L/path/to/raylib/lib

SRC = src/main.cpp \
      src/Game.cpp \
      src/Camera.cpp \
      src/World.cpp \
      src/Player.cpp \
      src/Tile.cpp \
      src/UI.cpp \
      src/Network.cpp \
      src/ResourceManager.cpp

OBJ = $(SRC:.cpp=.o)

NAME = zappy_gui

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(OBJ) -o $(NAME) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

install-raylib:
	@echo "Installing raylib..."
	git clone https://github.com/raysan5/raylib.git /tmp/raylib
	cd /tmp/raylib && mkdir build && cd build && \
	cmake .. && make && sudo make install
	rm -rf /tmp/raylib
	@echo "Raylib installed!"

.PHONY: all clean fclean re install-raylib
