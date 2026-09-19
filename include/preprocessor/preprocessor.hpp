#pragma once
#include <string>
#include <cstddef>
#include <memory>
#include <vector>
#include <utility>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <sstream>

namespace preprocessor {

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

std::vector<SkippedTokens> preprocess_file(const std::shared_ptr<FileLikeObject>& file_obj,
                                           std::string& output,
                                           std::vector<std::string>& include_paths,
                                           std::vector<std::string>& using_namespaces);

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

}  // namespace preprocessor
