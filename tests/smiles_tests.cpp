#include <gtest/gtest.h>

#include "smiles/smiles.h"

TEST(SmilesTest, ExposesProjectName) {
    EXPECT_EQ(smiles::project_name(), "Smiles");
}

TEST(SmilesTest, IsLearningProject) {
    EXPECT_TRUE(smiles::is_learning_project());
}
