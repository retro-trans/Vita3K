# Live palette changes in the texture cache

Cross Ange's hangar can rewrite a P8 texture's palette without touching its
pixel indices or texture descriptor. The protected-cache path watched only
the pixel buffer, so it kept displaying the earlier colors until the model
was loaded again. Vulkan enables this path regardless of the global hashless
texture-cache setting in the tested upstream revision.

The live reproduction used a 1024 by 1024 P8 texture with a separate 1024-byte
palette. Guest memory already decoded to the newly selected green paint,
while the cached texture still displayed white. Its cache entry had hashing
disabled and was clean. Marking only that host cache entry dirty immediately
refreshed the model to green without leaving the hangar or changing game data.

Paletted textures now use the existing hash path, which includes both pixel
indices and the complete P4/P8 palette. Unchanged textures remain cached;
non-paletted textures retain memory-protection tracking. This trades some
pixel hashing for correct palette updates, with no game-specific condition.

The renderer cache regressions exercise actual uploads through a mock GPU
backend for P4/P8, OpenGL/Vulkan conversion paths and both cache modes. They
check palette-only changes, repeated recoloring and unchanged-cache reuse.
Compilation and a fresh-runtime test must be recorded before calling a binary
validated. The earlier indexed-uniform HUD correction remains independent.

No original game executable, translation asset, license or save file is part
of this emulator correction.
