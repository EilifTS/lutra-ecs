#include <gtest/gtest.h>

#include "TBitMask.h"
#include "TByteIO.h"
#include "TContainer.h"
#include "TDeferred.h"
#include "TECS.h"
#include "TFileIO.h"
#include "THandle.h"
#include "THandleFreeList.h"
#include "TSaveRoundTrip.h"
#include "TSchema.h"
#include "TSparseSet.h"
#include "TSparseTagSet.h"

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}