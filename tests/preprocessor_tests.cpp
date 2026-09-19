#include <gtest/gtest.h>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>



#include "preprocessor/preprocessor.hpp"

namespace {

using preprocessor::FileObject;
using preprocessor::PreprocessedFileNavigator;
using preprocessor::ReplInputObject;
using preprocessor::SkippedTokens;
using preprocessor::preprocess_file;

std::string get_temp_test_path(const std::string& filename) {
    const auto directory = std::filesystem::temp_directory_path() / "smiles_tests";
    std::filesystem::create_directories(directory);
    return (directory / filename).string();
}

TEST(FileObjectTest, ReadReturnsWholeFileContents) {
    const std::string path = get_temp_test_path("preprocessor_file_object_read.txt");
    {
        std::ofstream file(path);
        ASSERT_TRUE(file.is_open());
        file << "first line\nsecond line\n";
    }

    FileObject file_object(path);

    EXPECT_EQ(file_object.get_name(), path);
    EXPECT_EQ(file_object.read(), "first line\nsecond line\n");
}

TEST(FileObjectTest, ReadStreamReturnsFreshInputStream) {
    const std::string path = get_temp_test_path("preprocessor_file_object_stream.txt");
    {
        std::ofstream file(path);
        ASSERT_TRUE(file.is_open());
        file << "alpha\nbeta\n";
    }

    FileObject file_object(path);

    auto stream = file_object.read_stream();
    std::string first_line;
    std::string second_line;
    ASSERT_TRUE(std::getline(*stream, first_line));
    ASSERT_TRUE(std::getline(*stream, second_line));
    EXPECT_EQ(first_line, "alpha");
    EXPECT_EQ(second_line, "beta");
}

TEST(FileObjectTest, MissingFileThrowsForReadAndStream) {
    FileObject file_object(get_temp_test_path("does_not_exist.smiles"));

    EXPECT_THROW(static_cast<void>(file_object.read()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(file_object.read_stream()), std::runtime_error);
}

TEST(ReplInputObjectTest, ExposesStoredInputAndName) {
    ReplInputObject input_object("hello\nworld", "repl");

    EXPECT_EQ(input_object.get_name(), "repl");
    EXPECT_EQ(input_object.read(), "hello\nworld");
}

TEST(ReplInputObjectTest, ReadStreamReturnsReadableInputStream) {
    ReplInputObject input_object("alpha\nbeta", "repl");

    auto stream = input_object.read_stream();
    std::string first_line;
    std::string second_line;
    ASSERT_TRUE(std::getline(*stream, first_line));
    ASSERT_TRUE(std::getline(*stream, second_line));
    EXPECT_EQ(first_line, "alpha");
    EXPECT_EQ(second_line, "beta");
}

TEST(PreprocessFileTest, ExtractsImportsUsingsAndComments) {
    auto input = std::make_shared<ReplInputObject>(
        "#import std/math\n"
        "#using std::math\n"
        "value = 42 # keep code remove comment\n"
        "plain line\n"
        "#full line comment\n",
        "snippet");
    std::string output;
    std::vector<std::string> include_paths;
    std::vector<std::string> using_namespaces;

    const auto skipped_tokens = preprocess_file(input, output, include_paths, using_namespaces);

    EXPECT_EQ(output,
              "value = 42 \n"
              "plain line\n");
    EXPECT_EQ(include_paths, std::vector<std::string>({"std/math"}));
    EXPECT_EQ(using_namespaces, std::vector<std::string>({"std::math"}));
    ASSERT_EQ(skipped_tokens.size(), 3u);
    EXPECT_EQ(skipped_tokens[0].start, 0u);
    EXPECT_EQ(skipped_tokens[0].num_skipped, 16u);
    EXPECT_EQ(skipped_tokens[1].start, 17u);
    EXPECT_EQ(skipped_tokens[1].num_skipped, 16u);
    EXPECT_EQ(skipped_tokens[2].start, 45u);
    EXPECT_EQ(skipped_tokens[2].num_skipped, 26u);
}

TEST(PreprocessFileTest, LeavesOutputEmptyForLineWithoutTextBeforeComment) {
    auto input = std::make_shared<ReplInputObject>("#comment only\n", "snippet");
    std::string output;
    std::vector<std::string> include_paths;
    std::vector<std::string> using_namespaces;

    const auto skipped_tokens = preprocess_file(input, output, include_paths, using_namespaces);

    EXPECT_TRUE(output.empty());
    EXPECT_TRUE(include_paths.empty());
    EXPECT_TRUE(using_namespaces.empty());
    ASSERT_EQ(skipped_tokens.size(), 1u);
    EXPECT_EQ(skipped_tokens[0].start, 0u);
    EXPECT_EQ(skipped_tokens[0].num_skipped, 13u);
}

TEST(PreprocessFileTest, PreservesWhitespaceInDirectives) {
    auto input = std::make_shared<ReplInputObject>(
        "#import   lib/core  \n"
        "#using   ns::inner  \n",
        "snippet");
    std::string output;
    std::vector<std::string> include_paths;
    std::vector<std::string> using_namespaces;

    const auto skipped_tokens = preprocess_file(input, output, include_paths, using_namespaces);

    EXPECT_TRUE(output.empty());
    EXPECT_EQ(include_paths, std::vector<std::string>({"  lib/core  "}));
    EXPECT_EQ(using_namespaces, std::vector<std::string>({"  ns::inner  "}));
    ASSERT_EQ(skipped_tokens.size(), 2u);
}

TEST(PreprocessedFileNavigatorTest, ReturnsFileNameAndMapsSkippedRanges) {
    PreprocessedFileNavigator navigator("sample.smiles", {{3, 2}, {8, 4}});

    EXPECT_EQ(navigator.get_file_name(), "sample.smiles");
    EXPECT_EQ(navigator.get_original_position(0), 0u);
    EXPECT_EQ(navigator.get_original_position(2), 2u);
    EXPECT_EQ(navigator.get_original_position(3), 5u);
    EXPECT_EQ(navigator.get_original_position(5), 7u);
    EXPECT_EQ(navigator.get_original_position(6), 8u);
    EXPECT_EQ(navigator.get_original_position(8), 14u);
    EXPECT_EQ(navigator.get_original_position(12), 18u);
}

TEST(PreprocessedFileNavigatorTest, RejectsUnsortedSkippedRanges) {
    EXPECT_THROW(static_cast<void>(PreprocessedFileNavigator("sample.smiles", {{5, 1}, {4, 2}})), std::invalid_argument);
}

}  // namespace
