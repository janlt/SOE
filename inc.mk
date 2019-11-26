CXX := g++
CC := gcc

SRC_DIR := src
INC_DIR := include
LIB_DIR := lib
BIN_DIR := bin
OPT_INC := -I/usr/include
OPT_INC += -I/opt/lzlabs/include

DEPS_INC_LIST := $(foreach word,$(DEPS_INC),-I../$(word)/$(INC_DIR))
DEPS_INC_LIST += $(foreach word,$(DEPS_INC),-I../../$(word)/$(INC_DIR))
DEPS_LIB_LIST := $(foreach word,$(DEPS_LIB),-L../$(word)/$(LIB_DIR) -L../../$(word)/$(LIB_DIR) -l$(word))

CPPFLAGS := -I$(SRC_DIR) -I$(INC_DIR) $(OPT_INC) $(DEPS_INC_LIST) 
CPPFLAGS += -g -O2 -fPIC -std=c++11 -Wno-deprecated -Wno-deprecated-declarations

CFLAGS := -I$(SRC_DIR) -I$(INC_DIR) $(OPT_INC) $(DEPS_INC_LIST)
CFLAGS += -g -O2 -fPIC -std=c99

LDFLAGS := $(DEPS_LIB_LIST)
LDFLAGS += -lpthread -L/opt/lzlabs/lib64 -lboost_thread -lboost_chrono -lboost_filesystem -lboost_log -lboost_system
LDBFLAGS := $(LDFLAGS)
LDBFLAGS += -lrocksdb -ljsoncpp

SRC_CPP_FILES := $(wildcard $(SRC_DIR)/*.cpp)
SRC_C_FILES := $(wildcard $(SRC_DIR)/*.c)
SRC_FILES := $(SRC_CPP_FILES) $(SRC_C_FILES)

OBJ_CPP_FILES  := $(SRC_CPP_FILES:$(SRC_DIR)/%.cpp=%.o)
OBJ_C_FILES  := $(SRC_C_FILES:$(SRC_DIR)/%.c=%.o)
OBJ_FILES := $(OBJ_CPP_FILES) $(OBJ_C_FILES)

TARGET_BIN := $(BIN_DIR)/$(BINARY)

TARGET_SO := $(LIB_DIR)/lib$(LIBRARY).so
TARGET_A := $(LIB_DIR)/lib$(LIBRARY).a

#$(TARGET): $(TARGET_SO) $(TARGET_A)

ifeq ($(LIBRARY),)
$(TARGET): $(TARGET_BIN)
$(TARGET_BIN): $(OBJ_CPP_FILES) $(OBJ_C_FILES)
	[ ! -z $(BINARY) ] && mkdir -p $(BIN_DIR)
	g++ $(LDBFLAGS) -o $@ $(OBJ_CPP_FILES) $(OBJ_C_FILES)
else
$(TARGET): $(TARGET_SO) $(TARGET_A)
$(TARGET_A): $(TARGET_SO)

$(TARGET_SO): $(OBJ_CPP_FILES) $(OBJ_C_FILES)
	[ ! -z $(LIBRARY) ] && mkdir -p $(LIB_DIR)
	g++ -shared $(LDFLAGS) -o $@ $(OBJ_CPP_FILES) $(OBJ_C_FILES)

$(TARGET_A): $(OBJ_CPP_FILES) $(OBJ_C_FILES)
	[ ! -z $(LIBRARY) ] && mkdir -p $(LIB_DIR)
	ar rcs -o $@ $(OBJ_CPP_FILES) $(OBJ_C_FILES)

endif

%.o: $(SRC_DIR)/%.cpp
	@echo "$(CXX) " $<
	@ $(CXX) $(CPPFLAGS) -o $@ -c $<

%.o: $(SRC_DIR)/%.c
	@echo "$(CC) " $<
	@ $(CC) $(CFLAGS) -o $@ -c $<

.PHONY : all
all: $(TARGET)

clean:
	rm -rf $(LIB_DIR)
	rm -rf $(BIN_DIR)
	rm -f $(OBJ_FILES)
