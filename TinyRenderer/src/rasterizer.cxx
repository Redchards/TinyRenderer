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

    SDL_SetRenderDrawColor(render_.get(), 0, 0, 0, SDL_ALPHA_OPAQUE);
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

    x0 = std::clamp(x0, 0u, window_dimensions_.width - 1);
    x1 = std::clamp(x1, 0u, window_dimensions_.width - 1);
    y0 = std::clamp(y0, 0u, window_dimensions_.height - 1);
    y1 = std::clamp(y1, 0u, window_dimensions_.height - 1);

    if (std::abs(static_cast<double>(x1) - x0) < std::abs(static_cast<double>(y1) - y0))
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

    for (double xi = x0; xi <= x1; ++xi)
    {
        size_t idx = 0;
        double t = 1. - static_cast<double>(x1 - xi) / (x1 - x0);

        uint32_t x = static_cast<uint32_t>(x0 + t * (x1 - x0));
        uint32_t y = static_cast<uint32_t>((1 - t) * y0 + t * y1);

        if (steep)
        {
            idx = static_cast<size_t>(x) * window_dimensions_.width + y;
        }
        else
        {
            idx = static_cast<size_t>(y) * window_dimensions_.width + x;
        }

        buffer_[idx] = 0xFF | color.r << 16 | color.g << 8 | color.b;
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
            draw_line(x0, y0, x1, y1, Color{ 255, 0, 0 });
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


}
