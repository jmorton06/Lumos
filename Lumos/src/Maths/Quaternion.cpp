#include "LM.h"
#include "Quaternion.h"
#include "MathsUtilities.h"

namespace Lumos
{
	namespace maths
	{
#ifdef LUMOS_SSEQUAT
		const Quaternion Quaternion::EMPTY = Quaternion(_mm_setzero_ps());
		const Quaternion Quaternion::IDENTITY = Quaternion(_mm_set_ps(1.0f, 0, 0, 0));


		Matrix4 Quaternion::ToMatrix4() const
		{
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

		Quaternion Quaternion::EulerAnglesToQuaternion(float pitch, float yaw, float roll)
		{
			float y2 = DegToRad(yaw / 2.0f);
			float p2 = DegToRad(pitch / 2.0f);
			float r2 = DegToRad(roll / 2.0f);

			float cosy = cos(y2);
			float cosp = cos(p2);
			float cosr = cos(r2);

			float siny = sin(y2);
			float sinp = sin(p2);
			float sinr = sin(r2);

			Quaternion q;

			q.x = sinr * cosp * siny + cosr * sinp * cosy;
			q.y = cosr * cosp * siny - sinr * sinp * cosy;
			q.z = sinr * cosp * cosy - cosr * sinp * siny;
			q.w = cosr * cosp * cosy + sinr * sinp * siny;

			return q;
		};

		Quaternion Quaternion::AxisAngleToQuaterion(const Vector3& vector, float degrees)
		{
			float theta = DegToRad(degrees);
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

		//TODO: Convert to sse
		Quaternion Quaternion::Inverse() const
		{
			Quaternion q = Conjugate();

			float m = q.Magnitude();
			m *= m;

			if (m == 0.0f)
				m = 1.0f;
			else
				m = 1.0f / m;

			return Quaternion(q.x * m, q.y * m, q.z * m, q.w * m);
		}

		float Quaternion::Magnitude() const
		{
			return sqrt(x * x + y * y + z * z + w * w);
		}
		//

		Quaternion Quaternion::FromMatrix(const Matrix4& m)
		{
			Quaternion q;

			q.w = sqrt(std::max(0.0f, (1.0f + m.values[0] + m.values[5] + m.values[10]))) * 0.5f;
			q.x = sqrt(std::max(0.0f, (1.0f + m.values[0] - m.values[5] - m.values[10]))) * 0.5f;
			q.y = sqrt(std::max(0.0f, (1.0f - m.values[0] + m.values[5] - m.values[10]))) * 0.5f;
			q.z = sqrt(std::max(0.0f, (1.0f - m.values[0] - m.values[5] + m.values[10]))) * 0.5f;

			q.x = static_cast<float>(copysign(q.x, m.values[9] - m.values[6]));
			q.y = static_cast<float>(copysign(q.y, m.values[2] - m.values[8]));
			q.z = static_cast<float>(copysign(q.z, m.values[4] - m.values[1]));

			return q;
		}

		Quaternion Quaternion::Interpolate(const Quaternion& pStart, const Quaternion& pEnd, float pFactor) const
		{
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
		}

		void Quaternion::RotatePointByQuaternion(const Quaternion &q, Vector3 &point)
		{
			const Quaternion inv(q.Inverse());
			Quaternion pos(point.GetX(), point.GetY(), point.GetZ(), 0.0f);

			pos = pos * inv;
			pos = q * pos;

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

			return Quaternion::AxisAngleToQuaterion(rotAxis, static_cast<float>(RadToDeg(theta)));
		}

		Vector3 Quaternion::Transform(const Vector3& point) const
		{
			return Vector3(x, y, z) * (2.0f * Vector3::Dot(Vector3(x, y, z), point))
				+ point * (w * w - Vector3::Dot(Vector3(x, y, z), Vector3(x, y, z)))
				+ Vector3::Cross(Vector3(x, y, z), point) * (2.0f * w);

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


		Quaternion Quaternion::Slerp(const Quaternion& start, const Quaternion& end, float factor)
		{
			//Clamp interpolation between start and end
			factor = Min(Max(factor, 0.0f), 1.0f);

			// Calc cos theta (Dot product)
			float cos_theta = Quaternion::Dot(start, end);

			// Quaternions can describe any rotation positively or negatively (e.g -90degrees is the same as 270degrees), however to interpolate
			// correctly we need /both/ quaternions to use the same coordinate system
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

#else
		Quaternion::Quaternion()
		{
			x = y = z = 0.0f;
			w = 1.0f;
		}

		Quaternion::Quaternion(float x, float y, float z, float w)
			: x(x)
			, y(y)
			, z(z)
			, w(w)
		{
		}

		Quaternion::Quaternion(const Vector3& xyz, float w)
			: x(xyz.GetX())
			, y(xyz.GetY())
			, z(xyz.GetZ())
			, w(w)
		{
		}

		Quaternion::~Quaternion()
		{
		}

		float Quaternion::Dot(const Quaternion &a, const Quaternion &b)
		{
			return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
		}

		void Quaternion::Normalise()
		{
			float magnitude = sqrt(Dot(*this, *this));

			if (magnitude > 0.0f)
			{
				float t = 1.0f / magnitude;

				x *= t;
				y *= t;
				z *= t;
				w *= t;
			}
		}

		Quaternion Quaternion::operator*(const Quaternion &b) const
		{
			Quaternion ans;

			ans.w = (w * b.w) - (x * b.x) - (y * b.y) - (z * b.z);
			ans.x = (x * b.w) + (w * b.x) + (y * b.z) - (z * b.y);
			ans.y = (y * b.w) + (w * b.y) + (z * b.x) - (x * b.z);
			ans.z = (z * b.w) + (w * b.z) + (x * b.y) - (y * b.x);

			return ans;
		}

		Quaternion Quaternion::operator*(const Vector3 &b) const
		{
			Quaternion ans;

			ans.w = -(x * b.GetX()) - (y * b.GetY()) - (z * b.GetZ());

			ans.x = (w * b.GetX()) + (b.GetY() * z) - (b.GetZ() * y);
			ans.y = (w * b.GetY()) + (b.GetZ() * x) - (b.GetX() * z);
			ans.z = (w * b.GetZ()) + (b.GetX() * y) - (b.GetY() * x);

			return ans;
		}

		Matrix4 Quaternion::ToMatrix4() const
		{
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

		Quaternion Quaternion::EulerAnglesToQuaternion(float pitch, float yaw, float roll)
		{
			float y2 = static_cast<float>(maths::DegToRad(yaw / 2.0f));
			float p2 = static_cast<float>(maths::DegToRad(pitch / 2.0f));
			float r2 = static_cast<float>(maths::DegToRad(roll / 2.0f));

			float cosy = static_cast<float>(cos(y2));
			float cosp = static_cast<float>(cos(p2));
			float cosr = static_cast<float>(cos(r2));

			float siny = static_cast<float>(sin(y2));
			float sinp = static_cast<float>(sin(p2));
			float sinr = static_cast<float>(sin(r2));

			Quaternion q;

			q.x = cosr * sinp * cosy + sinr * cosp * siny;
			q.y = cosr * cosp * siny - sinr * sinp * cosy;
			q.z = sinr * cosp * cosy - cosr * sinp * siny;
			q.w = cosr * cosp * cosy + sinr * sinp * siny;

			return q;
		};

		Quaternion Quaternion::AxisAngleToQuaterion(const Vector3 &vector, float degrees)
		{
			float theta = static_cast<float>(maths::DegToRad(degrees));
			float result = static_cast<float>(sin(theta / 2.0f));

			Quaternion q =
				Quaternion(static_cast<float>(vector.GetX() * result), static_cast<float>(vector.GetY() * result), static_cast<float>(vector.GetZ() * result), static_cast<float>(cos(theta / 2.0f)));
			q.Normalise();
			return q;
		}

		void Quaternion::RotatePointByQuaternion(const Quaternion &q, Vector3 &point)
		{
			const Quaternion inv(q.Inverse());
			Quaternion pos(point.GetX(), point.GetY(), point.GetZ(), 0.0f);

			pos = pos * inv;
			pos = q * pos;

			point.SetX(pos.x);
			point.SetY(pos.y);
			point.SetZ(pos.z);
		}

		void Quaternion::GenerateW()
		{
			w = 1.0f - (x * x) - (y * y) - (z * z);
			if (w < 0.0f)
			{
				w = 0.0f;
			}
			else
			{
				w = -sqrt(w);
			}
		}

		Quaternion Quaternion::Conjugate() const
		{
			return Quaternion(-x, -y, -z, w);
		}

		Quaternion Quaternion::Inverse() const
		{
			Quaternion q = Conjugate();

			float m = q.Magnitude();
			m *= m;

			if (m == 0.0f)
				m = 1.0f;
			else
				m = 1.0f / m;

			return Quaternion(q.x * m, q.y * m, q.z * m, q.w * m);
		}

		float Quaternion::Magnitude() const
		{
			return sqrt(x * x + y * y + z * z + w * w);
		}

		Quaternion Quaternion::FromMatrix(const Matrix4 &m)
		{
			Quaternion q;

			q.w = sqrt(maths::Max(0.0f, (1.0f + m.values[0] + m.values[5] + m.values[10]))) / 2;
			q.x = sqrt(maths::Max(0.0f, (1.0f + m.values[0] - m.values[5] - m.values[10]))) / 2;
			q.y = sqrt(maths::Max(0.0f, (1.0f - m.values[0] + m.values[5] - m.values[10]))) / 2;
			q.z = sqrt(maths::Max(0.0f, (1.0f - m.values[0] - m.values[5] + m.values[10]))) / 2;

			q.x = static_cast<float>(copysign(q.x, m.values[9] - m.values[6]));
			q.y = static_cast<float>(copysign(q.y, m.values[2] - m.values[8]));
			q.z = static_cast<float>(copysign(q.z, m.values[4] - m.values[1]));

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

		Quaternion Quaternion::Interpolate(const Quaternion &start, const Quaternion &end, float factor) const
		{
			// Clamp interpolation between start and end
			factor = maths::Min(maths::Max(factor, 0.0f), 1.0f);

			// Calc cos theta (Dot product)
			float cos_theta = Quaternion::Dot(start, end);

			// Quaternions can describe any rotation positively or negatively, however to interpolate
			// correctly we need /both/ quaternions to use the same coordinate system
			Quaternion real_end = end;
			if (cos_theta < 0.0f)
			{
				cos_theta = -cos_theta;
				real_end.x = -end.x;
				real_end.y = -end.y;
				real_end.z = -end.z;
				real_end.w = -end.w;
			}

			// Calculate interpolation coefficients
			float theta = acosf(cos_theta);    // extract theta from dot product's cos(theta)
			float inv_sin_theta = sinf(theta); // compute inverse rotation length 1.0f / sin(theta)

			if (fabs(inv_sin_theta) < 1e-6f)
				inv_sin_theta = 1e-6f;
			inv_sin_theta = 1.0f / inv_sin_theta;

			float factor_a = sinf((1.0f - factor) * theta) * inv_sin_theta;
			float factor_b = sinf(factor * theta) * inv_sin_theta;

			// Interpolate the two quaternions
			Quaternion out;
			out.x = factor_a * start.x + factor_b * real_end.x;
			out.y = factor_a * start.y + factor_b * real_end.y;
			out.z = factor_a * start.z + factor_b * real_end.z;
			out.w = factor_a * start.w + factor_b * real_end.w;

			return out;
		}

		std::ostream &operator<<(std::ostream &stream, const Quaternion &q)
		{
			stream << '[' << q.x << ',' << q.y << ',' << q.z << ',' << q.w << ']';
			return stream;
		}

		std::istream &operator>>(std::istream &stream, Quaternion &q)
		{
			float x, y, z, w;
			char delim;
			stream >> delim >> x >> delim >> y >> delim >> z >> delim >> w >> delim;
			q = Quaternion(x, y, z, w);
			return stream;
		}

		Vector3 Quaternion::Rotate(const Quaternion& quat, const Vector3& vec)
		{
			float tmpX = (((quat.w * vec.GetX()) + (quat.y * vec.GetZ())) - (quat.z * vec.GetY()));
			float tmpY = (((quat.w * vec.GetY()) + (quat.z * vec.GetX())) - (quat.x * vec.GetZ()));
			float tmpZ = (((quat.w * vec.GetZ()) + (quat.x * vec.GetY())) - (quat.y * vec.GetX()));
			float tmpW = (((quat.x * vec.GetX()) + (quat.y * vec.GetY())) + (quat.z * vec.GetZ()));
			return Vector3(
				((((tmpW * quat.x) + (tmpX * quat.w)) - (tmpY * quat.z)) + (tmpZ * quat.y)),
				((((tmpW * quat.y) + (tmpY * quat.w)) - (tmpZ * quat.x)) + (tmpX * quat.z)),
				((((tmpW * quat.z) + (tmpZ * quat.w)) - (tmpX * quat.y)) + (tmpY * quat.x))
			);
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

		Quaternion Quaternion::Slerp(const Quaternion& start, const Quaternion& end, float factor)
		{
			//Clamp interpolation between start and end
			factor = maths::Min(maths::Max(factor, 0.0f), 1.0f);

			// Calc cos theta (Dot product)
			float cos_theta = Quaternion::Dot(start, end);

			// Quaternions can describe any rotation positively or negatively (e.g -90degrees is the same as 270degrees), however to interpolate
			// correctly we need /both/ quaternions to use the same coordinate system
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

		Quaternion Quaternion::GetRotation(const Vector3& from_dir, const Vector3& to_dir, const Vector3& up)
		{
			float costheta = Vector3::Dot(from_dir, to_dir);

			//Edge cases preventing theta extraction:
			// - Same as default, no rotation
			if (fabs(costheta - 1.0f) < 1e-6f)
			{
				return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
			}
			// - Directly opposite default rotation
			else if (fabs(costheta + 1.0f) < 1e-6f)
			{
				return Quaternion(up, maths::PI);
			}


			//Otherwise build a new rotation
			float theta = acosf(costheta);
			Vector3 rotAxis = Vector3::Cross(from_dir, to_dir);
			rotAxis.Normalise();

			return Quaternion::AxisAngleToQuaterion(rotAxis, (float)maths::RadToDeg(theta));
		}

		Vector3 Quaternion::Transform(const Vector3& point) const
		{
			return Vector3(x, y, z) * (2.0f * Vector3::Dot(Vector3(x, y, z), point))
				+ point * (w * w - Vector3::Dot(Vector3(x, y, z), Vector3(x, y, z)))
				+ Vector3::Cross(Vector3(x, y, z), point) * (2.0f * w);

		}

#endif
	}
}

namespace std 
{
	template<>
	struct hash<Lumos::maths::Quaternion>
	{
		size_t operator()(const Lumos::maths::Quaternion& value) const
		{
			return std::hash<float>()(value.x) ^ std::hash<float>()(value.y)
				^ std::hash<float>()(value.z) ^ std::hash<float>()(value.w);
		}
	};
}
