dir = include_directories('.')
cc  = meson.get_compiler('c')
add_project_arguments('-D_POSIX_C_SOURCE=200809L', language: 'c')
add_project_arguments('-D_POSIX_C_SOURCE=200809L', language: 'cpp')

dep = [
    cc.find_library('m', required : false),
    dependency('threads', required : false)
]

fossil_crabdb_lib = library('fossil_crabdb',
    files(
        'database.c',
        'family.c',
        'storage.c',
        'media.c',
        'dsl.c'
        ),
    install: true,
    dependencies: dep,
    include_directories: dir
)

fossil_crabdb_dep = declare_dependency(
    link_with: [fossil_crabdb_lib],
    dependencies: dep,
    include_directories: dir)

meson.override_dependency('fossil-crabdb', fossil_crabdb_dep)
