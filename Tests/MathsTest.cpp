#include <catch.hpp>

#include <LumosEngine.h>

TEST_CASE("Vector2 Tests", "[Lumos::Maths]")
{
	using namespace Lumos;
	using namespace Maths;
	{
		Vector2 A(1.0f);
		Vector2 C = A + 1.0f;
		A += 1.0f;
		REQUIRE(A == Vector2(2.0f));
		REQUIRE(A == C);
	}

	{
		Vector2 A(1.0f);
		Vector2 B(2.0f, -1.0f);
		Vector2 C = A + B;
		A += B;
		REQUIRE(A == Vector2(3.0f, 0.0f));
		REQUIRE(A == C);
	}

	{
		Vector2 A(1.0f);
		Vector2 C = A - 1.0f;
		A -= 1.0f;
		REQUIRE(A == Vector2(0.0f));
		REQUIRE(A == C);
	}

	{
		Vector2 A(1.0f);
		Vector2 B(2.0f, -1.0f);
		Vector2 C = A - B;
		A -= B;
		REQUIRE(A == Vector2(-1.0f, 2.0f));
		REQUIRE(A == C);
	}

	{
		Vector2 A(1.0f);
		Vector2 C = A * 2.0f;
		A *= 2.0f;
		REQUIRE(A == Vector2(2.0f));
		REQUIRE(A == C);
	}

	{
		Vector2 A(2.0f);
		Vector2 B(2.0f);
		Vector2 C = A / B;
		A /= B;
		REQUIRE(A == Vector2(1.0f));
		REQUIRE(A == C);
	}
	
	{
		Vector2 A(1.0f, 2.0f);
		Vector2 B(4.0f, 5.0f);

		Vector2 C = A + B;
		REQUIRE(C == Vector2(5, 7));

		Vector2 D = B - A;
		REQUIRE(D == Vector2(3, 3));

		Vector2 E = A * B;
		REQUIRE(E == Vector2(4, 10));

		Vector2 F = B / A;
		REQUIRE(F == Vector2(4, 2.5));

		Vector2 G = A + 1.0f;
		REQUIRE(G == Vector2(2, 3));

		Vector2 H = B - 1.0f;
		REQUIRE(H == Vector2(3, 4));

		Vector2 I = A * 2.0f;
		REQUIRE(I == Vector2(2, 4));

		Vector2 J = B / 2.0f;
		REQUIRE(J == Vector2(2, 2.5));
	}
	
	LUMOS_LOG_INFO("Vector2 Test Passed");
}

TEST_CASE("Vector3 Tests", "[Lumos::Maths]")
{
    using namespace Lumos;
	using namespace Maths;

	{
		float const R(1.0f);
		float const S(2.0f);
		float const T(3.0f);
		Vector3 const O(1.0f, 2.0f, 3.0f);

		Vector3 const A(R);
		Vector3 const B(1.0f);
		Vector3 const C(R, S, T);
		Vector3 const D(R, 2.0f, 3.0f);
		Vector3 const E(1.0f, S, 3.0f);
		Vector3 const F(1.0f, S, T);
		Vector3 const G(R, 2.0f, T);
		Vector3 const H(R, S, 3.0f);

		REQUIRE(A == B);
		REQUIRE(C == O);
		REQUIRE(D == O);
		REQUIRE(E == O);
		REQUIRE(F == O);
		REQUIRE(G == O);
		REQUIRE(H == O);
	}

	{
		float const R(1.0);
		double const S(2.0);
		float const T(3.0);
		Vector3 const O(1.0f, 2.0f, 3.0f);

		Vector3 const A(R);
		Vector3 const B(1.0);
		Vector3 const C(R, S, T);
		Vector3 const D(R, 2.0, 3.0);
		Vector3 const E(1.0f, S, 3.0);
		Vector3 const F(1.0, S, T);
		Vector3 const G(R, 2.0, T);
		Vector3 const H(R, S, 3.0);
		REQUIRE(A == B);
		REQUIRE(C == O);
		REQUIRE(D == O);
		REQUIRE(E == O);
		REQUIRE(F == O);
		REQUIRE(G == O);
		REQUIRE(H == O);
	}

	{
		Vector3 const A(1.0f, 2.0f, 3.0f);
		Vector3 const B(4.0f, 5.0f, 6.0f);

		Vector3 const C = A + B;
		Vector3 const D = B - A;
		Vector3 const E = A * B;
		Vector3 const F = B / A;
		Vector3 const G = A + Vector3(1.0f);
		Vector3 const H = B - Vector3(1.0f);
		Vector3 const I = A * 2.0f;
		Vector3 const J = B / 2.0f;

		REQUIRE(C == Vector3(5, 7, 9));
		REQUIRE(D == Vector3(3, 3, 3));
		REQUIRE(E == Vector3(4, 10, 18));
		REQUIRE(F == Vector3(4, 2.5, 2));
		REQUIRE(G == Vector3(2, 3, 4));
		REQUIRE(H == Vector3(3, 4, 5));
		REQUIRE(I == Vector3(2, 4, 6));
		REQUIRE(J == Vector3(2, 2.5, 3));
	}

	LUMOS_LOG_INFO("Vector3 Test Passed");
}

TEST_CASE("Vector4 Tests", "[Lumos::Maths]")
{
	using namespace Lumos;
	using namespace Maths;
	
	{
		float const R(1.0f);
		float const S(2.0f);
		float const T(3.0f);
		float const U(4.0f);
		Vector4 const O(1.0f, 2.0f, 3.0f, 4.0f);

		Vector4 const A(R);
		Vector4 const B(1.0f);
		REQUIRE(A == B);

		Vector4 const C(R, S, T, U);
		REQUIRE(C == O);

		Vector4 const D(R, 2.0f, 3.0f, 4.0f);
		REQUIRE(D == O);

		Vector4 const E(1.0f, S, 3.0f, 4.0f);
		REQUIRE(E == O);

		Vector4 const F(R, S, 3.0f, 4.0f);
		REQUIRE(F == O);

		Vector4 const G(1.0f, 2.0f, T, 4.0f);
		REQUIRE(G == O);

		Vector4 const H(R, 2.0f, T, 4.0f);
		REQUIRE(H == O);

		Vector4 const I(1.0f, S, T, 4.0f);
		REQUIRE(I == O);

		Vector4 const J(R, S, T, 4.0f);
		REQUIRE(J == O);

		Vector4 const K(R, 2.0f, 3.0f, U);
		REQUIRE(K == O);

		Vector4 const L(1.0f, S, 3.0f, U);
		REQUIRE(L == O);

		Vector4 const M(R, S, 3.0f, U);
		REQUIRE(M == O);

		Vector4 const N(1.0f, 2.0f, T, U);
		REQUIRE(N == O);

		Vector4 const P(R, 2.0f, T, U);
		REQUIRE(P == O);

		Vector4 const Q(1.0f, S, T, U);
		REQUIRE(Q == O);

		Vector4 const V(R, S, T, U);
		REQUIRE(V == O);
	}

	{
		float const R(1.0f);
		double const S(2.0);
		float const T(3.0);
		double const U(4.0);
		Vector4 const O(1.0f, 2.0, 3.0f, 4.0);

		Vector4 const A(R);
		Vector4 const B(1.0);
		REQUIRE(A == B);

		Vector4 const C(R, S, T, U);
		REQUIRE(C == O);

		Vector4 const D(R, 2.0f, 3.0, 4.0f);
		REQUIRE(D == O);

		Vector4 const E(1.0, S, 3.0f, 4.0);
		REQUIRE(E == O);

		Vector4 const F(R, S, 3.0, 4.0f);
		REQUIRE(F == O);

		Vector4 const G(1.0f, 2.0, T, 4.0);
		REQUIRE(G == O);

		Vector4 const H(R, 2.0, T, 4.0);
		REQUIRE(H == O);

		Vector4 const I(1.0, S, T, 4.0f);
		REQUIRE(I == O);

		Vector4 const J(R, S, T, 4.0f);
		REQUIRE(J == O);

		Vector4 const K(R, 2.0f, 3.0, U);
		REQUIRE(K == O);

		Vector4 const L(1.0f, S, 3.0, U);
		REQUIRE(L == O);

		Vector4 const M(R, S, 3.0, U);
		REQUIRE(M == O);

		Vector4 const N(1.0f, 2.0, T, U);
		REQUIRE(N == O);

		Vector4 const P(R, 2.0, T, U);
		REQUIRE(P == O);

		Vector4 const Q(1.0f, S, T, U);
		REQUIRE(Q == O);

		Vector4 const V(R, S, T, U);
		REQUIRE(V == O);
	}

	{
		float const v1_0(1.0f);
		float const v1_1(2.0f);
		float const v1_2(3.0f);
		float const v1_3(4.0f);

		Vector2 const v2_0(1.0f, 2.0f);
		Vector2 const v2_1(2.0f, 3.0f);
		Vector2 const v2_2(3.0f, 4.0f);

		Vector3 const v3_0(1.0f, 2.0f, 3.0f);
		Vector3 const v3_1(2.0f, 3.0f, 4.0f);

		Vector4 const O(1.0f, 2.0f, 3.0f, 4.0f);

		Vector4 const A(v1_0, v1_1, v2_2);
		REQUIRE(A == O);

		Vector4 const B(1.0f, 2.0f, v2_2);
		REQUIRE(B == O);

		Vector4 const C(v1_0, 2.0f, v2_2);
		REQUIRE(C == O);

		Vector4 const D(1.0f, v1_1, v2_2);
		REQUIRE(D == O);

		Vector4 const E(v2_0, v1_2, v1_3);
		REQUIRE(E == O);

		Vector4 const F(v2_0, 3.0, v1_3);
		REQUIRE(F == O);

		Vector4 const G(v2_0, v1_2, 4.0);
		REQUIRE(G == O);

		Vector4 const H(v2_0, 3.0f, 4.0);
		REQUIRE(H == O);
	}

	{
		float const v1_0(1.0f);
		float const v1_1(2.0f);
		float const v1_2(3.0f);
		float const v1_3(4.0f);

		Vector2 const v2(2.0f, 3.0f);

		Vector4 const O(1.0f, 2.0, 3.0f, 4.0);

		Vector4 const A(v1_0, v2, v1_3);
		REQUIRE(A == O);

		Vector4 const B(v1_0, v2, 4.0);
		REQUIRE(B == O);

		Vector4 const C(1.0, v2, v1_3);
		REQUIRE(C == O);

		Vector4 const D(1.0f, v2, 4.0);
		REQUIRE(D == O);

		Vector4 const E(1.0, v2, 4.0f);
		REQUIRE(E == O);
	}

	{
		Vector4 const A(1.0f, 2.0f, 3.0f, 4.0f);
		Vector4 const B(4.0f, 5.0f, 6.0f, 7.0f);

		Vector4 const C = A + B;
		Vector4 const D = B - A;
		Vector4 const E = A * B;
		Vector4 const F = B / A;
		Vector4 const G = A + 1.0f;
		Vector4 const H = B - 1.0f;
		Vector4 const I = A * 2.0f;
		Vector4 const J = B / 2.0f;
		Vector4 const K = 1.0f + A;
		Vector4 const L = 1.0f - B;
		Vector4 const M = 2.0f * A;
		Vector4 const N = 2.0f / B;

		REQUIRE(C == Vector4(5, 7, 9, 11));
		REQUIRE(D == Vector4(3, 3, 3, 3));
		REQUIRE(E == Vector4(4, 10, 18, 28));
		REQUIRE(F == Vector4(4, 2.5, 2, 7.0f / 4.0f));
		REQUIRE(G == Vector4(2, 3, 4, 5));
		REQUIRE(H == Vector4(3, 4, 5, 6));
		REQUIRE(I == Vector4(2, 4, 6, 8));
		REQUIRE(J == Vector4(2, 2.5, 3, 3.5));
		REQUIRE(K == Vector4(2, 3, 4, 5));
		REQUIRE(L == Vector4(-3, -4, -5, -6));
		REQUIRE(M == Vector4(2, 4, 6, 8));
		REQUIRE(N == Vector4(0.5f, 2.0f / 5.0f, 2.0f / 6.0f, 2.0f / 7.0f));
	}

	LUMOS_LOG_INFO("Vector4 Test Passed");
}

SCENARIO("Quaternion Tests", "[Lumos::Maths]")
{
	using namespace Lumos;
	using namespace Maths;

	GIVEN("Default quaternion")
	{
		WHEN("We convert to a Matrix4")
		{
			Maths::Quaternion q;
			Matrix4 m = q.RotationMatrix4();
			THEN("It should be an identity Matrix")
			{
				REQUIRE(m == Matrix4());
			}
		}
	}

	{
		Maths::Quaternion firstQuaternion = Maths::Quaternion(180.f, Vector3(1.0f,0.0f,0.0f));
		Maths::Quaternion secondQuaternion(0.f, 1.f, 0.f, 0.0f);

		{
			REQUIRE(firstQuaternion.Equals(secondQuaternion));
			REQUIRE(firstQuaternion.Equals(secondQuaternion.Normalized()));
			REQUIRE(firstQuaternion.Conjugate().Equals(secondQuaternion.Inverse()));
			REQUIRE(Maths::Equals(firstQuaternion.DotProduct(secondQuaternion),1.f));
		}
	}


	GIVEN("The four unit quaternions")
	{
		Maths::Quaternion w(1.f, 0.f, 0.f, 0.f);
		Maths::Quaternion x(0.f, 1.f, 0.f, 0.f);
		Maths::Quaternion y(0.f, 0.f, 1.f, 0.f);
		Maths::Quaternion z(0.f, 0.f, 0.f, 1.f);

		Maths::Quaternion xyzw = x * y * z * w;

		WHEN("We ask for the norm")
		{
			THEN("They are all equal to 1")
			{
				REQUIRE(Maths::Equals(w.LengthSquared(), 1.0f));
				REQUIRE(Maths::Equals(x.LengthSquared(), 1.0f));
				REQUIRE(Maths::Equals(y.LengthSquared(), 1.0f));
				REQUIRE(Maths::Equals(z.LengthSquared(), 1.0f));
				REQUIRE(Maths::Equals(xyzw.LengthSquared(), 1.0f));
			}
		}

		WHEN("We multiply them")
		{
			THEN("Results should follow")
			{
				Maths::Quaternion oppositeOfW(-1.f, 0.f, 0.f, 0.0f);
				Maths::Quaternion oppositeOfX = x.Conjugate();
				Maths::Quaternion oppositeOfY = y.Conjugate();
				Maths::Quaternion oppositeOfZ = z.Conjugate();

				REQUIRE((x * x).Equals(oppositeOfW));
				REQUIRE((y * y).Equals(oppositeOfW));
				REQUIRE((z * z).Equals(oppositeOfW));
				REQUIRE((x * y * z).Equals(oppositeOfW));

				REQUIRE((x * y).Equals(z));
				REQUIRE((y * x).Equals(oppositeOfZ));
				REQUIRE((y * z).Equals(x));
				REQUIRE((z * y).Equals(oppositeOfX));
				REQUIRE((z * x).Equals(y));
				REQUIRE((x * z).Equals(oppositeOfY));
			}
		}
	}

	GIVEN("Two different quaternions (10, (1, 0, 0) and (20, (1, 0, 0))")
	{
		Maths::Quaternion x10 = Maths::Quaternion(10.0f, Vector3(1.0f,0.0f,0.0f));
		Maths::Quaternion x20 = x10 * x10;

		Maths::Quaternion x30a = x10 * x20;
		Maths::Quaternion x30b = x20 * x10;

		WHEN("We multiply them")
		{
			THEN("These results are expected")
			{
				REQUIRE(x20.Equals(Maths::Quaternion(20.0f, Vector3(1.0f, 0.0f, 0.0f))));
				REQUIRE(x30a.Equals(x30b));
			}
		}

		WHEN("Convert euler to quaternion")
		{
			Maths::Quaternion X45(Vector3((45.f), 0.f, 0.f));
			Maths::Quaternion Y45(Vector3(0.f, (45.f), 0.f));
			Maths::Quaternion Z45(Vector3(0.f, 0.f, (45.f)));

			THEN("They must be equal")
			{
				REQUIRE(X45.Equals(Maths::Quaternion(0.9238795f, 0.38268346f, 0.f, 0.f)));
				REQUIRE(Y45.Equals(Maths::Quaternion(0.9238795f, 0.f, 0.38268346f, 0.f)));
				REQUIRE(Z45.Equals(Maths::Quaternion(0.9238795f, 0.f, 0.f, 0.38268346f)));
			}
		}

		WHEN("We convert to euler angles and then to quaternions")
		{
			THEN("These results are expected")
			{
				REQUIRE(x30a.EulerAngles().Equals(x30b.EulerAngles()));
				REQUIRE(Maths::Quaternion(x30a.EulerAngles()).Equals(Maths::Quaternion(x30b.EulerAngles())));

				Maths::Quaternion tmp(1.f, 1.f, 0.f, 0.f);
				tmp.Normalize();
				REQUIRE(tmp.Equals(Maths::Quaternion(tmp.EulerAngles())));
			}
		}

		WHEN("We slerp")
		{
			THEN("The half of 10 and 30 is 20")
			{
				Maths::Quaternion slerpx10x30a = x10.Slerp(x30a, 0.5f);
				REQUIRE(slerpx10x30a.w == Approx(x20.w));
				REQUIRE(slerpx10x30a.x == Approx(x20.x));
				REQUIRE(slerpx10x30a.y == Approx(x20.y));
				REQUIRE(slerpx10x30a.z == Approx(x20.z));
				Maths::Quaternion slerpx10x30b = x10.Slerp(x30b, 0.5f);
				REQUIRE(slerpx10x30b.w == Approx(x20.w));
				REQUIRE(slerpx10x30b.x == Approx(x20.x));
				REQUIRE(slerpx10x30b.y == Approx(x20.y));
				REQUIRE(slerpx10x30b.z == Approx(x20.z));
				REQUIRE(x10.Slerp(x30a, 0.f).Equals(x10));
				REQUIRE(x10.Slerp(x30a, 1.f).Equals(x30a));
			}

			AND_THEN("The half of 45 is 22.5")
			{
				Maths::Quaternion quaternionA = Maths::Quaternion(0.f, Vector3(0.0f, 0.0f, 1.0f));
				Maths::Quaternion quaternionB = Maths::Quaternion(45.0f, Vector3(0.0f, 0.0f, 1.0f));
				Maths::Quaternion quaternionC = quaternionA.Slerp(quaternionB, 0.5f);

				Maths::Quaternion unitZ225 = Maths::Quaternion(22.5f, Vector3(0.0f, 0.0f, 1.0f));
				REQUIRE(quaternionC.Equals(unitZ225));
			}
		}

		WHEN("We get the rotation between two vectors")
		{
			THEN("The rotation in right-handed is 90 degree on z")
			{
				Maths::Quaternion rotationBetweenXY = Maths::Quaternion(Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
				Maths::Quaternion rotation90Z = Maths::Quaternion(-90.0f, Vector3(0.0f, 0.0f, -1.0f));
				rotation90Z.Normalize();
				REQUIRE(rotation90Z.Equals(rotationBetweenXY));
			}

			THEN("The rotation in right-handed is 90 degree on y")
			{
				Maths::Quaternion rotationBetweenXZ = Maths::Quaternion(Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f));
				Maths::Quaternion rotation90Y = Maths::Quaternion(90.0f, Vector3(0.0f, 1.0f, 0.0f));
				REQUIRE(rotation90Y.Equals(rotationBetweenXZ));
			}

			THEN("The rotation in right-handed is 90 degree on x")
			{
				Maths::Quaternion rotationBetweenYZ = Maths::Quaternion(Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f));
				Maths::Quaternion rotation90X = Maths::Quaternion(-90.0f, Vector3(1.0f, 0.0f, 0.0f));
				REQUIRE(rotation90X.Equals(rotationBetweenYZ));
			}

			THEN("The rotation in right-handed is 90 degree on y with non-unit vectors")
			{
				Vector3 origin(1.f, 1.f, 0.f);
				Vector3 extremity(-1.f, 1.f, 0.f);
				Maths::Quaternion rotation = Maths::Quaternion(origin, extremity);

				origin = rotation * origin;
				REQUIRE(origin.Equals(extremity));
			}
		}
	}

	{
		Vector3 zero = Vector3(0.0f);
		Maths::Quaternion test = Maths::Quaternion(90.0f, 0.0f, 0.0f);
		auto test2 = test + zero * test;
		REQUIRE(test.Equals(test2));
	}

	GIVEN("Different angles")
	{
		Maths::Quaternion rotation90X(0.707f, 0.707f, 0.f, 0.f);
		Maths::Quaternion rotation90Y(0.707f, 0.f, 0.707f, 0.f);
		Maths::Quaternion rotation90Z(0.707f, 0.f, 0.f, 0.707f);

		Maths::Quaternion rotation180X(0.f, 1.f, 0.f, 0.f);
		Maths::Quaternion rotation180Y(0.f, 0.f, 1.f, 0.f);
		Maths::Quaternion rotation180Z(0.f, 0.f, 0.f, 1.f);

		Maths::Quaternion rotation270X(-0.707f, 0.707f, 0.f, 0.f);
		Maths::Quaternion rotation270Y(-0.707f, 0.f, 0.707f, 0.f);
		Maths::Quaternion rotation270Z(-0.707f, 0.f, 0.f, 0.707f);

		Maths::Quaternion special(0.707f, 0.006f, 0.006f, 0.707f);

		WHEN("We convert them to euler angles")
		{
			THEN("Those are equal to")
			{
				CHECK(Maths::Equals(rotation90X.EulerAngles().x, 90.f, 0.1f));
				CHECK(Maths::Equals(rotation90Y.EulerAngles().y, 90.f, 0.1f));
				CHECK(Maths::Equals(rotation90Z.EulerAngles().z, 90.f, 0.1f));

				//CHECK(rotation180X.EulerAngles().Equals(Vector3(180.f, 0.f, 0.f)));
				//CHECK(rotation180Y.EulerAngles().Equals(Vector3(0.f, 180.f, 0.f)));
				//CHECK(rotation180Z.EulerAngles().Equals(Vector3(0.f, 0.f, 180.f)));

				CHECK(Maths::Equals(rotation270X.EulerAngles().x, (-90.f), 0.1f));
				CHECK(Maths::Equals(rotation270Y.EulerAngles().y, (-90.f), 0.1f));
				CHECK(Maths::Equals(rotation270Z.EulerAngles().z, (-90.f), 0.1f));

				CHECK(Maths::Equals(special.EulerAngles().x, (0.f), 0.1f));
				CHECK(Maths::Equals(special.EulerAngles().y, (1.f), 0.1f));
				CHECK(Maths::Equals(special.EulerAngles().z, (90.f), 0.1f));
			}
		}
	}


	{
		Maths::Quaternion q = Maths::Quaternion(Maths::M_PI * Maths::M_RADTODEG, Maths::Vector3(1.0f, 0.0f, 0.0f));

		Matrix4 m = q.RotationMatrix4();

		Vector4 v = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
		v = m * v;

		REQUIRE(v.Equals(Vector4(0.0f, -1.0f, 0.0f, 1.0f)));
	}

	{
		Maths::Quaternion q = Maths::Quaternion(Maths::M_PI * Maths::M_RADTODEG, Maths::Vector3(0.0f,1.0f,0.0f));

		Matrix4 m = q.RotationMatrix4();

		Vector4 v = Vector4(0.0f,0.0f,-1.0f,1.0f); //Forward
		v = m * v;

       REQUIRE(v.Equals(Vector4(0.0f, 0.0f, 1.0f, 1.0f)));
	}

	{
		Maths::Quaternion q = Maths::Quaternion(Maths::M_PI * Maths::M_RADTODEG, Vector3(0.0f,0.0f,-1.0f));

		Matrix4 m = q.RotationMatrix4();

		Vector4 v = Vector4(1.0f,0.0f,0.0f,1.0f);
		v = m * v;

		REQUIRE(v.Equals(Vector4(-1.0f, 0.0f, 0.0f, 1.0f)));
	}

	{
		Maths::Quaternion q = Maths::Quaternion(-Maths::M_PI / 2.0f * Maths::M_RADTODEG, Vector3(1.0f,0.0f,0.0f));
		Maths::Quaternion r = Maths::Quaternion(-40.0f, Vector3(0.0f,1.0f,0.0f));
		Maths::Quaternion t = Maths::Quaternion(-310.0f, Vector3(0.0f,0.0f, -1.0f));

		Maths::Quaternion k = q * r * t;

		Vector4 v = Vector4(1.0f);
		Matrix4 m = k.RotationMatrix4();
		v = m * v;

		//REQUIRE(v.Equals(Vector4(-0.737208f, 0.686816f, -1.40883f, 1.0f)));
	}

}
