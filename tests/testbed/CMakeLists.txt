file( GLOB_RECURSE SOURCES "*.*" )

if (DEFINED ANDROID)
	add_library( "testbed" STATIC ${SOURCES} )
else ()
	add_executable( "testbed" ${SOURCES} )
endif ()

source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "testbed" PROPERTY FOLDER "" )

target_include_directories( "testbed" PRIVATE "../../framegraph/Vulkan" )
target_include_directories( "testbed" PRIVATE "../../framegraph/Vulkan/Buffer" )
target_include_directories( "testbed" PRIVATE "../../framegraph/Vulkan/CommandBuffer" )
target_include_directories( "testbed" PRIVATE "../../framegraph/Vulkan/Debugger" )
target_include_directories( "testbed" PRIVATE "../../framegraph/Vulkan/Descriptors" )
target_include_directories( "testbed" PRIVATE "../../framegraph/Vulkan/Image" )
target_include_directories( "testbed" PRIVATE "../../framegraph/Vulkan/Instance" )
target_include_directories( "testbed" PRIVATE "../../framegraph/Vulkan/Memory" )
target_include_directories( "testbed" PRIVATE "../../framegraph/Vulkan/Pipeline" )
target_include_directories( "testbed" PRIVATE "../../framegraph/Vulkan/RenderPass" )
target_include_directories( "testbed" PRIVATE "../../framegraph/Vulkan/RayTracing" )
target_include_directories( "testbed" PRIVATE "../../framegraph/Vulkan/Swapchain" )
target_include_directories( "testbed" PRIVATE "../../framegraph/Vulkan/Utils" )

target_compile_definitions( "testbed" PUBLIC "FG_TEST_GRAPHS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/Graphs\"" )
target_compile_definitions( "testbed" PUBLIC "FG_TEST_DUMPS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/Dumps\"" )

target_link_libraries( "testbed" "FrameGraph" )
target_link_libraries( "testbed" "Framework" )

if (TARGET "GraphViz")
	target_link_libraries( "testbed" "GraphViz" )
endif()
if (TARGET "PipelineCompiler")
	target_link_libraries( "testbed" "PipelineCompiler" )
endif ()
if (TARGET "lodepng-lib")
	target_link_libraries( "testbed" "lodepng-lib" )
endif()

add_test( NAME "testbed" COMMAND "testbed" )
