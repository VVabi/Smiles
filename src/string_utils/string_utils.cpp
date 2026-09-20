/**
 * @file string_utils.cpp
 * @author vabi
 * @date Feb 8, 2024
 * @brief This file contains utility functions for string manipulation.
 */

#include <vector>
#include <string>
#include <sstream>

/**
 * @brief Splits a string into a vector of substrings based on a specified delimiter.
 * 
 * This function takes a string and a character as input. It splits the input string 
 * into substrings wherever the delimiter character appears, and returns a vector of 
 * these substrings. If the delimiter does not appear in the string, the function 
 * returns a vector containing the original string as its only element.
 *
 * @param in The string to be split.
 * @param delimiter The character to split the string on.
 * @return A vector of substrings resulting from the split.
 */
std::vector<std::string> string_split(const std::string& in, const char delimiter) {
    std::vector<std::string> strings;
    std::istringstream f(in);
    std::string s;
    while (std::getline(f, s, delimiter)) {
        strings.push_back(s);
    }

    return strings;
}

