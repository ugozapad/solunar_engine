#ifndef D3D11SHADERPROGRAMMANAGER_H
#define D3D11SHADERPROGRAMMANAGER_H

#include "graphics/ShaderProgramManager.h"

namespace engine
{

class D3D11ShaderProgramManager : public ShaderProgramManager
{
public:
	D3D11ShaderProgramManager();
	~D3D11ShaderProgramManager();

	IShaderProgram* createShaderProgram(const char* vsfilename, const char* fsfilename, const char* defines = nullptr) override;

	void setShaderProgram(IShaderProgram* program) override;
	void deleteProgram(IShaderProgram* program) override;

private:
	std::vector<IShaderProgram*> m_cachedPrograms;

};

}

#endif // !D3D11SHADERPROGRAMMANAGER_H
