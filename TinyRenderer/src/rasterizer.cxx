#include <rasterizer.hxx>

#include <mesh.hxx>

#include <snowhouse/snowhouse.h>

#include <Eigen/Dense>

#include <cmath>
#include <vector>

void test_line(SDL_Texture* screen_texture, size_t width, size_t height)
{
    std::vector<uint32_t> buffer;

    buffer.resize(width * height);

    for (size_t x = 0; x < width; ++x)
    {

        buffer[400 * width + x] = 0xFFFFFFFF;
    }

    SDL_UpdateTexture(screen_texture, nullptr, buffer.data(), static_cast<uint32_t>(width) * sizeof(uint32_t));
}

uint32_t factorial(uint32_t n)
{
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

namespace tinyrenderer
{

Rasterizer::Rasterizer(resource::WindowHandle&& window_handle)
: buffer_{}
, window_{ std::move(window_handle) }
, render_{}
, canvas_{}
, clear_color_{ 0, 0, 0, 0 }
, window_dimensions_{}
, text_overlay_{}
{
    using snowhouse::IsNull;

    int window_width{};
    int window_height{};

    SDL_SetWindowMinimumSize(window_.get(), MIN_WINDOW_DIM.width, MIN_WINDOW_DIM.height);
    SDL_GetWindowSize(window_.get(), &window_width, &window_height);
    window_dimensions_.width = window_width;
    window_dimensions_.height = window_height;

    render_ = SDL_CreateRenderer(window_.get(), -1, SDL_RENDERER_ACCELERATED);
    AssertThat(render_.get(), !IsNull());

    SDL_SetRenderDrawColor(render_.get(), clear_color_.r, clear_color_.g, clear_color_.b, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(render_.get());

    regenerate_canvas();
}

void Rasterizer::resize_canvas(uint32_t width, uint32_t height)
{
    window_dimensions_.width = width;
    window_dimensions_.height = height;

    regenerate_canvas();
}

void Rasterizer::render()
{
    SDL_RenderClear(render_.get());
    SDL_UpdateTexture(canvas_.get(), nullptr, buffer_.data(), static_cast<uint32_t>(window_dimensions_.width) * sizeof(uint32_t));
    SDL_RenderCopy(render_.get(), canvas_.get(), nullptr, nullptr);
    render_overlay();
    SDL_RenderPresent(render_.get());
    std::fill(buffer_.begin(), buffer_.end(), color_to_colorpoint(clear_color_));
}

void Rasterizer::render_overlay()
{
    int w, h;
    SDL_QueryTexture(text_overlay_.get(), nullptr, nullptr, &w, &h);
    RenderArea render_area{0, 0, w, h };
    SDL_RenderCopy(render_.get(), text_overlay_.get(), nullptr, &render_area);

    SDL_RenderPresent(render_.get());
}

void Rasterizer::draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, Color color)
{
    bool steep = false;

    if (!is_in_bounds(x0, y0) && !is_in_bounds(x1, y1)) return;

    x0 = std::min(x0, window_dimensions_.width - 1);
    x1 = std::min(x1, window_dimensions_.width - 1);
    y0 = std::min(y0, window_dimensions_.height - 1);
    y1 = std::min(y1, window_dimensions_.height - 1);

    auto dx = std::max(x0, x1) - std::min(x0, x1);
    auto dy = std::max(y0, y1) - std::min(y0, y1);

    if (dx < dy)
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }

    if (x1 < x0)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    dy = std::max(y0, y1) - std::min(y0, y1);
    int32_t error = 0;
    int32_t y = y0;

    const uint32_t dx_dy = dy * 2;
    const int32_t sdx = x1 - x0;
    const int8_t delta = y1 > y0 ? 1 : -1;

    if (steep)
    {
        for (uint32_t x = x0; x <= x1; ++x)
        {
            canvas_set(y, x, color);

            error += dx_dy;
            if (error > sdx)
            {
                y += delta;
                error -= sdx * 2;
            }
        }
    }
    else
    {
        for (uint32_t x = x0; x <= x1; ++x)
        {
            canvas_set(x, y, color);

            error += dx_dy;
            if (error > sdx)
            {
                y += delta;
                error -= sdx * 2;
            }
        }
    }
}

void Rasterizer::draw(const Mesh& mesh)
{
    for (int i = 0; i < mesh.get_num_faces(); i++) 
    {
        Eigen::Vector3i face = mesh.get_face(i);

        for (int j = 0; j < 3; j++) 
        {
            const Eigen::Vector3d& v0 = mesh.get_vertex(face[j]);
            const Eigen::Vector3d& v1 = mesh.get_vertex(face[(j + 1) % 3]);

            int32_t x0 = static_cast<int32_t>((v0(0) + 1.) * window_dimensions_.width / 2.);
            int32_t y0 = static_cast<int32_t>((v0(1) + 1.) * window_dimensions_.height / 2.);
            int32_t x1 = static_cast<int32_t>((v1(0) + 1.) * window_dimensions_.width / 2.);
            int32_t y1 = static_cast<int32_t>((v1(1) + 1.) * window_dimensions_.height / 2.);
            draw_line(x0, y0, x1, y1, Color{ 0, 255, 0 });
        }
    }
}

void Rasterizer::draw_text(const resource::SurfaceHandle& surface)
{
    text_overlay_ = SDL_CreateTextureFromSurface(render_.get(), surface.get());
}

void Rasterizer::regenerate_canvas()
{
    using snowhouse::IsNull;

    canvas_ = SDL_CreateTexture(render_.get(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, window_dimensions_.width, window_dimensions_.height);
    AssertThat(canvas_.get(), !IsNull());

    buffer_.clear();
    buffer_.resize(static_cast<size_t>(window_dimensions_.width) * static_cast<size_t>(window_dimensions_.height));
}

void Rasterizer::canvas_set(uint32_t x, uint32_t y, const Color& color)
{
    size_t idx = static_cast<size_t>(y) * window_dimensions_.width + x;
    buffer_[idx] = color_to_colorpoint(color);
}

bool Rasterizer::is_in_bounds(uint32_t x, uint32_t y)
{
    return x < window_dimensions_.width && y < window_dimensions_.height;
}

uint32_t Rasterizer::color_to_colorpoint(const Color& color)
{
    return 0xFF << 24 | color.r << 16 | color.g << 8 | color.b;
}


}
