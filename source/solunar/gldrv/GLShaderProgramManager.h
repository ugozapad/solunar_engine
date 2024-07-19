#ifndef GLSHADERMANAGER_H
#define GLSHADERMANAGER_H

#include "graphics/ShaderProgramManager.h"

namespace engine
{

class GLShaderProgramManager : public ShaderProgramManager
{
public:
	GLShaderProgramManager();
	~GLShaderProgramManager();

	IShaderProgram* createShaderProgram(
		const char* vsfilename, 
		const char* fsfilename,
		const char* defines = nullptr,
		InputLayoutDesc* inputLayout = nullptr,
		int inputLayoutCount = 0) override;

	void setShaderProgram(IShaderProgram* program) override;
	void deleteProgram(IShaderProgram* program) override;
};

}

#endif