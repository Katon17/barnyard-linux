#!/usr/bin/env python3
"""Real SDL2/GLES3 platform probe, NOT a Barnyard renderer or game.
Uses system native libraries through ctypes; no browser, Wine, or downloads.
"""
from __future__ import annotations
import argparse
import ctypes as C
import ctypes.util
import json
import math
import time


def function(library, name, result, *arguments):
    call = getattr(library, name)
    call.restype = result
    call.argtypes = list(arguments)
    return call


def run(seconds=10.0, frames=0, hidden=False):
    name = ctypes.util.find_library("SDL2-2.0") or "libSDL2-2.0.so.0"
    sdl = C.CDLL(name)
    init = function(sdl, "SDL_Init", C.c_int, C.c_uint32)
    quit_sdl = function(sdl, "SDL_Quit", None)
    get_error = function(sdl, "SDL_GetError", C.c_char_p)
    set_attr = function(sdl, "SDL_GL_SetAttribute", C.c_int, C.c_int, C.c_int)
    set_hint = function(sdl, "SDL_SetHint", C.c_int, C.c_char_p, C.c_char_p)
    window_create = function(sdl, "SDL_CreateWindow", C.c_void_p, C.c_char_p,
                             C.c_int, C.c_int, C.c_int, C.c_int, C.c_uint32)
    window_destroy = function(sdl, "SDL_DestroyWindow", None, C.c_void_p)
    context_create = function(sdl, "SDL_GL_CreateContext", C.c_void_p, C.c_void_p)
    context_delete = function(sdl, "SDL_GL_DeleteContext", None, C.c_void_p)
    swap_interval = function(sdl, "SDL_GL_SetSwapInterval", C.c_int, C.c_int)
    swap_window = function(sdl, "SDL_GL_SwapWindow", None, C.c_void_p)
    get_size = function(sdl, "SDL_GL_GetDrawableSize", None, C.c_void_p, C.POINTER(C.c_int), C.POINTER(C.c_int))
    poll = function(sdl, "SDL_PollEvent", C.c_int, C.c_void_p)
    address = function(sdl, "SDL_GL_GetProcAddress", C.c_void_p, C.c_char_p)
    window = context = None
    program = vao = 0
    delete_program = delete_vao = None

    def error(prefix):
        return RuntimeError(prefix + ": " + (get_error() or b"unknown SDL error").decode("utf8", "replace"))

    def gl(name, result, *args):
        ptr = address(name.encode("ascii"))
        if not ptr:
            raise RuntimeError("Missing GLES entrypoint: " + name)
        return C.CFUNCTYPE(result, *args)(ptr)

    try:
        if init(0x20) != 0:  # SDL_INIT_VIDEO; no audio devices opened by this probe.
            raise error("SDL video initialization failed")
        set_hint(b"SDL_VIDEO_X11_FORCE_EGL", b"1")
        for attr, value in ((21, 4), (17, 3), (18, 0), (5, 1)):  # ES profile, 3.0, doublebuffer
            if set_attr(attr, value) != 0:
                raise error("SDL GL attribute rejected")
        flags = 0x2 | (0x8 if hidden else 0x4) | 0x20
        window = window_create(b"Barnyard Linux - GLES3 diagnostic, NOT GAMEPLAY", 0x2FFF0000, 0x2FFF0000, 800, 450, flags)
        if not window:
            raise error("Native window creation failed")
        context = context_create(window)
        if not context:
            raise error("GLES3 context creation failed")
        vsync = swap_interval(1) == 0
        get_string = gl("glGetString", C.c_char_p, C.c_uint)
        info = {}
        for key, token in (("vendor", 0x1F00), ("renderer", 0x1F01), ("version", 0x1F02), ("glsl", 0x8B8C)):
            info[key] = (get_string(token) or b"").decode("utf8", "replace")
        if "OpenGL ES 3" not in info["version"]:
            raise RuntimeError("Context is not OpenGL ES 3.x: " + info["version"])
        create_shader = gl("glCreateShader", C.c_uint, C.c_uint)
        shader_source = gl("glShaderSource", None, C.c_uint, C.c_int, C.POINTER(C.c_char_p), C.POINTER(C.c_int))
        compile_shader = gl("glCompileShader", None, C.c_uint)
        shader_status = gl("glGetShaderiv", None, C.c_uint, C.c_uint, C.POINTER(C.c_int))
        shader_log = gl("glGetShaderInfoLog", None, C.c_uint, C.c_int, C.POINTER(C.c_int), C.c_void_p)
        delete_shader = gl("glDeleteShader", None, C.c_uint)
        create_program = gl("glCreateProgram", C.c_uint)
        attach = gl("glAttachShader", None, C.c_uint, C.c_uint)
        link = gl("glLinkProgram", None, C.c_uint)
        program_status = gl("glGetProgramiv", None, C.c_uint, C.c_uint, C.POINTER(C.c_int))
        program_log = gl("glGetProgramInfoLog", None, C.c_uint, C.c_int, C.POINTER(C.c_int), C.c_void_p)
        delete_program = gl("glDeleteProgram", None, C.c_uint)
        use = gl("glUseProgram", None, C.c_uint)
        uniform_location = gl("glGetUniformLocation", C.c_int, C.c_uint, C.c_char_p)
        uniform_float = gl("glUniform1f", None, C.c_int, C.c_float)
        viewport = gl("glViewport", None, C.c_int, C.c_int, C.c_int, C.c_int)
        clear_color = gl("glClearColor", None, C.c_float, C.c_float, C.c_float, C.c_float)
        clear = gl("glClear", None, C.c_uint)
        draw = gl("glDrawArrays", None, C.c_uint, C.c_int, C.c_int)
        get_gl_error = gl("glGetError", C.c_uint)
        gen_vao = gl("glGenVertexArrays", None, C.c_int, C.POINTER(C.c_uint))
        bind_vao = gl("glBindVertexArray", None, C.c_uint)
        delete_vao = gl("glDeleteVertexArrays", None, C.c_int, C.POINTER(C.c_uint))

        def shader(kind, source):
            obj = create_shader(kind)
            try:
                text = C.c_char_p(source)
                shader_source(obj, 1, C.byref(text), None)
                compile_shader(obj)
                ok = C.c_int()
                shader_status(obj, 0x8B81, C.byref(ok))
                if not ok.value:
                    message = C.create_string_buffer(4096)
                    shader_log(obj, 4096, None, message)
                    raise RuntimeError(message.value.decode("utf8", "replace"))
                return obj
            except Exception:
                delete_shader(obj)
                raise

        vertex = shader(0x8B31, b'''#version 300 es
uniform float angle;
out vec3 color;
void main() {
  vec2 p[3] = vec2[3](vec2(0.0,0.65),vec2(-0.65,-0.5),vec2(0.65,-0.5));
  vec3 c[3] = vec3[3](vec3(0.7,0.9,0.3),vec3(0.3,0.65,0.9),vec3(0.95,0.6,0.25));
  float s=sin(angle),co=cos(angle);
  gl_Position=vec4(mat2(co,s,-s,co)*p[gl_VertexID]*vec2(0.56,1.0),0.0,1.0);
  color=c[gl_VertexID];
}''')
        try:
            fragment = shader(0x8B30, b'''#version 300 es
precision mediump float;
in vec3 color;
out vec4 pixel;
void main(){ pixel=vec4(color,1.0); }
''')
            try:
                program = create_program()
                attach(program, vertex)
                attach(program, fragment)
                link(program)
                ok = C.c_int()
                program_status(program, 0x8B82, C.byref(ok))
                if not ok.value:
                    message = C.create_string_buffer(4096)
                    program_log(program, 4096, None, message)
                    raise RuntimeError(message.value.decode("utf8", "replace"))
            finally:
                delete_shader(fragment)
        finally:
            delete_shader(vertex)
        use(program)
        location = uniform_location(program, b"angle")
        handle = C.c_uint()
        gen_vao(1, C.byref(handle))
        vao = handle.value
        bind_vao(vao)
        event = (C.c_uint64 * 7)()  # SDL_Event: 56 bytes, aligned for pointers.
        width, height = C.c_int(), C.c_int()
        started = time.monotonic()
        count = 0
        times = []
        running = True
        while running:
            begin = time.monotonic()
            while poll(C.byref(event)):
                raw = bytes(event)
                kind = int.from_bytes(raw[:4], "little")
                if kind == 0x100 or (kind == 0x300 and int.from_bytes(raw[20:24], "little") == 27):
                    running = False
            if not running:
                break
            get_size(window, C.byref(width), C.byref(height))
            viewport(0, 0, width.value, height.value)
            clear_color(0.045, 0.065, 0.09, 1.0)
            clear(0x4000)
            uniform_float(location, float(begin-started))
            draw(0x4, 0, 3)
            problem = get_gl_error()
            if problem:
                raise RuntimeError(f"OpenGL error 0x{problem:x}")
            swap_window(window)
            count += 1
            if frames and count >= frames:
                break
            # Sleep even when a driver accepts swap interval but ignores it.
            remaining = 1.0 / 60 - (time.monotonic() - begin)
            if remaining > 0:
                time.sleep(remaining)
            times.append((time.monotonic()-begin)*1000)
            if time.monotonic() - started >= seconds:
                break
        elapsed = time.monotonic()-started
        times.sort()
        info.update(ok=True, component="SDL2/GLES3 diagnostic, NOT Barnyard gameplay",
                    frames=count, elapsed_seconds=round(elapsed, 3),
                    swap_interval_accepted=vsync,
                    p95_frame_ms=round(times[min(len(times)-1, int(len(times)*0.95))], 3) if times else None,
                    software_renderer=any(x in info["renderer"].lower() for x in ("llvmpipe", "softpipe", "swrast")),
                    warning="Diagnostic timings are not Barnyard performance benchmarks.")
        return info
    finally:
        if context:
            if vao and delete_vao:
                value = C.c_uint(vao)
                delete_vao(1, C.byref(value))
            if program and delete_program:
                delete_program(program)
            context_delete(context)
        if window:
            window_destroy(window)
        quit_sdl()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seconds", type=float, default=10.0)
    parser.add_argument("--frames", type=int, default=0)
    parser.add_argument("--hidden", action="store_true")
    args = parser.parse_args()
    if not 0 < args.seconds <= 300 or not 0 <= args.frames <= 18000:
        parser.error("seconds must be 0..300, frames 0..18000")
    try:
        result = run(args.seconds, args.frames, args.hidden)
    except (OSError, RuntimeError) as exc:
        print(json.dumps({"ok": False, "error": str(exc)}, indent=2))
        return 2
    print(json.dumps(result, indent=2))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
