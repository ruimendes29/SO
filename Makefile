CC      = gcc
LD      = gcc
CFLAGS  = -O2 -Wall -Wextra -g
CFLAGS += -Wno-unused-parameter -Wno-unused-function -Wno-unused-result
INCLDS  = -I $(INC_DIR)
BIN_DIR = bin
BLD_DIR = build
DOC_DIR = docs
INC_DIR = includes
LOG_DIR = log
OUT_DIR = out
SRC_DIR = src
TBL_DIR = tables
TST_DIR = scripts
TARGETS = ag cv ma sv

.DEFAULT_GOAL = all

.PHONY: $(TARGETS) all check checkdirs clean doc fmt lint test

all: checkdirs $(TARGETS)

$(BLD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $(INCLDS) $(CFLAGS) $< -o $@

ag: $(BLD_DIR)/io.o $(BLD_DIR)/arraylist.o
	$(CC) $(INCLDS) $(CFLAGS) -o $(BIN_DIR)/$@ $(SRC_DIR)/agregador.c $^

cv: $(BLD_DIR)/io.o
	$(CC) $(INCLDS) $(CFLAGS) -o $(BIN_DIR)/$@ $(SRC_DIR)/cliente.c $^

ma: $(BLD_DIR)/io.o
	$(CC) $(INCLDS) $(CFLAGS) -o $(BIN_DIR)/$@ $(SRC_DIR)/manutencao.c $^

sv: $(BLD_DIR)/io.o
	$(CC) $(INCLDS) $(CFLAGS) -o $(BIN_DIR)/$@ $(SRC_DIR)/servidor.c $^

start: all
	./$(BIN_DIR)/sv

stop:
	kill -s SIGTERM $(shell pidof sv)

fmt:
	@echo "C and Headers files:"
	@-clang-format -style="{BasedOnStyle: Google, IndentWidth: 4}" -verbose -i \
		$(SRC_DIR)/* $(INC_DIR)/*
	@echo ""
	@echo "Shell files:"
	@shfmt -l -w -i 2 .

lint:
	@splint -retvalint -hints -I $(INC_DIR) \
		$(SRC_DIR)/*

check: all
	@echo "Write something to check!"

doc:
	@doxygen $(DOC_DIR)/Doxyfile

test: all
	./$(TST_DIR)/test_manutencao.sh
	./$(TST_DIR)/test_agregador.sh
	./$(TST_DIR)/test_cliente.sh

checkdirs:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(BLD_DIR)
	@mkdir -p $(DOC_DIR)
	@mkdir -p $(LOG_DIR)
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(TBL_DIR)

clean:
	@echo "Cleaning..."
	@echo ""
	@cat .art/maid.ascii
	@-rm -rf $(BLD_DIR)/* $(BIN_DIR)/* $(OUT_DIR)/* $(LOG_DIR)/* \
		$(DOC_DIR)/html $(DOC_DIR)/latex
	@echo ""
	@echo "...✓ done!"
