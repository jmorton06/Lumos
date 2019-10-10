#include "lmpch.h"
#include "Quaternion.h"
#include "MathsUtilities.h"

namespace Lumos
{
	namespace Maths
	{
		Quaternion::Quaternion()
		{
#ifdef LUMOS_SSEQUAT
			mmvalue = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);
#else
			x = y = z = 0.0f;
			w = 1.0f;
#endif
		}

		Quaternion::Quaternion(const Vector3& vec, float w)
		{
#ifdef LUMOS_SSEQUAT
			mmvalue = _mm_set_ps(vec.GetZ(), vec.GetY(), vec.GetX(), w);
#else
			x = vec.GetX(); y = vec.GetY(); z = vec.GetZ(); this->w = w;
#endif
		}

		Quaternion::Quaternion(float lx, float ly, float lz, float lw)
		{
			this->x = lx;  this->y = ly;  this->z = lz; this->w = lw;
		}

		Quaternion::Quaternion(const Quaternion& v)
		{
#ifdef LUMOS_SSEQUAT
			mmvalue = v.mmvalue;
#else
			x = v.x; y = v.y; z = v.z; w = v.w;
#endif
		}

#ifdef LUMOS_SSEQUAT
		Quaternion::Quaternion(__m128 m): mmvalue(m)
		{
		}
#endif
    
        Quaternion::Quaternion(float pitch, float yaw, float roll)
        {
            FromEulerAngles(pitch, yaw, roll);
        }

        Quaternion::Quaternion(const Vector3& v)
        {
            FromEulerAngles(v.x,v.y,v.z);
        }

		void Quaternion::Normalise()
		{
#ifdef LUMOS_SSEQUAT
			__m128 q = mmvalue;
			__m128 n = _mm_mul_ps(q, q);
			n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(2, 3, 0, 1)));
			n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(0, 1, 2, 3)));
			__m128 e = _mm_rsqrt_ps(n);
			__m128 e3 = _mm_mul_ps(_mm_mul_ps(e, e), e);
			__m128 half = _mm_set1_ps(0.5f);
			n = _mm_add_ps(e, _mm_mul_ps(half, _mm_sub_ps(e, _mm_mul_ps(n, e3))));
			mmvalue = _mm_mul_ps(q, n);
#else
			float magnitude = sqrt(Dot(*this, *this));

			if (magnitude > 0.0f)
			{
				float t = 1.0f / magnitude;

				x *= t;
				y *= t;
				z *= t;
				w *= t;
			}
#endif
		}

		Quaternion Quaternion::Normal() const
		{
#ifdef LUMOS_SSEQUAT
			__m128 q = mmvalue;
			__m128 n = _mm_mul_ps(q, q);
			n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(2, 3, 0, 1)));
			n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(0, 1, 2, 3)));
			__m128 e = _mm_rsqrt_ps(n);
			__m128 e3 = _mm_mul_ps(_mm_mul_ps(e, e), e);
			__m128 half = _mm_set1_ps(0.5f);
			n = _mm_add_ps(e, _mm_mul_ps(half, _mm_sub_ps(e, _mm_mul_ps(n, e3))));
			return Quaternion(_mm_mul_ps(q, n));

#else
			float magnitude = sqrt(Dot(*this, *this));

			Quaternion q(*this);

			if (magnitude > 0.0f)
			{
				float t = 1.0f / magnitude;

				q.x *= t;
				q.y *= t;
				q.z *= t;
				q.w *= t;
			}

			return q;
#endif
		}

		Matrix4 Quaternion::ToMatrix4() const
		{
#ifdef LUMOS_SSEQUAT
			__m128 multiplier2 = _mm_set1_ps(2.0f);
			__m128 squaredTimes2 = _mm_mul_ps(_mm_mul_ps(mmvalue, mmvalue), multiplier2);
			__m128 offset1Times2 = _mm_mul_ps(_mm_mul_ps(mmvalue, _mm_set_ps(z, y, x, w)), multiplier2);
			__m128 offset2Times2 = _mm_mul_ps(_mm_mul_ps(mmvalue, _mm_set_ps(y, x, w, z)), multiplier2);

			Matrix4 mat;

			mat.values[0] = 1 - GetValue(squaredTimes2, 1) - GetValue(squaredTimes2, 2);
			mat.values[1] = GetValue(offset1Times2, 1) + GetValue(offset1Times2, 3);
			mat.values[2] = GetValue(offset2Times2, 0) - GetValue(offset2Times2, 1);

			mat.values[4] = GetValue(offset1Times2, 1) - GetValue(offset1Times2, 3);
			mat.values[5] = 1 - GetValue(squaredTimes2, 0) - GetValue(squaredTimes2, 2);
			mat.values[6] = GetValue(offset1Times2, 2) + GetValue(offset1Times2, 0);

			mat.values[8] = GetValue(offset2Times2, 0) + GetValue(offset2Times2, 1);
			mat.values[9] = GetValue(offset1Times2, 2) - GetValue(offset1Times2, 0);
			mat.values[10] = 1 - GetValue(squaredTimes2, 0) - GetValue(squaredTimes2, 1);

			return mat;
#else
			Matrix4 mat;

			float yy = y * y;
			float zz = z * z;
			float xy = x * y;
			float zw = z * w;
			float xz = x * z;
			float yw = y * w;
			float xx = x * x;
			float yz = y * z;
			float xw = x * w;

			mat.values[0] = 1 - 2 * yy - 2 * zz;
			mat.values[1] = 2 * xy + 2 * zw;
			mat.values[2] = 2 * xz - 2 * yw;

			mat.values[4] = 2 * xy - 2 * zw;
			mat.values[5] = 1 - 2 * xx - 2 * zz;
			mat.values[6] = 2 * yz + 2 * xw;

			mat.values[8] = 2 * xz + 2 * yw;
			mat.values[9] = 2 * yz - 2 * xw;
			mat.values[10] = 1 - 2 * xx - 2 * yy;

			return mat;
#endif
		}

		Matrix3 Quaternion::ToMatrix3() const
		{
			Matrix3 mat;

			float yy = y * y;
			float zz = z * z;
			float xy = x * y;
			float zw = z * w;
			float xz = x * z;
			float yw = y * w;
			float xx = x * x;
			float yz = y * z;
			float xw = x * w;

			mat.values[0] = 1 - 2 * yy - 2 * zz;
			mat.values[1] = 2 * xy + 2 * zw;
			mat.values[2] = 2 * xz - 2 * yw;

			mat.values[3] = 2 * xy - 2 * zw;
			mat.values[4] = 1 - 2 * xx - 2 * zz;
			mat.values[5] = 2 * yz + 2 * xw;

			mat.values[6] = 2 * xz + 2 * yw;
			mat.values[7] = 2 * yz - 2 * xw;
			mat.values[8] = 1 - 2 * xx - 2 * yy;

			return mat;
		}

		Quaternion Quaternion::AxisAngleToQuaterion(const Vector3& vector, float degrees)
		{
			float theta = DegreesToRadians(degrees);
			float result = sin(theta * 0.5f);

			return Quaternion(static_cast<float>(vector.GetX() * result), static_cast<float>(vector.GetY() * result), static_cast<float>(vector.GetZ() * result), static_cast<float>(cos(theta * 0.5f)));
		}

		void Quaternion::GenerateW()
		{
			w = 1.0f - (x*x) - (y*y) - (z*z);
			if (w < 0.0f)
				w = 0.0f;
			else
				w = -sqrtf(w);
		}

		Vector3 Quaternion::ToEuler() const
		{
			float check = 2.0f * (-y * z + w * x);

			if (check < -0.995f)
			{
				return Vector3(
					-90.0f,
					0.0f,
					-atan2f(2.0f * (x * z - w * y), 1.0f - 2.0f * (y * y + z * z)) * RADTODEG
				);
			}
			else if (check > 0.995f)
			{
				return Vector3(
					90.0f,
					0.0f,
					atan2f(2.0f * (x * z - w * y), 1.0f - 2.0f * (y * y + z * z)) * RADTODEG
				);
			}
			else
			{
				return Vector3(
					asinf(check) * RADTODEG,
					atan2f(2.0f * (x * z + w * y), 1.0f - 2.0f * (x * x + y * y)) * RADTODEG,
					atan2f(2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z)) * RADTODEG
				);
			}
		}

		void Quaternion::FromEulerAngles(float pitch, float yaw, float roll)
		{
			pitch *= DEGTORAD_2;
			yaw   *= DEGTORAD_2;
			roll  *= DEGTORAD_2;
			float sinX = sinf(pitch);
			float cosX = cosf(pitch);
			float sinY = sinf(yaw);
			float cosY = cosf(yaw);
			float sinZ = sinf(roll);
			float cosZ = cosf(roll);

			w = cosY * cosX * cosZ + sinY * sinX * sinZ;
			x = cosY * sinX * cosZ + sinY * cosX * sinZ;
			y = sinY * cosX * cosZ - cosY * sinX * sinZ;
			z = cosY * cosX * sinZ - sinY * sinX * cosZ;
		}

		Quaternion Quaternion::EulerAnglesToQuaternion(float pitch, float yaw, float roll)
		{
			Quaternion q;
			q.FromEulerAngles(pitch, yaw, roll);
			return q;
		};

		Quaternion Quaternion::Conjugate() const
		{
			return Quaternion(-x, -y, -z, w);
		}

		Quaternion Quaternion::Inverse() const
		{
#ifdef LUMOS_SSEQUAT
			__m128 n = _mm_mul_ps(mmvalue, mmvalue);
			n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(2, 3, 0, 1)));
			n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(0, 1, 2, 3)));
			return Quaternion(_mm_div_ps(_mm_xor_ps(mmvalue, _mm_castsi128_ps(_mm_set_epi32((int)0x80000000UL, (int)0x80000000UL, (int)0x80000000UL, 0))), n));
#else
			Quaternion q = Conjugate();

			float m = q.Magnitude();
			m *= m;

			if (m == 0.0f)
				m = 1.0f;
			else
				m = 1.0f / m;

			return Quaternion(q.x * m, q.y * m, q.z * m, q.w * m);
#endif
		}

		float Quaternion::Magnitude() const
		{
			return sqrt(LengthSquared());
		}

		float Quaternion::LengthSquared() const
		{
#ifdef LUMOS_SSEQUAT
			__m128 n = _mm_mul_ps(mmvalue, mmvalue);
			n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(2, 3, 0, 1)));
			n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(0, 1, 2, 3)));
			return _mm_cvtss_f32(n);
#else
			return x * x + y * y + z * z + w * w;
#endif
		}

		Quaternion Quaternion::FromMatrix(const Matrix4& matrix)
		{
			Quaternion q;
            
			float t = matrix[0] + matrix[5] + matrix[10];

			if (t > 0.0f)
			{
				float invS = 0.5f / sqrtf(1.0f + t);

				q.x = (matrix[9] - matrix[6]) * invS;
				q.y = (matrix[2] - matrix[8]) * invS;
				q.z = (matrix[4] - matrix[1]) * invS;
				q.w = 0.25f / invS;
			}
			else
			{
				if (matrix[0] > matrix[5] && matrix[0] > matrix[10])
				{
					float invS = 0.5f / sqrtf(1.0f + matrix[0] - matrix[5] - matrix[10]);

					q.x = 0.25f / invS;
					q.y = (matrix[1] + matrix[4]) * invS;
					q.z = (matrix[8] + matrix[2]) * invS;
					q.w = (matrix[9] - matrix[6]) * invS;
				}
				else if (matrix[5] > matrix[10])
				{
					float invS = 0.5f / sqrtf(1.0f + matrix[5] - matrix[0] - matrix[10]);

					q.x = (matrix[1] + matrix[4]) * invS;
					q.y = 0.25f / invS;
					q.z = (matrix[6] + matrix[9]) * invS;
					q.w = (matrix[2] - matrix[8]) * invS;
				}
				else
				{
					float invS = 0.5f / sqrtf(1.0f + matrix[10] - matrix[0] - matrix[5]);

					q.x = (matrix[2] + matrix[8]) * invS;
					q.y = (matrix[6] + matrix[9]) * invS;
					q.z = 0.25f / invS;
					q.w = (matrix[4] - matrix[1]) * invS;
				}
			}

			return q;
		}


		Quaternion Quaternion::FromVectors(const Vector3 &v1, const Vector3 &v2)
		{
			Quaternion q;
			Vector3 a = Vector3::Cross(v1, v2);
			q.x = a.GetX();
			q.y = a.GetY();
			q.z = a.GetZ();
			q.w = v1.LengthSquared() + v2.LengthSquared() + Vector3::Dot(v1, v2);
			q.Normalise();
			return q;
		}

		Quaternion Quaternion::Interpolate(const Quaternion& pStart, const Quaternion& pEnd, float pFactor) const
		{
#ifdef LUMOS_SSEQUAT
			// calc cosine theta
			float cosom = _mm_cvtss_f32(_mm_dp_ps(pStart.mmvalue, pEnd.mmvalue, 0x71));

			// adjust signs (if necessary)
			Quaternion end = pEnd;
			if (cosom < 0.0f)
			{
				end.mmvalue = _mm_mul_ps(end.mmvalue, _mm_set1_ps(-1.0f));
				cosom = -cosom;
			}

			// Calculate coefficients
			float sclp, sclq;
			if ((1.0f - cosom) > 0.0001f) // 0.0001 -> some epsillon
			{
				// Standard case (slerp)
				float omega, sinom;
				omega = acos(cosom); // extract theta from dot product's cos theta
				sinom = sin(omega);
				sclp = sin((1.0f - pFactor) * omega) / sinom;
				sclq = sin(pFactor * omega) / sinom;
			}
			else
			{
				// Very close, do linear interp (because it's faster)
				sclp = 1.0f - pFactor;
				sclq = pFactor;
			}

			return _mm_add_ps(_mm_mul_ps(_mm_set1_ps(sclp), pStart.mmvalue), _mm_mul_ps(_mm_set1_ps(sclq), end.mmvalue));
#else
			// Clamp interpolation between start and end
			pFactor = Maths::Min(Maths::Max(pFactor, 0.0f), 1.0f);

			// Calc cos theta (Dot product)
			float cos_theta = Quaternion::Dot(pStart, pEnd);

			// Quaternions can describe any rotation positively or negatively, however to interpolate
			// correctly we need /both/ quaternions to use the same coordinate System
			Quaternion real_end = pEnd;
			if (cos_theta < 0.0f)
			{
				cos_theta = -cos_theta;
				real_end.x = -pEnd.x;
				real_end.y = -pEnd.y;
				real_end.z = -pEnd.z;
				real_end.w = -pEnd.w;
			}

			// Calculate interpolation coefficients
			float theta = acosf(cos_theta);    // extract theta from dot product's cos(theta)
			float inv_sin_theta = sinf(theta); // compute inverse rotation length 1.0f / sin(theta)

			if (fabs(inv_sin_theta) < 1e-6f)
				inv_sin_theta = 1e-6f;
			inv_sin_theta = 1.0f / inv_sin_theta;

			float factor_a = sinf((1.0f - pFactor) * theta) * inv_sin_theta;
			float factor_b = sinf(pFactor * theta) * inv_sin_theta;

			// Interpolate the two quaternions
			Quaternion out;
			out.x = factor_a * pStart.x + factor_b * real_end.x;
			out.y = factor_a * pStart.y + factor_b * real_end.y;
			out.z = factor_a * pStart.z + factor_b * real_end.z;
			out.w = factor_a * pStart.w + factor_b * real_end.w;

			return out;
#endif
		}

		void Quaternion::RotatePointByQuaternion(const Quaternion &quat, Vector3 &point)
		{
//#ifdef LUMOS_SSEQUAT
//			__m128 q = quat.mmvalue;
//				q = _mm_shuffle_ps(q, q, _MM_SHUFFLE(0, 3, 2, 1));
//#ifdef LUMOS_SSEVEC3
//			__m128 v = point.m_Value;
//#else
//			__m128 v = _mm_set_ps(0.f, rhs.z, rhs.y, rhs.x);
//#endif
//			const __m128 W = _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 3, 3, 3));
//			const __m128 a_yzx = _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 0, 2, 1));
//			__m128 x = _mm_mul_ps(q, _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 0, 2, 1)));
//			__m128 qxv = _mm_sub_ps(x, _mm_mul_ps(a_yzx, v));
//			__m128 Wv = _mm_mul_ps(W, v);
//			__m128 s = _mm_add_ps(qxv, _mm_shuffle_ps(Wv, Wv, _MM_SHUFFLE(3, 1, 0, 2)));
//			__m128 qs = _mm_mul_ps(q, s);
//			__m128 y = _mm_shuffle_ps(qs, qs, _MM_SHUFFLE(3, 1, 0, 2));
//			s = _mm_sub_ps(_mm_mul_ps(a_yzx, s), y);
//			s = _mm_add_ps(s, s);
//			s = _mm_add_ps(s, v);
//
//			Vector3 pos(
//				_mm_cvtss_f32(s),
//				_mm_cvtss_f32(_mm_shuffle_ps(s, s, _MM_SHUFFLE(1, 1, 1, 1))),
//				_mm_cvtss_f32(_mm_movehl_ps(s, s)));
//#else
			Vector3 qVec(quat.x, quat.y, quat.z);
			Vector3 cross1(qVec.Cross(point));
			Vector3 cross2(qVec.Cross(cross1));

			Vector3 pos = (point + ((cross1 * quat.w + cross2) * 2.0f));
//#endif
			point.SetX(pos.x);
			point.SetY(pos.y);
			point.SetZ(pos.z);
		}

		Quaternion Quaternion::LookAt(const Vector3& from, const Vector3& to, const Vector3& up)
		{
			const Vector3 resting_forward_vector = Vector3(0, 0, -1);

			Vector3 forward = (from - to);
			forward.Normalise();

			//Create look at rotation
			Quaternion out = GetRotation(resting_forward_vector, forward);

			//Correct rotation to use given up vector
			Vector3 up_l = out.Transform(up);
			Vector3 right = Vector3::Cross(forward, up);
			right.Normalise();
			Vector3 up_w = Vector3::Cross(right, forward);
			up_w.Normalise();
			Quaternion fix_spin = GetRotation(up_l, up_w);

			out = fix_spin * out;
			out.Normalise();

			return out;
		}

		Quaternion Quaternion::GetRotation(const Vector3& from_dir, const Vector3& to_dir, const Vector3& up)
		{
			float costheta = Vector3::Dot(from_dir, to_dir);


			if (fabs(costheta - 1.0f) < 0.001f)// 1e-6f)
			{
				return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
			}

			else if (fabs(costheta + 1.0f) < 0.001f)//1e-6f)
			{
				return Quaternion(up, PI);
			}

			float theta = acosf(costheta);
			Vector3 rotAxis = Vector3::Cross(from_dir, to_dir);
			rotAxis.Normalise();

			return Quaternion::AxisAngleToQuaterion(rotAxis, static_cast<float>(RadiansToDegrees(theta)));
		}

		Vector3 Quaternion::Transform(const Vector3& point) const
		{
			return Vector3(x, y, z) * (2.0f * Vector3::Dot(Vector3(x, y, z), point))
				+ point * (w * w - Vector3::Dot(Vector3(x, y, z), Vector3(x, y, z)))
				+ Vector3::Cross(Vector3(x, y, z), point) * (2.0f * w);

		}

		float Quaternion::Dot(const Quaternion& q) const
		{
#ifdef LUMOS_SSEQUAT
			__m128 n = _mm_mul_ps(mmvalue, q.mmvalue);
			n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(2, 3, 0, 1)));
			n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(0, 1, 2, 3)));
			return _mm_cvtss_f32(n);
#else
			return (x * q.x) + (y * q.y) + (z * q.z) + (w * q.w);
#endif
		}

		Quaternion Quaternion::Lerp(const Quaternion& start, const Quaternion& end, float factor)
		{
			// Interpolate the two quaternions
			//  - This is not recomended over spherical interpolation
			const float factor_a = 1.0f - factor;
			const float factor_b = factor;

			Quaternion out;
			out.x = factor_a * start.x + factor_b * end.x;
			out.y = factor_a * start.y + factor_b * end.y;
			out.z = factor_a * start.z + factor_b * end.z;
			out.w = factor_a * start.w + factor_b * end.w;

			return out;
		}

		float Quaternion::Dot(const Quaternion &a, const Quaternion &b)
		{
			return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
		}

		Quaternion Quaternion::Rotation(const Vector3& unitVec0, const Vector3& unitVec1)
		{
			float cosHalfAngleX2 = sqrt((2.0f * (1.0f + Vector3::Dot(unitVec1, unitVec0))));
			float recipCosHalfAngleX2 = (1.0f / cosHalfAngleX2);
			return Quaternion((Vector3::Cross(unitVec0, unitVec1) * recipCosHalfAngleX2), (cosHalfAngleX2 * 0.5f));
		}

		Quaternion Quaternion::Rotation(float radians, const Vector3 & unitVec)
		{
			float angle = radians * 0.5f;
			return Quaternion((unitVec * sin(angle)), cos(angle));
		}
		
		Quaternion Quaternion::Slerp(const Quaternion& start, const Quaternion& end, float factor)
		{
			//Clamp interpolation between start and end
			factor = Min(Max(factor, 0.0f), 1.0f);

			// Calc cos theta (Dot product)
			float cos_theta = Quaternion::Dot(start, end);

			// Quaternions can describe any rotation positively or negatively (e.g -90degrees is the same as 270degrees), however to interpolate
			// correctly we need /both/ quaternions to use the same coordinate System
			Quaternion real_end = end;
			if (cos_theta < 0.0f)
			{
				cos_theta = -cos_theta;
				real_end.x = -end.x;
				real_end.y = -end.y;
				real_end.z = -end.z;
				real_end.w = -end.w;
			}

			//If cosTheta is close to one, just do linear interpolation to avoid divide by zero and floating point issues 
			if (cos_theta > 1.0f - 1e-6f)
			{
				return Lerp(start, end, factor);
			}
			else
			{
				// Calculate interpolation coefficients
				float theta = acosf(cos_theta);			// extract theta from dot product's cos(theta)
				float sin_theta = sinf(theta);		// compute inverse rotation length -> 1.0f / sin(theta)


				float factor_a = sinf((1.0f - factor) * theta) / sin_theta;
				float factor_b = sinf(factor * theta) / sin_theta;


				// Interpolate the two quaternions
				Quaternion out;
				out.x = factor_a * start.x + factor_b * real_end.x;
				out.y = factor_a * start.y + factor_b * real_end.y;
				out.z = factor_a * start.z + factor_b * real_end.z;
				out.w = factor_a * start.w + factor_b * real_end.w;
				out.Normalise();
				return out;
			}
		}
    
        Quaternion Quaternion::operator*(float rhs) const
        {
#ifdef LUMOS_SSEQUAT
            return Quaternion(_mm_mul_ps(mmvalue, _mm_set1_ps(rhs)));
#else
            return Quaternion(x * rhs, y * rhs, z * rhs, w * rhs);
#endif
        }

		Quaternion Quaternion::operator*(const Quaternion& q) const
		{
//#ifdef LUMOS_SSEQUAT
//			__m128 q1 = mmvalue;
//			__m128 q2 = q.mmvalue;
//			q2 = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(0, 3, 2, 1));
//			const __m128 signy = _mm_castsi128_ps(_mm_set_epi32((int)0x80000000UL, (int)0x80000000UL, 0, 0));
//			const __m128 signx = _mm_shuffle_ps(signy, signy, _MM_SHUFFLE(2, 0, 2, 0));
//			const __m128 signz = _mm_shuffle_ps(signy, signy, _MM_SHUFFLE(3, 0, 0, 3));
//			__m128 out = _mm_mul_ps(_mm_shuffle_ps(q1, q1, _MM_SHUFFLE(1, 1, 1, 1)), _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(0, 1, 2, 3)));
//			out = _mm_add_ps(_mm_mul_ps(_mm_xor_ps(signy, _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(2, 2, 2, 2))), _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(1, 0, 3, 2))), _mm_xor_ps(signx, out));
//			out = _mm_add_ps(_mm_mul_ps(_mm_xor_ps(signz, _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(3, 3, 3, 3))), _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(2, 3, 0, 1))), out);
//			out = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(q1, q1, _MM_SHUFFLE(0, 0, 0, 0)), q2), out);
//			return Quaternion(_mm_shuffle_ps(out, out, _MM_SHUFFLE(2, 1, 0, 3)));
//#else
			Quaternion ans;

			ans.w = (w * q.w) - (x * q.x) - (y * q.y) - (z * q.z);
			ans.x = (x * q.w) + (w * q.x) + (y * q.z) - (z * q.y);
			ans.y = (y * q.w) + (w * q.y) + (z * q.x) - (x * q.z);
			ans.z = (z * q.w) + (w * q.z) + (x * q.y) - (y * q.x);

			return ans;
//#endif
		}

		Quaternion Quaternion::operator*(const Vector3& v) const
		{
			return Quaternion(
				(w * v.GetX()) + (v.GetY() * z) - (v.GetZ() * y),
				(w * v.GetY()) + (v.GetZ() * x) - (v.GetX() * z),
				(w * v.GetZ()) + (v.GetX() * y) - (v.GetY() * x),
				-(x * v.GetX()) - (y * v.GetY()) - (z * v.GetZ())
			);
        }

		Quaternion Quaternion::operator-() const
		{
#ifdef LUMOS_SSEQUAT1
			return Quaternion(_mm_xor_ps(mmvalue, _mm_castsi128_ps(_mm_set1_epi32((int)0x80000000UL))));
#else
			return Quaternion(-x, -y, -z, -w);
#endif
		}
  
		Quaternion Quaternion::operator+(const Quaternion& rhs) const
		{
#ifdef LUMOS_SSEQUAT
			return Quaternion(_mm_add_ps(mmvalue, rhs.mmvalue));
#else
			return Quaternion(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
#endif
		}

		Quaternion Quaternion::operator-(const Quaternion& rhs) const
		{
#ifdef LUMOS_SSEQUAT
			return Quaternion(_mm_sub_ps(mmvalue, rhs.mmvalue));
#else
			return Quaternion(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
#endif
		}

		Quaternion& Quaternion::operator=(const Quaternion& rhs) noexcept
		{
#ifdef LUMOS_SSEQUAT
			mmvalue = rhs.mmvalue;
#else
			w = rhs.w;
			x = rhs.x;
			y = rhs.y;
			z = rhs.z;
#endif
			return *this;
		}

        Quaternion& Quaternion::operator+=(const Quaternion& rhs)
        {
#ifdef LUMOS_SSEQUAT
            mmvalue = _mm_add_ps(mmvalue, rhs.mmvalue);
#else
            w += rhs.w;
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
#endif
            return *this;
        }

        Quaternion& Quaternion::operator*=(float rhs)
        {
#ifdef LUMOS_SSEQUAT
            mmvalue = _mm_mul_ps(mmvalue, _mm_set1_ps(rhs));
#else
            w *= rhs;
            x *= rhs;
            y *= rhs;
            z *= rhs;
#endif
            return *this;
        }

        bool Quaternion::operator==(const Quaternion& rhs) const
        {
#ifdef LUMOS_SSEQUAT
            __m128 c = _mm_cmpeq_ps(mmvalue, rhs.mmvalue);
            c = _mm_and_ps(c, _mm_movehl_ps(c, c));
            c = _mm_and_ps(c, _mm_shuffle_ps(c, c, _MM_SHUFFLE(1, 1, 1, 1)));
            return _mm_cvtsi128_si32(_mm_castps_si128(c)) == -1;
#else
            return w == rhs.w && x == rhs.x && y == rhs.y && z == rhs.z;
#endif
        }
    
        bool Quaternion::operator!=(const Quaternion& rhs) const
        {
            return !(*this == rhs);
        }

		std::istream &operator>>(std::istream &stream, Quaternion &q)
		{
			float x, y, z, w;
			char delim;
			stream >> delim >> x >> delim >> y >> delim >> z >> delim >> w >> delim;
			return stream;
		}
	}
}

namespace std 
{
	template<>
	struct hash<Lumos::Maths::Quaternion>
	{
		size_t operator()(const Lumos::Maths::Quaternion& value) const
		{
			return std::hash<float>()(value.x) ^ std::hash<float>()(value.y)
				^ std::hash<float>()(value.z) ^ std::hash<float>()(value.w);
		}
	};
}
