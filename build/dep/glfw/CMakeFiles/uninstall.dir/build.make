# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

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
CMAKE_SOURCE_DIR = /home/fern/Desktop/mc_modding/pr_website

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/fern/Desktop/mc_modding/pr_website/build

# Utility rule file for uninstall.

# Include the progress variables for this target.
include dep/glfw/CMakeFiles/uninstall.dir/progress.make

dep/glfw/CMakeFiles/uninstall:
	cd /home/fern/Desktop/mc_modding/pr_website/build/dep/glfw && /usr/bin/cmake -P /home/fern/Desktop/mc_modding/pr_website/build/dep/glfw/cmake_uninstall.cmake

uninstall: dep/glfw/CMakeFiles/uninstall
uninstall: dep/glfw/CMakeFiles/uninstall.dir/build.make

.PHONY : uninstall

# Rule to build all files generated by this target.
dep/glfw/CMakeFiles/uninstall.dir/build: uninstall

.PHONY : dep/glfw/CMakeFiles/uninstall.dir/build

dep/glfw/CMakeFiles/uninstall.dir/clean:
	cd /home/fern/Desktop/mc_modding/pr_website/build/dep/glfw && $(CMAKE_COMMAND) -P CMakeFiles/uninstall.dir/cmake_clean.cmake
.PHONY : dep/glfw/CMakeFiles/uninstall.dir/clean

dep/glfw/CMakeFiles/uninstall.dir/depend:
	cd /home/fern/Desktop/mc_modding/pr_website/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/fern/Desktop/mc_modding/pr_website /home/fern/Desktop/mc_modding/pr_website/dep/glfw /home/fern/Desktop/mc_modding/pr_website/build /home/fern/Desktop/mc_modding/pr_website/build/dep/glfw /home/fern/Desktop/mc_modding/pr_website/build/dep/glfw/CMakeFiles/uninstall.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : dep/glfw/CMakeFiles/uninstall.dir/depend

