//
// Created by Xun Li on 2/14/19.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <ogrsf_frmts.h>

#include "AbstractWeights.h"
#include "ContiguityWeights.h"

#define FILE_PT_GAL "points.gal"

//https://github.com/google/googletest/tree/master/googletest/samples
// test_eq, test_neq, Zero, Positive, Negative, Trivial,

TEST(basic_check, test_eq) {
    EXPECT_EQ(1, 1);
    EXPECT_EQ(1, 0);
}

TEST(basic_check, test_neq) {
    EXPECT_NE(1, 0);
}

/**
 * Test case: create contiguity weights from points
 */
TEST(weights_creation_check, test_eq) {
    std::vector<OGRGeometry*> points;
    AbstractWeights *p_w = new PointContiWeights(wkbGeometryCollectionZM, points);
    // check ids
    std::vector<int64_t> ids = p_w->getM_ids();
    EXPECT_THAT(ids, testing::ElementsAre(5, 10, 15));

    // check weights content
    Gda::GalElement* gal = p_w->GetGalElements();

    bool success = p_w->Save(FILE_PT_GAL);
    if (success) {
        // check file content
    }
    delete p_w;
    EXPECT_TRUE(success);
}