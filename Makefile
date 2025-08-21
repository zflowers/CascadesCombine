# --- ROOT / Compiler settings ---
ROOTSYS = $(shell root-config --prefix)
CXX = g++
CXXFLAGS = -g -Wall -fPIC $(shell root-config --cflags)
CXXFLAGS += -I../../src -I./include/
LDFLAGS = $(shell root-config --glibs)
ROOTCFLAGS  = $(shell root-config --cflags)
LIBS = -lCombineHarvesterCombineTools
LIBPATH = -L../../lib/el9_amd64_gcc12/

# --- Directories ---
SRC_DIR = src
INC_DIR = include
OBJS_DIR = obj
BIN_DIR = .
LIB_DIR = libs

# --- pybind11 Python module ---
PYBIND_INCLUDES = $(shell python3 -m pybind11 --includes)
PYBIND_TARGET = $(LIB_DIR)/pySampleTool$(shell python3-config --extension-suffix)
PYBIND_CXXFLAGS = $(CXXFLAGS) $(PYBIND_INCLUDES)

# --- Source files ---
SRCS = $(SRC_DIR)/main.cpp $(SRC_DIR)/SampleTool.cpp $(SRC_DIR)/BuildFitInput.cpp $(SRC_DIR)/JSONFactory.cpp
SRCS_CONDOR = $(SRC_DIR)/BFI_condor.cpp $(SRC_DIR)/BuildFitInput.cpp $(SRC_DIR)/JSONFactory.cpp
CMSSWSRCS = $(SRC_DIR)/BFmain.cpp $(SRC_DIR)/BuildFit.cpp $(SRC_DIR)/JSONFactory.cpp
SRCS_MERGE = $(SRC_DIR)/mergeJSONs.cpp $(SRC_DIR)/JSONFactory.cpp $(SRC_DIR)/SampleTool.cpp $(SRC_DIR)/BuildFitInput.cpp
SRCS_FLATTEN = $(SRC_DIR)/flattenJSONs.cpp
PYBIND_SRCS = $(SRC_DIR)/pySampleTool.cpp $(SRC_DIR)/SampleTool.cpp

# --- Object files ---
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJS_DIR)/%.o,$(SRCS))
CONDOROBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJS_DIR)/%.o,$(SRCS_CONDOR))
CMSSWOBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJS_DIR)/%.o,$(CMSSWSRCS))
MERGEOBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJS_DIR)/%.o,$(SRCS_MERGE))
FLATTENOBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJS_DIR)/%.o,$(SRCS_FLATTEN))
PYBIND_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJS_DIR)/%.o,$(PYBIND_SRCS))

# --- Executables ---
TARGET = $(BIN_DIR)/BFI.x
CMSSWTARGET = $(BIN_DIR)/BF.x
CONDORTARGET = $(BIN_DIR)/BFI_condor.x
MERGETARGET = $(BIN_DIR)/mergeJSONs.x
FLATTENTARGET = $(BIN_DIR)/flattenJSONs.x

# --- Default target ---
all: $(TARGET) $(CMSSWTARGET) $(CONDORTARGET) $(MERGETARGET) $(FLATTENTARGET) $(PYBIND_TARGET)

# --- Executable targets ---
$(TARGET): $(OBJS_DIR) $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(ROOTCFLAGS)

$(CMSSWTARGET): $(OBJS_DIR) $(CMSSWOBJS)
	$(CXX) $(CMSSWOBJS) -o $@ $(LDFLAGS) $(ROOTCFLAGS) $(LIBPATH) $(LIBS)

$(CONDORTARGET): $(OBJS_DIR) $(CONDOROBJS)
	$(CXX) $(CONDOROBJS) -o $@ $(LDFLAGS) $(ROOTCFLAGS) $(LIBPATH)

$(MERGETARGET): $(OBJS_DIR) $(MERGEOBJS)
	$(CXX) $(MERGEOBJS) -o $@ $(LDFLAGS) $(ROOTCFLAGS) $(LIBPATH)

$(FLATTENTARGET): $(OBJS_DIR) $(FLATTENOBJS)
	$(CXX) $(FLATTENOBJS) -o $@ $(LDFLAGS) $(ROOTCFLAGS) $(LIBPATH)

$(PYBIND_TARGET): $(OBJS_DIR) $(PYBIND_OBJS) | $(LIB_DIR)
	$(CXX) -shared -std=c++17 -fPIC $(PYBIND_OBJS) -o $@ $(PYBIND_INCLUDES) $(LDFLAGS) $(ROOTCFLAGS)

# --- Pattern rules for compiling object files ---
$(OBJS_DIR)/pySampleTool.o: $(SRC_DIR)/pySampleTool.cpp | $(OBJS_DIR)
	$(CXX) $(CXXFLAGS) $(PYBIND_INCLUDES) -c $< -o $@

$(OBJS_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJS_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ensure obj and libs directories exist
$(OBJS_DIR):
	mkdir -p $(OBJS_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

# --- Header dependency tracking ---
-include $(OBJS:.o=.d)
-include $(PYBIND_OBJS:.o=.d)

# --- Clean ---
clean:
	rm -rf $(OBJS_DIR)/*.o $(OBJS_DIR)/*.d *.x $(LIB_DIR)/*.so
