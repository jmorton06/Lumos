#include "LM.h"
#include "Matrix4.h"
#include "Matrix3.h"
#include "MathsUtilities.h"

namespace Lumos {
	namespace maths {
#ifdef LUMOS_SSEMAT4

		Matrix4::Matrix4(const Matrix3 &mat) {
			const unsigned int size = 3 * sizeof(float);
			memcpy(&values[0], &mat.values[0], size);
			memcpy(&values[4], &mat.values[3], size);
			memcpy(&values[8], &mat.values[6], size);
			values[3] = values[7] = values[12] = values[13] = values[14] = 0.0f;
			values[15] = 1.0f;
		}


		Matrix4::Matrix4(const Matrix4 &mat) {
			memcpy(&values[0], &mat.values[0], sizeof(Matrix4));
		}

		Matrix4 Matrix4::operator*(const Matrix4 &m) const {
			//http://fhtr.blogspot.co.uk/2010/02/4x4-float-matrix-multiplication-using.html
			Matrix4 result;
			result.ToZero();
			for (int i = 0; i < 4; ++i)
				for (int j = 0; j < 4; ++j)
					result.mmvalues[j] = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(m.values[j * 4 + i]), mmvalues[i]),
													result.mmvalues[j]);
			return result;
		}

		Vector3 Matrix4::operator*(const Vector3 &v) const {
#define Test //TODO : Fix
#ifdef Test
			Vector3 vec;

			vec.SetX(v.GetX() * values[0] + v.GetY() * values[4] + v.GetZ() * values[8] + values[12]);
			vec.SetY(v.GetX() * values[1] + v.GetY() * values[5] + v.GetZ() * values[9] + values[13]);
			vec.SetZ(v.GetX() * values[2] + v.GetY() * values[6] + v.GetZ() * values[10] + values[14]);

			const float temp = v.GetX() * values[3] + v.GetY() * values[7] + v.GetZ() * values[11] + values[15];

			vec.SetX(vec.GetX() / temp);
			vec.SetY(vec.GetY() / temp);
			vec.SetZ(vec.GetZ() / temp);

			return vec;

			/*Vector4 vec4 = Vector4(v,1.0f);

            vec4 = *this * vec4;
            return vec4.ToVector3();*/
#else
            __m128 m0 = _mm_mul_ps(mmvalues[0], _mm_set1_ps(v.GetX()));
            __m128 m1 = _mm_mul_ps(mmvalues[1], _mm_set1_ps(v.GetY()));
            __m128 m2 = _mm_mul_ps(mmvalues[2], _mm_set1_ps(v.GetZ()));

#ifdef LUMOS_SSEVEC3
            __m128 tempValue = _mm_add_ps(_mm_add_ps(m0, m1), _mm_add_ps(m2, mmvalues[3]));
            return _mm_div_ps(tempValue, _mm_shuffle_ps(tempValue, tempValue, _MM_SHUFFLE(0, 0, 0, 0)));
#else
            __m128 tempValue = _mm_add_ps(_mm_add_ps(m0, m1), _mm_add_ps(m2, mmvalues[3]));
            tempValue = _mm_div_ps(tempValue, _mm_shuffle_ps(tempValue, tempValue, _MM_SHUFFLE(0, 0, 0, 0)));
            return Vector3(GetValue(tempValue,0), GetValue(tempValue,1), GetValue(tempValue,2));
#endif

#endif
		};


		Vector4 Matrix4::operator*(const Vector4 &v) const {
			__m128 m0 = _mm_mul_ps(mmvalues[0], _mm_set1_ps(v.GetX()));
			__m128 m1 = _mm_mul_ps(mmvalues[1], _mm_set1_ps(v.GetY()));
			__m128 m2 = _mm_mul_ps(mmvalues[2], _mm_set1_ps(v.GetZ()));
			__m128 m3 = _mm_mul_ps(mmvalues[3], _mm_set1_ps(v.GetW()));

#ifdef LUMOS_SSEVEC4
			return Vector4(_mm_add_ps(_mm_add_ps(m0, m1), _mm_add_ps(m2, m3)));
#else
            __m128 tempValue = _mm_add_ps(_mm_add_ps(m0, m1), _mm_add_ps(m2, m3));
            return Vector4(GetValue(tempValue,0), GetValue(tempValue,1), GetValue(tempValue,2), GetValue(tempValue,3));
#endif
		};

		void Matrix4::Transpose() {
			_MM_TRANSPOSE4_PS(mmvalues[0], mmvalues[1], mmvalues[2], mmvalues[3]);
		}

		Matrix4 Matrix4::GetRotation() const {
			Matrix4 result(*this);
			result.SetPositionVector(Vector3(0.0f, 0.0f, 0.0f));
			result.values[3] = result.values[7] = result.values[11] = 0.0f;
			result.values[15] = 1.0f;
			return result;
		}

//http://www.flipcode.com/documents/matrfaq.html#Q37
		Vector3 Matrix4::GetEulerAngles(const Matrix4 &mat) {
			float angle_x = 0.0f;
			float angle_y = -asin(mat.values[8]);
			float angle_z = 0.0f;

			float c = cos(angle_y);
			angle_y = Lumos::maths::RadToDeg(angle_y);

			if (fabs(c) > 0.005) {
				c = 1.0f / c;
				float tr_x = mat.values[10] * c;
				float tr_y = -mat.values[9] * c;

				angle_x = Lumos::maths::RadToDeg(atan2(tr_y, tr_x));

				tr_x = mat.values[0] * c;
				tr_y = -mat.values[4] * c;

				angle_z = Lumos::maths::RadToDeg(atan2(tr_y, tr_x));
			} else {
				float tr_x = mat.values[5];
				float tr_y = mat.values[1];

				angle_z = atan2(tr_y, tr_x);
			}

			return Vector3(Lumos::maths::Clamp(angle_x, 0.0f, 360.0f), Lumos::maths::Clamp(angle_y, 0.0f, 360.0f),
						   Lumos::maths::Clamp(angle_z, 0.0f, 360.0f));
		}

		Matrix4 Matrix4::Perspective(float znear, float zfar, float aspect, float fov) {
			Matrix4 m;
			const float h = 1.0f / tan(fov * Lumos::maths::PI_OVER_360);
			float neg_depth_r = 1.0f / (znear - zfar);

			m.values[0] = h / aspect;
			m.values[5] = h;
			m.values[10] = (zfar + znear) * neg_depth_r;
			m.values[11] = -1.0f;
			m.values[14] = 2.0f * (znear * zfar) * neg_depth_r;
			m.values[15] = 0.0f;

			return m;
		}

//http://www.opengl.org/sdk/docs/man/xhtml/glOrtho.xml
		Matrix4 Matrix4::Orthographic(float znear, float zfar, float right, float left, float top, float bottom) {
			Matrix4 m;
			float x_r = 1.0f / (right - left);
			float y_r = 1.0f / (top - bottom);
			float z_r = 1.0f / (zfar - znear);

			m.values[0] = 2.0f * x_r;
			m.values[5] = 2.0f * y_r;
			m.values[10] = -2.0f * z_r;

			m.values[12] = -(right + left) * x_r;
			m.values[13] = -(top + bottom) * y_r;
			m.values[14] = -(zfar + znear) * z_r;
			m.values[15] = 1.0f;

			return m;
		}

		Matrix4 Matrix4::BuildViewMatrix(const Vector3 &from, const Vector3 &lookingAt, const Vector3 &up) {
			Matrix4 r;
			r.SetPositionVector(Vector3(-from.GetX(), -from.GetY(), -from.GetZ()));

			Matrix4 m;

			Vector3 f = (lookingAt - from);
			f.Normalise();

			Vector3 s = Vector3::Cross(f, up);
			Vector3 u = Vector3::Cross(s, f);

			s.Normalise();
			u.Normalise();

			m.values[0] = s.GetX();
			m.values[4] = s.GetY();
			m.values[8] = s.GetZ();

			m.values[1] = u.GetX();
			m.values[5] = u.GetY();
			m.values[9] = u.GetZ();

			m.values[2] = -f.GetX();
			m.values[6] = -f.GetY();
			m.values[10] = -f.GetZ();

			return m * r;
		}

		Matrix4 Matrix4::RotationX(float degrees) {
			Matrix4 m;
			float rad = Lumos::maths::DegToRad(degrees);
			float c = cos(rad);
			float s = sin(rad);

			m.values[5] = c;
			m.values[6] = s;

			m.values[9] = -s;
			m.values[10] = c;

			return m;
		}

		Matrix4 Matrix4::RotationY(float degrees) {
			Matrix4 m;
			float rad = Lumos::maths::DegToRad(degrees);
			float c = cos(rad);
			float s = sin(rad);

			m.values[0] = c;
			m.values[2] = s;

			m.values[8] = -s;
			m.values[10] = c;

			return m;
		}

		Matrix4 Matrix4::RotationZ(float degrees) {
			Matrix4 m;
			float rad = Lumos::maths::DegToRad(degrees);
			float c = cos(rad);
			float s = sin(rad);

			m.values[0] = c;
			m.values[1] = -s;

			m.values[4] = s;
			m.values[5] = c;

			return m;
		}

		Matrix4 Matrix4::Rotation(float degrees, const Vector3 &axis) {
			Vector3 axisNorm = axis;
			axisNorm.Normalise();

			float rad = Lumos::maths::DegToRad(degrees);
			float c = cos(rad);
			float s = sin(rad);

			__m128 normXYZW = _mm_set_ps(0, axisNorm.GetZ(), axisNorm.GetY(), axisNorm.GetX());
			__m128 normXYZWWithC = _mm_mul_ps(normXYZW, _mm_set1_ps(1.0f - c));
			__m128 col0 = _mm_mul_ps(normXYZW, _mm_set1_ps(GetValue(normXYZWWithC, 0)));
			__m128 col1 = _mm_mul_ps(normXYZW, _mm_set1_ps(GetValue(normXYZWWithC, 1)));
			__m128 col2 = _mm_mul_ps(normXYZW, _mm_set1_ps(GetValue(normXYZWWithC, 2)));

			Matrix4 m;
			m.values[0] = GetValue(col0, 0) + c;
			m.values[1] = GetValue(col0, 1) + (axisNorm.GetZ() * s);
			m.values[2] = GetValue(col0, 2) - (axisNorm.GetY() * s);
			m.values[4] = GetValue(col1, 0) - (axisNorm.GetZ() * s);
			m.values[5] = GetValue(col1, 1) + c;
			m.values[6] = GetValue(col1, 2) + (axisNorm.GetX() * s);
			m.values[8] = GetValue(col2, 0) + (axisNorm.GetY() * s);
			m.values[9] = GetValue(col2, 1) - (axisNorm.GetX() * s);
			m.values[10] = GetValue(col2, 2) + c;

			return m;
		}

		Matrix4 Matrix4::Rotation(float degreesX, float degreesY, float degreesZ) {
			// Building this matrix directly is faster than multiplying three matrices for X, Y and Z
			float phi = Lumos::maths::DegToRad(degreesX), theta = Lumos::maths::DegToRad(degreesY), psi = Lumos::maths::DegToRad(
					degreesZ);
			float sinTh = sin(theta), cosTh = cos(theta),
					sinPh = sin(phi), cosPh = cos(phi),
					sinPs = sin(psi), cosPs = cos(psi);

			Matrix4 result;
			result.values[0] = cosTh * cosPs;
			result.values[1] = cosTh * sinPs;
			result.values[2] = -sinTh;
			result.values[4] = -cosPh * sinPs + sinPh * sinTh * cosPs;
			result.values[5] = cosPh * cosPs + sinPh * sinTh * sinPs;
			result.values[6] = sinPh * cosTh;
			result.values[8] = sinPh * sinPs + cosPh * sinTh * cosPs;
			result.values[9] = -sinPh * cosPs + cosPh * sinTh * sinPs;
			result.values[10] = cosPh * cosTh;
			return result;
		}

		Matrix4 Matrix4::Scale(const Vector3 &scale) {
			Matrix4 m;
			m.values[0] = scale.GetX();
			m.values[5] = scale.GetY();
			m.values[10] = scale.GetZ();
			return m;
		}

		Matrix4 Matrix4::Translation(const Vector3 &translation) {
			Matrix4 m;
			m.values[12] = translation.GetX();
			m.values[13] = translation.GetY();
			m.values[14] = translation.GetZ();
			return m;
		}

		std::ostream &operator<<(std::ostream &o, const Matrix4 &m) {
			return o << "Mat4(" << "/n" <<
					 "\t" << m.values[0] << ", " << m.values[4] << ", " << m.values[8] << ", " << m.values[12] << ", "
					 << "/n" <<
					 "\t" << m.values[1] << ", " << m.values[5] << ", " << m.values[9] << ", " << m.values[13] << ", "
					 << "/n" <<
					 "\t" << m.values[2] << ", " << m.values[6] << ", " << m.values[10] << ", " << m.values[14] << ", "
					 << "/n" <<
					 "\t" << m.values[3] << ", " << m.values[7] << ", " << m.values[11] << ", " << m.values[15] << "/n"
					 <<
					 " )";
		}

#else

        Matrix4::Matrix4(const float elements[16])
        {
            memcpy(this->values, elements, 16 * sizeof(float));
        }

        Matrix4::Matrix4(const Matrix3& mat33)
        {
            const unsigned int size = 3 * sizeof(float);
            memcpy(&values[0], &mat33.values[0], size);
            memcpy(&values[4], &mat33.values[3], size);
            memcpy(&values[8], &mat33.values[6], size);
        }

        Matrix4::~Matrix4()
        {
        }

        void Matrix4::ToIdentity()
        {
            ToZero();
            values[0] = 1.0f;
            values[5] = 1.0f;
            values[10] = 1.0f;
            values[15] = 1.0f;
        }

        void Matrix4::ToZero()
        {
            for (int i = 0; i < 16; i++)
            {
                values[i] = 0.0f;
            }
        }

        Vector3 Matrix4::GetPositionVector() const
        {
            return Vector3(values[12], values[13], values[14]);
        }

        void Matrix4::SetPositionVector(const Vector3 in)
        {
            values[12] = in.GetX();
            values[13] = in.GetY();
            values[14] = in.GetZ();
        }

        Vector3 Matrix4::GetScalingVector() const
        {
            return Vector3(values[0], values[5], values[10]);
        }

        void Matrix4::SetScalingVector(const Vector3 &in)
        {
            values[0] = in.GetX();
            values[5] = in.GetY();
            values[10] = in.GetZ();
        }

        Matrix4 Matrix4::Perspective(float znear, float zfar, float aspect, float fov)
        {
            Matrix4 m;

            const float h = 1.0f / tan(fov* maths::PI_OVER_360);
            float neg_depth = znear - zfar;

            m.values[0] = h / aspect;
            m.values[5] = h;
            m.values[10] = (zfar + znear) / neg_depth;
            m.values[11] = -1.0f;
            m.values[14] = 2.0f*(znear*zfar) / neg_depth;
            m.values[15] = 0.0f;

            return m;
        }

        //http://www.opengl.org/sdk/docs/man/xhtml/glOrtho.xml
        Matrix4 Matrix4::Orthographic(float znear, float zfar, float right, float left, float top, float bottom)
        {
            Matrix4 m;

            m.values[0] = 2.0f / (right - left);
            m.values[5] = 2.0f / (top - bottom);
            m.values[10] = -2.0f / (zfar - znear);

            m.values[12] = -(right + left) / (right - left);
            m.values[13] = -(top + bottom) / (top - bottom);
            m.values[14] = -(zfar + znear) / (zfar - znear);
            m.values[15] = 1.0f;

            return m;
        }

        Matrix4 Matrix4::BuildViewMatrix(const Vector3 &from, const Vector3 &lookingAt, const Vector3 up /*= Vector3(1,0,0)*/)
        {
            Matrix4 r;
            r.SetPositionVector(Vector3(-from.GetX(), -from.GetY(), -from.GetZ()));

            Matrix4 m;

            Vector3 f = (lookingAt - from);
            f.Normalise();

            Vector3 s = Vector3::Cross(f, up);
            Vector3 u = Vector3::Cross(s, f);

            s.Normalise();
            u.Normalise();

            m.values[0] = s.GetX();
            m.values[4] = s.GetY();
            m.values[8] = s.GetZ();

            m.values[1] = u.GetX();
            m.values[5] = u.GetY();
            m.values[9] = u.GetZ();

            m.values[2] = -f.GetX();
            m.values[6] = -f.GetY();
            m.values[10] = -f.GetZ();

            return m*r;
        }

        Matrix4 Matrix4::RotationX(float degrees)
        {
            Matrix4 m;
            float rad = maths::DegToRad(degrees);
            float c = cos(rad);
            float s = sin(rad);

            m.values[5] = c;
            m.values[6] = s;

            m.values[9] = -s;
            m.values[10] = c;

            return m;
        }

        Matrix4 Matrix4::RotationY(float degrees)
        {
            Matrix4 m;
            float rad = maths::DegToRad(degrees);
            float c = cos(rad);
            float s = sin(rad);

            m.values[0] = c;
            m.values[2] = s;

            m.values[8] = -s;
            m.values[10] = c;

            return m;
        }

        Matrix4 Matrix4::RotationZ(float degrees)
        {
            Matrix4 m;
            float rad = maths::DegToRad(degrees);
            float c = cos(rad);
            float s = sin(rad);

            m.values[0] = c;
            m.values[1] = -s;

            m.values[4] = s;
            m.values[5] = c;

            return m;
        }

        Matrix4 Matrix4::Rotation(float degrees, const Vector3 &inaxis)
        {
            Matrix4 m;

            Vector3 axis = inaxis;

            axis.Normalise();

            float c = cos(static_cast<float>(maths::DegToRad(degrees)));
            float s = sin(static_cast<float>(maths::DegToRad(degrees)));

            m.values[0] = (axis.GetX() * axis.GetX()) * (1.0f - c) + c;
            m.values[1] = (axis.GetY() * axis.GetX()) * (1.0f - c) + (axis.GetZ() * s);
            m.values[2] = (axis.GetZ() * axis.GetX()) * (1.0f - c) - (axis.GetY() * s);

            m.values[4] = (axis.GetX() * axis.GetY()) * (1.0f - c) - (axis.GetZ() * s);
            m.values[5] = (axis.GetY() * axis.GetY()) * (1.0f - c) + c;
            m.values[6] = (axis.GetZ() * axis.GetY()) * (1.0f - c) + (axis.GetX() * s);

            m.values[8] = (axis.GetX() * axis.GetZ()) * (1.0f - c) + (axis.GetY() * s);
            m.values[9] = (axis.GetY() * axis.GetZ()) * (1.0f - c) - (axis.GetX() * s);
            m.values[10] = (axis.GetZ() * axis.GetZ()) * (1.0f - c) + c;

            return m;
        }

        void Matrix4::Transpose()
        {
        }

        Matrix4 Matrix4::Scale(const Vector3 &scale)
        {
            Matrix4 m;

            m.values[0] = scale.GetX();
            m.values[5] = scale.GetY();
            m.values[10] = scale.GetZ();

            return m;
        }

        Matrix4 Matrix4::Translation(const Vector3 &translation)
        {
            Matrix4 m;

            m.values[12] = translation.GetX();
            m.values[13] = translation.GetY();
            m.values[14] = translation.GetZ();

            return m;
        }

        Matrix4 Matrix4::GetRotation() const
        {
            Matrix4 temp;
            temp.values[0] = values[0];
            temp.values[5] = values[5];
            temp.values[10] = values[10];
            temp.values[1] = values[1];
            temp.values[4] = values[4];
            temp.values[2] = values[2];
            temp.values[8] = values[8];
            temp.values[6] = values[6];
            temp.values[9] = values[9];
            return temp;
        }

        Matrix4 Matrix4::GetTransposedRotation() const
        {
            Matrix4 temp;
            temp.values[0] = values[0];
            temp.values[5] = values[5];
            temp.values[10] = values[10];
            temp.values[1] = values[4];
            temp.values[4] = values[1];
            temp.values[2] = values[8];
            temp.values[8] = values[2];
            temp.values[6] = values[9];
            temp.values[9] = values[6];
            return temp;
        }

#endif

	}
}
namespace std 
{
	template<>
	struct hash<Lumos::maths::Matrix4>
	{
		size_t operator()(const Lumos::maths::Matrix4& value) const
		{
			return std::hash<Lumos::maths::Vector4>()(Lumos::maths::Vector4(value.values[0], value.values[1], value.values[2], value.values[3])) ^ std::hash<Lumos::maths::Vector4>()(Lumos::maths::Vector4(value.values[4], value.values[5], value.values[6], value.values[7]))
				^ std::hash<Lumos::maths::Vector4>()(Lumos::maths::Vector4(value.values[8], value.values[9], value.values[10], value.values[11])) ^ std::hash<Lumos::maths::Vector4>()(Lumos::maths::Vector4(value.values[12], value.values[13], value.values[14], value.values[15]));
		}
	};
}
