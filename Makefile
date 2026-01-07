CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread -I./include

# Directories
SRCDIR = src
INCDIR = include
BUILDDIR = build
BINDIR = bin
CLIENTDIR = client

# Files
SERVER_SOURCES = $(wildcard $(SRCDIR)/*.cpp)
SERVER_OBJECTS = $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SERVER_SOURCES))

CLIENT_SOURCES = $(wildcard $(CLIENTDIR)/*.cpp)
CLIENT_OBJECTS = $(patsubst $(CLIENTDIR)/%.cpp,$(BUILDDIR)/client_%.o,$(CLIENT_SOURCES))

TARGET_SERVER = $(BINDIR)/server
TARGET_CLIENT = $(BINDIR)/client

# Rules
all: $(TARGET_SERVER) $(TARGET_CLIENT)

$(TARGET_SERVER): $(SERVER_OBJECTS)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TARGET_CLIENT): $(CLIENT_OBJECTS)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ -lncurses

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/client_%.o: $(CLIENTDIR)/%.cpp
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@





# Tests
TEST_THREADPOOL = $(BINDIR)/test_threadpool
TEST_PROTOCOL = $(BINDIR)/test_protocol

tests: $(TEST_THREADPOOL) $(TEST_PROTOCOL)

$(TEST_THREADPOOL): tests/test_threadpool.cpp src/threadpool.cpp
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_PROTOCOL): tests/test_protocol.cpp
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -rf $(BUILDDIR) $(BINDIR)

.PHONY: all clean tests tests
