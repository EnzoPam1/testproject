##
## EPITECH PROJECT, 2025
## Zappy
## File description:
## Makefile
##

NAME = zappy_ai
PYTHON = python3
SRC_DIR = src
MAIN_FILE = $(SRC_DIR)/main.py

all: $(NAME)

$(NAME):
	@echo "Creating $(NAME) executable..."
	@echo '#!/bin/bash' > $(NAME)
	@echo 'cd $$(dirname $$0)' >> $(NAME)
	@echo 'PYTHONPATH=$$PYTHONPATH:src $(PYTHON) $(MAIN_FILE) $$@' >> $(NAME)
	@chmod +x $(NAME)
	@echo "$(NAME) created successfully!"

clean:
	@find . -type f -name "*.pyc" -delete
	@find . -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
	@echo "Cleaned Python cache files"

fclean: clean
	@rm -f $(NAME)
	@echo "Removed $(NAME)"

re: fclean all

install:
	@if [ -f requirements.txt ]; then \
		echo "Installing Python dependencies..."; \
		pip3 install -r requirements.txt; \
	fi

.PHONY: all clean fclean re install