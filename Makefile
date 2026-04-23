# Makefile - RFC 9213 API Gateway Priority Scheduler
# Kullanim: make | make clean | make run | make valgrind

CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -Iinclude
DBGFLAGS = -g -fsanitize=address,undefined

TARGET  = scheduler
SRC_DIR = src
OBJ_DIR = obj

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/rfc9213_parser.c \
       $(SRC_DIR)/priority_queue.c

OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Varsayilan hedef
.PHONY: all
all: $(OBJ_DIR) $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	@echo "[OK] Build tamamlandi: ./$(TARGET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Debug build (AddressSanitizer + UBSan)
.PHONY: debug
debug: CFLAGS += $(DBGFLAGS)
debug: $(OBJ_DIR) $(TARGET)

# Calistir
.PHONY: run
run: all
	./$(TARGET)

# Valgrind bellek kontrolu
.PHONY: valgrind
valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all \
	         --track-origins=yes --verbose \
	         ./$(TARGET) 2>&1 | tee valgrind_output.txt

# Temizlik
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(TARGET) valgrind_output.txt
	@echo "[OK] Temizlendi."

# Yardim
.PHONY: help
help:
	@echo "Kullanim:"
	@echo "  make         - Normal build"
	@echo "  make debug   - Debug build (ASan+UBSan)"
	@echo "  make run     - Build ve calistir"
	@echo "  make valgrind- Bellek sizinti kontrolu"
	@echo "  make clean   - Temizle"
