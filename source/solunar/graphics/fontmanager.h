#ifndef GRAPHICS_FONTMANAGER_H
#define GRAPHICS_FONTMANAGER_H

#include "graphics/ifontmanager.h"

#include "stb_truetype.h"

namespace solunar
{

class ITexture2D;
struct IBufferBase;
class IShaderProgram;
class ISamplerState;
struct IRasterizerState;
struct IDepthStencilState;
struct IBlendState;

struct FontVertex
{
	glm::vec2 position;
	glm::vec2 texcoord;
	glm::vec4 color;
};

// Limited to 1024 characters per one sentence
const int kMaxFontVBSize = sizeof(FontVertex)	* 1024 * 12;
const int kMaxFontIBSize = sizeof(uint16_t)		* 1024 * 12;

class FontManager;

class FontImpl : public IFont
{
	friend class FontManager;
public:
	FontImpl();
	~FontImpl();

	void Create(const char* filename, float size);
	void Destroy();

	// Inherited via IFont
	void DrawText(const char* text, float x, float y, const glm::vec4& color) override;

private:
	stbtt_bakedchar m_fontChars[96];
	ITexture2D* m_fontTexture;
};

class FontManager : public IFontManager
{
public:
	FontManager();
	~FontManager();
	
	void Initialize() override;
	void Shutdown() override;

	IFont* CreateFont(const char* filename, float size) override;

	void DrawFontText(IFont* font, const char* text, float x, float y, const glm::vec4& color) override;
	void DrawSystemFont(const char* text, float x, float y, const glm::vec4& color) override;

	void FlushPrimitives() override;

private:
	void InitPrivate();

private:
	template <int Capacity>
	struct FixedDrawString
	{
		char buf[Capacity];

		FixedDrawString()
		{
			buf[0] = '\0';
		}

		~FixedDrawString()
		{
			buf[0] = '\0';
		}

		FixedDrawString& operator=(const char* text)
		{
			Assert(strlen(text) <= Capacity);
			strncpy(buf, text, Capacity);
			return *this;
		}

		char operator[](size_t index)
		{
			Assert(index < Capacity);
			return buf[index];
		}

		const int length() const { return strlen(buf); }
		const char* c_str() const { return buf; }
	};

	typedef FixedDrawString<256> DrawString256;

	struct StringDrawInfo
	{
		FontImpl* m_font;
		DrawString256 m_string;
		float m_x;
		float m_y;
		glm::vec4 m_color;
	};

	std::vector<StringDrawInfo> m_drawStrings;

	std::unordered_map<std::string, FontImpl*> m_fonts;

	// System font
	IFont* m_systemFont;
	
	// Shared font data
	ISamplerState* m_textureSampler;
	IBufferBase* m_vertexBuffer;
	IBufferBase* m_indexBuffer;
	IBufferBase* m_constantBuffer;
	IShaderProgram* m_shaderProgram;
	IRasterizerState* m_rasterizerState;
	IDepthStencilState* m_depthStencilState;
	IBlendState* m_pBlendState;

};

}

#endif // !GRAPHICS_FONTMANAGER_H
