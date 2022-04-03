set(SOURCE_FILES
	include/hlviewer/assets/asset.h
	include/hlviewer/assets/asset_future.h
	include/hlviewer/assets/asset_loader.h
	include/hlviewer/assets/asset_manager.h
	include/hlviewer/assets/level_asset.h
	include/hlviewer/assets/wad_asset.h
	include/hlviewer/assets/wad_material_asset.h

	include/hlviewer/entities/base_entity.h
	include/hlviewer/entities/light_entity.h
	include/hlviewer/entities/player_start_entity.h
	include/hlviewer/entities/trigger_entity.h

	include/hlviewer/brush_model.h
	include/hlviewer/level_view_renderer_iface.h
	include/hlviewer/scene_view.h
	include/hlviewer/vis.h
	include/hlviewer/world_state_base.h

	src/assets/asset.cpp
	src/assets/asset_loader.cpp
	src/assets/asset_manager.cpp
	src/assets/level_asset.cpp
	src/assets/wad_asset.cpp
	src/assets/wad_material_asset.cpp

	src/entities/base_entity.cpp
	src/entities/light_entity.cpp
	src/entities/player_start_entity.cpp
	src/entities/trigger_entity.cpp

	src/brush_model.cpp
	src/scene_view.cpp
	src/vis.cpp
	src/world_state_base.cpp
)

add_library(hlviewer_shared STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
	${SOURCE_FILES}
)

appfw_module(hlviewer_shared)
target_include_directories(hlviewer_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(hlviewer_shared PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(hlviewer_shared
	gui_app_base
	renderer
)

target_precompile_headers(hlviewer_shared PRIVATE
    ${PCH_GUI_APP_BASE_HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
