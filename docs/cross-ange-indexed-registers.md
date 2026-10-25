# Experimental indexed-register correction

Based on upstream `92bda548f3aa0691102471a72725587e00676383`
(2026-10-25). That revision still multiplies an indirect register index by two
before adding the operand offset.

Cross Ange's native vertex programs already apply the float2 array stride
before assigning the index register. Applying another factor of two selects
the wrong texture-animation coordinates, corrupting several battle HUD panels.
This branch removes the second scaling, updates the disassembler to match and
increments the shader cache version from 15 to 16 so shaders regenerate.

## Reference evidence

For the native vertex program with SHA-256
`8a204ef97f3663422c34b60bab3fc1dcaa7d82c3af636d87901664892f7621db`,
the `tanm` uniform is declared as float2[24], starting at scalar register 20.
The native instruction sequence computes `index = 2 * element + 6` and reads
the indexed operands at offsets 14 and 15. Thus element zero reads registers
20 and 21; the previous translation reads 26 and 27 and advances by four
registers per element instead of two.

Across 12 affected native programs, 256 scalar reads agree with independently
parsed GXP parameter/container declarations after removing the scaling. All
256 disagree with the old arithmetic. The native programs are game assets and
are not distributed in this fork.

## Validation and limits

Before this source branch, the equivalent correction was applied to cached
shaders in an isolated unpatched-game installation using Vita3K 4188
(`86b8e2eb5cabd9be9d937e29b22dddf3904dd661`). Twelve edited OpenGL vertex
shaders compile with glslang 16.6.0. All 12 original and edited Vulkan SPIR-V
modules pass SPIRV-Tools v2026.4 validation for Vulkan 1.1. Four local reference
and rejection tests pass, including the 256 scalar reads above.

The user confirmed a substantial improvement on Vulkan: timer and speed panels
align, the radar background is circular and the large dark rectangle is gone.
OpenGL improved only partially. A white outlined quad remains unexplained;
radio/counter behavior in all states, other games, physical hardware and the
complete emulator built from this branch have not been verified. This is an
experimental general translator change, not a claim of full game compatibility.

The optional Windows CI builds this branch, runs upstream CTest and uploads a
test artifact. It does not publish a GitHub release. Keep the normal emulator,
data and saves backed up and use a separate data directory for runtime testing.

## Changelog

- 2026-10-25: Remove repeated indirect-index scaling; align disassembly and
  invalidate stale generated shaders (cache version 16).
