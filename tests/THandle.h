#include <gtest/gtest.h>
#include <lutra-ecs/Handle.h>

TEST(Handle, IndexAndValidationMasksDoNotOverlap)
{
	using H8 = lcs::Handle<uint32_t, 8>;
	ASSERT_TRUE((H8::index_mask & H8::validation_id_mask) == 0);

	using H16 = lcs::Handle<uint32_t, 16>;
	ASSERT_TRUE((H16::index_mask & H16::validation_id_mask) == 0);
}

TEST(Handle, CreateRoundTrip)
{
	using H = lcs::Handle<uint32_t, 8>;

	constexpr H::data_t index = 12345;
	constexpr H::data_t validation_id = 3 * H::validation_id_increment;

	H h = H::Create(validation_id, index);
	ASSERT_TRUE(h.GetIndex() == index);
	ASSERT_TRUE(h.GetValidationID() == validation_id);
}

TEST(Handle, CreateNextIncrementsValidationOnly)
{
	using H = lcs::Handle<uint32_t, 8>;

	constexpr H::data_t index = 100;
	H h1 = H::CreateNew(index);
	ASSERT_TRUE(h1.GetIndex() == index);
	ASSERT_TRUE(h1.GetValidationID() == 0);

	H h2 = H::CreateNext(h1.GetValidationID(), index);
	ASSERT_TRUE(h2.GetIndex() == index);
	ASSERT_TRUE(h2.GetValidationID() == H::validation_id_increment);

	H h3 = H::CreateNext(h2.GetValidationID(), index);
	ASSERT_TRUE(h3.GetIndex() == index);
	ASSERT_TRUE(h3.GetValidationID() == 2 * H::validation_id_increment);
}

TEST(Handle, ValidationIdWrapsWithoutCorruptingIndex)
{
	using H = lcs::Handle<uint32_t, 8>;

	constexpr H::data_t index = 42;

	H h = H::CreateNext(H::max_validation_id, index);
	ASSERT_TRUE(h.GetIndex() == index);
	ASSERT_TRUE(h.GetValidationID() == 0);
}
