# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

# Default target executed when no arguments are given to make.
default_target: all
.PHONY : default_target

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
CMAKE_SOURCE_DIR = /home/dbaranchuk/hnsw

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/dbaranchuk/hnsw

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running interactive CMake command-line interface..."
	/usr/bin/cmake -i .
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache
.PHONY : edit_cache/fast

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/bin/cmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache
.PHONY : rebuild_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/dbaranchuk/hnsw/CMakeFiles /home/dbaranchuk/hnsw/CMakeFiles/progress.marks
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/dbaranchuk/hnsw/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean
.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named deep_test

# Build rule for target.
deep_test: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 deep_test
.PHONY : deep_test

# fast build rule for target.
deep_test/fast:
	$(MAKE) -f CMakeFiles/deep_test.dir/build.make CMakeFiles/deep_test.dir/build
.PHONY : deep_test/fast

#=============================================================================
# Target rules for targets named main

# Build rule for target.
main: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 main
.PHONY : main

# fast build rule for target.
main/fast:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/build
.PHONY : main/fast

#=============================================================================
# Target rules for targets named sift_test

# Build rule for target.
sift_test: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 sift_test
.PHONY : sift_test

# fast build rule for target.
sift_test/fast:
	$(MAKE) -f CMakeFiles/sift_test.dir/build.make CMakeFiles/sift_test.dir/build
.PHONY : sift_test/fast

deep_10m.o: deep_10m.cpp.o
.PHONY : deep_10m.o

# target to build an object file
deep_10m.cpp.o:
	$(MAKE) -f CMakeFiles/deep_test.dir/build.make CMakeFiles/deep_test.dir/deep_10m.cpp.o
.PHONY : deep_10m.cpp.o

deep_10m.i: deep_10m.cpp.i
.PHONY : deep_10m.i

# target to preprocess a source file
deep_10m.cpp.i:
	$(MAKE) -f CMakeFiles/deep_test.dir/build.make CMakeFiles/deep_test.dir/deep_10m.cpp.i
.PHONY : deep_10m.cpp.i

deep_10m.s: deep_10m.cpp.s
.PHONY : deep_10m.s

# target to generate assembly for a file
deep_10m.cpp.s:
	$(MAKE) -f CMakeFiles/deep_test.dir/build.make CMakeFiles/deep_test.dir/deep_10m.cpp.s
.PHONY : deep_10m.cpp.s

main.o: main.cpp.o
.PHONY : main.o

# target to build an object file
main.cpp.o:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/main.cpp.o
.PHONY : main.cpp.o

main.i: main.cpp.i
.PHONY : main.i

# target to preprocess a source file
main.cpp.i:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/main.cpp.i
.PHONY : main.cpp.i

main.s: main.cpp.s
.PHONY : main.s

# target to generate assembly for a file
main.cpp.s:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/main.cpp.s
.PHONY : main.cpp.s

sift_1b.o: sift_1b.cpp.o
.PHONY : sift_1b.o

# target to build an object file
sift_1b.cpp.o:
	$(MAKE) -f CMakeFiles/sift_test.dir/build.make CMakeFiles/sift_test.dir/sift_1b.cpp.o
.PHONY : sift_1b.cpp.o

sift_1b.i: sift_1b.cpp.i
.PHONY : sift_1b.i

# target to preprocess a source file
sift_1b.cpp.i:
	$(MAKE) -f CMakeFiles/sift_test.dir/build.make CMakeFiles/sift_test.dir/sift_1b.cpp.i
.PHONY : sift_1b.cpp.i

sift_1b.s: sift_1b.cpp.s
.PHONY : sift_1b.s

# target to generate assembly for a file
sift_1b.cpp.s:
	$(MAKE) -f CMakeFiles/sift_test.dir/build.make CMakeFiles/sift_test.dir/sift_1b.cpp.s
.PHONY : sift_1b.cpp.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... deep_test"
	@echo "... edit_cache"
	@echo "... main"
	@echo "... rebuild_cache"
	@echo "... sift_test"
	@echo "... deep_10m.o"
	@echo "... deep_10m.i"
	@echo "... deep_10m.s"
	@echo "... main.o"
	@echo "... main.i"
	@echo "... main.s"
	@echo "... sift_1b.o"
	@echo "... sift_1b.i"
	@echo "... sift_1b.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

