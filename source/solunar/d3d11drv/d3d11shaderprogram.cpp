#include "d3d11drv_pch.h"
#include "d3d11drv/d3d11shaderprogram.h"
#include "d3d11drv/d3d11shaderprogrammanager.h"

#include "core/file/contentmanager.h"

//#define CREATE_INPUT_LAYOUT_FROM_REFLECT

namespace solunar
{

class D3D11Include : public ID3DInclude
{
public:
	STDMETHOD(Open)(THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes);
	STDMETHOD(Close)(THIS_ LPCVOID pData);
};

// https://www.asawicki.info/news_1515_implementing_id3d10include

STDMETHODIMP D3D11Include::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
	std::string includeFilename = g_shaderManager->GetShaderPath();
	includeFilename += "/";
	includeFilename += pFileName;

	DataStreamPtr stream = g_contentManager->OpenStream(includeFilename);
	if (!stream)
		return E_FAIL;

	stream->Seek(Seek_End, 0);
	long length = stream->Tell();
	stream->Seek(Seek_Begin, 0);

	if (!length)
		return E_FAIL;

	void* fileBuffer = malloc(length);
	stream->Read(fileBuffer, length);

	*ppData = fileBuffer;
	*pBytes = (UINT)length;

	return S_OK;
}

STDMETHODIMP D3D11Include::Close(LPCVOID pData)
{
	if (!pData)
		return E_FAIL;

	free((void*)pData);

	return S_OK;
}

D3D11Include g_d3d11Include;

std::vector<std::string> split(const char* str, char c = ' ')
{
	std::vector<std::string> result;

	do
	{
		const char* begin = str;

		while (*str != c && *str)
			str++;

		result.push_back(std::string(begin, str));
	} while (0 != *str++);

	return result;
}

ID3DBlob* CreateShaderFromText(const char* text, ShaderType shaderType, const char* defines = nullptr)
{
	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorTextBlob = nullptr;

	const char* entryPoint = (shaderType == ShaderType_Vertex) ? "VSMain" : "PSMain";
	const char* shaderTarget = (shaderType == ShaderType_Vertex) ? "vs_4_0" : "ps_4_0";

	std::vector<std::string> shaderMacroStrings;
	if (defines && strlen(defines) > 0)
	{
		shaderMacroStrings = split(defines, '\n');

		/*std::string definesStr = defines;

		size_t iterator = definesStr.find_first_of('\n');
		while (iterator != std::string::npos) {
			std::string exactDefineStr = definesStr.substr(0, iterator);
			Core::Msg("CreateShaderFromText: %s", exactDefineStr.c_str());
			shderMacroStrings.push_back(exactDefineStr);
			iterator = definesStr.find_first_of('\n', iterator);
		}*/
	}

	std::vector<D3D_SHADER_MACRO> shaderMacro;
	for (auto& it : shaderMacroStrings)
		shaderMacro.push_back(D3D_SHADER_MACRO{ it.c_str(), NULL });

	shaderMacro.push_back(D3D_SHADER_MACRO{ NULL, NULL });

#ifndef FINAL_BUILD
	HRESULT hr = D3DCompile(text, strlen(text), "UNKNOWED", defines ? shaderMacro.data() : NULL,
		&g_d3d11Include, entryPoint, shaderTarget, D3DCOMPILE_DEBUG, 0, &shaderBlob, &errorTextBlob);
#else
	HRESULT hr = D3DCompile(text, strlen(text), "UNKNOWED", defines ? shaderMacro.data() : NULL,
		&g_d3d11Include, entryPoint, shaderTarget, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &shaderBlob, &errorTextBlob);
#endif // !FINAL_BUILD
	
	if (FAILED(hr))
	{
		std::string errorText = "Failed to compile shader!";

		errorText += " ";
		errorText += shaderTarget;
		errorText += " ";

		if (errorTextBlob)
		{
			errorText += " ";
			errorText += "Reason: ";
			errorText += (const char*)errorTextBlob->GetBufferPointer();
			errorText += " ";

			errorTextBlob->Release();
			errorTextBlob = nullptr;
		}
		else
		{
			errorText += " ";
			errorText += "Reason: Unknown";
			errorText += " ";
		}

		Core::Msg("CreateShaderFromText: %s", errorText.c_str());

		//std::terminate();
		//DebugBreak();
	}

	if (errorTextBlob)
	{
		errorTextBlob->Release();
		errorTextBlob = nullptr;
	}

	return shaderBlob;
}

inline D3D11_INPUT_CLASSIFICATION GetD3DInputClassification(InputAligment inputAligment)
{
	switch (inputAligment)
	{
	case INPUT_PER_VERTEX_DATA:
		return D3D11_INPUT_PER_VERTEX_DATA;
	case INPUT_PER_INSTANCE_DATA:
		return D3D11_INPUT_PER_INSTANCE_DATA;
	}

	Assert(0);
	return (D3D11_INPUT_CLASSIFICATION)0;
}

D3D11ShaderProgram::D3D11ShaderProgram(D3D11Device* device, 
	const char* vstext, 
	const char* pstext,
	const char* defines /*= nullptr*/,
	InputLayoutDesc* inputLayout /*= nullptr*/, 
	int inputLayoutCount /*= 0*/) :
	m_inputLayout(nullptr),
	m_vertexShader(nullptr),
	m_pixelShader(nullptr)
{
	Create(device, vstext, pstext, defines, inputLayout, inputLayoutCount);
}

D3D11ShaderProgram::~D3D11ShaderProgram()
{
	Destroy();
}

void D3D11ShaderProgram::Create(D3D11Device* device, const char* vstext, const char* pstext, const char* defines /*= nullptr*/, InputLayoutDesc* inputLayout /*= nullptr*/, int inputLayoutCount /*= 0*/)
{
	ID3DBlob* vertexShaderBlob = CreateShaderFromText(vstext, ShaderType_Vertex, defines);
	device->GetDevice()->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), NULL, &m_vertexShader);

	if (inputLayout && inputLayoutCount > 0)
	{
		std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;

		for (int i = 0; i < inputLayoutCount; i++)
		{
			D3D11_INPUT_ELEMENT_DESC inputElementDesc	= {};
			inputElementDesc.SemanticName				= inputLayout[i].m_semanticName;
			inputElementDesc.SemanticIndex				= inputLayout[i].m_semanticIndex;
			inputElementDesc.Format						= GetDXGIFormat(inputLayout[i].m_format);
			inputElementDesc.InputSlot					= inputLayout[i].m_inputSlot;
			inputElementDesc.AlignedByteOffset			= inputLayout[i].m_alignedByteOffset;
			inputElementDesc.InstanceDataStepRate		= inputLayout[i].m_instanceDataStepRate;
			inputElementDesc.InputSlotClass				= GetD3DInputClassification(inputLayout[i].m_inputSlotClass);
			inputLayoutDesc.push_back(inputElementDesc);
		}

		device->GetDevice()->CreateInputLayout(&inputLayoutDesc[0],
			(UINT)inputLayoutDesc.size(),
			vertexShaderBlob->GetBufferPointer(),
			vertexShaderBlob->GetBufferSize(),
			&m_inputLayout);
	}
#ifdef CREATE_INPUT_LAYOUT_FROM_REFLECT
	else
	{
		CreateInputLayout(device, vertexShaderBlob);
	}
#else
	Assert2(inputLayout, "Failed to create shader input layout");
#endif // CREATE_INPUT_LAYOUT_FROM_REFLECT

	vertexShaderBlob->Release();
	vertexShaderBlob = nullptr;

	ID3DBlob* pixelShaderBlob = CreateShaderFromText(pstext, ShaderType_Pixel, defines);
	device->GetDevice()->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), NULL, &m_pixelShader);

	pixelShaderBlob->Release();
	pixelShaderBlob = nullptr;
}

void D3D11ShaderProgram::Destroy()
{
	if (m_pixelShader)
	{
		m_pixelShader->Release();
		m_pixelShader = nullptr;
	}

	if (m_inputLayout)
	{
		m_inputLayout->Release();
		m_inputLayout = nullptr;
	}

	if (m_vertexShader)
	{
		m_vertexShader->Release();
		m_vertexShader = nullptr;
	}
}

void D3D11ShaderProgram::CreateInputLayout(D3D11Device* device, ID3DBlob* vertexShaderBlob)
{
	// #TODO: convert engine inputLayouts to D3D11 input layout

	// Reflect shader info
	ID3D11ShaderReflection* shaderReflection = nullptr;
	D3DReflect(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), __uuidof(ID3D11ShaderReflection), (void**)&shaderReflection);

	// Get shader info
	D3D11_SHADER_DESC shaderDesc;
	shaderReflection->GetDesc(&shaderDesc);

	// Read input layout description from shader info
	uint32_t byteOffset = 0;
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;
	for (uint32_t i = 0; i < shaderDesc.InputParameters; i++)
	{
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
		shaderReflection->GetInputParameterDesc(i, &paramDesc);

		// fill out input element desc
		D3D11_INPUT_ELEMENT_DESC elementDesc;
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = 0;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = byteOffset;
		elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		// determine DXGI format
		if (paramDesc.Mask == 1)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
			byteOffset += 4;
		}
		else if (paramDesc.Mask <= 3)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
			byteOffset += 8;
		}
		else if (paramDesc.Mask <= 7)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			byteOffset += 12;
		}
		else if (paramDesc.Mask <= 15)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			byteOffset += 16;
		}

		//save element desc
		inputLayoutDesc.push_back(elementDesc);
	}

	device->GetDevice()->CreateInputLayout(&inputLayoutDesc[0], 
		(UINT)inputLayoutDesc.size(), 
		vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(), 
		&m_inputLayout);

	shaderReflection->Release();
}

void D3D11ShaderProgram::SetDebugName(const char* vsname, const char* psname)
{
	D3D11_CHECK(m_vertexShader->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(vsname), vsname));
	D3D11_CHECK(m_pixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(psname), psname));
}

}