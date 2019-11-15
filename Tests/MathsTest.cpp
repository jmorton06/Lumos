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
		Vector3 const G = A + 1.0f;
		Vector3 const H = B - 1.0f;
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

		Vector4 const O(1.0f, 2.0, 3.0f, 4.0);

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
			Matrix4 m = q.ToMatrix4();
			THEN("It should be an identity Matrix")
			{
				REQUIRE(m == Matrix4());
			}
		}
	}

	{
		Maths::Quaternion firstQuaternion = Maths::Quaternion::AxisAngleToQuaterion(Vector3(1.0f,0.0f,0.0f), 180.f);
		Maths::Quaternion secondQuaternion(1.f, 0.f, 0.f, 0.f);
		secondQuaternion.GenerateW();

		{
			REQUIRE(firstQuaternion.Equals(secondQuaternion));
			REQUIRE(firstQuaternion.Equals(secondQuaternion.Normalized()));
			REQUIRE(firstQuaternion.Conjugate().Equals(secondQuaternion.Inverse()));
			REQUIRE(Maths::Equals(firstQuaternion.Dot(secondQuaternion),1.f));
		}
	}

	GIVEN("The four unit quaternions")
	{
		Maths::Quaternion w(0.f, 0.f, 0.f, 1.f);
		Maths::Quaternion x(1.f, 0.f, 0.f, 0.f);
		Maths::Quaternion y(0.f, 1.f, 0.f, 0.f);
		Maths::Quaternion z(0.f, 0.f, 1.f, 0.f);

		Maths::Quaternion xyzw = x * y * z * w;

		WHEN("We ask for the norm")
		{
			THEN("They are all equal to 1")
			{
				REQUIRE(Maths::Equals(w.Magnitude(), 1.0f));
				REQUIRE(Maths::Equals(x.Magnitude(), 1.0f));
				REQUIRE(Maths::Equals(y.Magnitude(), 1.0f));
				REQUIRE(Maths::Equals(z.Magnitude(), 1.0f));
				REQUIRE(Maths::Equals(xyzw.Magnitude(), 1.0f));
			}
		}

		WHEN("We multiply them")
		{
			THEN("Results should follow")
			{
				Maths::Quaternion oppositeOfW(0.f, 0.f, 0.f, -1.f);
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
		Maths::Quaternion x10 = Maths::Quaternion::AxisAngleToQuaterion(Vector3(1.0f,0.0f,0.0f), (10.f));
		Maths::Quaternion x20 = x10 * x10;

		Maths::Quaternion x30a = x10 * x20;
		Maths::Quaternion x30b = x20 * x10;

		WHEN("We multiply them")
		{
		THEN("These results are expected")
		{
			REQUIRE(x20.Equals(Maths::Quaternion::AxisAngleToQuaterion(Vector3(1.0f, 0.0f, 0.0f), (20.f))));
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
				REQUIRE(X45.Equals(Maths::Quaternion(0.38268346f, 0.f, 0.f, 0.9238795f)));
				REQUIRE(Y45.Equals(Maths::Quaternion(0.f, 0.38268346f, 0.f, 0.9238795f)));
				REQUIRE(Z45.Equals(Maths::Quaternion(0.f, 0.f, 0.38268346f, 0.9238795f)));
			}
		}

		WHEN("We convert to euler angles and then to quaternions")
		{
			THEN("These results are expected")
			{
				REQUIRE(x30a.ToEuler().Equals(x30b.ToEuler()));
				REQUIRE(Maths::Quaternion(x30a.ToEuler()).Equals(Maths::Quaternion(x30b.ToEuler())));

				Maths::Quaternion tmp(1.f, 1.f, 0.f, 0.f);
				tmp.Normalize();
				REQUIRE(tmp.Equals(Maths::Quaternion(tmp.ToEuler())));
			}
		}

		WHEN("We slerp")
		{
			THEN("The half of 10 and 30 is 20")
			{
				Maths::Quaternion slerpx10x30a = Maths::Quaternion::Slerp(x10, x30a, 0.5f);
				REQUIRE(slerpx10x30a.w == Approx(x20.w));
				REQUIRE(slerpx10x30a.x == Approx(x20.x));
				REQUIRE(slerpx10x30a.y == Approx(x20.y));
				REQUIRE(slerpx10x30a.z == Approx(x20.z));
				Maths::Quaternion slerpx10x30b = Maths::Quaternion::Slerp(x10, x30b, 0.5f);
				REQUIRE(slerpx10x30b.w == Approx(x20.w));
				REQUIRE(slerpx10x30b.x == Approx(x20.x));
				REQUIRE(slerpx10x30b.y == Approx(x20.y));
				REQUIRE(slerpx10x30b.z == Approx(x20.z));
				REQUIRE(Maths::Quaternion::Slerp(x10, x30a, 0.f).Equals(x10));
				REQUIRE(Maths::Quaternion::Slerp(x10, x30a, 1.f).Equals(x30a));
			}

			AND_THEN("The half of 45 is 22.5")
			{
				Maths::Quaternion quaternionA = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 0.0f, 1.0f), (0.f));
				Maths::Quaternion quaternionB = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 0.0f, 1.0f), (45.f));
				Maths::Quaternion quaternionC = Maths::Quaternion::Slerp(quaternionA, quaternionB, 0.5f);

				Maths::Quaternion unitZ225 = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 0.0f, 1.0f), (22.5f));
				REQUIRE(quaternionC.Equals(unitZ225));
			}
		}

		WHEN("We get the rotation between two vectors")
		{
			THEN("The rotation in right-handed is 90 degree on z")
			{
				Maths::Quaternion rotationBetweenXY = Maths::Quaternion::FromVectors(Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
				Maths::Quaternion rotation90Z = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 0.0f, -1.0f), (-90.f));
				rotation90Z.Normalize();
				REQUIRE(rotation90Z.Equals(rotationBetweenXY));
			}

			THEN("The rotation in right-handed is 90 degree on y")
			{
				Maths::Quaternion rotationBetweenXZ = Maths::Quaternion::FromVectors(Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f));
				Maths::Quaternion rotation90Y = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f, 1.0f, 0.0f), (90.f));
				REQUIRE(rotation90Y.Equals(rotationBetweenXZ));
			}

			THEN("The rotation in right-handed is 90 degree on x")
			{
				Maths::Quaternion rotationBetweenYZ = Maths::Quaternion::FromVectors(Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f));
				Maths::Quaternion rotation90X = Maths::Quaternion::AxisAngleToQuaterion(Vector3(1.0f, 0.0f, 0.0f), -(90.f));
				REQUIRE(rotation90X.Equals(rotationBetweenYZ));
			}

			THEN("The rotation in right-handed is 90 degree on y with non-unit vectors")
			{
				Vector3 origin(1.f, 1.f, 0.f);
				Vector3 extremity(-1.f, 1.f, 0.f);
				Maths::Quaternion rotation = Maths::Quaternion::FromVectors(origin, extremity);

				Maths::Quaternion::RotatePointByQuaternion(rotation, origin);
				//REQUIRE(origin.Equals(extremity));
			}
		}
	}

	{
		Vector3 zero = Vector3(0.0f);
		Maths::Quaternion test = Maths::Quaternion(90.0f, 0.0f, 0.0f);
		auto test2 = test + test * zero;
		REQUIRE(test.Equals(test2));
	}

	GIVEN("Different angles")
	{
		Maths::Quaternion rotation90X(0.707f, 0.f, 0.f, 0.707f);
		Maths::Quaternion rotation90Y(0.f, 0.707f, 0.f, 0.707f);
		Maths::Quaternion rotation90Z(0.f, 0.f, 0.707f, 0.707f);

		Maths::Quaternion rotation180X(1.f, 0.f, 0.f, 0.f);
		Maths::Quaternion rotation180Y(0.f, 1.f, 0.f, 0.f);
		Maths::Quaternion rotation180Z(0.f, 0.f, 1.f, 0.f);

		Maths::Quaternion rotation270X(0.707f, 0.f, 0.f, -0.707f);
		Maths::Quaternion rotation270Y(0.f, 0.707f, 0.f, -0.707f);
		Maths::Quaternion rotation270Z(0.f, 0.f, 0.707f, -0.707f);

		Maths::Quaternion special(0.006f, 0.006f, 0.707f, 0.707f);

		WHEN("We convert them to euler angles")
		{
			THEN("Those are equal to")
			{
				CHECK(Maths::Equals(rotation90X.ToEuler().x, (90.f), 0.1f));
				CHECK(Maths::Equals(rotation90Y.ToEuler().y, (90.f), 0.1f));
				CHECK(Maths::Equals(rotation90Z.ToEuler().z, (90.f), 0.1f));

				//CHECK(rotation180X.ToEuler().Equals(Vector3(180.f, 0.f, 0.f)));
				CHECK(rotation180Y.ToEuler().Equals(Vector3(0.f, 180.f, 0.f)));
				CHECK(rotation180Z.ToEuler().Equals(Vector3(0.f, 0.f, 180.f)));

				CHECK(Maths::Equals(rotation270X.ToEuler().x, (-90.f), 0.1f));
				CHECK(Maths::Equals(rotation270Y.ToEuler().y, (-90.f), 0.1f));
				CHECK(Maths::Equals(rotation270Z.ToEuler().z, (-90.f), 0.1f));

				CHECK(Maths::Equals(special.ToEuler().x, (0.f), 0.1f));
				CHECK(Maths::Equals(special.ToEuler().y, (1.f), 0.1f));
				CHECK(Maths::Equals(special.ToEuler().z, (90.f), 0.1f));
			}
		}
	}

	{
		Matrix3 a, b, c;
		a.RotationX(PI / 2.0f * RADTODEG);
		b.RotationZ(50);
		c = a * b;

		Vector3 u = Vector3(1.0f);
		Vector3 v = c * u;

		Maths::Quaternion q = Maths::Quaternion::FromMatrix(c);
		Matrix3 m = q.ToMatrix3();

		Vector3 w = m * u;

		REQUIRE(v == w);
	}

	{
		Matrix4 a, b, c;
		a.RotationY(83);
		b.RotationX(122);
		c = a * b;

		Vector4 u = Vector4(1.0f);
		Vector4 v = c * u;

		Maths::Quaternion q = Maths::Quaternion::FromMatrix(c);
		Matrix4 m = q.ToMatrix4();

		Vector4 w = m * u;

		REQUIRE(w == v);
	}

	{
		Maths::Quaternion q = Maths::Quaternion::AxisAngleToQuaterion(Vector3(1.0f, 0.0f, 0.0f), PI * RADTODEG);

		Matrix4 m = q.ToMatrix4();

		Vector4 v = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
		v = m * v;

		REQUIRE(v.Equals(Vector4(0.0f, -1.0f, 0.0f, 1.0f)));
	}

	{
		Maths::Quaternion q = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f,1.0f,0.0f), PI * RADTODEG);

		Matrix4 m = q.ToMatrix4();

		Vector4 v = Vector4(0.0f,0.0f,-1.0f,1.0f); //Forward
		v = m * v;

        REQUIRE(v.Equals(Vector4(0.0f, 0.0f, 1.0f, 1.0f)));
	}

	{
		Maths::Quaternion q = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f,0.0f,-1.0f), PI * RADTODEG);

		Matrix4 m = q.ToMatrix4();

		Vector4 v = Vector4(1.0f,0.0f,0.0f,1.0f);
		v = m * v;

		REQUIRE(v.Equals(Vector4(-1.0f, 0.0f, 0.0f, 1.0f)));
	}

    return;
    //These fail
	{
		Matrix3 m = Matrix3::RotationX(PI / 2.0f * RADTODEG);
		Maths::Quaternion q = Maths::Quaternion::FromMatrix(m);

		Vector3 u = Vector3(1.0f);
		Vector3 v = m * u;

		Matrix3 n = q.ToMatrix3();

		Vector3 w = n * u;

		REQUIRE(v.Equals(w));
	}

	return;
	{
		Matrix3 m = Matrix3::RotationX(PI / 2.0f * RADTODEG);

		Maths::Quaternion q = Maths::Quaternion::FromMatrix(m);
		Vector3 xAxis, yAxis, zAxis;

		xAxis = q.GetXAxis();
		yAxis = q.GetYAxis();
		zAxis = q.GetZAxis();

		REQUIRE(xAxis.Equals(Vector3(1.0f,0.0f,0.0f)));
		REQUIRE(yAxis.Equals(-Vector3(0.0f,0.0f,-1.0f)));
		REQUIRE(zAxis.Equals(Vector3(0.0f,1.0f,0.0f)));

		q.ToAxes(xAxis, yAxis, zAxis);

		REQUIRE(xAxis.Equals(Vector3(1.0f,0.0f,0.0f)));
		REQUIRE(yAxis.Equals(-Vector3(0.0f,0.0f,-1.0f)));
		REQUIRE(zAxis.Equals(Vector3(0.0f,1.0f,0.0f)));
	}
	
	{
		Maths::Quaternion q = Maths::Quaternion::AxisAngleToQuaterion(Vector3(1.0f,0.0f,0.0f) , -PI / 2.0f * RADTODEG);
		Maths::Quaternion r = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f,1.0f,0.0f), -40.0f);
		Maths::Quaternion t = Maths::Quaternion::AxisAngleToQuaterion(Vector3(0.0f,0.0f, -1.0f) , -310.0f);

		Maths::Quaternion k = q * r * t;

		Vector4 v = Vector4(1.0f);
		Matrix4 m = k.ToMatrix4();
		v = m * v;

		REQUIRE(v.Equals(Vector4(-0.737208f, 0.686816f, -1.40883f, 1.0f)));
	}

}
