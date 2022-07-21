V ?= 0
CC = g++
TEST_COV ?= 0

CFLAGS = -Wall -fPIC
FLAGS_OPTIMIZATION ?= -O3
FLAGS_DEBUG ?= -g -ggdb -O0

ifeq ($(TEST_COV),1)
	CFLAGS += -fprofile-arcs -ftest-coverage
endif

node := $(shell which node)
npm := $(shell which npm)
clang-format-bin := ./tools/node_modules/.bin/clang-format
cpplint-bin := ./tools/node_modules/.bin/cpplint

BUILDTYPE ?= Debug
RESIZABLE_BUFFER_HEADER_FILE := include/resizable_buffer.h
TEST_FILES := test/test.cc

TEST_EXE := build/test
RELEASE_FLAGS := $(FLAGS_OPTIMIZATION) $(CFLAGS)
TEST_G_EXE := build/test_g
DEBUG_FLAGS := $(FLAGS_DEBUG) $(CFLAGS)

.PHONY: all test
ifeq ($(BUILDTYPE),Release)
all: $(TEST_EXE)
test: all
	$(TEST_EXE) -d
else
all: $(TEST_G_EXE)
test: all
	$(TEST_G_EXE) -d
endif

.PHONY: dev-prepare clang-format cpplint
dev-prepare: .node-modules-installed
clang-format: .clang-formatted
cpplint: .cpplinted

.node-modules-installed: tools/package.json
	if [ ! -a $(npm) ]; then																			\
		echo "npm not found";																				\
		exit 1;																											\
	else																													\
	  cd tools && rm -rf node_modules && $(npm) install && cd ..; \
	fi
	touch .node-modules-installed

$(clang-format-bin): .node-modules-installed
$(cpplint-bin): .node-modules-installed

.clang-formatted: $(clang-format-bin)	$(RESIZABLE_BUFFER_HEADER_FILE) $(TEST_FILES)
	$(clang-format-bin) -i --style=file $(RESIZABLE_BUFFER_HEADER_FILE) $(TEST_FILES)
	touch .clang-formatted

.cpplinted: $(cpplint-bin) $(RESIZABLE_BUFFER_HEADER_FILE) $(TEST_FILES)
	$(cpplint-bin) $(RESIZABLE_BUFFER_HEADER_FILE) $(TEST_FILES)
	touch .cpplinted

$(TEST_EXE).o: test/* include/*.h
	mkdir -p build
	$(CC) $(RELEASE_FLAGS) -c test/test.cc -o $@

$(TEST_G_EXE).o: test/* include/*.h
	mkdir -p build
	$(CC) $(DEBUG_FLAGS) -c test/test.cc -o $@

$(TEST_EXE): $(TEST_EXE).o
	$(CC) $(RELEASE_FLAGS) $(TEST_EXE).o -o $(TEST_EXE)
	if [ ! -a $(TEST_EXE) ]; then																	\
		echo "test executable not found";														\
		exit 1;																											\
	fi

$(TEST_G_EXE): $(TEST_G_EXE).o
	$(CC) $(DEBUG_FLAGS) $(TEST_G_EXE).o -o $(TEST_G_EXE)
	if [ ! -a $(TEST_G_EXE) ]; then																\
		echo "test executable not found";														\
		exit 1;																											\
	fi

.PHONY: coverage
coverage:
	$(make) test TEST_COV=1
	lcov --capture \
		--directory $(CURDIR) \
		--base-directory $(CURDIR) \
		--include $(CURDIR)/include/resizable_buffer.h \
		--output-file $(CURDIR)/coverage.info
	genhtml $(CURDIR)/coverage.info --output-directory $(CURDIR)/coverage
	rm -f $(CURDIR)/coverage.info
