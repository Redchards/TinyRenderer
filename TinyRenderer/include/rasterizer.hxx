#ifndef TINYRENDERER_RASTERIZER_HXX
#define TINYRENDERER_RASTERIZER_HXX

#include <config.hxx>
#include <resource_handler.hxx>

#include <SDL2/SDL.h>

#include <Eigen/Dense>

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
    using Vector2i = Eigen::Vector2i;
    using Vector2d = Eigen::Vector2d;
    using Vector3i = Eigen::Vector3i;
    using Vector3d = Eigen::Vector3d;

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
    void draw_triangle_sweep(Vector2i v0, Vector2i v1, Vector2i v2, Color color);
    void draw_triangle(Vector2i v0, Vector2i v1, Vector2i v2, Color color);
    void draw(const Mesh& mesh);
    void draw_wireframe(const Mesh& mesh);
    void draw_overlay(const resource::SurfaceHandle& surface);
    void render();
    void render_overlay();

private:
    void regenerate_canvas();
    void canvas_set(uint32_t x, uint32_t y, const Color& color);

    // TODO : to utils
    bool is_in_bounds(uint32_t x, uint32_t y);
    bool is_in_bounds(const Vector2i& pos);
    Vector2i clamp_to_canvas(const Vector2i& pos);
    uint32_t color_to_colorpoint(const Color& color);
    Vector3d compute_barycentric_coords(const Vector2i& v0, const Vector2i& v1, const Vector2i& v2, const Vector2i& p);
    std::pair<Vector2i, Vector2i> compute_bounding_box(const Vector2i& v0, const Vector2i& v1, const Vector2i& v2);
    Vector2i world_to_screen(const Vector3d& v);

private:
    static constexpr WindowDimensions MIN_WINDOW_DIM{ 50, 50 };

    std::vector<uint32_t> buffer_;
    resource::WindowHandle window_;
    resource::RendererHandle render_;
    resource::TextureHandle canvas_;
    Color clear_color_;
    WindowDimensions window_dimensions_;
    resource::TextureHandle text_overlay_;
};

}

#endif // TINYRENDERER_RASTERIZER_HXX
