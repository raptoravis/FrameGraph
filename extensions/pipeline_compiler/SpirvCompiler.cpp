// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "SpirvCompiler.h"
#include "PrivateDefines.h"
#include "stl/include/ToString.h"


// glslang includes
#ifdef COMPILER_MSVC
# pragma warning (push, 1)
# pragma warning (disable: 4100)
#endif

#include "glslang/glslang/Include/revision.h"
#include "glslang/glslang/Public/ShaderLang.h"
#include "glslang/glslang/OSDependent/osinclude.h"
#include "glslang/glslang/MachineIndependent/localintermediate.h"
#include "glslang/glslang/Include/intermediate.h"
#include "glslang/SPIRV/doc.h"
#include "glslang/SPIRV/disassemble.h"
#include "glslang/SPIRV/GlslangToSpv.h"
#include "glslang/SPIRV/GLSL.std.450.h"

#ifdef COMPILER_MSVC
# pragma warning (pop)
#endif


namespace FG
{
	using namespace glslang;


	struct SpirvCompiler::GLSLangResult
	{
		TProgram				prog;
		UniquePtr< TShader >	shader;
	};
	

/*
=================================================
	constructor
=================================================
*/
	SpirvCompiler::SpirvCompiler () :
		_currentStage{ EShaderStages::Unknown }
	{
		glslang::InitializeProcess();
	}
	
/*
=================================================
	destructor
=================================================
*/
	SpirvCompiler::~SpirvCompiler ()
	{
		glslang::FinalizeProcess();
	}
	
/*
=================================================
	SetCompilationFlags
=================================================
*/
	bool SpirvCompiler::SetCompilationFlags (EShaderCompilationFlags flags)
	{
		_compilerFlags = flags;
		return true;
	}
	
/*
=================================================
	Compile
=================================================
*/
	bool SpirvCompiler::Compile (EShader shaderType, EShaderLangFormat srcShaderFmt, EShaderLangFormat dstShaderFmt,
								StringView entry, StringView source,
								OUT PipelineDescription::Shader &outShader, OUT ShaderReflection &outReflection, OUT String &log)
	{
		using SpirvShaderData = PipelineDescription::SharedShaderPtr<Array<uint>>;

		log.clear();
		COMP_CHECK_ERR( (dstShaderFmt & EShaderLangFormat::_StorageFormatMask) == EShaderLangFormat::SPIRV );

		switch ( shaderType )
		{
			case EShader::Vertex :			_currentStage = EShaderStages::Vertex;			break;
			case EShader::TessControl :		_currentStage = EShaderStages::TessControl;		break;
			case EShader::TessEvaluation :	_currentStage = EShaderStages::TessEvaluation;	break;
			case EShader::Geometry :		_currentStage = EShaderStages::Geometry;		break;
			case EShader::Fragment :		_currentStage = EShaderStages::Fragment;		break;
			case EShader::Compute :			_currentStage = EShaderStages::Compute;			break;
			default :						COMP_RETURN_ERR( "unsupported shader type!" );
		}

		GLSLangResult	glslang_data;
		COMP_CHECK_ERR( _ParseGLSL( shaderType, srcShaderFmt, dstShaderFmt, entry, source, OUT glslang_data, INOUT log ) );

		Array<uint>		spirv;
		COMP_CHECK_ERR( _CompileSPIRV( glslang_data, OUT spirv, INOUT log ) );

		COMP_CHECK_ERR( _BuildReflection( glslang_data, OUT outReflection ) );

		outShader.shaderType	= shaderType;
		outShader.specConstants	= outReflection.specConstants;
		outShader.AddShaderData( dstShaderFmt, entry, std::move(spirv) );
		return true;
	}

/*
=================================================
	GenerateResources
=================================================
*/
	static void GenerateResources (OUT TBuiltInResource& resources)
	{
		resources.maxLights = 32;
		resources.maxClipPlanes = 6;
		resources.maxTextureUnits = 32;
		resources.maxTextureCoords = 32;
		resources.maxVertexAttribs = 64;
		resources.maxVertexUniformComponents = 4096;
		resources.maxVaryingFloats = 64;
		resources.maxVertexTextureImageUnits = 32;
		resources.maxCombinedTextureImageUnits = 80;
		resources.maxTextureImageUnits = 32;
		resources.maxFragmentUniformComponents = 4096;
		resources.maxDrawBuffers = 32;
		resources.maxVertexUniformVectors = 128;
		resources.maxVaryingVectors = 8;
		resources.maxFragmentUniformVectors = 16;
		resources.maxVertexOutputVectors = 16;
		resources.maxFragmentInputVectors = 15;
		resources.minProgramTexelOffset = -8;
		resources.maxProgramTexelOffset = 7;
		resources.maxClipDistances = 8;
		resources.maxComputeWorkGroupCountX = 65535;
		resources.maxComputeWorkGroupCountY = 65535;
		resources.maxComputeWorkGroupCountZ = 65535;
		resources.maxComputeWorkGroupSizeX = 1024;
		resources.maxComputeWorkGroupSizeY = 1024;
		resources.maxComputeWorkGroupSizeZ = 64;
		resources.maxComputeUniformComponents = 1024;
		resources.maxComputeTextureImageUnits = 16;
		resources.maxComputeImageUniforms = 8;
		resources.maxComputeAtomicCounters = 8;
		resources.maxComputeAtomicCounterBuffers = 1;
		resources.maxVaryingComponents = 60;
		resources.maxVertexOutputComponents = 64;
		resources.maxGeometryInputComponents = 64;
		resources.maxGeometryOutputComponents = 128;
		resources.maxFragmentInputComponents = 128;
		resources.maxImageUnits = 8;
		resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
		resources.maxImageSamples = 0;
		resources.maxVertexImageUniforms = 0;
		resources.maxTessControlImageUniforms = 0;
		resources.maxTessEvaluationImageUniforms = 0;
		resources.maxGeometryImageUniforms = 0;
		resources.maxFragmentImageUniforms = 8;
		resources.maxCombinedImageUniforms = 8;
		resources.maxGeometryTextureImageUnits = 16;
		resources.maxGeometryOutputVertices = 256;
		resources.maxGeometryTotalOutputComponents = 1024;
		resources.maxGeometryUniformComponents = 1024;
		resources.maxGeometryVaryingComponents = 64;
		resources.maxTessControlInputComponents = 128;
		resources.maxTessControlOutputComponents = 128;
		resources.maxTessControlTextureImageUnits = 16;
		resources.maxTessControlUniformComponents = 1024;
		resources.maxTessControlTotalOutputComponents = 4096;
		resources.maxTessEvaluationInputComponents = 128;
		resources.maxTessEvaluationOutputComponents = 128;
		resources.maxTessEvaluationTextureImageUnits = 16;
		resources.maxTessEvaluationUniformComponents = 1024;
		resources.maxTessPatchComponents = 120;
		resources.maxPatchVertices = 32;
		resources.maxTessGenLevel = 64;
		resources.maxViewports = 16;
		resources.maxVertexAtomicCounters = 0;
		resources.maxTessControlAtomicCounters = 0;
		resources.maxTessEvaluationAtomicCounters = 0;
		resources.maxGeometryAtomicCounters = 0;
		resources.maxFragmentAtomicCounters = 8;
		resources.maxCombinedAtomicCounters = 8;
		resources.maxAtomicCounterBindings = 1;
		resources.maxVertexAtomicCounterBuffers = 0;
		resources.maxTessControlAtomicCounterBuffers = 0;
		resources.maxTessEvaluationAtomicCounterBuffers = 0;
		resources.maxGeometryAtomicCounterBuffers = 0;
		resources.maxFragmentAtomicCounterBuffers = 1;
		resources.maxCombinedAtomicCounterBuffers = 1;
		resources.maxAtomicCounterBufferSize = 16384;
		resources.maxTransformFeedbackBuffers = 4;
		resources.maxTransformFeedbackInterleavedComponents = 64;
		resources.maxCullDistances = 8;
		resources.maxCombinedClipAndCullDistances = 8;
		resources.maxSamples = 4;

		resources.limits.nonInductiveForLoops = 1;
		resources.limits.whileLoops = 1;
		resources.limits.doWhileLoops = 1;
		resources.limits.generalUniformIndexing = 1;
		resources.limits.generalAttributeMatrixVectorIndexing = 1;
		resources.limits.generalVaryingIndexing = 1;
		resources.limits.generalSamplerIndexing = 1;
		resources.limits.generalVariableIndexing = 1;
		resources.limits.generalConstantMatrixVectorIndexing = 1;
	}
	
/*
=================================================
	ConvertShaderType
=================================================
*/
	ND_ static EShLanguage  ConvertShaderType (EShader shaderType)
	{
		switch ( shaderType )
		{
			case EShader::Vertex :			return EShLangVertex;
			case EShader::TessControl :		return EShLangTessControl;
			case EShader::TessEvaluation :	return EShLangTessEvaluation;
			case EShader::Geometry :		return EShLangGeometry;
			case EShader::Fragment :		return EShLangFragment;
			case EShader::Compute :			return EShLangCompute;
		}
		RETURN_ERR( "unknown shader type", EShLangCount );
	}

/*
=================================================
	EShaderLangFormat_GetVersion
=================================================
*/
    ND_ static int  EShaderLangFormat_GetVersion (EShaderLangFormat fmt)
	{
        return int(fmt & EShaderLangFormat::_VersionMask) >> uint(EShaderLangFormat::_VersionOffset);
	}

/*
=================================================
	_ParseGLSL
=================================================
*/
	bool SpirvCompiler::_ParseGLSL (EShader shaderType, EShaderLangFormat srcShaderFmt, EShaderLangFormat dstShaderFmt,
									StringView entry, StringView source, OUT GLSLangResult &glslangData, OUT String &log) const
	{
		EShClient					client			= EShClientOpenGL;
		EshTargetClientVersion		client_version	= EShTargetOpenGL_450;

		EShTargetLanguage			target			= EShTargetNone;
		EShTargetLanguageVersion	target_version	= EShTargetLanguageVersion(0);
		
        int                         version			= 0;
		uint						sh_version		= 450;		// TODO
		EProfile					sh_profile		= ENoProfile;
		EShSource					sh_source;
		
		switch ( srcShaderFmt & EShaderLangFormat::_ApiMask )
		{
			case EShaderLangFormat::OpenGL :
				sh_source		= EShSourceGlsl;
				version			= EShaderLangFormat_GetVersion( srcShaderFmt );
				sh_profile		= version >= 330 ? ECoreProfile : ENoProfile;
				break;

			case EShaderLangFormat::OpenGLES :
				sh_source		= EShSourceGlsl;
				version			= EShaderLangFormat_GetVersion( srcShaderFmt );
				sh_profile		= EEsProfile;
				break;

			case EShaderLangFormat::DirectX :
				sh_source		= EShSourceHlsl;
				version			= EShaderLangFormat_GetVersion( srcShaderFmt );
				sh_profile		= ENoProfile;	// TODO
				break;

			case EShaderLangFormat::Vulkan :
				sh_source		= EShSourceGlsl;
				version			= EShaderLangFormat_GetVersion( srcShaderFmt );
				sh_profile		= ECoreProfile;
				break;

			default :
				COMP_RETURN_ERR( "unsupported shader format" );
		}
		
		switch ( dstShaderFmt & EShaderLangFormat::_ApiMask )
		{
			case EShaderLangFormat::Vulkan :
			{
				version			= EShaderLangFormat_GetVersion( dstShaderFmt );
				client			= EShClientVulkan;
				client_version	= (version == 110 ? EShTargetVulkan_1_1 : EShTargetVulkan_1_0);
				target			= EshTargetSpv;
				target_version	= (version == 110 ? EShTargetSpv_1_3 : EShTargetSpv_1_0);
				break;
			}

			case EShaderLangFormat::OpenGL :
				if ( (dstShaderFmt & EShaderLangFormat::_FormatMask) == EShaderLangFormat::SPIRV )
				{
					target			= EshTargetSpv;
					target_version	= EShTargetSpv_1_0;
				}
				break;
		}


		EShMessages			messages	= EShMsgDefault;
		TBuiltInResource	resources;	  GenerateResources( OUT resources );
		EShLanguage			stage		= ConvertShaderType( shaderType );
		auto&				shader		= glslangData.shader;
		String				temp_src	= source.data();
		const char *		sources[]	= { temp_src.data() };
		//ShaderIncluder	includer	{ baseFolder };
		const bool			auto_map	= EnumEq( _compilerFlags, EShaderCompilationFlags::AutoMapLocations );

		shader.reset( new TShader( stage ));
		shader->setStrings( sources, int(std::size(sources)) );
		shader->setEntryPoint( entry.data() );
        shader->setEnvInput( sh_source, stage, client, version );
		shader->setEnvClient( client, client_version );
		shader->setEnvTarget( target, target_version );
		
		shader->setAutoMapLocations( auto_map );
		shader->setAutoMapBindings( auto_map );

		if ( not shader->parse( &resources, sh_version, sh_profile, false, true, messages/*, includer*/ ) )
		{
			log += shader->getInfoLog();
			_OnCompilationFailed( {source}, INOUT log );
			return false;
		}
		
		glslangData.prog.addShader( shader.get() );

		if ( not glslangData.prog.link( messages ) )
		{
			log += glslangData.prog.getInfoLog();
			_OnCompilationFailed( {source}, INOUT log );
			return false;
		}

		if ( auto_map and not glslangData.prog.mapIO() )
		{
			log += "mapIO - failed";
			return false;
		}

		return true;
	}

/*
=================================================
	_CompileSPIRV
=================================================
*/
	bool SpirvCompiler::_CompileSPIRV (const GLSLangResult &glslangData, OUT Array<uint> &spirv, OUT String &log) const
	{
		const TIntermediate* intermediate = glslangData.prog.getIntermediate( glslangData.shader->getStage() );
		COMP_CHECK_ERR( intermediate );

		SpvOptions				spv_options;
		spv::SpvBuildLogger		logger;

		spv_options.generateDebugInfo	= EnumEq( _compilerFlags, EShaderCompilationFlags::GenerateDebugInfo );
		spv_options.disableOptimizer	= EnumEq( _compilerFlags, EShaderCompilationFlags::DisableOptimizer );
		spv_options.optimizeSize		= EnumEq( _compilerFlags, EShaderCompilationFlags::OptimizeSize );
		
		GlslangToSpv( *intermediate, OUT spirv, &logger, &spv_options );

		log += logger.getAllMessages();
		return true;
	}

/*
=================================================
	_OnCompilationFailed
=================================================
*/
	bool SpirvCompiler::_OnCompilationFailed (ArrayView<StringView> source, INOUT String &log) const
	{
		// glslang errors format:
		// pattern: <error/warning>: <number>:<line>: <description>
		// pattern: <error/warning>: <file>:<line>: <description>
		/*	// TODO
		Array< StringView >		lines;
		Array< StringView >		tokens;
		Array< size_t >			num_lines;	num_lines.Resize( source.Count() );
		String					str;		str.Reserve( log.Length() * 2 );
		size_t					prev_line = UMax;

		StringParser::DivideLines( log, OUT lines );
		
		FOR( i, lines )
		{
			bool			added = false;
			GLSLErrorInfo	error_info;

			if ( ParseGLSLError( lines[i], OUT error_info ) )
			{
				// unite error in same source lines
				if ( prev_line == error_info.line )
				{
					str << lines[i] << "\n";
					continue;
				}

				prev_line = error_info.line;

				if ( error_info.fileName.Empty() )
				{
					// search in sources
					StringCRef	cur_source	= error_info.sourceIndex < source.Count() ? source[ error_info.sourceIndex ] : "";
					usize		lines_count	= error_info.sourceIndex < num_lines.Count() ? num_lines[ error_info.sourceIndex ] : 0;

					if ( lines_count == 0 )
					{
						lines_count = StringParser::CalculateNumberOfLines( cur_source ) + 1;

						if ( error_info.sourceIndex < num_lines.Count() )
							num_lines[ error_info.sourceIndex ] = lines_count;
					}
					
					CHECK( error_info.line < lines_count );

					usize		pos = 0;
					CHECK( StringParser::MoveToLine( cur_source, INOUT pos, error_info.line-1 ) );

					StringCRef	line_str;
					StringParser::ReadLineToEnd( cur_source, INOUT pos, OUT line_str );

					str << "in source (" << error_info.sourceIndex << ": " << error_info.line << "):\n\"" << line_str << "\"\n" << lines[i] << "\n";
					added = true;
				}
				else
				{
					// search in header
					StringCRef	src;
					if ( includer.GetHeaderSource( error_info.fileName, OUT src ) )
					{
						const usize	lines_count = StringParser::CalculateNumberOfLines( src ) + 1;
						const usize	local_line	= error_info.line;
						usize		pos			= 0;
						StringCRef	line_str;

						CHECK( local_line < lines_count );

						CHECK( StringParser::MoveToLine( src, INOUT pos, local_line-1 ) );
						
						StringParser::ReadLineToEnd( src, INOUT pos, OUT line_str );

						str << "in source (" << error_info.fileName << ": " << local_line << "):\n\"" << line_str << "\"\n" << lines[i] << "\n";
						added = true;
					}
				}
			}

			if ( not added )
			{
				str << DEBUG_ONLY( "<unknown> " << ) lines[i] << "\n";
			}
		}
		
		log = std::move(str);

		WARNING( log.cstr() );*/
		return false;
	}
	
/*
=================================================
	_CompileSPIRV
=================================================
*/
	bool SpirvCompiler::_BuildReflection (const GLSLangResult &glslangData, OUT ShaderReflection &result)
	{
		_intermediate = glslangData.prog.getIntermediate( glslangData.shader->getStage() );
		COMP_CHECK_ERR( _intermediate );

		// deserialize shader
		TIntermNode*	root	= _intermediate->getTreeRoot();
		
		COMP_CHECK_ERR( _ProcessExternalObjects( null, root, OUT result ) );
		//COMP_CHECK_ERR( _RecursiveProcessAggregateNode( null, root, OUT result ) );
		COMP_CHECK_ERR( _ProcessShaderInfo( INOUT result ) );

		_intermediate = null;
		return true;
	}
	
/*
=================================================
	_ProcessExternalObjects
=================================================
*/
	bool SpirvCompiler::_ProcessExternalObjects (TIntermNode*, TIntermNode* node, INOUT ShaderReflection &result) const
	{
		TIntermAggregate* aggr = node->getAsAggregate();

		switch ( aggr->getOp() )
		{
			// continue deserializing
			case TOperator::EOpSequence :
			{
				for (auto& seq : aggr->getSequence())
				{
					COMP_CHECK_ERR( _ProcessExternalObjects( aggr, seq, INOUT result ) );
				}
				return true;
			}

			// uniforms, buffers, ...
			case TOperator::EOpLinkerObjects :
			{
				for (auto& seq : aggr->getSequence())
				{
					COMP_CHECK_ERR( _DeserializeExternalObjects( seq, INOUT result ) );
				}
				return true;
			}
		}
		return true;
	}
	
/*
=================================================
	_ToBindingIndex
=================================================
*/
	BindingIndex  SpirvCompiler::_ToBindingIndex (uint index) const
	{
		if ( _targetVulkan )
			return BindingIndex{ ~0u, index };
		else
			return BindingIndex{ index, ~0u };
	}
	
/*
=================================================
	GetDesciptorSet
=================================================
*/
	ND_ PipelineDescription::DescriptorSet&  GetDesciptorSet (uint dsIndex, INOUT SpirvCompiler::ShaderReflection &reflection)
	{
		for (auto& ds : reflection.layout.descriptorSets)
		{
			if ( ds.bindingIndex == dsIndex )
				return ds;
		}

		PipelineDescription::DescriptorSet	ds;
		ds.bindingIndex	= dsIndex;
		ds.id			= DescriptorSetID( ToString(dsIndex) );

		reflection.layout.descriptorSets.push_back( std::move(ds) );

		return reflection.layout.descriptorSets.back();
	}
	
/*
=================================================
	_ExtractImageType
=================================================
*/
	EImage  SpirvCompiler::_ExtractImageType (const TType &type) const
	{
		if ( type.getBasicType() == TBasicType::EbtSampler and not type.isSubpass() )
		{
			TSampler const&	samp = type.getSampler();

			switch ( samp.dim )
			{
				case TSamplerDim::Esd1D :
				{
					if ( samp.isShadow() and samp.isArrayed() )		return EImage::Tex1DArray;		else
					if ( samp.isShadow() )							return EImage::Tex1D;			else
					if ( samp.isArrayed() )							return EImage::Tex1DArray;		else
																	return EImage::Tex1D;
				}
				case TSamplerDim::Esd2D :
				{
					if ( samp.isShadow() and samp.isArrayed() )			return EImage::Tex2DArray;		else
					if ( samp.isShadow() )								return EImage::Tex2D;			else
					if ( samp.isMultiSample() and samp.isArrayed() )	return EImage::Tex2DMSArray;	else
					if ( samp.isArrayed() )								return EImage::Tex2DArray;		else
					if ( samp.isMultiSample() )							return EImage::Tex2DMS;			else
																		return EImage::Tex2D;
				}
				case TSamplerDim::Esd3D :
				{
					return EImage::Tex3D;
				}
				case TSamplerDim::EsdCube :
				{
					if ( samp.isShadow() )		return EImage::TexCube;			else
					if ( samp.isArrayed() )		return EImage::TexCubeArray;	else
												return EImage::TexCube;
					break;
				}
				case TSamplerDim::EsdBuffer :
				{
					return EImage::Buffer;
				}
				default :
					COMP_RETURN_ERR( "unknown sampler dimension type!" );
			}
		}
		COMP_RETURN_ERR( "type is not image/sampler type!" );
	}
	
/*
=================================================
	_ExtractImageFormat
=================================================
*/
	EPixelFormat  SpirvCompiler::_ExtractImageFormat (const TLayoutFormat &format) const
	{
		switch ( format )
		{
			case TLayoutFormat::ElfRgba32f :		return EPixelFormat::RGBA32F;
			case TLayoutFormat::ElfRgba16f :		return EPixelFormat::RGBA16F;
			case TLayoutFormat::ElfR32f :			return EPixelFormat::R32F;
			case TLayoutFormat::ElfRgba8 :			return EPixelFormat::RGBA8_UNorm;
			case TLayoutFormat::ElfRgba8Snorm :		return EPixelFormat::RGBA8_SNorm;
			case TLayoutFormat::ElfRg32f :			return EPixelFormat::RG32F;
			case TLayoutFormat::ElfRg16f :			return EPixelFormat::RG16F;
			case TLayoutFormat::ElfR11fG11fB10f :	return EPixelFormat::RGB_11_11_10F;
			case TLayoutFormat::ElfR16f :			return EPixelFormat::R16F;
			case TLayoutFormat::ElfRgba16 :			return EPixelFormat::RGBA16_UNorm;
			case TLayoutFormat::ElfRgb10A2 :		return EPixelFormat::RGB10_A2_UNorm;
			case TLayoutFormat::ElfRg16 :			return EPixelFormat::RG16_UNorm;
			case TLayoutFormat::ElfRg8 :			return EPixelFormat::RG8_UNorm;
			case TLayoutFormat::ElfR16 :			return EPixelFormat::R16_UNorm;
			case TLayoutFormat::ElfR8 :				return EPixelFormat::R8_UNorm;
			case TLayoutFormat::ElfRgba16Snorm :	return EPixelFormat::RGBA16_SNorm;
			case TLayoutFormat::ElfRg16Snorm :		return EPixelFormat::RG16_SNorm;
			case TLayoutFormat::ElfRg8Snorm :		return EPixelFormat::RG8_SNorm;
			case TLayoutFormat::ElfR16Snorm :		return EPixelFormat::R16_SNorm;
			case TLayoutFormat::ElfR8Snorm :		return EPixelFormat::R8_SNorm;
			case TLayoutFormat::ElfRgba32i :		return EPixelFormat::RGBA32I;
			case TLayoutFormat::ElfRgba16i :		return EPixelFormat::RGBA16I;
			case TLayoutFormat::ElfRgba8i :			return EPixelFormat::RGBA8I;
			case TLayoutFormat::ElfR32i :			return EPixelFormat::R32I;
			case TLayoutFormat::ElfRg32i :			return EPixelFormat::RG32I;
			case TLayoutFormat::ElfRg16i :			return EPixelFormat::RG16I;
			case TLayoutFormat::ElfRg8i :			return EPixelFormat::RG8I;
			case TLayoutFormat::ElfR16i :			return EPixelFormat::R16I;
			case TLayoutFormat::ElfR8i :			return EPixelFormat::R8I;
			case TLayoutFormat::ElfRgba32ui :		return EPixelFormat::RGBA32U;
			case TLayoutFormat::ElfRgba16ui :		return EPixelFormat::RGBA16U;
			case TLayoutFormat::ElfRgba8ui :		return EPixelFormat::RGBA8U;
			case TLayoutFormat::ElfR32ui :			return EPixelFormat::R32U;
			case TLayoutFormat::ElfRg32ui :			return EPixelFormat::RG32U;
			case TLayoutFormat::ElfRg16ui :			return EPixelFormat::RG16U;
			case TLayoutFormat::ElfRgb10a2ui :		return EPixelFormat::RGB10_A2U;
			case TLayoutFormat::ElfRg8ui :			return EPixelFormat::RG8U;
			case TLayoutFormat::ElfR16ui :			return EPixelFormat::R16U;
			case TLayoutFormat::ElfR8ui :			return EPixelFormat::R8U;
		}
		COMP_RETURN_ERR( "Unsupported image format!" );
	}
	
/*
=================================================
	ExtractShaderAccessType
=================================================
*/
	ND_ static EShaderAccess  ExtractShaderAccessType (const TQualifier &q, EShaderCompilationFlags flags)
	{
		if ( q.coherent or
			 q.volatil	or
			 q.restrict )
		{
			return EShaderAccess::ReadWrite;
		}

		if ( q.readonly )
			return EShaderAccess::ReadOnly;

		if ( q.writeonly )
			return EnumEq( flags, EShaderCompilationFlags::AlwaysWriteDiscard ) ? EShaderAccess::WriteDiscard : EShaderAccess::WriteOnly;

		// defualt:
		return EShaderAccess::ReadWrite;
	}

/*
=================================================
	ExtractNodeName
=================================================
*/
	ND_ static String  ExtractNodeName (TIntermNode *node)
	{
		CHECK_ERR( node and node->getAsSymbolNode() );
		
		String				name	= node->getAsSymbolNode()->getName().c_str();
		const StringView	prefix	= "anon@";

		if ( not name.compare( 0, prefix.size(), prefix ) )
			name.clear();

		return name;
	}
	
/*
=================================================
	Extract***ID
=================================================
*/
	ND_ static UniformID  ExtractUniformID (TIntermNode *node)
	{
		return UniformID( ExtractNodeName( node ) );
	}
	
	ND_ static VertexID  ExtractVertexID (TIntermNode *node)
	{
		return VertexID( ExtractNodeName( node ) );
	}

	ND_ static RenderTargetID  ExtractRenderTargetID (TIntermNode *node)
	{
		return RenderTargetID( ExtractNodeName( node ) );
	}

	ND_ static SpecializationID  ExtractSpecializationID (TIntermNode *node)
	{
		return SpecializationID( ExtractNodeName( node ) );
	}

/*
=================================================
	ExtractBufferUniformID
=================================================
*/
	ND_ static UniformID  ExtractBufferUniformID (const TType &type)
	{
		return UniformID( type.getTypeName().c_str() );
	}

/*
=================================================
	_ExtractVertexType
=================================================
*/
	EVertexType  SpirvCompiler::_ExtractVertexType (const TType &type) const
	{
		EVertexType		result = EVertexType(0);

		COMP_CHECK_ERR( not type.isArray() );

		switch ( type.getBasicType() )
		{
			case TBasicType::EbtFloat :		result |= EVertexType::_Float;	break;
			case TBasicType::EbtDouble :	result |= EVertexType::_Double;	break;
			case TBasicType::EbtFloat16 :	result |= EVertexType::_Half;	break;
			case TBasicType::EbtInt8 :		result |= EVertexType::_Byte;	break;
			case TBasicType::EbtUint8 :		result |= EVertexType::_UByte;	break;
			case TBasicType::EbtInt16 :		result |= EVertexType::_Short;	break;
			case TBasicType::EbtUint16 :	result |= EVertexType::_UShort;	break;
			case TBasicType::EbtInt :		result |= EVertexType::_Int;	break;
			case TBasicType::EbtUint :		result |= EVertexType::_UInt;	break;
			case TBasicType::EbtInt64 :		result |= EVertexType::_Long;	break;
			case TBasicType::EbtUint64 :	result |= EVertexType::_ULong;	break;
			//case TBasicType::EbtBool :	result |= EVertexType::_Bool;	break;
			default :						COMP_RETURN_ERR( "unsupported basic type!" );
		}

		if ( type.isScalarOrVec1() )
			return result;

		if ( type.isVector() )
		{
			switch ( type.getVectorSize() )
			{
				case 1 :	result |= EVertexType::_Vec1;	break;
				case 2 :	result |= EVertexType::_Vec2;	break;
				case 3 :	result |= EVertexType::_Vec3;	break;
				case 4 :	result |= EVertexType::_Vec4;	break;
				default :	COMP_RETURN_ERR( "unsupported vector size!" );
			}
			return result;
		}

		if ( type.isMatrix() )
		{
			COMP_RETURN_ERR( "not supported, yet" );
		}

		COMP_RETURN_ERR( "unknown vertex type" );
	}
	
/*
=================================================
	_ExtractFragmentOutputType
=================================================
*/
	EFragOutput  SpirvCompiler::_ExtractFragmentOutputType (const TType &type) const
	{
		COMP_CHECK_ERR( type.getVectorSize() == 4 );
		
		switch ( type.getBasicType() )
		{
			case TBasicType::EbtFloat :		return EFragOutput::Float4;
			case TBasicType::EbtInt :		return EFragOutput::Int4;
			case TBasicType::EbtUint :		return EFragOutput::UInt4;
		}
		COMP_RETURN_ERR( "unsupported fragment output" );
	}
	
/*
=================================================
	_CalculateStructSize
----
	based on TParseContext::fixBlockUniformOffsets
=================================================
*/
	bool SpirvCompiler::_CalculateStructSize (const TType &bufferType, OUT BytesU &staticSize, OUT BytesU &arrayStride) const
	{
		staticSize = arrayStride = 0_b;

		COMP_CHECK_ERR( bufferType.isStruct() );
		COMP_CHECK_ERR( bufferType.getQualifier().isUniformOrBuffer() );
		COMP_CHECK_ERR( bufferType.getQualifier().layoutPacking == ElpStd140 or
						bufferType.getQualifier().layoutPacking == ElpStd430 );

		int member_size = 0;
		int offset = 0;

		for (size_t member = 0; member < bufferType.getStruct()->size(); ++member)
		{
			const TType&		member_type			= *(*bufferType.getStruct())[member].type;
			const TQualifier&	member_qualifier	= (*bufferType.getStruct())[member].type->getQualifier();
			TLayoutMatrix		sub_matrix_layout	= (*bufferType.getStruct())[member].type->getQualifier().layoutMatrix;

			int dummy_stride;
			int member_alignment = _intermediate->getBaseAlignment( member_type, OUT member_size, OUT dummy_stride,
																	bufferType.getQualifier().layoutPacking == ElpStd140,
																	sub_matrix_layout != ElmNone ? sub_matrix_layout == ElmRowMajor
																		: bufferType.getQualifier().layoutMatrix == ElmRowMajor );

			if ( member_qualifier.hasOffset() )
			{
				ASSERT( IsMultipleOfPow2( member_qualifier.layoutOffset, member_alignment ));

				if ( _intermediate->getSpv().spv == 0 )
				{
					ASSERT( member_qualifier.layoutOffset >= offset );

					offset = std::max( offset, member_qualifier.layoutOffset );
				}
				else {
					offset = member_qualifier.layoutOffset;
				}
			}

			if ( member_qualifier.hasAlign() )
				member_alignment = std::max( member_alignment, member_qualifier.layoutAlign );

			RoundToPow2( offset, member_alignment );
			//(*bufferType.getStruct())[member].type->getQualifier().layoutOffset = offset;
			offset += member_size;

			// for last member
			if ( member+1 == bufferType.getStruct()->size() and member_type.isUnsizedArray() )
			{
				ASSERT( member_size == 0 );

				arrayStride = BytesU(dummy_stride);
			}
		}
		staticSize = BytesU(offset);
		return true;
	}

/*
=================================================
	_DeserializeExternalObjects
=================================================
*/
	bool SpirvCompiler::_DeserializeExternalObjects (TIntermNode* node, INOUT ShaderReflection &result) const
	{
		using Uniform_t = PipelineDescription::Uniform_t;

		TIntermTyped *		tnode			= node->getAsTyped();
		auto const&			type			= tnode->getType();
		auto const&			qual			= type.getQualifier();
		
		auto&				descriptor_set	= GetDesciptorSet( qual.hasSet() ? uint(qual.layoutSet) : 0, result );


		if ( type.getBasicType() == TBasicType::EbtSampler )
		{
			// sampler
			if ( type.getSampler().isCombined() )
			{
				PipelineDescription::Texture	tex;
				tex.textureType		= _ExtractImageType( type );
				tex.index			= _ToBindingIndex( qual.hasBinding() ? uint(qual.layoutBinding) : ~0u );
				tex.stageFlags		= _currentStage;

				descriptor_set.uniforms.insert({ ExtractUniformID( node ), std::move(tex) });
				return true;
			}
			
			// image
			if ( type.getSampler().isImage() )
			{
				PipelineDescription::Image		image;
				image.imageType		= _ExtractImageType( type );
				image.format		= _ExtractImageFormat( qual.layoutFormat );
				image.access		= ExtractShaderAccessType( qual, _compilerFlags );
				image.index			= _ToBindingIndex( qual.hasBinding() ? uint(qual.layoutBinding) : ~0u );
				image.stageFlags	= _currentStage;
				
				descriptor_set.uniforms.insert({ ExtractUniformID( node ), std::move(image) });
				return true;
			}
			
			// subpass
			if ( type.getSampler().isSubpass() )
			{
				PipelineDescription::SubpassInput	subpass;
				subpass.attachmentIndex	= qual.hasAttachment() ? uint(qual.layoutAttachment) : ~0u;
				subpass.isMultisample	= false;	// TODO
				subpass.index			= _ToBindingIndex( qual.hasBinding() ? uint(qual.layoutBinding) : ~0u );
				subpass.stageFlags		= _currentStage;

				descriptor_set.uniforms.insert({ ExtractUniformID( node ), std::move(subpass) });
				return true;
			}
		}
		
		// push constants
		if ( qual.layoutPushConstant )
		{
			// TODO
			return true;
		}
		
		// uniform buffer or storage buffer
		if ( type.getBasicType() == TBasicType::EbtBlock	and
			 (qual.storage == TStorageQualifier::EvqUniform	or qual.storage == TStorageQualifier::EvqBuffer) )
		{
			COMP_CHECK_ERR( type.isStruct() );
			
			// uniform block
			if ( qual.storage == TStorageQualifier::EvqUniform )
			{
				PipelineDescription::UniformBuffer	ubuf;
				ubuf.index			= _ToBindingIndex( qual.hasBinding() ? uint(qual.layoutBinding) : ~0u );
				ubuf.stageFlags		= _currentStage;
				
				BytesU	stride;
				COMP_CHECK_ERR( _CalculateStructSize( type, OUT ubuf.size, OUT stride ));

				descriptor_set.uniforms.insert({ ExtractBufferUniformID( type ), std::move(ubuf) });
				return true;
			}

			// storage block
			if ( qual.storage == TStorageQualifier::EvqBuffer )
			{
				PipelineDescription::StorageBuffer	sbuf;
				sbuf.access			= ExtractShaderAccessType( qual, _compilerFlags );
				sbuf.index			= _ToBindingIndex( qual.hasBinding() ? uint(qual.layoutBinding) : ~0u );
				sbuf.stageFlags		= _currentStage;
			
				COMP_CHECK_ERR( _CalculateStructSize( type, OUT sbuf.staticSize, OUT sbuf.arrayStride ));

				descriptor_set.uniforms.insert({ ExtractBufferUniformID( type ), std::move(sbuf) });
				return true;
			}
		}
		
		// uniform
		if ( qual.storage == TStorageQualifier::EvqUniform )
		{
			COMP_RETURN_ERR( "uniform is not supported for Vulkan!" );
		}
		
		// shader input
		if ( qual.storage == TStorageQualifier::EvqVaryingIn )
		{
			if ( _currentStage != EShaderStages::Vertex )
				return true;	// skip

            GraphicsPipelineDesc::VertexAttrib	attrib;
			attrib.id		= ExtractVertexID( node );
			attrib.index	= (qual.hasLocation() ? uint(qual.layoutLocation) : ~0u);
			attrib.type		= _ExtractVertexType( type );

			result.vertex.vertexAttribs.push_back( std::move(attrib) );
			return true;
		}
		
		// shader output
		if ( qual.storage == TStorageQualifier::EvqVaryingOut )
		{
			if ( _currentStage != EShaderStages::Fragment )
				return true;	// skip

			GraphicsPipelineDesc::FragmentOutput	frag_out;
			frag_out.id		= ExtractRenderTargetID( node );
			frag_out.index	= (qual.hasLocation() ? uint(qual.layoutLocation) : ~0u);
			frag_out.type	= _ExtractFragmentOutputType( type );

			result.fragment.fragmentOutput.push_back( std::move(frag_out) );
			return true;
		}
		
		// skip builtin
		if ( type.isBuiltIn() )
			return true;

		// specialization constant
		if ( qual.storage == EvqConst and
			 qual.layoutSpecConstantId != TQualifier::layoutSpecConstantIdEnd )
		{
			result.specConstants.insert({ ExtractSpecializationID( node ), qual.layoutSpecConstantId });
			return true;
		}

		// global variable or global constant
		if ( qual.storage == EvqGlobal or qual.storage == EvqConst )
			return true;

		COMP_RETURN_ERR( "unknown external type!" );
	}

/*
=================================================
	_MergeWithGeometryInputPrimitive
=================================================
*/
	void SpirvCompiler::_MergeWithGeometryInputPrimitive (INOUT GraphicsPipelineDesc::TopologyBits_t &topologyBits, const TLayoutGeometry &type) const
	{
		switch ( type )
		{
			case TLayoutGeometry::ElgPoints : {
				topologyBits.set( uint(EPrimitive::Point) );
				break;
			}
			case TLayoutGeometry::ElgLines : {
				topologyBits.set( uint(EPrimitive::LineList) );
				topologyBits.set( uint(EPrimitive::LineStrip) );
				break;
			}
			case TLayoutGeometry::ElgLinesAdjacency : {
				topologyBits.set( uint(EPrimitive::LineListAdjacency) );
				topologyBits.set( uint(EPrimitive::LineStripAdjacency) );
				break;
			}
			case TLayoutGeometry::ElgTriangles : {
				topologyBits.set( uint(EPrimitive::TriangleList) );
				topologyBits.set( uint(EPrimitive::TriangleStrip) );
				topologyBits.set( uint(EPrimitive::TriangleFan) );
				break;
			}
			case TLayoutGeometry::ElgTrianglesAdjacency : {
				topologyBits.set( uint(EPrimitive::TriangleListAdjacency) );
				topologyBits.set( uint(EPrimitive::TriangleStripAdjacency) );
				break;
			}
		}
		COMP_RETURN_ERR( "invalid geometry input primitive type!", void() );
	}
	
/*
=================================================
	_ProcessShaderInfo
=================================================
*/
	bool SpirvCompiler::_ProcessShaderInfo (INOUT ShaderReflection &result) const
	{
		switch ( _intermediate->getStage() )
		{
			case EShLangVertex :
			{
				break;
			}

			case EShLangTessControl :
			{
				result.tessellation.patchControlPoints	= _intermediate->getVertices();

				result.vertex.supportedTopology.set( uint(EPrimitive::Patch) );
				break;
			}

			case EShLangTessEvaluation :
			{
				break;
			}

			case EShLangGeometry :
			{
				_MergeWithGeometryInputPrimitive( INOUT result.vertex.supportedTopology, _intermediate->getInputPrimitive() );
				break;
			}

			case EShLangFragment :
			{
				result.fragment.earlyFragmentTests = (_intermediate->getEarlyFragmentTests() or not _intermediate->isDepthReplacing());

				break;
			}

			case EShLangCompute :
			{
				result.compute.localGroupSize.x	= _intermediate->getLocalSize(0);
				result.compute.localGroupSize.y = _intermediate->getLocalSize(1);
				result.compute.localGroupSize.z	= _intermediate->getLocalSize(2);

				result.compute.localGroupSpecialization.x = uint(_intermediate->getLocalSizeSpecId(0));
				result.compute.localGroupSpecialization.y = uint(_intermediate->getLocalSizeSpecId(1));
				result.compute.localGroupSpecialization.z = uint(_intermediate->getLocalSizeSpecId(2));
				break;
			}
		}
		return true;
	}

}	// FG