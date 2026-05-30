example_src = ['helloworld_scene.c']

example_hello_exe = executable(
  'hello',
  c_args: arg_c,
  link_args: arg_linker,
  sources: example_src,
  include_directories: inc,
  dependencies: dep,
  link_with: integrity_lib,
  install: true,
)

# Only create the run target if we aren't cross-compiling,
# OR if an executor wrapper is explicitly available.
if not meson.is_cross_build() or meson.can_run_host_binaries()
  run_target('run_hello', command: [example_hello_exe])
endif
