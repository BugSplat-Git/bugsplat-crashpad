#ifndef PATHS_H
#define PATHS_H

#include <string>

/**
 * Gets the directory containing the current executable.
 * Cross-platform function that returns the absolute path to the directory
 * where the current executable is located.
 * 
 * @return String containing the executable directory path, or empty string on failure
 */
std::string getExecutableDir();

#endif // PATHS_H
