#include <gtest/gtest.h>

#include <getopt.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "smiles/options.hpp"
#include "smiles/smiles.h"

namespace {

struct ArgvHolder {
    std::vector<std::string> values;
    std::vector<std::unique_ptr<char[]>> storage;
    std::vector<char*> argv;

    explicit ArgvHolder(std::vector<std::string> input_values) : values(std::move(input_values)) {
        storage.reserve(values.size());
        argv.reserve(values.size() + 1);
        for (const auto& value : values) {
            auto buffer = std::make_unique<char[]>(value.size() + 1);
            std::copy(value.begin(), value.end(), buffer.get());
            buffer[value.size()] = '\0';
            argv.push_back(buffer.get());
            storage.push_back(std::move(buffer));
        }
        argv.push_back(nullptr);
    }
};

smiles::options::ParseCliResult parse_args(std::vector<std::string> args) {
    optind = 1;
    opterr = 0;
    optopt = 0;
    ArgvHolder holder(std::move(args));
    return smiles::options::parse_cli_args(static_cast<int>(holder.argv.size() - 1), holder.argv.data());
}

}  // namespace

TEST(SmilesTest, ExposesProjectName) {
    EXPECT_EQ(smiles::project_name(), "Smiles");
}

TEST(SmilesTest, IsLearningProject) {
    EXPECT_TRUE(smiles::is_learning_project());
}

TEST(SmilesCliTest, UsageMatchesExpectedFormat) {
    EXPECT_EQ(smiles::options::usage(), "Usage: smiles [-e|--early_exit {preprocessor|lexer|ast}] [path_to_file]");
}

TEST(SmilesCliTest, RejectsMissingInputFilePath) {
    const auto result = parse_args({"smiles"});

    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error.find("Missing input file path"), std::string::npos);
    EXPECT_NE(result.error.find(smiles::options::usage()), std::string::npos);
}

TEST(SmilesCliTest, AcceptsSinglePathArgument) {
    const auto result = parse_args({"smiles", "program.smiles"});

    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.options.path_to_file, "program.smiles");
    EXPECT_EQ(result.options.early_exit, smiles::options::EarlyExitStage::none);
    EXPECT_TRUE(result.error.empty());
}

TEST(SmilesCliTest, AcceptsShortEarlyExitOption) {
    const auto result = parse_args({"smiles", "-e", "lexer", "program.smiles"});

    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.options.path_to_file, "program.smiles");
    EXPECT_EQ(result.options.early_exit, smiles::options::EarlyExitStage::lexer);
}

TEST(SmilesCliTest, AcceptsLongEarlyExitOptionAndPath) {
    const auto result = parse_args({"smiles", "--early_exit", "ast", "program.smiles"});

    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.options.early_exit, smiles::options::EarlyExitStage::ast);
    EXPECT_EQ(result.options.path_to_file, "program.smiles");
}

TEST(SmilesCliTest, RejectsMissingEarlyExitValue) {
    const auto result = parse_args({"smiles", "-e"});

    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error.find("Failed to parse command line arguments"), std::string::npos);
    EXPECT_NE(result.error.find(smiles::options::usage()), std::string::npos);
}

TEST(SmilesCliTest, RejectsInvalidEarlyExitValue) {
    const auto result = parse_args({"smiles", "--early_exit", "bad", "program.smiles"});

    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error.find("Invalid early exit stage 'bad'"), std::string::npos);
    EXPECT_NE(result.error.find(smiles::options::usage()), std::string::npos);
}

TEST(SmilesCliTest, RejectsMultiplePathArguments) {
    const auto result = parse_args({"smiles", "a.smiles", "b.smiles"});

    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error.find("Expected a single input file path"), std::string::npos);
    EXPECT_NE(result.error.find(smiles::options::usage()), std::string::npos);
}

TEST(SmilesCliTest, RejectsUnknownOption) {
    const auto result = parse_args({"smiles", "--unknown", "program.smiles"});

    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error.find("Failed to parse command line arguments"), std::string::npos);
    EXPECT_NE(result.error.find(smiles::options::usage()), std::string::npos);
}
