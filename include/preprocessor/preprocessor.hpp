#pragma once
#include <string>
#include <cstddef>
#include <memory>
#include <vector>
#include <utility>
#include <fstream>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <sstream>
#include "smiles/options.hpp"

namespace smiles::preprocessor {

class FileLikeObject;

class InputGenerator {
 public:
    virtual ~InputGenerator() = default;
    virtual std::unique_ptr<FileLikeObject> get_next() = 0;
};

std::unique_ptr<InputGenerator> get_input_generator(const smiles::options::CliOptions& options);

/**
 * Common interface for objects that expose file-like input to the preprocessor.
 */
class FileLikeObject {
 public:
    virtual ~FileLikeObject() = default;

    /**
     * Reads the full contents into a string.
     */
    virtual std::string read() = 0;

    /**
     * Returns the logical name associated with the input.
     */
    virtual std::string get_name() const = 0;

    /**
     * Returns the full contents as a newly created input stream.
     */
    virtual std::unique_ptr<std::istream> read_stream() const = 0;
};

/**
 * File-backed implementation of `FileLikeObject`.
 */
class FileObject : public FileLikeObject {
    std::string filename;
    std::ifstream get_input_stream() const {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        return file;
    }

 public:
    /**
     * Creates a file-backed object for the given path.
     */
    FileObject(std::string name) : filename(std::move(name)) {}

    std::string read() override {
        auto file = get_input_stream();
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        return content;
    }

    std::string get_name() const override {
        return filename;
    }

    std::unique_ptr<std::istream> read_stream() const override {
        auto stream = std::make_unique<std::ifstream>(filename);
        if (!stream->is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        return stream;
    }
};

/**
 * In-memory implementation of `FileLikeObject`, useful for REPL or tests.
 */
class ReplInputObject : public FileLikeObject {
    std::string input;
    std::string filename;

 public:
    /**
     * Creates an input object from the provided contents and optional name.
     */
    ReplInputObject(std::string input, std::string name = "") : input(std::move(input)), filename(std::move(name)) {}

    std::string read() override {
        return input;
    }

    std::string get_name() const override {
        return filename;
    }

    std::unique_ptr<std::istream> read_stream() const override {
        return std::make_unique<std::istringstream>(input);
    }
};


/**
 * Describes a contiguous range of tokens removed during preprocessing.
 */
struct SkippedTokens {
    std::size_t start;
    std::size_t num_skipped;
};

struct PrettyPrintSkippedTokens {
    std::size_t start;
    std::size_t num_skipped;
    bool standalone;
};

/**
 * Maps positions in preprocessed output back to positions in the original input.
 */
class PreprocessedFileNavigator {
    std::string file_name;
    std::vector<SkippedTokens> skipped_tokens;

 public:
    /**
     * Creates a navigator for a file using skipped token ranges sorted by start.
     */
    PreprocessedFileNavigator(std::string file_name, std::vector<SkippedTokens>&& skipped_tokens)
            : file_name(file_name), skipped_tokens(std::move(skipped_tokens)) {
        for (std::vector<SkippedTokens>::size_type index = 1; index < this->skipped_tokens.size(); ++index) {
            if (this->skipped_tokens[index - 1].start > this->skipped_tokens[index].start) {
                throw std::invalid_argument("Skipped tokens must be sorted by start position");
            }
        }
    }

    /**
     * Returns the original file name associated with this mapping.
     */
    std::string get_file_name() const {
        return file_name;
    }

    const std::vector<SkippedTokens>& get_skipped_tokens() const {
        return skipped_tokens;
    }

    /**
     * Converts a position in preprocessed output to its original input position.
     */
    std::size_t get_original_position(const std::size_t preprocessed_position) const {
        std::size_t original_position = preprocessed_position;
        for (const auto& skipped : skipped_tokens) {
            if (skipped.start > preprocessed_position) {
                break;
            }
            original_position += skipped.num_skipped;
        }
        return original_position;
    }
};

class PreprocessedFile {
    std::string output;
    std::vector<std::string> include_paths;
    std::vector<std::string> using_namespaces;
    PreprocessedFileNavigator navigator;
    std::size_t skipped_character_count;
    std::vector<PrettyPrintSkippedTokens> pretty_print_skipped_tokens;

 public:
    PreprocessedFile(std::string output,
                     std::vector<std::string>&& include_paths,
                     std::vector<std::string>&& using_namespaces,
                     PreprocessedFileNavigator&& navigator,
                     const std::size_t skipped_character_count,
                     std::vector<PrettyPrintSkippedTokens>&& pretty_print_skipped_tokens)
            : output(std::move(output)),
              include_paths(std::move(include_paths)),
              using_namespaces(std::move(using_namespaces)),
              navigator(std::move(navigator)),
              skipped_character_count(skipped_character_count),
              pretty_print_skipped_tokens(std::move(pretty_print_skipped_tokens)) {}

    const std::string& get_output() const {
        return output;
    }

    const std::vector<std::string>& get_include_paths() const {
        return include_paths;
    }

    const std::vector<std::string>& get_using_namespaces() const {
        return using_namespaces;
    }

    const PreprocessedFileNavigator& get_navigator() const {
        return navigator;
    }

    std::size_t get_skipped_character_count() const {
        return skipped_character_count;
    }

    void pretty_print(std::ostream& output_stream) const;
};

PreprocessedFile preprocess_file(const FileLikeObject& file_obj);

}  // namespace smiles::preprocessor
