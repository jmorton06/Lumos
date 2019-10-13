#include <catch.hpp>

#include <LumosEngine.h>

TEST_CASE("Vector2 Tests", "[LumosEngine]")
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

TEST_CASE("Vector3 Tests", "[LumosEngine]")
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

TEST_CASE("Vector4 Tests", "[LumosEngine]")
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
		REQUIRE(N == Vector4(0.5, 2.0 / 5.0, 2.0 / 6.0, 2.0 / 7.0));
	}

	LUMOS_LOG_INFO("Vector4 Test Passed");
}

TEST_CASE("Quaternion Tests", "[LumosEngine]")
{
	using namespace Lumos;
	using namespace Maths;

	{
		Maths::Quaternion q;
		Matrix4 m = q.ToMatrix4();

		REQUIRE(m == Matrix4());
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
		//q.MakeFromAngleAxis(PI, Vector3::RIGHT);

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

		REQUIRE(v.Equals(Vector4(-1.0f, 0.0f, 0.0f, -1.0f)));
	}

	{
		Matrix3 m = Matrix3::RotationX(PI / 2.0f * RADTODEG);
		Matrix3 n = Matrix3::RotationZ(50);
		Maths::Quaternion q = Maths::Quaternion::FromMatrix(m);

		Vector3 u = Vector3(1.0f);
		Vector3 v = m * u;

		n = q.ToMatrix3();

		Vector3 w = n * u;

		REQUIRE(v.Equals(w));
	}

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
