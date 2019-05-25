#include "LM.h"
#include "GLDebug.h"

#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Maths/MathsUtilities.h"

namespace lumos
{

	DebugDrawData* GLDebug::s_OrthoDebugData	   = nullptr;
	DebugDrawData* GLDebug::s_PerspectiveDebugData = nullptr;
	graphics::GLRenderer*    GLDebug::s_DebugDrawingRenderer = nullptr;

	Material*	   GLDebug::s_DebugDrawShader		= nullptr;
	bool		   GLDebug::s_DrawnDebugOrtho		= false;
	bool		   GLDebug::s_DrawnDebugPerspective = false;

	GLDebug::GLDebug()
	{
		s_DrawnDebugOrtho = false;
		s_DrawnDebugPerspective = false;
	}


	GLDebug::~GLDebug()
	{
		delete s_OrthoDebugData;
		delete s_PerspectiveDebugData;
		delete s_DebugDrawShader;
	}

	void GLDebug::SwapBuffers()
	{
		{
			if (!s_DrawnDebugOrtho)
			{
				DrawDebugOrtho();
			}

			if (!s_DrawnDebugPerspective)
			{
				DrawDebugPerspective();
			}
			s_DrawnDebugOrtho = false;
			s_DrawnDebugPerspective = false;
		}
	}

	void GLDebug::DrawDebugPerspective(maths::Matrix4* matrix)
	{
		if (matrix)
		{
			//s_DebugDrawShader->SetUniform("viewProjMatrix", &matrix);
			//glUniformMatrix4fv(glGetUniformLocation(debugDrawShader->GetProgram(), "viewProjMatrix"), 1, false, (float*)matrix);
		}
		else
		{
            maths::Matrix4 temp;// = projMatrix*viewMatrix;
			//s_DebugDrawShader->SetUniform("viewProjMatrix", temp);
			//glUniformMatrix4fv(glGetUniformLocation(debugDrawShader->GetProgram(), "viewProjMatrix"), 1, false, (float*)&temp);
		}

		//s_DebugDrawShader->Bind();
		s_PerspectiveDebugData->Draw();
		s_PerspectiveDebugData->Clear();
		s_DrawnDebugPerspective = true;
	}

	void GLDebug::DrawDebugOrtho(maths::Matrix4*matrix)
	{
		if (matrix)
		{
			//s_DebugDrawShader->SetUniform("viewProjMatrix", &matrix);
			//glUniformMatrix4fv(glGetUniformLocation(debugDrawShader->GetProgram(), "viewProjMatrix"), 1, false, (float*)matrix);
		}
		else 
		{
			//static maths::Matrix4 ortho = maths::Matrix4::Orthographic(-1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f);
			//s_DebugDrawShader->SetUniform("viewProjMatrix", ortho);
			//glUniformMatrix4fv(glGetUniformLocation(debugDrawShader->GetProgram(), "viewProjMatrix"), 1, false, (float*)&ortho);
		}

		//s_DebugDrawShader->Bind();
		s_OrthoDebugData->Draw();

		s_OrthoDebugData->Clear();
		s_DrawnDebugOrtho = true;
	}

	void GLDebug::DrawDebugLine(const DebugDrawMode mode, const maths::Vector3 &from, const maths::Vector3 &to, const maths::Vector3 &fromColour, const maths::Vector3 &toColour)
	{
		DebugDrawData*target = (mode == DEBUGDRAW_ORTHO ? target = s_OrthoDebugData : target = s_PerspectiveDebugData);

		target->AddLine(from, to, fromColour, toColour);
	}

	void GLDebug::DrawDebugBox(const DebugDrawMode mode, const maths::Vector3 &at, const maths::Vector3 &scale, const maths::Vector3 &colour)
	{
		DebugDrawData*target = (mode == DEBUGDRAW_ORTHO ? target = s_OrthoDebugData : target = s_PerspectiveDebugData);

		target->AddLine(at + maths::Vector3(-scale.GetX()  * 0.5f, scale.GetY() * 0.5f, 0),
			at + maths::Vector3(-scale.GetX() * 0.5f, -scale.GetY() * 0.5f, 0), colour, colour);

		target->AddLine(at + maths::Vector3(-scale.GetX() * 0.5f, -scale.GetY() * 0.5f, 0),
			at + maths::Vector3(scale.GetX() * 0.5f, -scale.GetY() * 0.5f, 0), colour, colour);

		target->AddLine(at + maths::Vector3(scale.GetX() * 0.5f, -scale.GetY() * 0.5f, 0),
			at + maths::Vector3(scale.GetX() * 0.5f, scale.GetY() * 0.5f, 0), colour, colour);

		target->AddLine(at + maths::Vector3(scale.GetX() * 0.5f, scale.GetY() * 0.5f, 0),
			at + maths::Vector3(-scale.GetX() * 0.5f, scale.GetY() * 0.5f, 0), colour, colour);
	}

	void GLDebug::DrawDebugCross(const DebugDrawMode mode, const maths::Vector3 &at, const maths::Vector3 &scale, const maths::Vector3 &colour)
	{
		DebugDrawData*target = (mode == DEBUGDRAW_ORTHO ? target = s_OrthoDebugData : target = s_PerspectiveDebugData);

		target->AddLine(at + maths::Vector3(-scale.GetX() * 0.5f, -scale.GetY() * 0.5f, 0),
			at + maths::Vector3(scale.GetX() * 0.5f, scale.GetY() * 0.5f, 0), colour, colour);

		target->AddLine(at + maths::Vector3(scale.GetX() * 0.5f, -scale.GetY() * 0.5f, 0),
			at + maths::Vector3(-scale.GetX() * 0.5f, scale.GetY() * 0.5f, 0), colour, colour);
	}

	void GLDebug::DrawDebugCircle(const DebugDrawMode mode, const maths::Vector3 &at, const float radius, const maths::Vector3 &colour)
	{
		DebugDrawData*target = (mode == DEBUGDRAW_ORTHO ? target = s_OrthoDebugData : target = s_PerspectiveDebugData);

		const int stepCount = 18;
		const float divisor = 360.0f / stepCount;

		for (int i = 0; i < stepCount; ++i)
		{
			const float startx = radius * static_cast<float>(cos(maths::DegToRad(i * divisor))) + at.GetX();
			const float endx = radius * static_cast<float>(cos(maths::DegToRad((i + 1) * divisor))) + at.GetX();

			const float starty = radius * static_cast<float>(sin(maths::DegToRad(i * divisor))) + at.GetY();
			const float endy = radius * static_cast<float>(sin(maths::DegToRad((i + 1) * divisor))) + at.GetY();

			target->AddLine(maths::Vector3(startx, starty, at.GetZ()),
                            maths::Vector3(endx, endy, at.GetZ()), colour, colour);
		}
	}

	DebugDrawData::DebugDrawData()
	{
		glGenVertexArrays(1, &array);
		glGenBuffers(2, buffers);
	}

	void DebugDrawData::Draw()
	{
		if (lines.empty())
		{
			return;
		}
		glBindVertexArray(array);
		glGenBuffers(2, buffers);

		glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
		glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(maths::Vector3), &lines[0], GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
		glBufferData(GL_ARRAY_BUFFER, colours.size() * sizeof(maths::Vector3), &colours[0], GL_DYNAMIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		glEnableVertexAttribArray(1);

		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size()));

		glBindVertexArray(0);
		glDeleteBuffers(2, buffers);

		Clear();
	}
}
