##
## EPITECH PROJECT, 2025
## Makefile zappy
## File description:
## Main Makefile
##

all: zappy_server zappy_gui zappy_ai

zappy_server:
	$(MAKE) -C server
	cp server/bin/zappy_server .

zappy_gui:
	$(MAKE) -C gui
	cp gui/zappy_gui .

zappy_ai:
	cp ai/src/*.py .
	cp ai/src/main.py zappy_ai
	chmod +x zappy_ai

clean:
	$(MAKE) -C server clean
	$(MAKE) -C gui clean
	$(MAKE) -C ai clean
	rm -f *.py __pycache__ -rf

fclean: clean
	$(MAKE) -C server clean
	$(MAKE) -C gui fclean
	$(MAKE) -C ai fclean
	rm -f zappy_server zappy_gui zappy_ai
	rm -f *.py __pycache__ -rf

re: fclean all

.PHONY: all zappy_server zappy_gui zappy_ai clean fclean re
