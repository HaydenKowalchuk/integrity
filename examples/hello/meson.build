example_src = ['helloworld_scene.c']

example_hello_exe = executable(
  'hello',
  c_args: arg_c,
  link_args: arg_linker,
  sources: example_src,
  include_directories: inc,
  dependencies: dep,
  link_with: gamejam_lib,
  install: true,
)

run_target('run_hello', command: [example_hello_exe])
