#include <catch.hpp>

#include <LumosEngine.h>

TEST_CASE("Reference Tests", "[LumosEngine]")
{
	using namespace Lumos;
	Ref<Maths::Vector4> testRef = CreateRef<Maths::Vector4>(1.0f, 0.0f, 0.0f, 1.0f);

	auto testRef2 = testRef;

	REQUIRE(testRef.GetCounter()->GetReferenceCount() == 2);

	{
		auto testRef3 = testRef2;
		REQUIRE(testRef.GetCounter()->GetReferenceCount() == 3);
	}

	REQUIRE(testRef.GetCounter()->GetReferenceCount() == 2);

	testRef2.reset();

	REQUIRE(testRef.GetCounter()->GetReferenceCount() == 1);

	{
		auto testVector = std::vector<Ref<Maths::Vector4>>();
		testVector.emplace_back(testRef);
		REQUIRE(testRef.GetCounter()->GetReferenceCount() == 2);
		testVector.clear();
		REQUIRE(testRef.GetCounter()->GetReferenceCount() == 1);
	}

	REQUIRE(testRef.GetCounter()->GetReferenceCount() == 1);

	auto vec = testRef.release();

	REQUIRE(vec->Equals(Maths::Vector4(1.0f, 0.0f, 0.0f, 1.0f)));

	LUMOS_LOG_INFO("Reference Test Passed");
}