TARGET_EXEC ? = emfconv

                    include../
                    ../ makefiles / config.mak

                                        check
            : @echo $(SRCS)

                      .PHONY : test

                                   test : debug
                                              cmd.exe /
                  k "cd release\\windows && gdb .\\$(TARGET_EXEC)_d.exe  & exit"