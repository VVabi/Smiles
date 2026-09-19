#include <gtest/gtest.h>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>



#include "preprocessor/preprocessor.hpp"

namespace {

using smiles::preprocessor::FileObject;
using smiles::preprocessor::PreprocessedFileNavigator;
using smiles::preprocessor::ReplInputObject;
using smiles::preprocessor::SkippedTokens;
using smiles::preprocessor::preprocess_file;

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
    ReplInputObject input(
        "#import std/math\n"
        "#using std::math\n"
        "value = 42 # keep code remove comment\n"
        "plain line\n"
        "#full line comment\n",
        "snippet");

    const auto result = preprocess_file(input);

    EXPECT_EQ(result.get_output(),
              "value = 42\n"
              "plain line\n");
    EXPECT_EQ(result.get_include_paths(), std::vector<std::string>({"std/math"}));
    EXPECT_EQ(result.get_using_namespaces(), std::vector<std::string>({"std::math"}));
    EXPECT_EQ(result.get_skipped_character_count(), 81u);
    const auto& navigator = result.get_navigator();
    EXPECT_EQ(navigator.get_file_name(), "snippet");
    EXPECT_EQ(navigator.get_original_position(0), 34u);
    EXPECT_EQ(navigator.get_original_position(17), 78u);
    const auto& skipped_tokens = navigator.get_skipped_tokens();
    ASSERT_EQ(skipped_tokens.size(), 3u);
    EXPECT_EQ(skipped_tokens[0].start, 0u);
    EXPECT_EQ(skipped_tokens[0].num_skipped, 34u);
    EXPECT_EQ(skipped_tokens[1].start, 10u);
    EXPECT_EQ(skipped_tokens[1].num_skipped, 27u);
    EXPECT_EQ(skipped_tokens[2].start, 22u);
    EXPECT_EQ(skipped_tokens[2].num_skipped, 19u);
}

TEST(PreprocessedFileTest, PrettyPrintIncludesMetadataOutputAndSkippedCount) {
    ReplInputObject input(
        "#import std/math\n"
        "#using std::math\n"
        "value = 42 # keep code remove comment\n"
        "plain line\n",
        "snippet");

    const auto result = preprocess_file(input);
    std::ostringstream output_stream;

    result.pretty_print(output_stream);

    EXPECT_EQ(output_stream.str(),
              "Preprocessed file: snippet\n"
              "Skipped characters: \033[31m62\033[0m\n"
              "Include paths (1):\n"
              "  - std/math\n"
              "Using namespaces (1):\n"
              "  - std::math\n"
              "Output:\n"
              "\033[31m---SKIPPED 34 tokens---\033[0m\n"
              "value = 42\033[31m---SKIPPED 28 tokens---\033[0m\n"
              "plain line\n");
}

TEST(PreprocessedFileTest, PrettyPrintShowsSkippedTokensInsideOutput) {
    ReplInputObject input(
        "foo bar #123\n"
        "foobar",
        "snippet");

    const auto result = preprocess_file(input);
    std::ostringstream output_stream;

    result.pretty_print(output_stream);

    EXPECT_EQ(output_stream.str(),
              "Preprocessed file: snippet\n"
              "Skipped characters: \033[31m6\033[0m\n"
              "Include paths (0):\n"
              "Using namespaces (0):\n"
              "Output:\n"
              "foo bar\033[31m---SKIPPED 6 tokens---\033[0m\n"
              "foobar\n");
}

TEST(PreprocessedFileTest, SkipsWhitespaceBeforeCommentMarker) {
    ReplInputObject input(
        "keep this   # remove this\n"
        "\t   # full line comment\n",
        "snippet");

    const auto result = preprocess_file(input);
    std::ostringstream output_stream;

    EXPECT_EQ(result.get_output(), "keep this\n");
    EXPECT_EQ(result.get_skipped_character_count(), 41u);

    result.pretty_print(output_stream);

    EXPECT_EQ(output_stream.str(),
              "Preprocessed file: snippet\n"
              "Skipped characters: \033[31m41\033[0m\n"
              "Include paths (0):\n"
              "Using namespaces (0):\n"
              "Output:\n"
              "keep this\033[31m---SKIPPED 17 tokens---\033[0m\n"
              "\033[31m---SKIPPED 24 tokens---\033[0m\n");
}

TEST(PreprocessedFileNavigatorTest, AppliesLaterInlineCommentAfterEarlierDirectiveRemoval) {
    ReplInputObject input(
        "#import std/math\n"
        "value = 42 # comment\n",
        "snippet");

    const auto result = preprocess_file(input);
    const auto& navigator = result.get_navigator();

    EXPECT_EQ(result.get_output(), "value = 42\n");
    EXPECT_EQ(navigator.get_original_position(0), 17u);
    EXPECT_EQ(navigator.get_original_position(9), 26u);
    EXPECT_EQ(navigator.get_original_position(10), 37u);
    const auto& skipped_tokens = navigator.get_skipped_tokens();
    ASSERT_EQ(skipped_tokens.size(), 2u);
    EXPECT_EQ(skipped_tokens[0].start, 0u);
    EXPECT_EQ(skipped_tokens[0].num_skipped, 17u);
    EXPECT_EQ(skipped_tokens[1].start, 10u);
    EXPECT_EQ(skipped_tokens[1].num_skipped, 10u);
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

TEST(PreprocessedFileTest, LeavesOutputEmptyForLineWithoutTextBeforeComment) {
    ReplInputObject input("#comment only\n", "snippet");

    const auto result = preprocess_file(input);

    EXPECT_TRUE(result.get_output().empty());
    EXPECT_TRUE(result.get_include_paths().empty());
    EXPECT_TRUE(result.get_using_namespaces().empty());
    EXPECT_EQ(result.get_skipped_character_count(), 14u);
    const auto& navigator = result.get_navigator();
    EXPECT_EQ(navigator.get_file_name(), "snippet");
    EXPECT_EQ(navigator.get_original_position(0), 14u);
    const auto& skipped_tokens = navigator.get_skipped_tokens();
    ASSERT_EQ(skipped_tokens.size(), 1u);
    EXPECT_EQ(skipped_tokens[0].start, 0u);
    EXPECT_EQ(skipped_tokens[0].num_skipped, 14u);
}

TEST(PreprocessedFileTest, SkipsEmptyLinesInOutput) {
    ReplInputObject input(
        "first line\n"
        "\n"
        "second line\n",
        "snippet");

    const auto result = preprocess_file(input);

    EXPECT_EQ(result.get_output(),
              "first line\n"
              "second line\n");
    EXPECT_TRUE(result.get_include_paths().empty());
    EXPECT_TRUE(result.get_using_namespaces().empty());
    EXPECT_EQ(result.get_skipped_character_count(), 1u);
    const auto& skipped_tokens = result.get_navigator().get_skipped_tokens();
    ASSERT_EQ(skipped_tokens.size(), 1u);
    EXPECT_EQ(skipped_tokens[0].start, 11u);
    EXPECT_EQ(skipped_tokens[0].num_skipped, 1u);
}

TEST(PreprocessedFileTest, PreservesWhitespaceInDirectives) {
    ReplInputObject input(
        "#import   lib/core  \n"
        "#using   ns::inner  \n",
        "snippet");

    const auto result = preprocess_file(input);

    EXPECT_TRUE(result.get_output().empty());
    EXPECT_EQ(result.get_include_paths(), std::vector<std::string>({"  lib/core  "}));
    EXPECT_EQ(result.get_using_namespaces(), std::vector<std::string>({"  ns::inner  "}));
    EXPECT_EQ(result.get_skipped_character_count(), 42u);
    EXPECT_EQ(result.get_navigator().get_file_name(), "snippet");
    const auto& skipped_tokens = result.get_navigator().get_skipped_tokens();
    ASSERT_EQ(skipped_tokens.size(), 1u);
    EXPECT_EQ(skipped_tokens[0].start, 0u);
    EXPECT_EQ(skipped_tokens[0].num_skipped, 42u);
}

TEST(PreprocessedFileTest, PrettyPrintFusesAdjacentRemovedLines) {
    ReplInputObject input(
        "#import lalalala\n"
        "\n"
        "# some comment\n"
        "println(7) # another comment\n"
        "println(3)\n",
        "snippet");

    const auto result = preprocess_file(input);
    std::ostringstream output_stream;

    result.pretty_print(output_stream);

    EXPECT_EQ(output_stream.str(),
              "Preprocessed file: snippet\n"
              "Skipped characters: \033[31m52\033[0m\n"
              "Include paths (1):\n"
              "  - lalalala\n"
              "Using namespaces (0):\n"
              "Output:\n"
              "\033[31m---SKIPPED 33 tokens---\033[0m\n"
              "println(7)\033[31m---SKIPPED 19 tokens---\033[0m\n"
              "println(3)\n");
}

TEST(PreprocessedFileNavigatorTest, RejectsUnsortedSkippedRanges) {
    EXPECT_THROW(static_cast<void>(PreprocessedFileNavigator("sample.smiles", {{5, 1}, {4, 2}})), std::invalid_argument);
}

}  // namespace
