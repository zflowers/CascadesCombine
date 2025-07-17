# Define the ROOTSYS environment variable if not already set
# This points to your ROOT installation directory
ROOTSYS = $(shell root-config --prefix)

# Define the C++ compiler
CXX = g++

# Define compiler flags
# -g for debugging, -Wall for warnings, -fPIC for position-independent code (often needed for shared libraries)
# $(shell root-config --cflags) gets ROOT-specific compiler flags
CXXFLAGS = -g -Wall -fPIC $(shell root-config --cflags)

# Define linker flags
# $(shell root-config --glibs) gets ROOT-specific linker flags (libraries)
LDFLAGS = $(shell root-config --glibs)

# Define the executable name
TARGET = BFI.x
CMSSWTARGET = BF.x
# Define source files
SRCS = main.cpp SampleTool.cpp BuildFitInput.cpp BuildFitTools.h JSONFactory.cpp
CMSSWSRCS= BFmain.cpp BuildFit.cpp JSONFactory.cpp BuildFitTools.h

# Define object files
OBJS = $(SRCS:.cpp=.o)
CMSSWOBJS = $(CMSSWSRCS:.cpp=.o)

# Default target: build the executable
all: $(TARGET)
cmssw: $(CMSSWTARGET)

# Rule to build the executable from object files
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)
$(CMSSWTARGET): $
	$(CXX) $(CMSSWOBJS) -o $@ $(LDFLAGS)

# Rule to compile C++ source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target: remove generated files, this deleted my headers!
clean:
	rm *.o
	rm myprogram
	rm BFI.x
	rm BF.x
