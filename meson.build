project(
  'gamejam',
  'c',
  version: '1.0',
  default_options: ['c_std=c99', 'b_staticpic=false', 'default_library=static'],
  meson_version: '>=0.65.0',
)

# Compiler arguments
#arg_base = ['-Wall', '-Wextra', '-Wshadow', '-Wstack-protector', '-Wno-strict-prototypes', '-Wformat=0', '-std=gnu11', '-fsingle-precision-constant']
arg_base = [
  '-Wshadow',
  '-Wstack-protector',
  '-Wno-strict-prototypes',
  '-Wno-missing-braces',
  '-Wformat=0',
  '-std=gnu11',
  '-Wno-switch',
]
arg_code = []
arg_linker = []

# Dependencies
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)
umka_dep = dependency('umka-lang', fallback: ['umka', 'umka_dep'])

# Options
opt_platform = get_option('platform')
opt_buildtype = get_option('buildtype')

inc_base = []
inc_base += include_directories('include')
inc_base += include_directories('third-party/cute_c2')
inc_base += include_directories('third-party/tinyobj_loader')
#inc_base += include_directories('src')

src_files = []
src_files += files('src/common/2d_physics/aabb_2d.c')
src_files += files('src/common/2d_physics/2d_rigidbody.c')
src_files += files('src/common/2d_physics/2d_physics.c')
src_files += files('src/common/voxel_physics/aabb_3d.c')
src_files += files('src/common/voxel_physics/voxel_physics.c')
src_files += files('src/common/voxel_physics/sweep.c')
src_files += files('src/common/voxel_physics/rigidbody.c')
src_files += files('src/common/resource_manager.c')
src_files += files('src/common/snd_wavload.c')
src_files += files('src/common/renderer.c')
src_files += files('src/common/input.c')
src_files += files('src/common/image_loader.c')
src_files += files('src/common/obj_loader.c')
src_files += files('src/common/stack.c')
src_files += files('src/common/lz4x.c')
src_files += files('src/common/util.c')
src_files += files('src/common/file_access.c')
src_files += files('src/common/main.c')
src_files += files('src/scene/scene.c')
src_files += files('src/scene/null_scene.c')
src_files += files('src/ui/gl_batcher.c')
src_files += files('src/ui/ui_backend.c')

# Platform-specific configuration
platform_dep = []
platform_src = []
platform_inc = []

if opt_platform == 'linux'
  arg_code += ['-DPLATFORM_LINUX', '-DDESKTOP']
  arg_base += ['-fdiagnostics-color']

  if opt_buildtype == 'debug'
    arg_code += ['-DDEBUG', '-g', '-O0', '-fno-inline', '-fno-omit-frame-pointer']
  else
    arg_code += [
      '-DNDEBUG',
      '-DQUIET',
      '-g0',
      '-Os',
      '-ffast-math',
      '-funsafe-math-optimizations',
      '-fomit-frame-pointer',
    ]
  endif

  #glad_dep = dependency('glad', fallback: ['glad', 'glad_dep'])
  glfw_dep = dependency('glfw3', fallback: ['glfw', 'glfw_dep'])
  openal_dep = dependency('openal', fallback: ['openal', 'openal_dep'])
  cglm_dep = dependency('cglm', fallback: ['cglm', 'cglm_dep'])

  platform_dep += [glfw_dep, openal_dep, cglm_dep]
  platform_src += files('src/linux/glad.c', 'src/linux/sys_linux.c', 'src/linux/sys_sound.c')
  platform_inc += include_directories('third-party/glad/include')

elif opt_platform == 'macos'
  arg_code += ['-DPLATFORM_MACOS', '-DDESKTOP']
  arg_base += ['-fdiagnostics-color']

  if opt_buildtype == 'debug'
    arg_code += ['-DDEBUG', '-g', '-O0', '-fno-inline', '-fno-omit-frame-pointer']
  else
    arg_code += [
      '-DNDEBUG',
      '-DQUIET',
      '-g0',
      '-Os',
      '-ffast-math',
      '-funsafe-math-optimizations',
      '-fomit-frame-pointer',
    ]
  endif

  #glad_dep = dependency('glad', fallback: ['glad', 'glad_dep'])
  glfw_dep = dependency('glfw3', fallback: ['glfw', 'glfw_dep'])
  openal_dep = dependency('openal-soft', fallback: ['openal-soft', 'openal_dep'])
  stb_dep = dependency('stb', fallback: ['stb', 'stb_dep'])
  cglm_dep = dependency('cglm', fallback: ['cglm', 'cglm_dep'])

  platform_dep += [ glfw_dep, openal_dep, stb_dep, cglm_dep]
  platform_src += files('src/macos/glad.c', 'src/macos/sys_macos.c', 'src/macos/sys_sound.c')
  platform_inc += include_directories('third-party/glad/include')

elif opt_platform == 'windows'
  arg_code += ['-DPLATFORM_WINDOWS', '-DDESKTOP']
  arg_base += ['-fdiagnostics-color']

  if opt_buildtype == 'debug'
    arg_code += ['-DDEBUG', '-g', '-O0', '-fno-inline', '-fno-omit-frame-pointer']
  else
    arg_code += [
      '-DNDEBUG',
      '-DQUIET',
      '-g0',
      '-Os',
      '-ffast-math',
      '-funsafe-math-optimizations',
      '-fomit-frame-pointer',
    ]
  endif

  #glad_dep = dependency('glad', fallback: ['glad', 'glad_dep'])
  glfw_dep = dependency('glfw3', fallback: ['glfw', 'glfw_dep'])
  openal_dep = dependency('openal', fallback: ['openal', 'openal_dep'])
  cglm_dep = dependency('cglm', fallback: ['cglm', 'cglm_dep'])

  platform_dep += [ glfw_dep, openal_dep, cglm_dep]
  platform_src += files(
    'src/windows/glad.c',
    'src/windows/sys_sound.c',
    'src/windows/sys_windows.c',
  )
  platform_inc += include_directories('deps/glad/include', 'deps/openal-win/include')
  platform_inc += include_directories('third-party/glad/include')

elif opt_platform == 'dreamcast'
  arg_code += ['-DPLATFORM_DREAMCAST', '-DBUILD_LIBGL']
  arg_base += [
    '-mpretend-cmove',
    '-fsingle-precision-constant',
    '-fomit-frame-pointer',
    '-finline-functions',
    '-mfsrra',
    '-mfsca',
  ]

  if opt_buildtype == 'debug'
    arg_code += ['-DDEBUG', '-g', '-O0', '-fno-inline', '-fno-omit-frame-pointer']
  else
    arg_code += [
      '-DNDEBUG',
      '-DQUIET',
      '-g0',
      '-Os',
      '-funsafe-math-optimizations',
      '-fomit-frame-pointer',
    ]
  endif

  gldc_dep = dependency('gldc', fallback: ['gldc', 'libgldc_dep'])
  crayonvmu_dep = dependency('crayonvmu', fallback: ['crayonvmu', 'libcrayonvmu_dep'])
  cglm_dep = dependency('cglm', fallback: ['cglm', 'cglm_dep'])

  platform_dep += [gldc_dep, crayonvmu_dep, cglm_dep]
  platform_src += files(
    'src/dreamcast/perfctr.c',
    'src/dreamcast/snd_wavload.c',
    'src/dreamcast/sys_dreamcast.c',
    'src/dreamcast/sys_sound.c',
  )
  platform_inc += include_directories('deps/openal-dc/include') #'deps/minilzo',
  #platform_inc += include_directories('src', 'src/common')
  platform_inc += include_directories(
    '/opt/toolchains/dc/kos/include',
    '/opt/toolchains/dc/kos/kernel/arch/dreamcast/include',
    '/opt/toolchains/dc/kos/addons/include',
  )

elif opt_platform == 'psp'
  arg_code += ['-DPLATFORM_PSP']
  arg_base += [
    '-fsingle-precision-constant',
    '-ffast-math',
    '-fexpensive-optimizations',
    '-fomit-frame-pointer',
    '-finline-functions',
    '-Wdouble-promotion',
  ]

  if opt_buildtype == 'debug'
    arg_code += ['-DDEBUG', '-g3']
  else
    arg_code += ['-DNDEBUG', '-DQUIET', '-g0', '-Os']
  endif

  arg_linker += [
    '-Wl,--end-group',
    '-lpspgu',
    '-lpspgum',
    '-lpspdebug',
    '-lpspvfpu',
    '-lpspctrl',
    '-lpspge',
    '-lpspdisplay',
    '-lm',
    '-lpspsdk',
    '-lpsprtc',
    '-lpspaudio',
    '-lpsppower',
    '-Wl,--start-group',
    '-lc',
  ]
  arg_linker += [
    '-specs=/opt/toolchains/pspdev/psp/sdk/lib/prxspecs',
  ]

  #openal_dep = dependency('openal', fallback : ['openal', 'openal_dep'])
  pspgl_dep = dependency('pspgl', fallback: ['pspgl', 'pspgl_dep'])
  cglm_dep = dependency('cglm', fallback: ['cglm', 'cglm_dep'])

  #openal_lib_path = cc.find_library(
  #    'openal-soft',
  #    dirs: [meson.source_root() / 'deps' / 'openal-psp' / 'lib'],
  #    static: true,
  #)
  #openal_dep = declare_dependency(
  #    dependencies: openal_lib_path,
  #    include_directories: include_directories('deps/openal-psp/include'),
  #)

  platform_dep += [pspgl_dep, cglm_dep]
  platform_src += files('src/psp/sys_psp.c', 'src/psp/sys_sound.c')

else
  error('Unsupported platform: ' + opt_platform)
endif

# Combine all sources and includes
src_files += platform_src
inc = [inc_base, platform_inc]
dep = [m_dep, umka_dep, platform_dep]
arg_c = [arg_base, arg_code]

# Build the library
gamejam_lib = static_library(
  'gamejam',
  c_args: arg_c,
  link_args: arg_linker,
  sources: src_files,
  include_directories: inc,
  dependencies: dep,
  install: true,
)

# Cooker (asset processing tool)
subdir('src/cooker')

# Examples
# Declare gamejam library for examples
gamejam_dep = declare_dependency(
  include_directories: include_directories('include'),
  compile_args: arg_c,
  link_args: arg_linker,
  dependencies: [platform_dep],
  link_with: gamejam_lib,
)

#subdir('examples')
