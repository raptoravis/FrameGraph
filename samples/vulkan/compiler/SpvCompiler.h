// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Vulkan/VulkanDevice.h"
#include "stl/Algorithms/ArrayUtils.h"

// glslang includes
#include "glslang/glslang/Public/ShaderLang.h"
#include "glslang/Include/ResourceLimits.h"

using namespace FG;


class SpvCompiler
{
private:
	mutable Array<uint>		_tempBuf;

public:
	SpvCompiler ();
	~SpvCompiler ();

	bool Compile (OUT VkShaderModule&	shaderModule,
				  const VulkanDevice&	device,
				  const char *			source,
				  const char *			entry,
				  EShLanguage			shaderType,
				  glslang::EShTargetLanguageVersion	spvVersion	= glslang::EShTargetSpv_1_3,
				  bool					autoMapLocations	= true) const;

	bool Compile (OUT Array<uint>&		spirvData,
				  const char *			source,
				  const char *			entry,
				  EShLanguage			shaderType,
				  glslang::EShTargetLanguageVersion	spvVersion	= glslang::EShTargetSpv_1_3,
				  bool					autoMapLocations	= true) const;
};