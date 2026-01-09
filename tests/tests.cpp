#include <gtest/gtest.h>

#include "TECS.h"
#include "THandleFreeList.h"
#include "TSparseSet.h"
#include "TSparseSetChunked.h"
#include "TSparseTagSet.h"

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}