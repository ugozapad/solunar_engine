#include "gldrv_pch.h"
#include "gldrv/glshaderprogram.h"
#include "core/file/contentmanager.h"

namespace engine
{

GLuint createShader(GLenum target, const char* filename, const char* defines = nullptr)
{
	std::string content;

	DataStreamPtr f = g_contentManager->openStream(filename);

	f->seek(Seek_End, 0);
	size_t fileLength = f->tell();
	f->seek(Seek_Begin, 0);

	content.resize(fileLength + 1);
	f->read((void*)content.data(), fileLength);
	content[fileLength] = '\0';

	if (defines && strlen(defines) > 0) {
		size_t versionLocation = content.find("#version 330 core");

		size_t definesStringLength = versionLocation + strlen("#version 330 core") + 2;
		std::string newContentString = content.substr(0, definesStringLength);

		newContentString.append(defines);
		newContentString.append(content.substr(definesStringLength));

		content = newContentString;
	}

	const char* contentCStr = content.c_str();

	GLuint shader = glCreateShader(target);
	glShaderSource(shader, 1, &contentCStr, NULL);
	glCompileShader(shader);

	int  success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		Core::msg("=== SHADER COMPILE ERROR ===");
		Core::msg("%s", contentCStr);
		Core::msg("============================");
		Core::error("createShader: failed to compile shader %s\n%s", filename, infoLog);
	}

	Core::msg("[graphics]: created %s shader from file %s", (target == GL_VERTEX_SHADER) ? "vertex" : "fragment", filename);

	return shader;
}

GLShaderProgram::GLShaderProgram(const std::string& vsfilename, const std::string& fsfilename)
{
	m_defines = "";

	GLuint vertex_shader = createShader(GL_VERTEX_SHADER, vsfilename.c_str());
	GLuint fragment_shader = createShader(GL_FRAGMENT_SHADER, fsfilename.c_str());

	m_program = glCreateProgram();
	glAttachShader(m_program, vertex_shader);
	glAttachShader(m_program, fragment_shader);
	glLinkProgram(m_program);

	glDeleteShader(fragment_shader);
	glDeleteShader(vertex_shader);

	int  success;
	char infoLog[512];
	glGetProgramiv(m_program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(m_program, 512, NULL, infoLog);
		Core::error("IShaderProgram::IShaderProgram: failed to link program %s", infoLog);
	}
}

GLShaderProgram::GLShaderProgram(const std::string& vsfilename, const std::string& fsfilename, const char* defines)
{
	m_defines = defines ? defines : "";

	GLuint vertex_shader = createShader(GL_VERTEX_SHADER, vsfilename.c_str(), defines);
	GLuint fragment_shader = createShader(GL_FRAGMENT_SHADER, fsfilename.c_str(), defines);

	m_program = glCreateProgram();
	glAttachShader(m_program, vertex_shader);
	glAttachShader(m_program, fragment_shader);
	glLinkProgram(m_program);

	glDeleteShader(fragment_shader);
	glDeleteShader(vertex_shader);

	int  success;
	char infoLog[512];
	glGetProgramiv(m_program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(m_program, 512, NULL, infoLog);
		Core::error("IShaderProgram::IShaderProgram: failed to link program %s", infoLog);
	}
}

GLShaderProgram::~GLShaderProgram()
{
	glDeleteProgram(m_program);
}

GLuint GLShaderProgram::getProgramhandle()
{
	return m_program;
}

}