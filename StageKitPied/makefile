COMPILER_G++          := g++
COMPILER_ARM          := arm-linux-gnueabihf-gcc

COMPILER              := g++
OBJ_DIR               := objects
HELPERS_SRC_DIR       := helpers
HELPERS_SRC_FILES     := $(wildcard $(HELPERS_SRC_DIR)/*.cpp)
HELPERS_OBJ_FILES     := $(patsubst $(HELPERS_SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(HELPERS_SRC_FILES))
LEDS_SRC_DIR          := leds
LEDS_SRC_FILES        := $(wildcard $(LEDS_SRC_DIR)/*.cpp)
LEDS_OBJ_FILES        := $(patsubst $(LEDS_SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(LEDS_SRC_FILES))
SERIAL_SRC_DIR        := serial
SERIAL_SRC_FILES      := $(wildcard $(SERIAL_SRC_DIR)/*.cpp)
SERIAL_OBJ_FILES      := $(patsubst $(SERIAL_SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SERIAL_SRC_FILES))
STAGEKIT_SRC_DIR      := stagekit
STAGEKIT_SRC_FILES    := $(wildcard $(STAGEKIT_SRC_DIR)/*.cpp)
STAGEKIT_OBJ_FILES    := $(patsubst $(STAGEKIT_SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(STAGEKIT_SRC_FILES))
CONTROLLER_SRC_DIR    := controller
CONTROLLER_SRC_FILES  := $(wildcard $(CONTROLLER_SRC_DIR)/*.cpp)
CONTROLLER_OBJ_FILES  := $(patsubst $(CONTROLLER_SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(CONTROLLER_SRC_FILES))
SKP_SRC               := stagekitpied.cpp
SKP_OBJ_FILES         := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SKP_SRC))
SKP_OUT               := skp
FLAGS                 := -g -Wall -std=c++11
LUSB_PATH             := -I/usr/include/libusb-1.0/
LUSB_FLAG             := -lusb-1.0
LPTHREAD_FLAG         := -lpthread
INC_PATHS             := -I./

all: skp

skp: $(HELPERS_OBJ_FILES) $(STAGEKIT_OBJ_FILES) $(LEDS_OBJ_FILES) $(SERIAL_OBJ_FILES) $(CONTROLLER_OBJ_FILES) $(SKP_OBJ_FILES)
	$(COMPILER) $(FLAGS) $(INC_PATHS) $^ -o $(SKP_OUT) $(LUSB_FLAG)

helpers: $(HELPERS_OBJ_FILES)

leds: $(LEDS_OBJ_FILES)

serial: $(SERIAL_OBJ_FILES)

stagekit: $(STAGEKIT_OBJ_FILES)

controller: $(CONTROLLER_OBJ_FILES)

$(OBJ_DIR)/%.o: %.cpp
	$(COMPILER) $(FLAGS) $(INC_PATHS) $(LUSB_PATH) -c -o $@ $< 

$(OBJ_DIR)/%.o: $(HELPERS_SRC_DIR)/%.cpp
	$(COMPILER) $(FLAGS) $(INC_PATHS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(LEDS_SRC_DIR)/%.cpp
	$(COMPILER) $(FLAGS) $(INC_PATHS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SERIAL_SRC_DIR)/%.cpp
	$(COMPILER) $(FLAGS) $(INC_PATHS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(STAGEKIT_SRC_DIR)/%.cpp
	$(COMPILER) $(FLAGS) $(INC_PATHS) $(LUSB_PATH) -c -o $@ $<

$(OBJ_DIR)/%.o: $(CONTROLLER_SRC_DIR)/%.cpp
	$(COMPILER) $(FLAGS) $(INC_PATHS) $(LUSB_PATH) -c -o $@ $<

print-%  : ; @echo $* = $($*)

clean:
	rm -f $(HELPERS_OBJ_FILES) $(LEDS_OBJ_FILES) $(SERIAL_OBJ_FILES) $(STAGEKIT_OBJ_FILES) $(CONTROLLER_OBJ_FILES) $(SKP_OBJ_FILES)
