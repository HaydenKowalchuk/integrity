#Hello example
example_src = [] example_src += files('helloworld_scene.c')

    executable(
        'hello',
        c_args : arg_c,
        link_args : arg_linker,
        sources : example_src,
        include_directories : inc,
        dependencies : dep,
        link_with : gamejam_lib,
        install : true, )
