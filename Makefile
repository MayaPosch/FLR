# Makefile for the FLR server and client.
#
# Allows one to build the server, client, or both.
#
# (c) 2018, Maya Posch, Nyanko.ws

# OS detection & specific settings.
ifeq ($(OS),Windows_NT)
    CCFLAGS := -DPOCO_WIN32_UTF8  -U__STRICT_ANSI__
# else
    # UNAME_S := $(shell uname -s)
    # ifeq ($(UNAME_S),Linux)
        # CCFLAGS += -D LINUX
    # endif
    # ifeq ($(UNAME_S),Darwin)
        # CCFLAGS += -D OSX
    # endif
    # UNAME_P := $(shell uname -p)
    # ifeq ($(UNAME_P),x86_64)
        # CCFLAGS += -D AMD64
    # endif
    # ifneq ($(filter %86,$(UNAME_P)),)
        # CCFLAGS += -D IA32
    # endif
    # ifneq ($(filter arm%,$(UNAME_P)),)
        # CCFLAGS += -D ARM
    # endif
endif


GCC = g++
MAKEDIR = mkdir -p
RM = rm

SERVER_BIN = flrserver
CLIENT_BIN = flrclient
INCLUDE := -I./common
CFLAGS := $(INCLUDE) -g3 -std=c++11 $(CCFLAGS) -llzma -lnymphrpc -lPocoNet -lPocoUtil -lPocoFoundation -lPocoJSON -lPocoData -lPocoDataSQLite -lPocoCrypto
SERVER_SRC = $(wildcard common/*.cpp) $(wildcard server/*.cpp)
CLIENT_SRC = $(wildcard common/*.cpp) $(wildcard client/*.cpp)
#SERVER_OBJ = $(addprefix obj/,$(notdir) $(SERVER_SRC:.cpp=.o))
#CLIENT_OBJ = $(addprefix obj/,$(notdir) $(CLIENT_SRC:.cpp=.o))

all: client server

client: makedir client_build

server: makedir server_build

makedir:
	$(MAKEDIR) bin
	
client_build:
	$(GCC) -o bin/$(CLIENT_BIN) $(CLIENT_SRC) $(CFLAGS)
	
server_build:
	$(GCC) -o bin/$(SERVER_BIN) $(SERVER_SRC) $(CFLAGS)
