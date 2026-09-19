#include <vector>
#include <cctype>
#include <string>
#include <sstream>
#include <fstream>
#include <utility>

#include "preprocessor/preprocessor.hpp"

namespace smiles::preprocessor {

namespace {

constexpr const char* red_text = "\033[31m";
constexpr const char* reset_text = "\033[0m";

std::string make_skipped_token_marker(const std::size_t skipped_token_count) {
    return std::string("---SKIPPED ") + std::to_string(skipped_token_count) +
           (skipped_token_count == 1 ? " token---" : " tokens---");
}

void append_skipped_tokens(std::vector<SkippedTokens>& skipped_tokens,
                           const std::size_t start,
                           const std::size_t num_skipped) {
    if (num_skipped == 0) {
        return;
    }

    if (!skipped_tokens.empty() && skipped_tokens.back().start == start) {
        skipped_tokens.back().num_skipped += num_skipped;
        return;
    }

    skipped_tokens.push_back({start, num_skipped});
}

void append_pretty_print_skipped_tokens(std::vector<PrettyPrintSkippedTokens>& skipped_tokens,
                                        const std::size_t start,
                                        const std::size_t num_skipped,
                                        const bool standalone) {
    if (num_skipped == 0) {
        return;
    }

    if (!skipped_tokens.empty() && skipped_tokens.back().standalone == standalone && skipped_tokens.back().start == start) {
        skipped_tokens.back().num_skipped += num_skipped;
        return;
    }

    skipped_tokens.push_back({start, num_skipped, standalone});
}

}  // namespace

void PreprocessedFile::pretty_print(std::ostream& output_stream) const {
    output_stream << "Preprocessed file: " << navigator.get_file_name() << "\n";
    output_stream << "Skipped characters: " << red_text << skipped_character_count << reset_text << "\n";

    output_stream << "Include paths (" << include_paths.size() << "):\n";
    for (const auto& include_path : include_paths) {
        output_stream << "  - " << include_path << "\n";
    }

    output_stream << "Using namespaces (" << using_namespaces.size() << "):\n";
    for (const auto& using_namespace : using_namespaces) {
        output_stream << "  - " << using_namespace << "\n";
    }

    output_stream << "Output:\n";

    std::size_t output_position = 0;
    for (const auto& skipped_token : pretty_print_skipped_tokens) {
        output_stream << output.substr(output_position, skipped_token.start - output_position);
        output_stream << red_text << make_skipped_token_marker(skipped_token.num_skipped) << reset_text;
        if (skipped_token.standalone) {
            output_stream << "\n";
        }
        output_position = skipped_token.start;
    }

    output_stream << output.substr(output_position);
}

PreprocessedFile preprocess_file(const FileLikeObject& file_obj) {
    auto istrm = file_obj.read_stream();
    std::stringstream output_strm;
    std::vector<SkippedTokens> skipped_tokens;
    std::vector<PrettyPrintSkippedTokens> pretty_print_skipped_tokens;
    std::vector<std::string> include_paths;
    std::vector<std::string> using_namespaces;
    std::size_t skipped_character_count = 0;
    std::string line;
    std::size_t current_position = 0;
    std::size_t output_position = 0;
    std::size_t last_output_end_position = 0;
    bool has_output = false;
    const std::string import_marker = "#import ";
    const std::string using_marker = "#using ";
    while (std::getline(*istrm, line)) {
        const std::size_t line_start = current_position;
        const std::size_t delimiter_length = istrm->eof() ? 0u : 1u;

        if (line.rfind(import_marker, 0) == 0) {
            auto path = line.substr(import_marker.size());  // Extract the path after #import (including space)
            // TODO(vabi): trim whitespaces from path
            include_paths.push_back(path);
            append_skipped_tokens(skipped_tokens, output_position, line.size() + delimiter_length);
            skipped_character_count += line.size() + delimiter_length;
            append_pretty_print_skipped_tokens(pretty_print_skipped_tokens, output_position, line.size() + delimiter_length, true);
            current_position += line.size() + delimiter_length;
            continue;
        }

        if (line.rfind(using_marker, 0) == 0) {
            auto ns = line.substr(using_marker.size());  // Extract the namespace after #using (including space)
            // TODO(vabi): trim whitespaces from ns
            using_namespaces.push_back(ns);
            append_skipped_tokens(skipped_tokens, output_position, line.size() + delimiter_length);
            skipped_character_count += line.size() + delimiter_length;
            append_pretty_print_skipped_tokens(pretty_print_skipped_tokens, output_position, line.size() + delimiter_length, true);
            current_position += line.size() + delimiter_length;
            continue;
        }

        const std::size_t comment_position = line.find('#');
        if (comment_position != std::string::npos) {
            std::size_t skipped_prefix_start = comment_position;
            while (skipped_prefix_start > 0 && (line[skipped_prefix_start - 1] == ' ' || line[skipped_prefix_start - 1] == '\t')) {
                --skipped_prefix_start;
            }

            bool has_non_whitespace_prefix = false;
            for (std::size_t index = 0; index < skipped_prefix_start; ++index) {
                if (!std::isspace(static_cast<unsigned char>(line[index]))) {
                    has_non_whitespace_prefix = true;
                    break;
                }
            }

            if (!has_non_whitespace_prefix) {
                append_skipped_tokens(skipped_tokens, output_position, line.size() + delimiter_length);
                skipped_character_count += line.size() + delimiter_length;
                append_pretty_print_skipped_tokens(pretty_print_skipped_tokens, output_position, line.size() + delimiter_length, true);
            } else {
                append_skipped_tokens(skipped_tokens, output_position + skipped_prefix_start, line.size() - skipped_prefix_start);
                skipped_character_count += line.size() - skipped_prefix_start + delimiter_length;
                append_pretty_print_skipped_tokens(pretty_print_skipped_tokens,
                                                   output_position + skipped_prefix_start,
                                                   line.size() - skipped_prefix_start + delimiter_length,
                                                   false);
            }

            if (has_non_whitespace_prefix) {
                output_strm << line.substr(0, skipped_prefix_start);
                if (delimiter_length == 1) {
                    output_strm << "\n";
                }
                output_position += skipped_prefix_start + delimiter_length;
                has_output = true;
            }
            current_position += line.size() + delimiter_length;
            if (has_non_whitespace_prefix) {
                last_output_end_position = current_position;
            }
            continue;
        }

        current_position += line.size() + delimiter_length;
        if (line.empty()) {
            append_skipped_tokens(skipped_tokens, output_position, delimiter_length);
            skipped_character_count += delimiter_length;
            append_pretty_print_skipped_tokens(pretty_print_skipped_tokens, output_position, delimiter_length, true);
            continue;
        }

        output_strm << line;
        if (delimiter_length == 1) {
            output_strm << "\n";
        }
        output_position += line.size() + delimiter_length;
        has_output = true;
        last_output_end_position = current_position;
    }

    return PreprocessedFile(output_strm.str(),
                            std::move(include_paths),
                            std::move(using_namespaces),
                            PreprocessedFileNavigator(file_obj.get_name(), std::move(skipped_tokens)),
                            skipped_character_count,
                            std::move(pretty_print_skipped_tokens));
}

}  // namespace smiles::preprocessor
