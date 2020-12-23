// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Window/IWindow.h"
#include "framework/Vulkan/VulkanDevice.h"
#include "framegraph/FG.h"
#include "stl/Algorithms/StringUtils.h"

namespace FG
{

	//
	// Frame Graph Test Application
	//

	class FGApp final : public IWindowEventListener
	{
		// types
	private:
		using TestFunc_t = bool (FGApp::*) ();
		using TestQueue_t = Deque<Pair< TestFunc_t, uint >>;
		using VPipelineCompilerPtr = SharedPtr< class VPipelineCompiler >;
		using DeviceProperties = IFrameGraph::DeviceProperties;


		// variables
	private:
#ifdef FG_ENABLE_VULKAN
		VulkanDeviceInitializer	_vulkan;
#endif

		WindowPtr				_window;
		FrameGraph				_frameGraph;
		VPipelineCompilerPtr	_pplnCompiler;
		SwapchainID				_swapchainId;

		TestQueue_t				_tests;
		uint					_testInvocations = 0;
		uint					_testsPassed = 0;
		uint					_testsFailed = 0;

		bool					_hasShaderDebugger;
		DeviceProperties		_properties;


		// methods
	public:
		FGApp();
		~FGApp();

		static void Run(void* nativeHandle);


		// IWindowEventListener
	private:
		void OnResize(const uint2 &size) override;
		void OnRefresh() override {}
		void OnDestroy() override {}
		void OnUpdate() override {}
		void OnKey(StringView, EKeyAction) override {}
		void OnMouseMove(const float2 &) override {}


		// helpers
	private:
		bool _Initialize(WindowPtr &&wnd);
		bool _InitializeForVulkan(WindowPtr &&wnd);
		bool _Update();
		void _Destroy();

		bool Visualize(StringView name) const;
		bool CompareDumps(StringView filename) const;
		bool SavePNG(const String &filename, const ImageView &imageData) const;

		template <typename Arg0, typename ...Args>
		void DeleteResources(Arg0 &arg0, Args& ...args);

		ND_ Array<uint8_t>	CreateData(BytesU size) const;

		ND_ static String  GetFuncName(StringView src);


		// drawing tests
	private:

		bool testBed();
	};


	template <typename Arg0, typename ...Args>
	inline void FGApp::DeleteResources(Arg0 &arg0, Args& ...args)
	{
		_frameGraph->ReleaseResource(INOUT arg0);

		if constexpr (CountOf<Args...>())
			DeleteResources(std::forward<Args&>(args)...);
	}


	inline String  FGApp::GetFuncName(StringView src)
	{
		size_t	pos = src.find_last_of("::");

		if (pos != StringView::npos)
			return String{ src.substr(pos + 1) };
		else
			return String{ src };
	}

#	define TEST_NAME	GetFuncName( FG_FUNCTION_NAME )


}	// FG
