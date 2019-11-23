#include "lmpch.h"
#include "GLDebug.h"

#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Maths/Maths.h"

namespace Lumos
{

	DebugDrawData* GLDebug::s_OrthoDebugData	   = nullptr;
	DebugDrawData* GLDebug::s_PerspectiveDebugData = nullptr;
	Graphics::GLRenderer*    GLDebug::s_DebugDrawingRenderer = nullptr;

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

	void GLDebug::DrawDebugPerspective(Maths::Matrix4* matrix)
	{
		if (matrix)
		{
			//s_DebugDrawShader->SetUniform("viewProjMatrix", &matrix);
			//glUniformMatrix4fv(glGetUniformLocation(debugDrawShader->GetProgram(), "viewProjMatrix"), 1, false, (float*)matrix);
		}
		else
		{
            Maths::Matrix4 temp;// = projMatrix*viewMatrix;
			//s_DebugDrawShader->SetUniform("viewProjMatrix", temp);
			//glUniformMatrix4fv(glGetUniformLocation(debugDrawShader->GetProgram(), "viewProjMatrix"), 1, false, (float*)&temp);
		}

		//s_DebugDrawShader->Bind();
		s_PerspectiveDebugData->Draw();
		s_PerspectiveDebugData->Clear();
		s_DrawnDebugPerspective = true;
	}

	void GLDebug::DrawDebugOrtho(Maths::Matrix4*matrix)
	{
		if (matrix)
		{
			//s_DebugDrawShader->SetUniform("viewProjMatrix", &matrix);
			//glUniformMatrix4fv(glGetUniformLocation(debugDrawShader->GetProgram(), "viewProjMatrix"), 1, false, (float*)matrix);
		}
		else 
		{
			//static Maths::Matrix4 ortho = Maths::Matrix4::Orthographic(-1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f);
			//s_DebugDrawShader->SetUniform("viewProjMatrix", ortho);
			//glUniformMatrix4fv(glGetUniformLocation(debugDrawShader->GetProgram(), "viewProjMatrix"), 1, false, (float*)&ortho);
		}

		//s_DebugDrawShader->Bind();
		s_OrthoDebugData->Draw();

		s_OrthoDebugData->Clear();
		s_DrawnDebugOrtho = true;
	}

	void GLDebug::DrawDebugLine(const DebugDrawMode mode, const Maths::Vector3 &from, const Maths::Vector3 &to, const Maths::Vector3 &fromColour, const Maths::Vector3 &toColour)
	{
		DebugDrawData*target = (mode == DEBUGDRAW_ORTHO ? target = s_OrthoDebugData : target = s_PerspectiveDebugData);

		target->AddLine(from, to, fromColour, toColour);
	}

	void GLDebug::DrawDebugBox(const DebugDrawMode mode, const Maths::Vector3 &at, const Maths::Vector3 &scale, const Maths::Vector3 &colour)
	{
		DebugDrawData*target = (mode == DEBUGDRAW_ORTHO ? target = s_OrthoDebugData : target = s_PerspectiveDebugData);

		target->AddLine(at + Maths::Vector3(-scale.x  * 0.5f, scale.y * 0.5f, 0),
			at + Maths::Vector3(-scale.x * 0.5f, -scale.y * 0.5f, 0), colour, colour);

		target->AddLine(at + Maths::Vector3(-scale.x * 0.5f, -scale.y * 0.5f, 0),
			at + Maths::Vector3(scale.x * 0.5f, -scale.y * 0.5f, 0), colour, colour);

		target->AddLine(at + Maths::Vector3(scale.x * 0.5f, -scale.y * 0.5f, 0),
			at + Maths::Vector3(scale.x * 0.5f, scale.y * 0.5f, 0), colour, colour);

		target->AddLine(at + Maths::Vector3(scale.x * 0.5f, scale.y * 0.5f, 0),
			at + Maths::Vector3(-scale.x * 0.5f, scale.y * 0.5f, 0), colour, colour);
	}

	void GLDebug::DrawDebugCross(const DebugDrawMode mode, const Maths::Vector3 &at, const Maths::Vector3 &scale, const Maths::Vector3 &colour)
	{
		DebugDrawData*target = (mode == DEBUGDRAW_ORTHO ? target = s_OrthoDebugData : target = s_PerspectiveDebugData);

		target->AddLine(at + Maths::Vector3(-scale.x * 0.5f, -scale.y * 0.5f, 0),
			at + Maths::Vector3(scale.x * 0.5f, scale.y * 0.5f, 0), colour, colour);

		target->AddLine(at + Maths::Vector3(scale.x * 0.5f, -scale.y * 0.5f, 0),
			at + Maths::Vector3(-scale.x * 0.5f, scale.y * 0.5f, 0), colour, colour);
	}

	void GLDebug::DrawDebugCircle(const DebugDrawMode mode, const Maths::Vector3 &at, const float radius, const Maths::Vector3 &colour)
	{
		DebugDrawData*target = (mode == DEBUGDRAW_ORTHO ? target = s_OrthoDebugData : target = s_PerspectiveDebugData);

		const int stepCount = 18;
		const float divisor = 360.0f / stepCount;

		for (int i = 0; i < stepCount; ++i)
		{
			const float startx = radius * static_cast<float>(cos(Maths::ToRadians(i * divisor))) + at.x;
			const float endx = radius * static_cast<float>(cos(Maths::ToRadians((i + 1) * divisor))) + at.x;

			const float starty = radius * static_cast<float>(sin(Maths::ToRadians(i * divisor))) + at.y;
			const float endy = radius * static_cast<float>(sin(Maths::ToRadians((i + 1) * divisor))) + at.y;

			target->AddLine(Maths::Vector3(startx, starty, at.z),
                            Maths::Vector3(endx, endy, at.z), colour, colour);
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
		glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(Maths::Vector3), &lines[0], GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
		glBufferData(GL_ARRAY_BUFFER, colours.size() * sizeof(Maths::Vector3), &colours[0], GL_DYNAMIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		glEnableVertexAttribArray(1);

		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size()));

		glBindVertexArray(0);
		glDeleteBuffers(2, buffers);

		Clear();
	}
}
