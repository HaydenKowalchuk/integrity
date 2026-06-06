# Dependencies
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: true)

# Get the target platform to cook for
target_platform = get_option('platform')

cgltf_dep = dependency('cgltf', fallback: ['cgltf', 'cgltf_dep'])
stb_dep = dependency('stb', fallback: ['stb', 'stb_dep'])

cooker_inc = include_directories(
  '../../include',
  '../../include/cooker',
)

cooker_lib_src = files(
  'crclib.c',
  'gltf_loader.c',
)

# Built for the TARGET platform (can be linked into the game)
cooker_lib = static_library(
  'cooker_lib',
  sources: cooker_lib_src,
  include_directories: cooker_inc,
  dependencies: [cgltf_dep, stb_dep, m_dep],
  native: false,
)

cooker_lib_dep = declare_dependency(
  link_with: cooker_lib,
  include_directories: cooker_inc,
)

# Built for the BUILD platform (runs on the host machine)
cooker_lib_native = static_library(
  'cooker_lib_native',
  sources: cooker_lib_src,
  include_directories: cooker_inc,
  dependencies: [cgltf_dep, stb_dep, m_dep],
  native: true,
)

cooker_lib_native_dep = declare_dependency(
  link_with: cooker_lib_native,
  include_directories: cooker_inc,
)

cooker_src = files('cooker.c')

# Add target platform define for the cooker executable
cooker_c_args = []
if target_platform == 'dreamcast'
  cooker_c_args += ['-DCOOKER_TARGET_DREAMCAST']
endif

cooker_exe = executable(
  'cooker',
  cooker_src,
  include_directories: cooker_inc,
  dependencies: [cgltf_dep, stb_dep, cooker_lib_native_dep, m_dep],
  install: false,
  native: true,
  c_args: cooker_c_args,
)
