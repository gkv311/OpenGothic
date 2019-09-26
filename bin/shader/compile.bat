glslangValidator -V sky.vert -o sky.vert.sprv
glslangValidator -V sky.frag -o sky.frag.sprv

glslangValidator -V shadow_compose.vert -o shadow_compose.vert.sprv
glslangValidator -V shadow_compose.frag -o shadow_compose.frag.sprv

# Ubershader flags:
#   OBJ        - enable object matrix
#   SKINING    - animation skeleton
#   SHADOW_MAP - output is shadowmap
#   ATEST      - use alpha test
#   PFX        - use color modulation

glslangValidator -V -DATEST                              anim.vert   -o land.vert.sprv
glslangValidator -V -DATEST                              anim.frag   -o land.frag.sprv
                    -DATEST
glslangValidator -V -DATEST -DOBJ                        anim.vert   -o object.vert.sprv
glslangValidator -V -DATEST -DOBJ                        anim.frag   -o object.frag.sprv
                    -DATEST
glslangValidator -V -DATEST -DOBJ -DSKINING              anim.vert   -o anim.vert.sprv
glslangValidator -V -DATEST -DOBJ -DSKINING              anim.frag   -o anim.frag.sprv
                    -DATEST
glslangValidator -V -DPFX                                anim.vert   -o pfx.vert.sprv
glslangValidator -V -DPFX                                anim.frag   -o pfx.frag.sprv


glslangValidator -V -DATEST -DSHADOW_MAP                 anim.vert   -o land_shadow.vert.sprv
glslangValidator -V -DATEST -DSHADOW_MAP                 anim.frag   -o land_shadow.frag.sprv
                    -DATEST
glslangValidator -V -DATEST -DOBJ -DSHADOW_MAP           anim.vert   -o object_shadow.vert.sprv
glslangValidator -V -DATEST -DOBJ -DSHADOW_MAP           anim.frag   -o object_shadow.frag.sprv
                    -DATEST
glslangValidator -V -DATEST -DOBJ -DSKINING -DSHADOW_MAP anim.vert   -o anim_shadow.vert.sprv
glslangValidator -V -DATEST -DOBJ -DSKINING -DSHADOW_MAP anim.frag   -o anim_shadow.frag.sprv

glslangValidator -V -DSHADOW_MAP                         anim.vert   -o pfx_shadow.vert.sprv
glslangValidator -V -DSHADOW_MAP                         anim.frag   -o pfx_shadow.frag.sprv
