# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/hp/Desktop/DShowMPEGParse/my/Tests/my_buffer_test

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/hp/Desktop/DShowMPEGParse/my/Tests/build-my_buffer_test-gcc_32-Release

# Include any dependencies generated for this target.
include CMakeFiles/my_buffer_test.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/my_buffer_test.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/my_buffer_test.dir/flags.make

CMakeFiles/my_buffer_test.dir/main.cpp.o: CMakeFiles/my_buffer_test.dir/flags.make
CMakeFiles/my_buffer_test.dir/main.cpp.o: /home/hp/Desktop/DShowMPEGParse/my/Tests/my_buffer_test/main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hp/Desktop/DShowMPEGParse/my/Tests/build-my_buffer_test-gcc_32-Release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/my_buffer_test.dir/main.cpp.o"
	/usr/bin/x86_64-linux-gnu-g++-7  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/my_buffer_test.dir/main.cpp.o -c /home/hp/Desktop/DShowMPEGParse/my/Tests/my_buffer_test/main.cpp

CMakeFiles/my_buffer_test.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/my_buffer_test.dir/main.cpp.i"
	/usr/bin/x86_64-linux-gnu-g++-7 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/hp/Desktop/DShowMPEGParse/my/Tests/my_buffer_test/main.cpp > CMakeFiles/my_buffer_test.dir/main.cpp.i

CMakeFiles/my_buffer_test.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/my_buffer_test.dir/main.cpp.s"
	/usr/bin/x86_64-linux-gnu-g++-7 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/hp/Desktop/DShowMPEGParse/my/Tests/my_buffer_test/main.cpp -o CMakeFiles/my_buffer_test.dir/main.cpp.s

CMakeFiles/my_buffer_test.dir/main.cpp.o.requires:

.PHONY : CMakeFiles/my_buffer_test.dir/main.cpp.o.requires

CMakeFiles/my_buffer_test.dir/main.cpp.o.provides: CMakeFiles/my_buffer_test.dir/main.cpp.o.requires
	$(MAKE) -f CMakeFiles/my_buffer_test.dir/build.make CMakeFiles/my_buffer_test.dir/main.cpp.o.provides.build
.PHONY : CMakeFiles/my_buffer_test.dir/main.cpp.o.provides

CMakeFiles/my_buffer_test.dir/main.cpp.o.provides.build: CMakeFiles/my_buffer_test.dir/main.cpp.o


# Object files for target my_buffer_test
my_buffer_test_OBJECTS = \
"CMakeFiles/my_buffer_test.dir/main.cpp.o"

# External object files for target my_buffer_test
my_buffer_test_EXTERNAL_OBJECTS =

my_buffer_test: CMakeFiles/my_buffer_test.dir/main.cpp.o
my_buffer_test: CMakeFiles/my_buffer_test.dir/build.make
my_buffer_test: CMakeFiles/my_buffer_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/hp/Desktop/DShowMPEGParse/my/Tests/build-my_buffer_test-gcc_32-Release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable my_buffer_test"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/my_buffer_test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/my_buffer_test.dir/build: my_buffer_test

.PHONY : CMakeFiles/my_buffer_test.dir/build

CMakeFiles/my_buffer_test.dir/requires: CMakeFiles/my_buffer_test.dir/main.cpp.o.requires

.PHONY : CMakeFiles/my_buffer_test.dir/requires

CMakeFiles/my_buffer_test.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/my_buffer_test.dir/cmake_clean.cmake
.PHONY : CMakeFiles/my_buffer_test.dir/clean

CMakeFiles/my_buffer_test.dir/depend:
	cd /home/hp/Desktop/DShowMPEGParse/my/Tests/build-my_buffer_test-gcc_32-Release && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/hp/Desktop/DShowMPEGParse/my/Tests/my_buffer_test /home/hp/Desktop/DShowMPEGParse/my/Tests/my_buffer_test /home/hp/Desktop/DShowMPEGParse/my/Tests/build-my_buffer_test-gcc_32-Release /home/hp/Desktop/DShowMPEGParse/my/Tests/build-my_buffer_test-gcc_32-Release /home/hp/Desktop/DShowMPEGParse/my/Tests/build-my_buffer_test-gcc_32-Release/CMakeFiles/my_buffer_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/my_buffer_test.dir/depend

