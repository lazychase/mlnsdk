find_program(GIT_EXECUTABLE git)

ExternalProject_Add(
    struct_mapping
    PREFIX ${CMAKE_BINARY_DIR}/vendor/struct_mapping
    GIT_REPOSITORY "https://github.com/bk192077/struct_mapping.git"
    GIT_TAG "v0.6.0"
    TIMEOUT 10
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
)

ExternalProject_Get_Property(struct_mapping source_dir)
set(STRUCT_MAPPING_INCLUDE_DIR ${source_dir}/include)
