#include <gtest/gtest.h>

#include "TECS.h"
#include "TIndexFreeList.h"
#include "THandleFreeList.h"
#include "TSparseSet.h"
#include "TSparseSet2.h"
#include "TSparseTagSet.h"
#include "TSparseTagSet2.h"

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}