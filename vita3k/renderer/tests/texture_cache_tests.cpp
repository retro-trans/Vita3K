// Vita3K emulator project
// Copyright (C) 2026 Vita3K team
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gxm/functions.h>
#include <mem/ptr.h>
#include <renderer/texture_cache.h>
#include <renderer/types.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <tuple>

namespace {
class ProbeCache final : public renderer::TextureCache {
public:
    size_t uploads = 0;
    uint32_t first_pixel = 0;

    void select(size_t, const SceGxmTexture &) override {}
    void configure_texture(const SceGxmTexture &) override {}
    void upload_texture_impl(SceGxmTextureBaseFormat, uint32_t, uint32_t, uint32_t, const void *pixels, int, uint32_t) override {
        std::memcpy(&first_pixel, pixels, sizeof(first_pixel));
        ++uploads;
    }
    void import_configure_impl(SceGxmTextureBaseFormat, uint32_t, uint32_t, bool, uint16_t, uint16_t, bool) override {}

    bool using_hash() const {
        return current_info->use_hash;
    }
};

class PaletteCacheTest : public ::testing::TestWithParam<std::tuple<SceGxmTextureBaseFormat, renderer::Backend, bool>> {
protected:
    MemState mem;

    void SetUp() override {
        ASSERT_TRUE(init(mem, false));
    }

    void TearDown() override {
        deinit_mem(mem);
    }
};

TEST_P(PaletteCacheTest, PaletteOnlyChangesRefreshAnExistingTexture) {
    const auto [format, backend, protect] = GetParam();
    const uint32_t pixel_bytes = format == SCE_GXM_TEXTURE_BASE_FORMAT_P4 ? 512 * 512 / 2 : 512 * 512;
    const Address pixels = alloc_aligned(mem, pixel_bytes, "texture-test-indices", mem.host_page_size);
    const Address palette = alloc_aligned(mem, 1024, "texture-test-palette", mem.host_page_size);
    ASSERT_NE(pixels, 0U);
    ASSERT_NE(palette, 0U);
    std::memset(Ptr<uint8_t>(pixels).get(mem), 0, pixel_bytes);
    auto colors = Ptr<uint32_t>(palette).get(mem);
    std::fill_n(colors, 256, 0xff0000ffU);

    SceGxmTexture texture{};
    texture.type = SCE_GXM_TEXTURE_LINEAR >> 29;
    texture.width = 511;
    texture.height = 511;
    texture.base_format = (format >> 24) & 31;
    texture.format0 = format >> 31;
    texture.data_addr = pixels >> 2;
    texture.palette_addr = palette >> 6;

    ProbeCache cache;
    cache.backend = backend;
    ASSERT_TRUE(cache.init(protect, fs::path{}, "palette-cache-regression", 0));
    cache.cache_and_bind_texture(texture, mem);
    ASSERT_EQ(cache.uploads, 1U);
    EXPECT_EQ(cache.first_pixel, 0xff0000ffU);
    EXPECT_TRUE(cache.using_hash());

    cache.cache_and_bind_texture(texture, mem);
    EXPECT_EQ(cache.uploads, 1U);

    // Recolor at the same address: no descriptor or pixel index changes.
    for (uint32_t color : { 0xff00ff00U, 0xffff0000U, 0xff00ff00U }) {
        const size_t previous_uploads = cache.uploads;
        colors[0] = color;
        cache.cache_and_bind_texture(texture, mem);
        EXPECT_EQ(cache.uploads, previous_uploads + 1);
        EXPECT_EQ(cache.first_pixel, color);
        cache.cache_and_bind_texture(texture, mem);
        EXPECT_EQ(cache.uploads, previous_uploads + 1);
    }
}

INSTANTIATE_TEST_SUITE_P(PalettedTextures, PaletteCacheTest,
    ::testing::Combine(
        ::testing::Values(SCE_GXM_TEXTURE_BASE_FORMAT_P4, SCE_GXM_TEXTURE_BASE_FORMAT_P8),
        ::testing::Values(renderer::Backend::OpenGL, renderer::Backend::Vulkan),
        ::testing::Bool()));

TEST(TextureCache, NonPalettedTexturesRetainProtectedTracking) {
    MemState mem;
    ASSERT_TRUE(init(mem, false));
    {
        const Address pixels = alloc_aligned(mem, 512 * 512 * 4, "texture-test-rgba", mem.host_page_size);
        ASSERT_NE(pixels, 0U);
        std::memset(Ptr<uint8_t>(pixels).get(mem), 0, 512 * 512 * 4);
        SceGxmTexture texture{};
        texture.type = SCE_GXM_TEXTURE_LINEAR >> 29;
        texture.width = 511;
        texture.height = 511;
        texture.base_format = SCE_GXM_TEXTURE_BASE_FORMAT_U8U8U8U8 >> 24;
        texture.data_addr = pixels >> 2;
        ProbeCache cache;
        cache.backend = renderer::Backend::Vulkan;
        ASSERT_TRUE(cache.init(true, fs::path{}, "rgba-cache-regression", 0));
        cache.cache_and_bind_texture(texture, mem);
        EXPECT_FALSE(cache.using_hash());
        EXPECT_EQ(cache.uploads, 1U);
        cache.cache_and_bind_texture(texture, mem);
        EXPECT_EQ(cache.uploads, 1U);
    }
    deinit_mem(mem);
}
} // namespace
