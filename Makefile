# Define the ROOTSYS environment variable if not already set
# This points to your ROOT installation directory
ROOTSYS = $(shell root-config --prefix)

# Define the C++ compiler
CXX = g++

# Define compiler flags
# -g for debugging, -Wall for warnings, -fPIC for position-independent code (often needed for shared libraries)
# $(shell root-config --cflags) gets ROOT-specific compiler flags
CXXFLAGS = -g -Wall -fPIC $(shell root-config --cflags)
CXXFLAGS += -I../../src -I./include/
# Define linker flags
# $(shell root-config --glibs) gets ROOT-specific linker flags (libraries)
LDFLAGS = $(shell root-config --glibs)
ROOTCFLAGS  = $(shell root-config --cflags)
LIBS = -lCombineHarvesterCombineTools 
LIBPATH = -L../../lib/el9_amd64_gcc12/

#Define src and objects
SRC_DIR = src
INC_DIR = include
OBJS_DIR = obj
BIN_DIR = .
SRCS = $(SRC_DIR)/main.cpp $(SRC_DIR)/SampleTool.cpp $(SRC_DIR)/BuildFitInput.cpp $(INC_DIR)/BuildFitTools.h $(SRC_DIR)/JSONFactory.cpp
SRCS_CONDOR = $(SRC_DIR)/BFI_condor.cpp $(SRC_DIR)/SampleTool.cpp $(SRC_DIR)/BuildFitInput.cpp $(INC_DIR)/BuildFitTools.h $(SRC_DIR)/JSONFactory.cpp
CMSSWSRCS = $(SRC_DIR)/BFmain.cpp $(SRC_DIR)/BuildFit.cpp $(SRC_DIR)/JSONFactory.cpp $(INC_DIR)/BuildFitTools.h
SRCS_MERGE = $(SRC_DIR)/mergeJSONs.cpp $(SRC_DIR)/JSONFactory.cpp $(SRC_DIR)/SampleTool.cpp $(SRC_DIR)/BuildFitInput.cpp
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJS_DIR)/%.o,$(SRCS))
CMSSWOBJS=  $(patsubst $(SRC_DIR)/%.cpp,$(OBJS_DIR)/%.o,$(CMSSWSRCS))
CONDOROBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJS_DIR)/%.o,$(SRCS_CONDOR))
MERGEOBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJS_DIR)/%.o,$(SRCS_MERGE))

# Define the executable names
TARGET = $(BIN_DIR)/BFI.x
CMSSWTARGET = $(BIN_DIR)/BF.x
CONDORTARGET = $(BIN_DIR)/BFI_condor.x
MERGETARGET = $(BIN_DIR)/mergeJSONs.x

$(TARGET): $(OBJS_DIR) $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(ROOTCFLAGS)

# Default target: build the executable
all: $(TARGET) $(CMSSWTARGET) $(CONDORTARGET) $(MERGETARGET)
cmssw: $(CMSSWTARGET)
condor: $(CONDORTARGET)
merge: $(MERGETARGET)

$(CMSSWTARGET): $(OBJS_DIR) $(CMSSWOBJS) 
	$(CXX) $(CMSSWOBJS) $(LDFLAGS) $(ROOTCFLAGS) $(LIBPATH) $(LIBS) -o $@

$(CONDORTARGET): $(OBJS_DIR) $(CONDOROBJS)
	$(CXX) $(CONDOROBJS) -o $@ $(LDFLAGS) $(ROOTCFLAGS) $(LIBPATH)

$(MERGETARGET): $(OBJS_DIR) $(MERGEOBJS)
	$(CXX) $(MERGEOBJS) -o $@ $(LDFLAGS) $(ROOTCFLAGS) $(LIBPATH)

# Rule to compile C++ source files into object files
$(OBJS_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target: remove generated files
clean:
	rm -f ./obj/*.o *.x
