#pragma once

#include <bgfx/bgfx.h>

#include <string>

namespace Struktur
{
namespace Renderer
{
// bgfx shaders are precompiled per-platform at build time (see CMakeLists.txt's struktur_compile_shader) and
// embedded into the binary, rather than loaded from disk at runtime like raylib's raw-GLSL shaders were.
// name must be one of "vs_sprite", "fs_sprite", "vs_spriteCoverage", "fs_spriteCoverage", "vs_soulEffect",
// "fs_soulEffect" (the compiled shader names).
bgfx::ShaderHandle GetEmbeddedShader(const char* name);

// Convenience: creates (and caches) the linked vs+fs program for a name pair, e.g. GetEmbeddedProgram("sprite").
bgfx::ProgramHandle GetEmbeddedProgram(const char* name);

// bgfx dedupes uniforms by name internally, but each bgfx::createUniform() call still needs a matching destroy;
// caching process-lifetime handles here (never destroyed until Shutdown() below) sidesteps tracking that
// per-caller, which fits this project's small, fixed set of shader uniform names (see ShaderSystem::ApplyUniforms).
bgfx::UniformHandle GetOrCreateUniform(const std::string& name);

// Destroys every shader/program/uniform handle the three functions above have ever cached and empties the
// caches. Must run before bgfx::shutdown() (see GraphicsDevice::Shutdown, which calls this first) - bgfx::destroy()
// is invalid once bgfx itself has shut down.
void Shutdown();
}  // namespace Renderer
}  // namespace Struktur
