CXX = g++
AR = ar
TYPE ?= STREAM
TARGET ?= ONLINE 
CXXFLAGS = -mavx -Wall -Wformat -Wunused-variable -Wsign-compare -Wunused-local-typedefs -Winit-self -DHAVE_CXXABI_H -std=c++11 -fPIC -g -D$(TYPE) -D$(TARGET)

INCLUDES =  -I./include/ 

LIBS = -lm -lpthread -ldl  

COMMON_SRCFILES = $(wildcard src/common/*.cc)
COMMON_OBJFILES = $(patsubst %.cc,%.o,$(COMMON_SRCFILES))

CLIENT_SRCFILES = $(wildcard src/client/*.cc)
CLIENT_OBJFILES = $(patsubst %.cc,%.o,$(CLIENT_SRCFILES))

SERVER_SRCFILES = $(wildcard src/server/*.cc)
SERVER_OBJFILES = $(patsubst %.cc,%.o,$(SERVER_SRCFILES))

FILE_STORE_SERVER = bin/FileStoreServer
FILE_STORE_CLIENT = bin/FileStoreClient
END = phony_end

.PHONY: all clean

all: $(END)
$(END) : $(FILE_STORE_CLIENT) $(FILE_STORE_SERVER)

$(FILE_STORE_CLIENT) : $(CLIENT_OBJFILES)  $(COMMON_OBJFILES) 
	@echo "[\033[32mFile Store Client: BUILD\033[0m]" "[\033[36mTarget:$@\033[0m]"
	@$(CXX) $(CLIENT_OBJFILES) -Xlinker "-(" $(COMMON_OBJFILES) $(LIBS) -Xlinker "-)" -o $@
	@cp src/client/client.conf bin/

$(FILE_STORE_SERVER) : $(SERVER_OBJFILES) $(COMMON_OBJFILES) 
	@echo "[\033[32mFile Store Server: BUILD\033[0m]" "[\033[36mTarget:$@\033[0m]"
	@$(CXX) $(SERVER_OBJFILES) -Xlinker "-(" $(COMMON_OBJFILES) $(LIBS) -Xlinker "-)" -o $@
	@cp src/server/server.conf bin/

$(SERVER_OBJFILES): %.o :%.cc
	@echo "[\033[32mServer Obj Files:BUILD\033[0m]" "[\033[36mTarget:$@\033[0m]"
	@$(CXX) -c $< -o $@ $(INCLUDES) $(CXXFLAGS)

$(CLIENT_OBJFILES): %.o :%.cc
	@echo "[\033[32mClient Obj Files:BUILD\033[0m]" "[\033[36mTarget:$@\033[0m]"
	@$(CXX) -c $< -o $@ $(INCLUDES) $(CXXFLAGS)

$(COMMON_OBJFILES): %.o: %.cc
	@echo "[\033[32mCommon Files:BUILD\033[0m]" "[\033[36mTarget:$@\033[0m]"
	@$(CXX) -c $< -o $@ $(INCLUDES) $(CXXFLAGS)

clean: 
	@echo "[\033[32mClean:BUILD\033[0m]" "[\033[36mTarget:None\033[0m]"
	@rm -f $(COMMON_OBJFILES) $(DECODERLIB) src/common/*.o src/client/*.o src/server/*.o  bin/*
