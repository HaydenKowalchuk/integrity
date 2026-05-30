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

if opt_platform == 'psp'
  psp_exe = example_hello_exe
  psp_title = 'Integrity: Hello'

  mksfo = find_program('mksfo', required: true)
  psp_fixup = find_program('psp-fixup-imports', required: true)
  psp_prxgen = find_program('psp-prxgen', required: true)
  pack_pbp = find_program('pack-pbp', required: true)

  icon0 = meson.project_source_root() / 'assets' / 'icon0.png'
  pic1 = meson.project_source_root() / 'assets' / 'pic1.png'

  sfo = custom_target(
    'hello_sfo',
    output: 'param.sfo',
    command: [mksfo, psp_title, '@OUTPUT@'],
  )

  custom_target(
    'hello_pbp',
    input: [psp_exe, sfo],
    output: 'EBOOT.PBP',
    command: [
      find_program('sh'),
      '-c', 'psp-fixup-imports $1 && '
      + 'psp-prxgen $1 $1.prx && '
      + 'pack-pbp $2 $3 $4  NULL NULL $5 NULL $1.prx NULL',
      'psp_pack_shim',
      psp_exe.full_path(), # $1
      '@OUTPUT@', # $2
      sfo.full_path(), # $3
      meson.project_source_root() / 'assets/icon0.png', # $4
      meson.project_source_root() / 'assets/pic1.png',
      psp_fixup,
      psp_prxgen,
      pack_pbp,
    ],
    build_by_default: true,
    install: true,
    install_dir: '.',
  )
endif
