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
TARGET = myprogram

# Define source files
SRCS = main.cpp SampleTool.cpp BuildFitInput.cpp BuildFitTools.h JSONFactory.h

# Define object files
OBJS = $(SRCS:.cpp=.o)

# Default target: build the executable
all: $(TARGET)

# Rule to build the executable from object files
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Rule to compile C++ source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target: remove generated files, this deleted my headers!
clean:
	rm *.o
	#rm -f $(OBJS) $(TARGET)
