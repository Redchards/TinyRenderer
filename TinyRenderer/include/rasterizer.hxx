#ifndef TINYRENDERER_RASTERIZER_HXX
#define TINYRENDERER_RASTERIZER_HXX

#include <config.hxx>
#include <resource_handler.hxx>

#include <SDL2/SDL.h>

#include <cstdint>
#include <optional>
#include <vector>

DLL_API uint32_t factorial(uint32_t n);
DLL_API void test_line(SDL_Texture* screen_texture, size_t width, size_t height);

class Mesh;

namespace tinyrenderer
{

class DLL_API Rasterizer
{
private:
    using Color = SDL_Color;

public:
    using RenderArea = SDL_Rect;

private:
    struct WindowDimensions
    {
        uint32_t width{};
        uint32_t height{};
    };

public:
    Rasterizer(resource::WindowHandle&& window_handle);
    Rasterizer(const Rasterizer&) = delete;
    Rasterizer& operator=(const Rasterizer&) = delete;
    Rasterizer(Rasterizer&&) = delete;
    Rasterizer& operator=(Rasterizer&&) = delete;

    void resize_canvas(uint32_t width, uint32_t height);
    void draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, Color color);
    void draw(const Mesh& mesh);
    void draw_text(const resource::SurfaceHandle& surface);
    void render();
    void render_overlay();

private:
    void regenerate_canvas();

private:
    static constexpr WindowDimensions MIN_WINDOW_DIM{ 50, 50 };

    std::vector<uint32_t> buffer_;
    resource::WindowHandle window_;
    resource::RendererHandle render_;
    resource::TextureHandle canvas_;
    WindowDimensions window_dimensions_;
    resource::TextureHandle text_overlay_;
};

}

#endif // TINYRENDERER_RASTERIZER_HXX
