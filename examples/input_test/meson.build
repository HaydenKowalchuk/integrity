#Input test example
example_src = ['input_test.c']

input_test_exe = executable(
  'input_test',
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
  run_target('run_input_test', command: [input_test_exe])
endif

#PSP post - processing
if opt_platform == 'psp'
  mksfo = find_program('mksfo', required: true)
  psp_fixup = find_program('psp-fixup-imports', required: true)
  psp_strip_tool = find_program('psp-strip', required: true)
  pack_pbp = find_program('pack-pbp', required: true)

  exe_dir = meson.current_build_dir()
  custom_target(
    'input_test_psp_pbp',
    output: 'EBOOT.PBP',
    command: [
      find_program('sh'),
      '-c', 'cd "'
      + exe_dir
      + '" && '
      + mksfo.full_path()
      + ' "input_test" param.sfo && '
      + 'psp-fixup-imports input_test && '
      + 'psp-prxgen input_test -o input_test.prx && '
      + 'pack-pbp EBOOT.PBP param.sfo NULL NULL NULL NULL NULL input_test.prx NULL',
    ],
    depends: input_test_exe,
    install: true,
    install_tag: 'input_test',
    install_dir: '.',
  )

endif
