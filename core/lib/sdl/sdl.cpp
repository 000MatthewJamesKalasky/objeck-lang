/***************************************************************************
 * SDL support for Objeck
 *
 * Copyright (c) 2015-2019, Randy Hollines
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *
 * - Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.
 * - Neither the name of the Objeck Team nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

#ifdef _OSX
#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#else
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include "SDL2_gfxPrimitives.h"
#endif
#include <stdio.h>
#include "../../vm/lib_api.h"
#include "../../shared/sys.h"

using namespace std;

extern "C" {
  void sdl_color_raw_read(SDL_Color* color, size_t* color_obj);
  void sdl_color_raw_write(SDL_Color* color, size_t* color_obj);

  void sdl_point_raw_read(SDL_Point* point, size_t* point_obj);
  void sdl_point_raw_write(SDL_Point* point, size_t* point_obj);

  void sdl_rect_raw_read(SDL_Rect* rect, size_t* rect_obj);
  void sdl_rect_raw_write(SDL_Rect* rect, size_t* rect_obj);

  void sdl_pixel_format_raw_read(SDL_PixelFormat* pixel_format, size_t* pixel_format_obj);
  void sdl_pixel_format_raw_write(SDL_PixelFormat* pixel_format, size_t* pixel_format_obj);

  void sdl_palette_raw_read(SDL_Palette* palette_format, size_t* palette_format_obj);
  void sdl_palette_raw_write(SDL_Palette* palette_format, size_t* palette_format_obj);

  void sdl_display_mode_raw_read(SDL_DisplayMode* mode, size_t* display_mode_obj);
  void sdl_display_mode_raw_write(SDL_DisplayMode* mode, size_t* display_mode_obj);

  //
  // initialize library
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void load_lib() {
  }

  //
  // release library
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void unload_lib() {
  }

  //
  // SDL Core
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_core_gl_get_swap_interval(VMContext& context) {
    APITools_SetIntValue(context, 0, SDL_GL_GetSwapInterval());
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_core_get_error(VMContext& context) {
    const string return_value = SDL_GetError();
    const wstring w_return_value(return_value.begin(), return_value.end());
    APITools_SetStringValue(context, 0, w_return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_core_clear_error(VMContext& context) {
    SDL_ClearError();
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_core_gl_set_swap_interval(VMContext& context) {
    const int interval = (int)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, SDL_GL_SetSwapInterval(interval));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_core_gl_get_current(VMContext& context) {
    SDL_Window* window = SDL_GL_GetCurrentWindow();
    APITools_SetIntValue(context, 0, (size_t)window);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_core_gl_get_attribute(VMContext& context) {
    const int attr = (int)APITools_GetIntValue(context, 1);
    
    int value;
    const int return_value = SDL_GL_GetAttribute((SDL_GLattr)attr, &value);

    APITools_SetIntValue(context, 2, value);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_core_gl_set_attribute(VMContext& context) {
    const int attr = (int)APITools_GetIntValue(context, 1);
    const int value = (int)APITools_GetIntValue(context, 2);
    APITools_SetIntValue(context, 0, SDL_GL_SetAttribute((SDL_GLattr)attr, value));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_core_gl_reset_attributes(VMContext& context) {
    SDL_GL_ResetAttributes();
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_core_gl_extension_supported(VMContext& context) {
    const wstring w_extension = APITools_GetStringValue(context, 1);
    const string extension = UnicodeToBytes(w_extension);
    const int return_value = SDL_GL_ExtensionSupported(extension.c_str());
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_core_gl_load_library(VMContext& context) {
    const wchar_t* w_path = APITools_GetStringValue(context, 1);
    if(w_path) {
      const string path = UnicodeToBytes(w_path);
      APITools_SetIntValue(context, 0, SDL_GL_LoadLibrary(path.c_str()));
    }
    else {
      APITools_SetIntValue(context, 0, SDL_GL_LoadLibrary(NULL));
    }
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_core_gl_unload_library(VMContext& context) {
    SDL_GL_UnloadLibrary();
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_core_init(VMContext& context) {
    const int flags = (int)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, SDL_Init(flags));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_core_init_sub_system(VMContext& context) {
    const int flags = (int)APITools_GetIntValue(context, 1);
    const int return_value = (int)SDL_InitSubSystem(flags);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_core_quit_sub_system(VMContext& context) {
    const int flags = (int)APITools_GetIntValue(context, 0);
    SDL_QuitSubSystem(flags);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_core_was_init(VMContext& context) {
    const int flags = (int)APITools_GetIntValue(context, 1);
    const int return_value = SDL_WasInit(flags);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_core_quit(VMContext& context) {
    SDL_Quit();
  }

  //
  // Hints
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_hints_set_hint_with_priority(VMContext& context) {
    const wstring w_name = APITools_GetStringValue(context, 1);
    const string name = UnicodeToBytes(w_name);

    const wstring w_value = APITools_GetStringValue(context, 2);
    const string value = UnicodeToBytes(w_value);

    const int priority = (int)APITools_GetIntValue(context, 3);

    const int return_value = SDL_SetHintWithPriority(name.c_str(), value.c_str(), (SDL_HintPriority)priority);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_hints_set_hint(VMContext& context) {
    const wstring w_name = APITools_GetStringValue(context, 1);
    const string name = UnicodeToBytes(w_name);

    const wstring w_value = APITools_GetStringValue(context, 2);
    const string value = UnicodeToBytes(w_value);

    const int return_value = SDL_SetHint(name.c_str(), value.c_str());
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_hints_get_hint(VMContext& context) {
    const wstring w_name = APITools_GetStringValue(context, 1);
    const string name = UnicodeToBytes(w_name);
    const string return_value = SDL_GetHint(name.c_str());

    const wstring w_return_value(return_value.begin(), return_value.end());
    APITools_SetStringValue(context, 0, w_return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_hints_clear(VMContext& context) {
    SDL_ClearHints();
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_creatergb(VMContext& context) {
    const Uint32 flags = (int)APITools_GetIntValue(context, 1);
    const int width = (int)APITools_GetIntValue(context, 2);
    const int height = (int)APITools_GetIntValue(context, 3);
    const int depth = (int)APITools_GetIntValue(context, 4);
    const Uint32 Rmask = (int)APITools_GetIntValue(context, 5);
    const Uint32 Gmask = (int)APITools_GetIntValue(context, 6);
    const Uint32 Bmask = (int)APITools_GetIntValue(context, 7);
    const Uint32 Amask = (int)APITools_GetIntValue(context, 8);

    SDL_Surface* surface = SDL_CreateRGBSurface(flags, width, height, depth, Rmask, Gmask, Bmask, Amask);
    APITools_SetIntValue(context, 0, (size_t)surface);
  }
  
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_free(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 0);
    SDL_FreeSurface(surface);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_get_pixel_format(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
	  size_t* pixel_format_obj = APITools_GetObjectValue(context, 0);
	  sdl_pixel_format_raw_read(surface->format, pixel_format_obj);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_get_w(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, (size_t)surface->w);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_get_h(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, (size_t)surface->h);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_get_pitch(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, (size_t)surface->pitch);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_create_texture(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    size_t* renderer_obj = APITools_GetObjectValue(context, 2);
    SDL_Renderer* renderer = (SDL_Renderer*)renderer_obj[0];
    APITools_SetIntValue(context, 0, (size_t)SDL_CreateTextureFromSurface(renderer, surface));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_set_palette(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);

    const size_t* palette_obj = APITools_GetObjectValue(context, 2);
    SDL_Palette* palette = palette_obj ? (SDL_Palette*)palette_obj[0] : NULL;

    APITools_SetIntValue(context, 0, SDL_SetSurfacePalette(surface, palette));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_lock(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    const int return_value = SDL_LockSurface(surface);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_unlock(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 0);
    SDL_UnlockSurface(surface);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_pixels(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    
    size_t* pixel_obj = context.alloc_obj(L"Game.SDL2.PixelData", context.op_stack, *context.stack_pos, false);
    pixel_obj[0] = (size_t)surface->pixels;
    pixel_obj[1] = (size_t)surface->pitch;
    pixel_obj[2] = (size_t)surface->h;
    APITools_SetObjectValue(context, 0, pixel_obj);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_loadbmp(VMContext& context) {
    const wstring w_file = APITools_GetStringValue(context, 1);
    const string file = UnicodeToBytes(w_file);

    SDL_Surface* surface = SDL_LoadBMP(file.c_str());
    APITools_SetIntValue(context, 0, (size_t)surface);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_savebmp(VMContext& context) {
    const size_t* surface_obj = APITools_GetObjectValue(context, 1);
    SDL_Surface* surface = surface_obj ? (SDL_Surface*)surface_obj[0] : NULL;
    const wstring w_file = APITools_GetStringValue(context, 2);
    const string file = UnicodeToBytes(w_file);

    APITools_SetIntValue(context, 0, SDL_SaveBMP(surface, file.c_str()));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_setrle(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    const int flag = (int)APITools_GetIntValue(context, 2);
    APITools_SetIntValue(context, 0, SDL_SetSurfaceRLE(surface, flag));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_set_color_key(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    const int flag = (int)APITools_GetIntValue(context, 2);
    const int key = (int)APITools_GetIntValue(context, 3);
    APITools_SetIntValue(context, 0, SDL_SetColorKey(surface, flag, key));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_get_color_key(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    
    Uint32 key;
    const int return_value = SDL_GetColorKey(surface, &key);

    APITools_SetIntValue(context, 2, key);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_set_color_mod(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    const int r = (int)APITools_GetIntValue(context, 2);
    const int g = (int)APITools_GetIntValue(context, 3);
    const int b = (int)APITools_GetIntValue(context, 4);
    APITools_SetIntValue(context, 0, SDL_SetSurfaceColorMod(surface, r, g, b));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_get_color_mod(VMContext& context) {
    Uint8 r, g, b;
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, SDL_GetSurfaceColorMod(surface, &r, &g, &b));
    APITools_SetIntValue(context, 1, r);
    APITools_SetIntValue(context, 2, g);
    APITools_SetIntValue(context, 3, b);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_set_alpha_mod(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    const int alpha = (int)APITools_GetIntValue(context, 2);
    APITools_SetIntValue(context, 0, SDL_SetSurfaceAlphaMod(surface, alpha));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_get_alpha_mod(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    
    Uint8 alpha;
    const int return_value = SDL_GetSurfaceAlphaMod(surface, &alpha);

    APITools_SetIntValue(context, 2, alpha);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_set_blend_mode(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    const SDL_BlendMode blendMode = (SDL_BlendMode)APITools_GetIntValue(context, 2);
    APITools_SetIntValue(context, 0, SDL_SetSurfaceBlendMode(surface, blendMode));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_get_blend_mode(VMContext& context) {
    
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);

    SDL_BlendMode blendMode;
    const int return_value = SDL_GetSurfaceBlendMode(surface, &blendMode);

    APITools_SetIntValue(context, 2, blendMode);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_get_clip_rect(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 0);
    size_t* rect_obj = APITools_GetObjectValue(context, 1);
    
    SDL_Rect rect;
    SDL_GetClipRect(surface, &rect);
    sdl_rect_raw_read(&rect, rect_obj);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_set_clip_rect(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    size_t* rect_obj = APITools_GetObjectValue(context, 2);
    
    SDL_Rect rect;
    sdl_rect_raw_write(&rect, rect_obj);

    APITools_SetIntValue(context, 0, SDL_SetClipRect(surface, rect_obj ? &rect : NULL));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_convert(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    size_t* fmt_obj = APITools_GetObjectValue(context, 2);
    const int flags = (int)APITools_GetIntValue(context, 3);

    SDL_PixelFormat fmt;
    sdl_pixel_format_raw_write(&fmt, fmt_obj);

    APITools_SetIntValue(context, 0, (size_t)SDL_ConvertSurface(surface, fmt_obj ? &fmt : NULL, flags));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_convert_format(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    const int pixel_format = (int)APITools_GetIntValue(context, 2);
    const int flags = (int)APITools_GetIntValue(context, 3);

    APITools_SetIntValue(context, 0, (size_t)SDL_ConvertSurfaceFormat(surface, pixel_format, flags));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_pixelformat_maprgb(VMContext& context) {
    size_t* pixel_format_obj = APITools_GetObjectValue(context, 1);

    SDL_PixelFormat pixel_format;
    sdl_pixel_format_raw_write(&pixel_format, pixel_format_obj);

    const int r = (int)APITools_GetIntValue(context, 2);
    const int g = (int)APITools_GetIntValue(context, 3);
    const int b = (int)APITools_GetIntValue(context, 4);

    APITools_SetIntValue(context, 0, SDL_MapRGB(pixel_format_obj ? &pixel_format : NULL, r, g, b));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_pixelformat_maprgba(VMContext& context) {
    size_t* pixel_format_obj = APITools_GetObjectValue(context, 1);

    SDL_PixelFormat pixel_format;
    sdl_pixel_format_raw_write(&pixel_format, pixel_format_obj);

    const int r = (int)APITools_GetIntValue(context, 2);
    const int g = (int)APITools_GetIntValue(context, 3);
    const int b = (int)APITools_GetIntValue(context, 4);
    const int a = (int)APITools_GetIntValue(context, 5);

    APITools_SetIntValue(context, 0, SDL_MapRGBA(pixel_format_obj ? &pixel_format : NULL, r, g, b, a));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_pixelformat_alloc(VMContext& context) {
    size_t* pixel_format_obj = APITools_GetObjectValue(context, 0);
    const int format = (int)APITools_GetIntValue(context, 1);

    SDL_PixelFormat* pixel_format = SDL_AllocFormat(format);
    sdl_pixel_format_raw_read(pixel_format, pixel_format_obj);
    SDL_FreeFormat(pixel_format);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_fill_rect(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);
    size_t* rect_obj = APITools_GetObjectValue(context, 2);
    const int color = (int)APITools_GetIntValue(context, 3);

    SDL_Rect rect;
    sdl_rect_raw_write(&rect, rect_obj);

    APITools_SetIntValue(context, 0, SDL_FillRect(surface, rect_obj ? &rect : NULL, color));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_upper_blit(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);

    size_t* srcrect_obj = APITools_GetObjectValue(context, 2);
    SDL_Rect srcrect;
    sdl_rect_raw_write(&srcrect, srcrect_obj);

    const size_t* dst_obj = APITools_GetObjectValue(context, 3);
    SDL_Surface* dst = dst_obj ? (SDL_Surface*)dst_obj[0] : NULL;

    size_t* dstrect_obj = APITools_GetObjectValue(context, 4);
    SDL_Rect dstrect;
    sdl_rect_raw_write(&dstrect, dstrect_obj);

    APITools_SetIntValue(context, 0, SDL_BlitSurface(surface,
      srcrect_obj ? &srcrect : NULL, dst, dstrect_obj ? &dstrect : NULL));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_lower_blit(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);

    size_t* srcrect_obj = APITools_GetObjectValue(context, 2);
    SDL_Rect srcrect;
    sdl_rect_raw_write(&srcrect, srcrect_obj);

    const size_t* dst_obj = APITools_GetObjectValue(context, 3);
    SDL_Surface* dst = dst_obj ? (SDL_Surface*)dst_obj[0] : NULL;

    size_t* dstrect_obj = APITools_GetObjectValue(context, 4);
    SDL_Rect dstrect;
    sdl_rect_raw_write(&dstrect, dstrect_obj);

    APITools_SetIntValue(context, 0, SDL_LowerBlit(surface, srcrect_obj ? &srcrect : NULL, 
      dst, dstrect_obj ? &dstrect : NULL));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_blit_scaled(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);

    size_t* srcrect_obj = APITools_GetObjectValue(context, 2);
    SDL_Rect srcrect;
    sdl_rect_raw_write(&srcrect, srcrect_obj);

    const size_t* dst_obj = APITools_GetObjectValue(context, 3);
    SDL_Surface* dst = dst_obj ? (SDL_Surface*)dst_obj[0] : NULL;

    size_t* dstrect_obj = APITools_GetObjectValue(context, 4);
    SDL_Rect dstrect;
    sdl_rect_raw_write(&dstrect, dstrect_obj);

    APITools_SetIntValue(context, 0, SDL_BlitScaled(surface,
      srcrect_obj ? &srcrect : NULL, dst, dstrect_obj ? &dstrect : NULL));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_surface_lower_blit_scaled(VMContext& context) {
    SDL_Surface* surface = (SDL_Surface*)APITools_GetIntValue(context, 1);

    size_t* srcrect_obj = APITools_GetObjectValue(context, 2);
    SDL_Rect srcrect;
    sdl_rect_raw_write(&srcrect, srcrect_obj);

    const size_t* dst_obj = APITools_GetObjectValue(context, 3);
    SDL_Surface* dst = dst_obj ? (SDL_Surface*)dst_obj[0] : NULL;

    size_t* dstrect_obj = APITools_GetObjectValue(context, 4);
    SDL_Rect dstrect;
    sdl_rect_raw_write(&dstrect, dstrect_obj);

    APITools_SetIntValue(context, 0, SDL_LowerBlitScaled(surface, srcrect_obj ? &srcrect : NULL, dst, dstrect_obj ? &dstrect : NULL));
  }

  //
  // SDL_PixelFormat
  //
  void sdl_pixel_format_raw_read(SDL_PixelFormat* pixel_format, size_t* pixel_format_obj) {
    if(pixel_format_obj) {
      pixel_format_obj[0] = pixel_format->format;
      if(pixel_format->palette) {
        size_t* palette_obj = (size_t*)pixel_format_obj[1];
        sdl_palette_raw_read(pixel_format->palette, palette_obj);
      }
      else {
        pixel_format_obj[1] = 0;
      }
      pixel_format_obj[2] = pixel_format->BitsPerPixel;
      pixel_format_obj[3] = pixel_format->BytesPerPixel;
      pixel_format_obj[4] = pixel_format->Rmask;
      pixel_format_obj[5] = pixel_format->Gmask;
      pixel_format_obj[6] = pixel_format->Bmask;
      pixel_format_obj[7] = pixel_format->Amask;
      pixel_format_obj[8] = pixel_format->Rloss;
      pixel_format_obj[9] = pixel_format->Gloss;
      pixel_format_obj[10] = pixel_format->Bloss;
      pixel_format_obj[11] = pixel_format->Aloss;
      pixel_format_obj[12] = pixel_format->Rshift;
      pixel_format_obj[13] = pixel_format->Gshift;
      pixel_format_obj[14] = pixel_format->Bshift;
      pixel_format_obj[15] = pixel_format->Ashift;
    }
  }

  void sdl_pixel_format_raw_write(SDL_PixelFormat* pixel_format, size_t* pixel_format_obj) {
    if(pixel_format_obj) {
      pixel_format->format = (Uint32)pixel_format_obj[0];
      if(pixel_format_obj[1]) {
          size_t* palette_obj = (size_t*)pixel_format_obj[1];
          sdl_palette_raw_write(pixel_format->palette, palette_obj);
      }
      else {
        pixel_format->palette = NULL;
      }
      pixel_format->BitsPerPixel = (Uint8)pixel_format_obj[2];
      pixel_format->BytesPerPixel = (Uint8)pixel_format_obj[3];
      pixel_format->Rmask = (Uint32)pixel_format_obj[4];
      pixel_format->Gmask = (Uint32)pixel_format_obj[5];
      pixel_format->Bmask = (Uint32)pixel_format_obj[6];
      pixel_format->Amask = (Uint32)pixel_format_obj[7];
      pixel_format->Rloss = (Uint8)pixel_format_obj[8];
      pixel_format->Gloss = (Uint8)pixel_format_obj[9];
      pixel_format->Bloss = (Uint8)pixel_format_obj[10];
      pixel_format->Aloss = (Uint8)pixel_format_obj[11];
      pixel_format->Rshift = (Uint8)pixel_format_obj[12];
      pixel_format->Gshift = (Uint8)pixel_format_obj[13];
      pixel_format->Bshift = (Uint8)pixel_format_obj[14];
      pixel_format->Ashift = (Uint8)pixel_format_obj[15];
    }
  }

  //
  // Color
  //
  void sdl_color_raw_read(SDL_Color* color, size_t* color_obj) {
    if(color) {
      color_obj[0] = color->r;
      color_obj[1] = color->g;
      color_obj[2] = color->b;
      color_obj[3] = color->a;
    }
  }
  
  void sdl_color_raw_write(SDL_Color* color, size_t* color_obj) {
    if(color_obj) {
      color->r = (Uint8)color_obj[0];
      color->g = (Uint8)color_obj[1];
      color->b = (Uint8)color_obj[2];
      color->a = (Uint8)color_obj[3];
    }
  }

  //
  // SDL_Point
  //
  void sdl_point_raw_read(SDL_Point* point, size_t* point_obj) {
    if(point) {
      point_obj[0] = point->x;
      point_obj[1] = point->y;
    }
  }

  void sdl_point_raw_write(SDL_Point* point, size_t* point_obj) {
    if(point_obj) {
      point->x = (int)point_obj[0];
      point->y = (int)point_obj[1];
    }
  }

  //
  // SDL_Rect
  //
  void sdl_rect_raw_read(SDL_Rect* rect, size_t* rect_obj) {
    if(rect && rect_obj) {
      rect_obj[0] = rect->x;
      rect_obj[1] = rect->y;
      rect_obj[2] = rect->w;
      rect_obj[3] = rect->h;
    }
  }

  void sdl_rect_raw_write(SDL_Rect* rect, size_t* rect_obj) {
    if(rect && rect_obj) {
      rect->x = (int)rect_obj[0];
      rect->y = (int)rect_obj[1];
      rect->w = (int)rect_obj[2];
      rect->h = (int)rect_obj[3];
    }
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_rect_has_intersection(VMContext& context) {
    size_t* rect_obj = APITools_GetObjectValue(context, 1);
    SDL_Rect rect;
    sdl_rect_raw_write(&rect, rect_obj);

    size_t* B_obj = APITools_GetObjectValue(context, 2);
    SDL_Rect B;
    sdl_rect_raw_write(&B, B_obj);

    APITools_SetIntValue(context, 0, SDL_HasIntersection(rect_obj ? &rect : NULL, B_obj ? &B : NULL));
  }
  
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_rect_intersect(VMContext& context) {
    size_t* rect_obj = APITools_GetObjectValue(context, 1);
    SDL_Rect rect;
    sdl_rect_raw_write(&rect, rect_obj);

    size_t* B_obj = APITools_GetObjectValue(context, 2);
    SDL_Rect B;
    sdl_rect_raw_write(&B, B_obj);

    size_t* C_obj = APITools_GetObjectValue(context, 3);
    SDL_Rect C;
    const int return_value = SDL_IntersectRect(rect_obj ? &rect : NULL, B_obj ? &B : NULL, C_obj ? &C : NULL);
    sdl_rect_raw_read(&C, C_obj);

    APITools_SetIntValue(context, 0, return_value);
  }
  
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_rect_union(VMContext& context) {
    size_t* rect_obj = APITools_GetObjectValue(context, 0);
    SDL_Rect rect;
    sdl_rect_raw_write(&rect, rect_obj);

    size_t* B_obj = APITools_GetObjectValue(context, 1);
    SDL_Rect B;
    sdl_rect_raw_write(&B, B_obj);

    size_t* C_obj = APITools_GetObjectValue(context, 2);
    
    SDL_Rect C;
    SDL_UnionRect(rect_obj ? &rect : NULL, B_obj ? &B : NULL, &C);
    sdl_rect_raw_read(&C, C_obj);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_rect_intersect_and_line(VMContext& context) {
    size_t* rect_obj = APITools_GetObjectValue(context, 1);
    SDL_Rect rect;
    sdl_rect_raw_write(&rect, rect_obj);

    int X1 = (int)APITools_GetIntValue(context, 2);
    int Y1 = (int)APITools_GetIntValue(context, 3);
    int X2 = (int)APITools_GetIntValue(context, 4);
    int Y2 = (int)APITools_GetIntValue(context, 5);

    APITools_SetIntValue(context, 0, SDL_IntersectRectAndLine(rect_obj ? &rect : NULL, &X1, &Y1, &X2, &Y2));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_rect_point_in_rect(VMContext& context) {
    SDL_Rect rect;
    size_t* rect_obj = APITools_GetObjectValue(context, 1);
    sdl_rect_raw_write(&rect, rect_obj);

    SDL_Point point;
    size_t* point_obj = (size_t*)APITools_GetObjectValue(context, 2);
    sdl_point_raw_write(&point, point_obj);

    APITools_SetIntValue(context, 0, SDL_PointInRect(point_obj ? &point : NULL, rect_obj ? &rect : NULL));
  }

  //
  // Display
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_display_get_num_video_drivers(VMContext& context) {
    APITools_SetIntValue(context, 0, SDL_GetNumVideoDrivers());
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_display_get_video_driver(VMContext& context) {
    const int index = (int)APITools_GetIntValue(context, 1);
    const string value = SDL_GetVideoDriver(index);
    const wstring w_value(value.begin(), value.end());
    APITools_SetStringValue(context, 0, w_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_display_video_init(VMContext& context) {
    const wstring w_driver_name = APITools_GetStringValue(context, 1);
    const string driver_name = UnicodeToBytes(w_driver_name);
    APITools_SetIntValue(context, 0, SDL_VideoInit(driver_name.c_str()));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_display_video_quit(VMContext& context) {
    SDL_VideoQuit();
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_display_get_current_video_driver(VMContext& context) {
    const string value = SDL_GetCurrentVideoDriver();
    const wstring w_value(value.begin(), value.end());
    APITools_SetStringValue(context, 0, w_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_display_get_num_video_displays(VMContext& context) {
    APITools_SetIntValue(context, 0, SDL_GetNumVideoDisplays());
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_display_get_display_name(VMContext& context) {
    const int displayIndex = (int)APITools_GetIntValue(context, 1);
    const string value = SDL_GetDisplayName(displayIndex);
    const wstring w_value = BytesToUnicode(value);
    APITools_SetStringValue(context, 0, w_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_display_get_display_bounds(VMContext& context) {
    const int displayIndex = (int)APITools_GetIntValue(context, 1);
    size_t* rect_obj = APITools_GetObjectValue(context, 2);

    SDL_Rect rect;
    const int return_value = SDL_GetDisplayBounds(displayIndex, &rect);
    sdl_rect_raw_read(&rect, rect_obj);

    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_display_get_num_display_modes(VMContext& context) {
    const int displayIndex = (int)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, SDL_GetNumDisplayModes(displayIndex));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_display_get_display_mode(VMContext& context) {
    const int displayIndex = (int)APITools_GetIntValue(context, 1);
    const int modeIndex = (int)APITools_GetIntValue(context, 2);
    size_t* mode_obj = APITools_GetObjectValue(context, 3);
    
    SDL_DisplayMode mode;
    const int return_value = SDL_GetDisplayMode(displayIndex, modeIndex, &mode);
    sdl_display_mode_raw_read(&mode, mode_obj);

    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_display_get_current_display_mode(VMContext& context) {
    const int displayIndex = (int)APITools_GetIntValue(context, 1);
    size_t* mode_obj = APITools_GetObjectValue(context, 2);

    SDL_DisplayMode mode;
    const int return_value = SDL_GetCurrentDisplayMode(displayIndex, &mode);
    sdl_display_mode_raw_read(&mode, mode_obj);
    
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_display_get_closest_display_mode(VMContext& context) {
    const int displayIndex = (int)APITools_GetIntValue(context, 1);
    size_t* mode_obj = APITools_GetObjectValue(context, 2);
    size_t* closest_obj = APITools_GetObjectValue(context, 3);

    SDL_DisplayMode mode;
    sdl_display_mode_raw_write(&mode, mode_obj);

    SDL_DisplayMode closest;
    SDL_DisplayMode* return_value = SDL_GetClosestDisplayMode(displayIndex, mode_obj ? &mode : NULL, &closest);
    sdl_display_mode_raw_read(&closest, closest_obj);

    APITools_SetIntValue(context, 0, return_value == NULL);
  }

  void sdl_display_mode_raw_read(SDL_DisplayMode* mode, size_t* display_mode_obj) {
    if(mode && display_mode_obj) {
      display_mode_obj[0] = mode->format;
      display_mode_obj[1] = mode->w;
      display_mode_obj[2] = mode->h;
      display_mode_obj[3] = mode->refresh_rate;
    }
  }

  void sdl_display_mode_raw_write(SDL_DisplayMode* mode, size_t* display_mode_obj) {
    if(mode && display_mode_obj) {
      mode->format = (int)display_mode_obj[0];
      mode->w = (int)display_mode_obj[1];
      mode->h = (int)display_mode_obj[2];
      mode->refresh_rate = (int)display_mode_obj[3];
    }
  }

  //
  // Window
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_create(VMContext& context) {
    const wstring w_title = APITools_GetStringValue(context, 1);
    const string title = UnicodeToBytes(w_title);
    const int x = (int)APITools_GetIntValue(context, 2);
    const int y = (int)APITools_GetIntValue(context, 3);
    const int w = (int)APITools_GetIntValue(context, 4);
    const int h = (int)APITools_GetIntValue(context, 5);
    const Uint32 flags = (int)APITools_GetIntValue(context, 6);
    SDL_Window* window = SDL_CreateWindow(title.c_str(), x, y, w, h, flags);
    APITools_SetIntValue(context, 0, (size_t)window);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_gl_swap(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);
    SDL_GL_SwapWindow(window);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_gl_get_drawable_size(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);

    int w, h;
    SDL_GL_GetDrawableSize(window, &w, &h);
    APITools_SetIntValue(context, 1, w);
    APITools_SetIntValue(context, 2, h);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_get_display_index(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, SDL_GetWindowDisplayIndex(window));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_set_display_mode(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 1);
    size_t* mode_obj = APITools_GetObjectValue(context, 2);

    SDL_DisplayMode mode;
    sdl_display_mode_raw_write(&mode, mode_obj);

    APITools_SetIntValue(context, 0, SDL_SetWindowDisplayMode(window, mode_obj ? &mode : NULL));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_get_pixel_format(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, SDL_GetWindowPixelFormat(window));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_getid(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, SDL_GetWindowID(window));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_get_fromid(VMContext& context) {
    const Uint32 id = (int)APITools_GetIntValue(context, 1);
    SDL_Window* window = SDL_GetWindowFromID(id);
    APITools_SetIntValue(context, 0, (size_t)window);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_get_flags(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, SDL_GetWindowFlags(window));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_set_title(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);
    const wstring w_title = APITools_GetStringValue(context, 1);
    const string title = UnicodeToBytes(w_title);
    SDL_SetWindowTitle(window, title.c_str());
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_set_icon(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);

    const size_t* icon_obj = APITools_GetObjectValue(context, 1);
    SDL_Surface* icon = icon_obj ? (SDL_Surface*)icon_obj[0] : NULL;
    
    SDL_SetWindowIcon(window, icon);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_set_position(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);
    const int x = (int)APITools_GetIntValue(context, 1);
    const int y = (int)APITools_GetIntValue(context, 2);
    SDL_SetWindowPosition(window, x, y);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_get_position(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);

    int x, y;
    SDL_GetWindowPosition(window, &x, &y);
    APITools_SetIntValue(context, 1, x);
    APITools_SetIntValue(context, 2, y);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_set_size(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);
    const int w = (int)APITools_GetIntValue(context, 1);
    const int h = (int)APITools_GetIntValue(context, 2);
    SDL_SetWindowSize(window, w, h);
  }


#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_get_size(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    APITools_SetIntValue(context, 1, w);
    APITools_SetIntValue(context, 2, h);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_set_minimum_size(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);
    const int min_w = (int)APITools_GetIntValue(context, 1);
    const int min_h = (int)APITools_GetIntValue(context, 2);
    SDL_SetWindowMinimumSize(window, min_w, min_h);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_get_minimum_size(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);

    int w, h;
    SDL_GetWindowMinimumSize(window, &w, &h);
    APITools_SetIntValue(context, 1, w);
    APITools_SetIntValue(context, 2, h);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_set_maximum_size(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);
    const int max_w = (int)APITools_GetIntValue(context, 1);
    const int max_h = (int)APITools_GetIntValue(context, 2);
    SDL_SetWindowMaximumSize(window, max_w, max_h);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_get_maximum_size(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);

    int w, h;
    SDL_GetWindowMaximumSize(window, &w, &h);
    APITools_SetIntValue(context, 1, w);
    APITools_SetIntValue(context, 2, h);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_set_bordered(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);
    const SDL_bool bordered = (SDL_bool)APITools_GetIntValue(context, 1);
    SDL_SetWindowBordered(window, bordered);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_show(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);
    SDL_ShowWindow(window);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_hide(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);
    SDL_HideWindow(window);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_raise(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);
    SDL_RaiseWindow(window);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_maximize(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);
    SDL_MaximizeWindow(window);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_minimize(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);
    SDL_MinimizeWindow(window);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_restore(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);
    SDL_RestoreWindow(window);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_set_fullscreen(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 1);
    const int flags = (int)APITools_GetIntValue(context, 2);
    APITools_SetIntValue(context, 0, SDL_SetWindowFullscreen(window, flags));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_get_surface(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 1);
    SDL_Surface* surface = SDL_GetWindowSurface(window);
    APITools_SetIntValue(context, 0, (size_t)surface);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_update_surface(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, SDL_UpdateWindowSurface(window));
  }
  
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_set_grab(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);
    const SDL_bool grabbed = (SDL_bool)APITools_GetIntValue(context, 1);
    SDL_SetWindowGrab(window, grabbed);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_get_grab(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, SDL_GetWindowGrab(window));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_set_brightness(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 1);
    const float brightness = (float)APITools_GetFloatValue(context, 2);
    APITools_SetIntValue(context, 0, SDL_SetWindowBrightness(window, brightness));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_get_brightness(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 1);
    APITools_SetFloatValue(context, 0, SDL_GetWindowBrightness(window));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_set_gamma_ramp(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 1);
    Uint16 red = (Uint16)APITools_GetIntValue(context, 2);
    Uint16 green = (Uint16)APITools_GetIntValue(context, 3);
    Uint16 blue = (Uint16)APITools_GetIntValue(context, 4);

    APITools_SetIntValue(context, 0, SDL_SetWindowGammaRamp(window, &red, &green, &blue));
    APITools_SetIntValue(context, 1, red);
    APITools_SetIntValue(context, 2, green);
    APITools_SetIntValue(context, 3, blue);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_get_gamma_ramp(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 1);

    Uint16 red, green, blue;
    APITools_SetIntValue(context, 0, SDL_GetWindowGammaRamp(window, &red, &green, &blue));
    APITools_SetIntValue(context, 1, red);
    APITools_SetIntValue(context, 2, green);
    APITools_SetIntValue(context, 3, blue);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_destroy(VMContext& context) {
    SDL_Window* window = (SDL_Window*)APITools_GetIntValue(context, 0);
    SDL_DestroyWindow(window);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_is_screen_saver_enabled(VMContext& context) {
    APITools_SetIntValue(context, 0, SDL_IsScreenSaverEnabled());
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_enable_screen_saver(VMContext& context) {
    SDL_EnableScreenSaver();
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_window_disable_screen_saver(VMContext& context) {
    SDL_DisableScreenSaver();
  }

  //
  // Event
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_event_new(VMContext& context) {
    SDL_Event* event = new SDL_Event;
    APITools_SetIntValue(context, 0, (size_t)event);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_event_free(VMContext& context) {
    SDL_Event* event = (SDL_Event*)APITools_GetIntValue(context, 1);
    delete event;
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_event_type(VMContext& context) {
    SDL_Event* event = (SDL_Event*)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, event->type);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_event_key(VMContext& context) {
    SDL_Event* event = (SDL_Event*)APITools_GetIntValue(context, 1);
    if(event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) {
      size_t* key_obj = APITools_GetObjectValue(context, 2);
      key_obj[0] = event->key.type;
      key_obj[1] = event->key.timestamp;
      key_obj[2] = event->key.windowID;
      key_obj[3] = event->key.state;
      key_obj[4] = event->key.repeat;
      
      size_t* key_sym_obj = (size_t*)key_obj[5];
      key_sym_obj[0] = event->key.keysym.scancode;
      key_sym_obj[1] = event->key.keysym.sym;
      key_sym_obj[2] = event->key.keysym.mod;

      APITools_SetIntValue(context, 0, 0);
    }
    else {
      APITools_SetIntValue(context, 0, -1);
    }
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_axis_key(VMContext& context) {
    SDL_Event* event = (SDL_Event*)APITools_GetIntValue(context, 1);
    if(event->type == SDL_JOYAXISMOTION) {
      size_t* axis_obj = APITools_GetObjectValue(context, 2);
      axis_obj[0] = event->jaxis.type;
      axis_obj[1] = event->jaxis.timestamp;
      axis_obj[2] = event->jaxis.which;
      axis_obj[3] = event->jaxis.axis;
      axis_obj[4] = event->jaxis.value;

      APITools_SetIntValue(context, 0, 0);
    }
    else {
      APITools_SetIntValue(context, 0, -1);
    }
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_text_key(VMContext& context) {
    SDL_Event* event = (SDL_Event*)APITools_GetIntValue(context, 1);
    if(event->type == SDL_TEXTINPUT) {
      size_t* text_obj = APITools_GetObjectValue(context, 2);
      text_obj[0] = event->text.type;
      text_obj[1] = event->text.timestamp;
      text_obj[2] = event->text.windowID;

      const wstring w_text = BytesToUnicode(event->text.text);
      text_obj[3] = (size_t)APITools_CreateStringValue(context, w_text);

      APITools_SetIntValue(context, 0, 0);
    }
    else {
      APITools_SetIntValue(context, 0, -1);
    }
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mouse_motion(VMContext& context) {
    SDL_Event* event = (SDL_Event*)APITools_GetIntValue(context, 1);
    if(event->type == SDL_MOUSEMOTION) {
      size_t* motion_obj = APITools_GetObjectValue(context, 2);
      motion_obj[0] = event->motion.type;
      motion_obj[1] = event->motion.timestamp;
      motion_obj[2] = event->motion.windowID;
      motion_obj[3] = event->motion.which;
      motion_obj[4] = event->motion.state;
      motion_obj[5] = event->motion.x;
      motion_obj[6] = event->motion.y;
      motion_obj[7] = event->motion.xrel;
      motion_obj[8] = event->motion.yrel;

      APITools_SetIntValue(context, 0, 0);
    }
    else {
      APITools_SetIntValue(context, 0, -1);
    }
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mouse_wheel(VMContext& context) {
    SDL_Event* event = (SDL_Event*)APITools_GetIntValue(context, 1);
    if(event->type == SDL_MOUSEWHEEL) {
      size_t* wheel_obj = APITools_GetObjectValue(context, 2);
      wheel_obj[0] = event->wheel.type;
      wheel_obj[1] = event->wheel.timestamp;
      wheel_obj[2] = event->wheel.windowID;
      wheel_obj[3] = event->wheel.which;
      wheel_obj[4] = event->wheel.x;
      wheel_obj[5] = event->wheel.y;
      wheel_obj[6] = event->wheel.direction;

      APITools_SetIntValue(context, 0, 0);
    }
    else {
      APITools_SetIntValue(context, 0, -1);
    }
  }
  
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_jbutton_key(VMContext& context) {
    SDL_Event* event = (SDL_Event*)APITools_GetIntValue(context, 1);
    if(event->type == SDL_JOYBUTTONDOWN || event->type == SDL_JOYBUTTONUP) {
      size_t* button_obj = APITools_GetObjectValue(context, 2);
      button_obj[0] = event->cbutton.type;
      button_obj[1] = event->cbutton.timestamp;
      button_obj[2] = event->cbutton.which;
      button_obj[3] = event->cbutton.button;
      button_obj[4] = event->cbutton.state;

      APITools_SetIntValue(context, 0, 0);
    }
    else {
      APITools_SetIntValue(context, 0, -1);
    }
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mbutton_key(VMContext& context) {
    SDL_Event* event = (SDL_Event*)APITools_GetIntValue(context, 1);
    if(event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) {
      size_t* button_obj = APITools_GetObjectValue(context, 2);
      button_obj[0] = event->button.type;
      button_obj[1] = event->button.timestamp;
      button_obj[2] = event->button.windowID;
      button_obj[3] = event->button.which;
      button_obj[4] = event->button.button;
      button_obj[5] = event->button.state;
      button_obj[6] = event->button.x;
      button_obj[7] = event->button.y;
      button_obj[8] = event->button.y;

      APITools_SetIntValue(context, 0, 0);
    }
    else {
      APITools_SetIntValue(context, 0, -1);
    }
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
    void sdl_hat_key(VMContext& context) {
    SDL_Event* event = (SDL_Event*)APITools_GetIntValue(context, 1);
    if(event->type == SDL_JOYHATMOTION) {
      size_t* button_obj = APITools_GetObjectValue(context, 2);
      button_obj[0] = event->jhat.type;
      button_obj[1] = event->jhat.timestamp;
      button_obj[2] = event->jhat.which;
      button_obj[3] = event->jhat.hat;
      button_obj[4] = event->jhat.value;

      APITools_SetIntValue(context, 0, 0);
    }
    else {
      APITools_SetIntValue(context, 0, -1);
    }
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_event_peeps(VMContext& context) {
    SDL_Event* event = (SDL_Event*)APITools_GetIntValue(context, 1);
    const int numevents = (int)APITools_GetIntValue(context, 2);
    const SDL_eventaction action = (SDL_eventaction)APITools_GetIntValue(context, 3);
    const int minType = (int)APITools_GetIntValue(context, 4);
    const int maxType = (int)APITools_GetIntValue(context, 5);
    APITools_SetIntValue(context, 0, SDL_PeepEvents(event, numevents, action, minType, maxType));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_event_has(VMContext& context) {
    const int type = (int)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, SDL_HasEvent(type));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_event_flush(VMContext& context) {
    SDL_PumpEvents();
  }

  #ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_event_pump(VMContext& context) {
    SDL_FlushEvent((int)APITools_GetIntValue(context, 0));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_event_poll(VMContext& context) {
    SDL_Event* event = (SDL_Event*)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, SDL_PollEvent(event));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_event_wait(VMContext& context) {
    SDL_Event* event = (SDL_Event*)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, SDL_WaitEvent(event));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_event_wait_timeout(VMContext& context) {
    SDL_Event* event = (SDL_Event*)APITools_GetIntValue(context, 1);
    const int timeout = (int)APITools_GetIntValue(context, 2);
    APITools_SetIntValue(context, 0, SDL_WaitEventTimeout(event, timeout));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_event_push(VMContext& context) {
    SDL_Event* event = (SDL_Event*)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, SDL_PushEvent(event));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_event_state(VMContext& context) {
    const int type = (int)APITools_GetIntValue(context, 1);
    const int state = (int)APITools_GetIntValue(context, 2);
    APITools_SetIntValue(context, 0, SDL_EventState(type, state));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_event_registers(VMContext& context) {
    const int numevents = (int)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, SDL_RegisterEvents(numevents));
  }

  //
  // Palette
  //
  void sdl_palette_raw_read(SDL_Palette* palette, size_t* palette_obj) {
    if(palette_obj) {
      palette_obj[0] = palette->ncolors;
//       palette_obj[1] = palette->format;
      palette_obj[2] = palette->version;
    }
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_palette_read(VMContext& context) {
    SDL_Palette* palette = (SDL_Palette*)APITools_GetIntValue(context, 0);
    size_t* palette_obj = APITools_GetObjectValue(context, 1);
    sdl_palette_raw_read(palette, palette_obj);
  }
  
  void sdl_palette_raw_write(SDL_Palette* palette, size_t* palette_obj) {
    if(palette_obj) {
      palette->ncolors = (int)palette_obj[0];
//      palette->format = palette_obj[1];
      palette->version = (Uint8)palette_obj[2];
    }
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_palette_write(VMContext& context) {
    SDL_Palette* palette = (SDL_Palette*)APITools_GetIntValue(context, 0);
    size_t* palette_obj = APITools_GetObjectValue(context, 1);
    sdl_palette_raw_read(palette, palette_obj);
  }

  //
  // Image library
  // 
  // 
  #ifdef _WIN32
  __declspec(dllexport)
  #endif
  void sdl_image_init(VMContext& context) {
    const int flags = (int)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, IMG_Init(flags));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_image_quit(VMContext& context) {
    IMG_Quit();
  }

  #ifdef _WIN32
  __declspec(dllexport)
  #endif
  void sdl_image_load(VMContext& context) {
    const wstring wfile = APITools_GetStringValue(context, 1);
    const string file = UnicodeToBytes(wfile);
    APITools_SetIntValue(context, 0, (size_t)IMG_Load(file.c_str()));
  }

  //
  // Renderer
  //
  #ifdef _WIN32
  __declspec(dllexport)
  #endif
  void sdl_renderer_create(VMContext& context) {
    size_t* window_obj = APITools_GetObjectValue(context, 1);
    SDL_Window* window = (SDL_Window*)window_obj[0];

    const int index = (int)APITools_GetIntValue(context, 2);
    const Uint32 flags = (int)APITools_GetIntValue(context, 3);

    APITools_SetIntValue(context, 0, (size_t)SDL_CreateRenderer(window, index, flags));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_destroy(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 0);
    SDL_DestroyRenderer(renderer);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_create_texture(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    const int format = (int)APITools_GetIntValue(context, 2);
    const int access = (int)APITools_GetIntValue(context, 3);
    const int w = (int)APITools_GetIntValue(context, 4);
    const int h = (int)APITools_GetIntValue(context, 5);

    APITools_SetIntValue(context, 0, (size_t)SDL_CreateTexture(renderer, format, access, w, h));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
   void sdl_renderer_get_num_render_drivers(VMContext& context) {
    APITools_SetIntValue(context, 0, SDL_GetNumRenderDrivers());
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_set_target(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);

    const size_t* texture_obj = APITools_GetObjectValue(context, 2);
    SDL_Texture* texture = texture_obj ? (SDL_Texture*)texture_obj[0] : NULL;

    APITools_SetIntValue(context, 0, SDL_SetRenderTarget(renderer, texture));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_get_target(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, (size_t)SDL_GetRenderTarget(renderer));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_render_fill_rect(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);

    size_t* rect_obj = (size_t*)APITools_GetObjectValue(context, 2);
    SDL_Rect rect;
    sdl_rect_raw_write(&rect, rect_obj);

    APITools_SetIntValue(context, 0, SDL_RenderFillRect(renderer, rect_obj ? &rect : NULL));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_render_draw_line(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);

    const int x1 = (int)APITools_GetIntValue(context, 2);
    const int y1 = (int)APITools_GetIntValue(context, 3);
    const int x2 = (int)APITools_GetIntValue(context, 4);
    const int y2 = (int)APITools_GetIntValue(context, 5);

    APITools_SetIntValue(context, 0, SDL_RenderDrawLine(renderer, x1, y1, x2, y2));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_render_draw_rect(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);

    size_t* rect_obj = (size_t*)APITools_GetObjectValue(context, 2);
    SDL_Rect rect;
    sdl_rect_raw_write(&rect, rect_obj);

    APITools_SetIntValue(context, 0, SDL_RenderDrawRect(renderer, rect_obj ? &rect : NULL));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_render_set_viewport(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    size_t* rect_obj = (size_t*)APITools_GetObjectValue(context, 2);
    
    SDL_Rect rect;
    sdl_rect_raw_write(&rect, rect_obj);

    APITools_SetIntValue(context, 0, SDL_RenderSetViewport(renderer, rect_obj ? &rect : NULL));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_render_get_viewport(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 0);
    size_t* rect_obj = APITools_GetObjectValue(context, 1);

    SDL_Rect rect;
    SDL_RenderGetViewport(renderer, &rect);
    sdl_rect_raw_read(&rect, rect_obj);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_render_draw_point(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    
    const int x = (int)APITools_GetIntValue(context, 2);
    const int y = (int)APITools_GetIntValue(context, 3);

    APITools_SetIntValue(context, 0, SDL_RenderDrawPoint(renderer, x, y));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_get_render_driver_info(VMContext& context) {
    const int index = (int)APITools_GetIntValue(context, 1);
    size_t* info_obj = APITools_GetObjectValue(context, 2);

    if(info_obj) {
      SDL_RendererInfo info;
      const int return_value = SDL_GetRenderDriverInfo(index, &info);

      string name(info.name);
      wstring wname(name.begin(), name.end());

      info_obj[0] = (size_t)APITools_CreateStringValue(context, wname);
      info_obj[1] = info.flags;
      info_obj[2] = info.num_texture_formats;

      size_t* dest_formats = (size_t*)info_obj[3];
      Uint32* src_formats = info.texture_formats;
      for(int i = 0; i < 16; ++i) {
        dest_formats[i] = src_formats[i];
      }

      info_obj[4] = info.max_texture_width;
      info_obj[5] = info.max_texture_height;

      APITools_SetIntValue(context, 0, return_value);
    }
    else {
      APITools_SetIntValue(context, 0, -1);
    }
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_create_software(VMContext& context) {
    size_t* surface_obj = APITools_GetObjectValue(context, 1);
    SDL_Surface* surface = (SDL_Surface*)surface_obj[0];
    APITools_SetIntValue(context, 0, (size_t)SDL_CreateSoftwareRenderer(surface));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_get(VMContext& context) {
    size_t* window_obj = APITools_GetObjectValue(context, 1);
    SDL_Window* window = (SDL_Window*)window_obj[0];
    APITools_SetIntValue(context, 0, (size_t)SDL_GetRenderer(window));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_get_info(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    size_t* info_obj = APITools_GetObjectValue(context, 2);

    if(info_obj) {
      SDL_RendererInfo info;
      const int return_value = SDL_GetRendererInfo(renderer, &info);

      string name(info.name);
      wstring wname(name.begin(), name.end());

      info_obj[0] = (size_t)APITools_CreateStringValue(context, wname);
      info_obj[1] = info.flags;
      info_obj[2] = info.num_texture_formats;

      size_t* dest_formats = (size_t*)info_obj[3];
      Uint32* src_formats = info.texture_formats;
      for(int i = 0; i < 16; ++i) {
        dest_formats[i] = src_formats[i];
      }

      info_obj[4] = info.max_texture_width;
      info_obj[5] = info.max_texture_height;

      APITools_SetIntValue(context, 0, return_value);
    }
    else {
      APITools_SetIntValue(context, 0, -1);
    }
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_render_clear(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, SDL_RenderClear(renderer));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_render_copy(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    
    const size_t* texture_obj = APITools_GetObjectValue(context, 2);
    SDL_Texture* texture = texture_obj ? (SDL_Texture*)texture_obj[0] : NULL;

    size_t* srcrect_obj = APITools_GetObjectValue(context, 3);
    SDL_Rect srcrect;
    sdl_rect_raw_write(&srcrect, srcrect_obj);

    size_t* dstrect_obj = APITools_GetObjectValue(context, 4);
    SDL_Rect dstrect;
    sdl_rect_raw_write(&dstrect, dstrect_obj);

    APITools_SetIntValue(context, 0, SDL_RenderCopy(renderer, texture, 
      srcrect_obj ? &srcrect : NULL, dstrect_obj ? &dstrect : NULL));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_render_copy_ex(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);

    const size_t* texture_obj = APITools_GetObjectValue(context, 2);
    SDL_Texture* texture = texture_obj ? (SDL_Texture*)texture_obj[0] : NULL;

    size_t* srcrect_obj = APITools_GetObjectValue(context, 3);
    SDL_Rect srcrect;
    sdl_rect_raw_write(&srcrect, srcrect_obj);

    size_t* dstrect_obj = APITools_GetObjectValue(context, 4);
    SDL_Rect dstrect;
    sdl_rect_raw_write(&dstrect, dstrect_obj);

    const double angle = APITools_GetFloatValue(context, 5);

    SDL_Point center;
    size_t* center_obj = (size_t*)APITools_GetObjectValue(context, 6);
    sdl_point_raw_write(&center, center_obj);

    const SDL_RendererFlip flip = (SDL_RendererFlip)APITools_GetIntValue(context, 7);

    APITools_SetIntValue(context, 0, SDL_RenderCopyEx(renderer, texture, 
      srcrect_obj ? &srcrect : NULL, dstrect_obj ? &dstrect : NULL, angle, center_obj ? &center : NULL, flip));
}

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_render_present(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 0);
    SDL_RenderPresent(renderer);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_set_render_draw_color(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    const int r = (int)APITools_GetIntValue(context, 2);
    const int g = (int)APITools_GetIntValue(context, 3);
    const int b = (int)APITools_GetIntValue(context, 4);
    const int a = (int)APITools_GetIntValue(context, 5);
    APITools_SetIntValue(context, 0, SDL_SetRenderDrawColor(renderer, r, g, b, a));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_get_render_draw_color(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);

    Uint8 r, g, b, a;
    const int return_value = SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
    
    APITools_SetIntValue(context, 2, r);
    APITools_SetIntValue(context, 3, g);
    APITools_SetIntValue(context, 4, b);
    APITools_SetIntValue(context, 5, a);
    
    APITools_SetIntValue(context, 0, return_value);
  }

  //
  // pixeldata
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_pixeldata_get(VMContext& context) {
    size_t* pixels_obj = APITools_GetObjectValue(context, 1);
    const Uint32 index = (Uint32)APITools_GetIntValue(context, 2);
    const Uint32 max_pixels = ((Uint32)pixels_obj[1] / sizeof(Uint32)) * (Uint32)pixels_obj[2];

    if(index < max_pixels) {
      Uint32* pixels = (Uint32*)pixels_obj[0];
      APITools_SetIntValue(context, 0, pixels[index]);
    }
    else {
      APITools_SetIntValue(context, 0, -1);
    }
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_pixeldata_set(VMContext& context) {
    size_t* pixels_obj = APITools_GetObjectValue(context, 1);
    const Uint32 index = (Uint32)APITools_GetIntValue(context, 2);
    const Uint32 max_pixels = ((Uint32)pixels_obj[1] / sizeof(Uint32)) * (Uint32)pixels_obj[2];

    if (index < max_pixels) {
      Uint32* pixels = (Uint32*)pixels_obj[0];
      pixels[index] = (Uint32)APITools_GetIntValue(context, 3);;
      APITools_SetIntValue(context, 0, 1);
    }
    else {
      APITools_SetIntValue(context, 0, 0);
    }
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_pixeldata_copy(VMContext& context) {
    size_t* to_obj = APITools_GetObjectValue(context, 1);
    size_t* from_obj = APITools_GetObjectValue(context, 2);

    const Uint32 to_size = (Uint32)to_obj[1] * (Uint32)to_obj[2];
    void* to = (void*)(to_obj[0]);

    const Uint32 from_size = (Uint32)from_obj[1] * (Uint32)from_obj[2];
    void* from = (void*)(from_obj[0]);
    
    if(to && from && to_size >= from_size) {
      memcpy(to, from, from_size);
      APITools_SetIntValue(context, 0, 1);
    }
    else {
      APITools_SetIntValue(context, 0, 0);
    }
  }

  //
  // texture
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_texture_create(VMContext& context) {
    size_t* renderer_obj = APITools_GetObjectValue(context, 1);
    SDL_Renderer* renderer = (SDL_Renderer*)renderer_obj[0];
    const int format = (int)APITools_GetIntValue(context, 2);
    const int access = (int)APITools_GetIntValue(context, 3);
    const int w = (int)APITools_GetIntValue(context, 4);
    const int h = (int)APITools_GetIntValue(context, 5);
    APITools_SetIntValue(context, 0, (size_t)SDL_CreateTexture(renderer, format, access, w, h));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_texture_destroy(VMContext& context) {
    SDL_Texture* texture = (SDL_Texture*)APITools_GetIntValue(context, 0);
    SDL_DestroyTexture(texture);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_texture_query(VMContext& context) {
    SDL_Texture* texture = (SDL_Texture*)APITools_GetIntValue(context, 1);

    size_t* format_obj = APITools_GetObjectValue(context, 2);
    size_t* access_obj = APITools_GetObjectValue(context, 3);

    Uint32 format; int access, w, h;
    APITools_SetIntValue(context, 0, SDL_QueryTexture(texture, format_obj ? &format : NULL, access_obj ? &access : NULL, &w, &h));

    if(format_obj) {
      APITools_SetIntValue(context, 2, format);
    }

    if(access_obj) {
      APITools_SetIntValue(context, 3, access);
    }

    APITools_SetIntValue(context, 4, w);
    APITools_SetIntValue(context, 5, h);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_texture_lock(VMContext& context) {
    SDL_Texture* texture = (SDL_Texture*)APITools_GetIntValue(context, 1);

    size_t* rect_obj = APITools_GetObjectValue(context, 2);
    SDL_Rect rect;
    sdl_rect_raw_write(&rect, rect_obj);
    
    // create PixelData
    void* pixels; int pitch;
    if(!SDL_LockTexture(texture, rect_obj ? &rect : NULL, &pixels, &pitch)) {
      int width, height;
      SDL_QueryTexture(texture, NULL, NULL, &width, &height);
#ifdef _DEBUG
      assert(width == pitch / sizeof(Uint32));
#endif

      size_t* pixel_obj = context.alloc_obj(L"Game.SDL2.PixelData", context.op_stack, *context.stack_pos, false);
      pixel_obj[0] = (size_t)pixels;
      pixel_obj[1] = (size_t)pitch;
      pixel_obj[2] = (size_t)height;
      APITools_SetObjectValue(context, 0, pixel_obj);
    }
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_texture_unlock(VMContext& context) {
    SDL_Texture* texture = (SDL_Texture*)APITools_GetIntValue(context, 0);
    SDL_UnlockTexture(texture);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_texture_set_color_mod(VMContext& context) {
    SDL_Texture* texture = (SDL_Texture*)APITools_GetIntValue(context, 1);
    const int r = (int)APITools_GetIntValue(context, 2);
    const int g = (int)APITools_GetIntValue(context, 3);
    const int b = (int)APITools_GetIntValue(context, 4);
    APITools_SetIntValue(context, 0, SDL_SetTextureColorMod(texture, r, g, b));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_texture_get_color_mod(VMContext& context) {
    SDL_Texture* texture = (SDL_Texture*)APITools_GetIntValue(context, 1);

    Uint8 r, g, b;
    const int return_value = SDL_GetTextureColorMod(texture, &r, &g, &b);

    APITools_SetIntValue(context, 2, r);
    APITools_SetIntValue(context, 3, g);
    APITools_SetIntValue(context, 4, b);

    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_texture_set_alpha_mod(VMContext& context) {
    SDL_Texture* texture = (SDL_Texture*)APITools_GetIntValue(context, 1);
    const int alpha = (int)APITools_GetIntValue(context, 2);
    APITools_SetIntValue(context, 0, SDL_SetTextureAlphaMod(texture, alpha));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_texture_get_alpha_mod(VMContext& context) {
    SDL_Texture* texture = (SDL_Texture*)APITools_GetIntValue(context, 1);
    
    Uint8 alpha;
    const int return_value = SDL_GetTextureAlphaMod(texture, &alpha);
    APITools_SetIntValue(context, 2, alpha);

    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_texture_set_blend_mode(VMContext& context) {
    SDL_Texture* texture = (SDL_Texture*)APITools_GetIntValue(context, 1);
    const SDL_BlendMode blendMode = (SDL_BlendMode)APITools_GetIntValue(context, 2);
    APITools_SetIntValue(context, 0, SDL_SetTextureBlendMode(texture, blendMode));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_texture_get_blend_mode(VMContext& context) {
    SDL_Texture* texture = (SDL_Texture*)APITools_GetIntValue(context, 1);
    
    SDL_BlendMode blendMode;
    const int return_value = SDL_GetTextureBlendMode(texture, &blendMode);
    APITools_SetIntValue(context, 2, blendMode);

    APITools_SetIntValue(context, 0, return_value);
  }

  //
  // Timer
  //

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_timer_get_ticks(VMContext& context) {
    APITools_SetIntValue(context, 0, SDL_GetTicks());
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_timer_delay(VMContext& context) {
    SDL_Delay((Uint32)APITools_GetIntValue(context, 0));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_ticks_passed(VMContext& context) {
    const Uint32 a = (Uint32)APITools_GetIntValue(context, 1);
    const Uint32 b = (Uint32)APITools_GetIntValue(context, 2);
    APITools_SetIntValue(context, 0, SDL_TICKS_PASSED(a, b));
  }

  //
  // Font
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_font_init(VMContext& context) {
    APITools_SetIntValue(context, 0, TTF_Init());
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_font_quit(VMContext& context) {
    TTF_Quit();
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_font_open(VMContext& context) {
    const wstring wfile = APITools_GetStringValue(context, 1);
    const string file = UnicodeToBytes(wfile);
    const int ptsize = (int)APITools_GetIntValue(context, 2);

    TTF_Font* return_value = TTF_OpenFont(file.c_str(), ptsize);
    APITools_SetIntValue(context, 0, (size_t)return_value);
  }

  #ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_font_open_index(VMContext& context) {
    const wstring wfile = APITools_GetStringValue(context, 1);
    const string file = UnicodeToBytes(wfile);
    const int ptsize = (int)APITools_GetIntValue(context, 2);
    const int index = (int)APITools_GetIntValue(context, 3);

    TTF_Font* return_value = TTF_OpenFontIndex(file.c_str(), ptsize, index);
    APITools_SetIntValue(context, 0, (size_t)return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_font_render_text_solid(VMContext& context) {
    TTF_Font* font = (TTF_Font*)APITools_GetIntValue(context, 1);
    
    const wstring wtext = APITools_GetStringValue(context, 2);
    const string text = UnicodeToBytes(wtext);

    SDL_Color fg;
    size_t* fg_obj = APITools_GetObjectValue(context, 3);
    sdl_color_raw_write(&fg, fg_obj);

    APITools_SetIntValue(context, 0, (size_t)TTF_RenderText_Solid(font, text.c_str(), fg));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_font_render_utf8_solid(VMContext& context) {
    TTF_Font* font = (TTF_Font*)APITools_GetIntValue(context, 1);

    const wstring wtext = APITools_GetStringValue(context, 2);
    const string text = UnicodeToBytes(wtext);

    SDL_Color fg;
    size_t* fg_obj = APITools_GetObjectValue(context, 3);
    sdl_color_raw_write(&fg, fg_obj);

    APITools_SetIntValue(context, 0, (size_t)TTF_RenderUTF8_Solid(font, text.c_str(), fg));
  }
   
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_font_render_text_blended(VMContext& context) {
    TTF_Font* font = (TTF_Font*)APITools_GetIntValue(context, 1);

    const wstring wtext = APITools_GetStringValue(context, 2);
    const string text = UnicodeToBytes(wtext);

    SDL_Color fg;
    size_t* fg_obj = APITools_GetObjectValue(context, 3);
    sdl_color_raw_write(&fg, fg_obj);

    APITools_SetIntValue(context, 0, (size_t)TTF_RenderText_Blended(font, text.c_str(), fg));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_font_render_utf8_blended(VMContext& context) {
    TTF_Font* font = (TTF_Font*)APITools_GetIntValue(context, 1);

    const wstring wtext = APITools_GetStringValue(context, 2);
    const string text = UnicodeToBytes(wtext);

    SDL_Color fg;
    size_t* fg_obj = APITools_GetObjectValue(context, 3);
    sdl_color_raw_write(&fg, fg_obj);

    APITools_SetIntValue(context, 0, (size_t)TTF_RenderUTF8_Blended(font, text.c_str(), fg));
  }
  
#ifdef _WIN32
  __declspec(dllexport)
#endif
    void sdl_font_render_text_shaded(VMContext& context) {
    TTF_Font* font = (TTF_Font*)APITools_GetIntValue(context, 1);

    const wstring wtext = APITools_GetStringValue(context, 2);
    const string text = UnicodeToBytes(wtext);

    SDL_Color fg;
    size_t* fg_obj = APITools_GetObjectValue(context, 3);
    sdl_color_raw_write(&fg, fg_obj);

    SDL_Color bg;
    size_t* bg_obj = APITools_GetObjectValue(context, 4);
    sdl_color_raw_write(&bg, bg_obj);

    APITools_SetIntValue(context, 0, (size_t)TTF_RenderText_Shaded(font, text.c_str(), fg, bg));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
    void sdl_font_render_utf8_shaded(VMContext& context) {
    TTF_Font* font = (TTF_Font*)APITools_GetIntValue(context, 1);

    const wstring wtext = APITools_GetStringValue(context, 2);
    const string text = UnicodeToBytes(wtext);

    SDL_Color fg;
    size_t* fg_obj = APITools_GetObjectValue(context, 3);
    sdl_color_raw_write(&fg, fg_obj);

    SDL_Color bg;
    size_t* bg_obj = APITools_GetObjectValue(context, 4);
    sdl_color_raw_write(&bg, bg_obj);

    APITools_SetIntValue(context, 0, (size_t)TTF_RenderUTF8_Shaded(font, text.c_str(), fg, bg));
  }

  //
  // Cursor
  //

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cursor_get_global_mouse_state(VMContext& context) {
    int x, y;
    const int return_value = SDL_GetGlobalMouseState(&x, &y);

    APITools_SetIntValue(context, 1, x);
    APITools_SetIntValue(context, 2, y);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cursor_get_mouse_state(VMContext& context) {
    int x, y;
    const int return_value = SDL_GetMouseState(&x, &y);

    APITools_SetIntValue(context, 1, x);
    APITools_SetIntValue(context, 2, y);
    APITools_SetIntValue(context, 0, return_value);
  }

  //
  // Clipboard
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_power_set_clipboard_text(VMContext& context) {
    const wstring w_text = APITools_GetStringValue(context, 1);
    const string text = UnicodeToBytes(w_text);
    APITools_SetIntValue(context, 0, SDL_SetClipboardText(text.c_str()));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_power_get_clipboard_text(VMContext& context) {
    char* text_ptr = SDL_GetClipboardText();
    const wstring w_text = BytesToUnicode(text_ptr);
    APITools_SetStringValue(context, 0, w_text);
    SDL_free(text_ptr);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_power_has_clipboard_text(VMContext& context) {
    APITools_SetIntValue(context, 0, SDL_HasClipboardText());
  }

  //
  // Keyboard
  //

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_keyboard_start_text_input(VMContext& context) {
    SDL_StartTextInput();
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_keyboard_stop_text_input(VMContext& context) {
    SDL_StopTextInput();
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_keyboard_set_text_input_rect(VMContext& context) {
    size_t* rect_obj = APITools_GetObjectValue(context, 0);

    SDL_Rect rect;
    SDL_SetTextInputRect(&rect);
    sdl_rect_raw_read(&rect, rect_obj);
  }

  #ifdef _WIN32
  __declspec(dllexport)
  #endif
  void sdl_keyboard_get_state(VMContext& context) {
    int numkeys;
    const Uint8* states = SDL_GetKeyboardState(&numkeys);

    size_t* array = APITools_MakeByteArray(context, numkeys);
    Uint8* byte_array = (Uint8*)(array + 3);
    memcpy(byte_array, states, numkeys);

    /*
    for(int i = 0; i < numkeys; ++i) {
      byte_array[i] = state[i];
    }
    */

    // create 'ByteArrayHolder' holder
    size_t* byte_obj = context.alloc_obj(L"System.ByteArrayHolder", context.op_stack, *context.stack_pos, false);
    byte_obj[0] = (size_t)array;
    
    APITools_SetObjectValue(context, 0, byte_obj);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_keyboard_get_mod_state(VMContext& context) {
    APITools_SetIntValue(context, 0, SDL_GetModState());
  }

  //
  // Joystick
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_joystick_open(VMContext& context) {
    const int device_index = (int)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, (size_t)SDL_JoystickOpen(device_index));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_joystick_close(VMContext& context) {
    SDL_Joystick* joystick = (SDL_Joystick*)APITools_GetIntValue(context, 0);
    SDL_JoystickClose(joystick);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_joystick_nums(VMContext& context) {
    APITools_SetIntValue(context, 0, SDL_NumJoysticks());
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_joystick_name(VMContext& context) {
    SDL_Joystick* joystick = (SDL_Joystick*)APITools_GetIntValue(context, 1);
    const string return_value = SDL_JoystickName(joystick);
    
    const wstring w_return_value(return_value.begin(), return_value.end());
    APITools_SetStringValue(context, 0, w_return_value);
  }

  //
  // Mixer
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_open_audio(VMContext& context) {
    const int frequency = (int)APITools_GetIntValue(context, 1);
    const int format = (int)APITools_GetIntValue(context, 2);
    const int channels = (int)APITools_GetIntValue(context, 3);
    const int chunksize = (int)APITools_GetIntValue(context, 4);
    
    APITools_SetIntValue(context, 0, Mix_OpenAudio(frequency, format, channels, chunksize));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mix_quit(VMContext& context) {
    Mix_Quit();
  }
  
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_load_wav(VMContext& context) {
    const wstring w_file = APITools_GetStringValue(context, 1);
    const string extension = UnicodeToBytes(w_file);

    APITools_SetIntValue(context, 0, (size_t)Mix_LoadWAV(extension.c_str()));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_play_channel_timed(VMContext& context) {
    const int channel = (int)APITools_GetIntValue(context, 1);
    Mix_Chunk* chunk = (Mix_Chunk*)APITools_GetIntValue(context, 2);
    const int loops = (int)APITools_GetIntValue(context, 3);
    const int ticks = (int)APITools_GetIntValue(context, 4);

    APITools_SetIntValue(context, 0, Mix_PlayChannelTimed(channel, chunk, loops, ticks));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_fade_in_channel_timed(VMContext& context) {
    const int channel = (int)APITools_GetIntValue(context, 1);
    Mix_Chunk* chunk = (Mix_Chunk*)APITools_GetIntValue(context, 2);
    const int loops = (int)APITools_GetIntValue(context, 3);
    const int ms = (int)APITools_GetIntValue(context, 4);
    const int ticks = (int)APITools_GetIntValue(context, 5);

    APITools_SetIntValue(context, 0, Mix_FadeInChannelTimed(channel, chunk, loops, ms, ticks));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_fade_out_channel(VMContext& context) {
    const int which = (int)APITools_GetIntValue(context, 1);
    const int ms = (int)APITools_GetIntValue(context, 2);

    APITools_SetIntValue(context, 0, Mix_FadeOutChannel(which, ms));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_playing(VMContext& context) {
    const int channel = (int)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, Mix_Playing(channel));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_free_wav(VMContext& context) {
    Mix_Chunk* chunk = (Mix_Chunk*)APITools_GetIntValue(context, 0);
    Mix_FreeChunk(chunk);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_load_mus(VMContext& context) {
    const wstring w_file = APITools_GetStringValue(context, 1);
    const string file = UnicodeToBytes(w_file);

    Mix_Music* music = Mix_LoadMUS(file.c_str());
    APITools_SetIntValue(context, 0, (size_t)music);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_play_music(VMContext& context) {
    Mix_Music* music = (Mix_Music*)APITools_GetIntValue(context, 1);
    const int loops = (int)APITools_GetIntValue(context, 2);

    APITools_SetIntValue(context, 0, Mix_PlayMusic(music, loops));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_volume(VMContext& context) {
    const int channel = (int)APITools_GetIntValue(context, 1);
    const int volume = (int)APITools_GetIntValue(context, 2);

    APITools_SetIntValue(context, 0, Mix_Volume(channel, volume));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_volume_music(VMContext& context) {
    const int volume = (int)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, Mix_VolumeMusic(volume));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_halt_channel(VMContext& context) {
    const int channel = (int)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, Mix_HaltChannel(channel));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_fade_in_music(VMContext& context) {
    Mix_Music* music = (Mix_Music*)APITools_GetIntValue(context, 1);
    const int loops = (int)APITools_GetIntValue(context, 2);
    const int ms = (int)APITools_GetIntValue(context, 3);

    APITools_SetIntValue(context, 0, Mix_FadeInMusic(music, loops, ms));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_fade_out_music(VMContext& context) {
    const int ms = (int)APITools_GetIntValue(context, 1);
    APITools_SetIntValue(context, 0, Mix_FadeOutMusic(ms));
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_playing_music(VMContext& context) {
    APITools_SetIntValue(context, 0, Mix_PlayingMusic());
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_paused_music(VMContext& context) {
    APITools_SetIntValue(context, 0, Mix_PausedMusic());
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_halt_music(VMContext& context) {
    Mix_HaltMusic();
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_resume_music(VMContext& context) {
    Mix_ResumeMusic();
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_pause_music(VMContext& context) {
    Mix_PauseMusic();
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_mixer_free_mus(VMContext& context) {
    Mix_Music* music = (Mix_Music*)APITools_GetIntValue(context, 0);
    Mix_FreeMusic(music);
  }

  // platform
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_platform_get(VMContext& context) {
    const string value = SDL_GetPlatform();
    const wstring return_value(value.begin(), value.end());
    APITools_SetStringValue(context, 0, return_value);
  }

  // cpu
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cpu_get_count(VMContext& context) {
    APITools_SetIntValue(context, 0, SDL_GetCPUCount());
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cpu_get_cache_line_size(VMContext& context) {
    APITools_SetIntValue(context, 0, SDL_GetCPUCacheLineSize());
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cpu_hasrdtsc(VMContext& context) {
    const int return_value = SDL_HasRDTSC();
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cpu_has_alti_vec(VMContext& context) {
    const int return_value = SDL_HasAltiVec();
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cpu_hasmmx(VMContext& context) {
    const int return_value = SDL_HasMMX();
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cpu_has3d_now(VMContext& context) {
    const int return_value = SDL_Has3DNow();
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cpu_hassse(VMContext& context) {
    const int return_value = SDL_HasSSE();
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cpu_hassse2(VMContext& context) {
    const int return_value = SDL_HasSSE2();
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cpu_hassse3(VMContext& context) {
    const int return_value = SDL_HasSSE3();
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cpu_hassse41(VMContext& context) {
    const int return_value = SDL_HasSSE41();
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cpu_hassse42(VMContext& context) {
    const int return_value = SDL_HasSSE42();
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cpu_hasavx(VMContext& context) {
    const int return_value = SDL_HasAVX();
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cpu_hasavx2(VMContext& context) {
    const int return_value = SDL_HasAVX2();
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cpu_hasneon(VMContext& context) {
    const int return_value = SDL_HasNEON();
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_cpu_get_systemram(VMContext& context) {
    const int return_value = SDL_GetSystemRAM();
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_filesystem_get_base_path(VMContext& context) {
    const string value = SDL_GetBasePath();
    const wstring return_value = BytesToUnicode(value);
    APITools_SetStringValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_filesystem_get_pref_path(VMContext& context) {
    const wstring w_org = APITools_GetStringValue(context, 1);
    const string org = UnicodeToBytes(w_org);

    const wstring w_app = APITools_GetStringValue(context, 2);
    const string app = UnicodeToBytes(w_app);

    const string value = SDL_GetPrefPath(org.c_str(), app.c_str());

    const wstring return_value = BytesToUnicode(value);
    APITools_SetStringValue(context, 0, return_value);
  }

  // power
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_power_get_info(VMContext& context) {
    int secs, pct;
    const SDL_PowerState return_value = SDL_GetPowerInfo(&secs, &pct);

    APITools_SetIntValue(context, 0, return_value);
    APITools_SetIntValue(context, 1, secs);
    APITools_SetIntValue(context, 2, pct);
  }

  //////////////////// SDL2_gfx routines ////////////////////

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_pixel_color(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    const int x = (int)APITools_GetIntValue(context, 2);
    const int y = (int)APITools_GetIntValue(context, 3);

    SDL_Color color;
    size_t* color_obj = APITools_GetObjectValue(context, 4);
    sdl_color_raw_write(&color, color_obj);

    const int return_value = pixelRGBA(renderer, x, y, color.r, color.g, color.b, color.a);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_hline_color(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    const int x1 = (int)APITools_GetIntValue(context, 2);
    const int x2 = (int)APITools_GetIntValue(context, 3);
    const int y = (int)APITools_GetIntValue(context, 4);

    SDL_Color color;
    size_t* color_obj = APITools_GetObjectValue(context, 5);
    sdl_color_raw_write(&color, color_obj);

    const int return_value = hlineRGBA(renderer, x1, x2, y, color.r, color.g, color.b, color.a);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_vline_color(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    const int x = (int)APITools_GetIntValue(context, 2);
    const int y1 = (int)APITools_GetIntValue(context, 3);
    const int y2 = (int)APITools_GetIntValue(context, 4);

    SDL_Color color;
    size_t* color_obj = APITools_GetObjectValue(context, 5);
    sdl_color_raw_write(&color, color_obj);

    const int return_value = vlineRGBA(renderer, x, y1, y2, color.r, color.g, color.b, color.a);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_rectangle_color(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    const int x1 = (int)APITools_GetIntValue(context, 2);
    const int y1 = (int)APITools_GetIntValue(context, 3);
    const int x2 = (int)APITools_GetIntValue(context, 4);
    const int y2 = (int)APITools_GetIntValue(context, 5);

    SDL_Color color;
    size_t* color_obj = APITools_GetObjectValue(context, 6);
    sdl_color_raw_write(&color, color_obj);

    const int return_value = rectangleRGBA(renderer, x1, y1, x2, y2, color.r, color.g, color.b, color.a);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_rounded_rectangle_color(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    const int x1 = (int)APITools_GetIntValue(context, 2);
    const int y1 = (int)APITools_GetIntValue(context, 3);
    const int x2 = (int)APITools_GetIntValue(context, 4);
    const int y2 = (int)APITools_GetIntValue(context, 5);
    const int rad = (int)APITools_GetIntValue(context, 6);

    SDL_Color color;
    size_t* color_obj = APITools_GetObjectValue(context, 7);
    sdl_color_raw_write(&color, color_obj);

    const int return_value = roundedRectangleRGBA(renderer, x1, y1, x2, y2, rad, color.r, color.g, color.b, color.a);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
    void sdl_renderer_box_color(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    const int x1 = (int)APITools_GetIntValue(context, 2);
    const int y1 = (int)APITools_GetIntValue(context, 3);
    const int x2 = (int)APITools_GetIntValue(context, 4);
    const int y2 = (int)APITools_GetIntValue(context, 5);

    SDL_Color color;
    size_t* color_obj = APITools_GetObjectValue(context, 6);
    sdl_color_raw_write(&color, color_obj);

    const int return_value = boxRGBA(renderer, x1, y1, x2, y2, color.r, color.g, color.b, color.a);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_rounded_box_color(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    const int x1 = (int)APITools_GetIntValue(context, 2);
    const int y1 = (int)APITools_GetIntValue(context, 3);
    const int x2 = (int)APITools_GetIntValue(context, 4);
    const int y2 = (int)APITools_GetIntValue(context, 5);
    const int rad = (int)APITools_GetIntValue(context, 6);

    SDL_Color color;
    size_t* color_obj = APITools_GetObjectValue(context, 7);
    sdl_color_raw_write(&color, color_obj);

    const int return_value = roundedBoxRGBA(renderer, x1, y1, x2, y2, rad, color.r, color.g, color.b, color.a);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_line_color(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    const int x1 = (int)APITools_GetIntValue(context, 2);
    const int y1 = (int)APITools_GetIntValue(context, 3);
    const int x2 = (int)APITools_GetIntValue(context, 4);
    const int y2 = (int)APITools_GetIntValue(context, 5);

    SDL_Color color;
    size_t* color_obj = APITools_GetObjectValue(context, 6);
    sdl_color_raw_write(&color, color_obj);

    const int return_value = lineRGBA(renderer, x1, y1, x2, y2, color.r, color.g, color.b, color.a);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_aaline_color(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    const int x1 = (int)APITools_GetIntValue(context, 2);
    const int y1 = (int)APITools_GetIntValue(context, 3);
    const int x2 = (int)APITools_GetIntValue(context, 4);
    const int y2 = (int)APITools_GetIntValue(context, 5);

    SDL_Color color;
    size_t* color_obj = APITools_GetObjectValue(context, 6);
    sdl_color_raw_write(&color, color_obj);

    const int return_value = aalineRGBA(renderer, x1, y1, x2, y2, color.r, color.g, color.b, color.a);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_thick_line_color(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    const int x1 = (int)APITools_GetIntValue(context, 2);
    const int y1 = (int)APITools_GetIntValue(context, 3);
    const int x2 = (int)APITools_GetIntValue(context, 4);
    const int y2 = (int)APITools_GetIntValue(context, 5);
    const int width = (int)APITools_GetIntValue(context, 6);

    SDL_Color color;
    size_t* color_obj = APITools_GetObjectValue(context, 7);
    sdl_color_raw_write(&color, color_obj);

    const int return_value = thickLineRGBA(renderer, x1, y1, x2, y2, width, color.r, color.g, color.b, color.a);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_circle_color(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    const int x = (int)APITools_GetIntValue(context, 2);
    const int y = (int)APITools_GetIntValue(context, 3);
    const int rad = (int)APITools_GetIntValue(context, 4);

    SDL_Color color;
    size_t* color_obj = APITools_GetObjectValue(context, 5);
    sdl_color_raw_write(&color, color_obj);

    const int return_value = circleRGBA(renderer, x, y, rad, color.r, color.g, color.b, color.a);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_arc_color(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    const int x = (int)APITools_GetIntValue(context, 2);
    const int y = (int)APITools_GetIntValue(context, 3);
    const int rad = (int)APITools_GetIntValue(context, 4);
    const int start = (int)APITools_GetIntValue(context, 5);
    const int end = (int)APITools_GetIntValue(context, 6);

    SDL_Color color;
    size_t* color_obj = APITools_GetObjectValue(context, 7);
    sdl_color_raw_write(&color, color_obj);

    const int return_value = arcRGBA(renderer, x, y, rad, start, end, color.r, color.g, color.b, color.a);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_aacircle_color(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    const int x = (int)APITools_GetIntValue(context, 2);
    const int y = (int)APITools_GetIntValue(context, 3);
    const int rad = (int)APITools_GetIntValue(context, 4);

    SDL_Color color;
    size_t* color_obj = APITools_GetObjectValue(context, 5);
    sdl_color_raw_write(&color, color_obj);

    const int return_value = aacircleRGBA(renderer, x, y, rad, color.r, color.g, color.b, color.a);
    APITools_SetIntValue(context, 0, return_value);
  }

#ifdef _WIN32
  __declspec(dllexport)
#endif
  void sdl_renderer_filled_circle_color(VMContext& context) {
    SDL_Renderer* renderer = (SDL_Renderer*)APITools_GetIntValue(context, 1);
    const int x = (int)APITools_GetIntValue(context, 2);
    const int y = (int)APITools_GetIntValue(context, 3);
    const int r = (int)APITools_GetIntValue(context, 4);

    SDL_Color color;
    size_t* color_obj = APITools_GetObjectValue(context, 5);
    sdl_color_raw_write(&color, color_obj);

    const int return_value = filledCircleRGBA(renderer, x, y, r, color.r, color.g, color.b, color.a);
    APITools_SetIntValue(context, 0, return_value);
  }
}
