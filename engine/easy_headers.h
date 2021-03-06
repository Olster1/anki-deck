#include <GL/gl3w.h>

#include "../SDL2/sdl.h"
#include "../SDL2/SDL_syswm.h"

#include <stdio.h>
#include <time.h> // to init random generator
#include "easy_types.h"
// #include <string.h>

#if DEVELOPER_MODE
// #include "stb_stretch_buffer.h"
#endif

#include "easy_assert.h"
#include "easy_logger.h"
#include "easy_debug_variables.h"
#include "easy_time.h"
#include "easy_profiler.h"
#include "easy.h"
#include "easy_platform.h"

static char* globalExeBasePath;

#include "easy_files.h"
#include "easy_math.h"
#include "easy_error.h"
#include "easy_array.h"
#include "sdl_audio.h"
#include "easy_lex.h"
#include "easy_transform.h"
#include "easy_render.h"

#include "easy_perlin.h"

#include "easy_utf8.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "easy_timer.h"
#include "easy_animation.h"
#include "easy_assets.h"
#include "easy_text_io.h"
#include "easy_texture_atlas.h"
#include "easy_font.h"





#include "easy_3d.h"
#include "easy_terrain.h"

//#include "easy_sdl_joystick.h"

#include "easy_tweaks.h"
#include "easy_particle_effects.h"
#include "easy_string_compile.h"
// #include "easy_skeleton.h"


#include "easy_color.h"
#include "easy_transition.h"

#include "easy_asset_loader.h"
#include "easy_os.h"
#include "easy_editor.h"
#include "easy_camera.h"
#include "easy_tile.h"
// #include "easy_ui.h"
#include "easy_console.h"
#include "easy_flash_text.h"
#include "easy_ast.h"

#define GJK_IMPLEMENTATION 
#include "easy_gjk.h"

#include "easy_collision.h"
#include "easy_physics.h"

// #include "easy_write.h"
#include "easy_profiler_draw.h"


