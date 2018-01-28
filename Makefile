# Makefile for the FLR server and client.
#
# Allows one to build the server, client, or both.
#
# (c) 2018, Maya Posch, Nyanko.ws


GCC = g++
MAKEDIR = mkdir -p
RM = rm

SERVER_BIN = flrserver
CLIENT_BIN = flrclient
INCLUDE := -I./common
CFLAGS := $(INCLUDE) -g3 -std=c++11 -U__STRICT_ANSI__ -llzma -lnymphrpc -lPocoNet -lPocoUtil -lPocoFoundation -lPocoJSON -lPocoData -lPocoDataSQLite -lPocoCrypto
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
