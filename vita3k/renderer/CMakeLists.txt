add_library(
	renderer
	STATIC
	src/gl/attribute_formats.cpp
	src/gl/color_formats.cpp
	src/gl/compile_program.cpp
	src/gl/draw.cpp
	src/gl/fence.cpp
	src/gl/renderer.cpp
	src/gl/ring_buffer.cpp
	src/gl/overlay_renderer.cpp
	src/gl/screen_render.cpp
	src/gl/surface_cache.cpp
	src/gl/sync_state.cpp
	src/gl/texture_formats.cpp
	src/gl/texture.cpp
	src/gl/uniforms.cpp

	src/vulkan/allocator.cpp
	src/vulkan/context.cpp
	src/vulkan/creation.cpp
	src/vulkan/gxm_to_vulkan.cpp
	src/vulkan/pipeline_cache.cpp
	src/vulkan/renderer.cpp
	src/vulkan/scene.cpp
	src/vulkan/overlay_renderer.cpp
	src/vulkan/screen_filters.cpp
	src/vulkan/screen_renderer.cpp
	src/vulkan/surface_cache.cpp
	src/vulkan/sync_state.cpp
	src/vulkan/texture.cpp

	src/texture/cache.cpp
	src/texture/format.cpp
	src/texture/palette.cpp
	src/texture/pvrt-dec.cpp
	src/texture/replacement.cpp
	src/texture/yuv.cpp

	src/batch.cpp
	src/creation.cpp
	src/renderer.cpp
	src/scene.cpp
	src/shaders.cpp
	src/state_set.cpp
	src/sync.cpp
	src/transfer.cpp
)

target_include_directories(renderer PUBLIC include)
target_link_libraries(renderer PUBLIC display mem stb shader glutil threads config util vkutil overlay)
target_link_libraries(renderer PRIVATE dialog ddspp SDL3::SDL3 stb ffmpeg xxHash::xxhash concurrentqueue)

if(NOT ANDROID)
	# Exercise the production cache without pulling in the app/session launcher
	# through the complete GPU renderer and overlay dependency graph.
	add_executable(renderer-texture-cache-tests
		tests/texture_cache_tests.cpp
		src/texture/cache.cpp
		src/texture/format.cpp
		src/texture/palette.cpp
		src/texture/pvrt-dec.cpp
		src/texture/replacement.cpp
		src/texture/yuv.cpp
	)
	target_include_directories(renderer-texture-cache-tests PRIVATE include)
	target_link_libraries(renderer-texture-cache-tests PRIVATE mem gxm shader googletest util ddspp stb ffmpeg xxHash::xxhash)
	add_test(NAME renderer-texture-cache COMMAND renderer-texture-cache-tests)
	if(WIN32)
		# FFmpeg's runtime DLLs are deployed beside the main executable.
		set_tests_properties(renderer-texture-cache PROPERTIES
			ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
	endif()
endif()

if(ANDROID)
	target_link_libraries(renderer PRIVATE android adrenotools)
endif()

if(APPLE)
	target_sources(renderer PRIVATE src/vulkan/metal_layer.mm)
	target_link_libraries(renderer PRIVATE "-framework QuartzCore" "-framework Cocoa")
endif()

if(X11_FOUND)
	target_link_libraries(renderer PRIVATE ${X11_LIBRARIES})
	target_include_directories(renderer PRIVATE ${X11_INCLUDE_DIR})
endif()

if(WAYLAND_FOUND)
	target_link_libraries(renderer PRIVATE ${WAYLAND_LIBRARIES})
	target_include_directories(renderer PRIVATE ${WAYLAND_INCLUDE_DIRS})
endif()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	# silence all vma warnings
	target_compile_options(renderer PRIVATE "-Wno-nullability-completeness")
endif()

# Marshmallow Tracy linking
if(TRACY_ENABLE_ON_CORE_COMPONENTS)
	target_link_libraries(renderer PRIVATE tracy)
endif()
