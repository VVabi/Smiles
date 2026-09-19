#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include "preprocessor/preprocessor.hpp"

namespace preprocessor {

std::vector<SkippedTokens> preprocess_file(const std::shared_ptr<FileLikeObject>& file_obj,
                                            std::string& output,
                                            std::vector<std::string>& include_paths,
                                            std::vector<std::string>& using_namespaces) {
    auto istrm = file_obj->read_stream();
    std::stringstream output_strm;
    std::vector<SkippedTokens> skipped_tokens;
    std::string line;
    std::size_t current_position = 0;
    std::size_t last_output_end_position = 0;
    bool has_output = false;
    const std::string import_marker = "#import ";
    const std::string using_marker = "#using ";
    while (std::getline(*istrm, line)) {
        if (line.rfind(import_marker, 0) == 0) {
            auto path = line.substr(import_marker.size());  // Extract the path after #import (including space)
            // TODO(vabi): trim whitespaces from path
            include_paths.push_back(path);
            skipped_tokens.push_back({current_position, line.size()});
            current_position += line.size() + 1;  // +1 for the newline character
            continue;
        }

        if (line.rfind(using_marker, 0) == 0) {
            auto ns = line.substr(using_marker.size());  // Extract the namespace after #using (including space)
            // TODO(vabi): trim whitespaces from ns
            using_namespaces.push_back(ns);
            skipped_tokens.push_back({current_position, line.size()});
            current_position += line.size() + 1;  // +1 for the newline character
            continue;
        }

        const std::size_t comment_position = line.find('#');
        if (comment_position != std::string::npos) {
            skipped_tokens.push_back({current_position + comment_position, line.size() - comment_position});
            if (comment_position > 0) {
                output_strm << line.substr(0, comment_position) << "\n";
                has_output = true;
            }
            current_position += line.size() + 1;  // +1 for the newline character
            if (comment_position > 0) {
                last_output_end_position = current_position;
            }
            continue;
        }

        current_position += line.size() + 1;  // +1 for the newline character
        if (line.empty()) {
            // Empty line, just add a newline character
            output_strm << "\n";
            has_output = true;
            last_output_end_position = current_position;
            continue;
        }

        output_strm << line << "\n";
        has_output = true;
        last_output_end_position = current_position;
    }
    output = output_strm.str();
    while (has_output && !skipped_tokens.empty() && skipped_tokens.back().start >= last_output_end_position) {
        skipped_tokens.pop_back();
    }
    return skipped_tokens;
}

}  // namespace preprocessor
