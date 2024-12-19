#include "global.h"
#include "string_util.h"
#include "test/test.h"
#include "species.h"

TEST("Ensure Corm species id is 1045"){
    u32 corm = SPECIES_CORM;
    EXPECT_EQ(corm, 1045);
}