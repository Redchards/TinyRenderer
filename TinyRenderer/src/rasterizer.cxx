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

static int32_t linear_interpolation(int32_t a, int32_t b, double alpha)
{
    return static_cast<int32_t>(a + alpha * (b - a));
}

// Line sweep triangle algorithm
void Rasterizer::draw_triangle_sweep(Vector2i v0, Vector2i v1, Vector2i v2, Color color)
{
    if (!(is_in_bounds(v0) || is_in_bounds(v1) || is_in_bounds(v2))) return;

    std::array<Vector2i, 3> vertices = { 
        clamp_to_canvas(v0), 
        clamp_to_canvas(v1), 
        clamp_to_canvas(v2)
    };

    std::sort(vertices.begin(), vertices.end(), [](const Vector2i& lhs, const Vector2i& rhs)
    {
        return lhs.y() < rhs.y();
    });
    
    const double total_height          = vertices[2].y() - vertices[0].y() + 1;
    const double top_segment_height    = vertices[1].y() - vertices[0].y() + 1;
    const double bottom_segment_height = vertices[2].y() - vertices[1].y() + 1;

    for (int32_t h = 0; h <= top_segment_height; ++h)
    {
        int32_t x0 = linear_interpolation(vertices[0].x(), vertices[2].x(), (h / total_height));
        int32_t x1 = linear_interpolation(vertices[0].x(), vertices[1].x(), (h / top_segment_height));

        for (auto x = std::min(x0, x1); x <= std::max(x0, x1); ++x)
        {
            canvas_set(x, vertices[0].y() + h, color);
        }
    }

    for (int32_t h = 0; h <= bottom_segment_height; ++h)
    {
        int32_t x0 = linear_interpolation(vertices[0].x(), vertices[2].x(), ((h + top_segment_height) / total_height));
        int32_t x1 = linear_interpolation(vertices[1].x(), vertices[2].x(), (h / bottom_segment_height));

        for (auto x = std::min(x0, x1); x <= std::max(x0, x1); ++x)
        {
            canvas_set(x, vertices[1].y() + h, color);
        }
    }
}

auto Rasterizer::compute_barycentric_coords(const Vector2i& v0, const Vector2i& v1, const Vector2i& v2, const Vector2i& p)
-> Vector3d
{
    const Vector2d v0v1 = (v1 - v0).cast<double>();
    const Vector2d v0v2 = (v2 - v0).cast<double>();
    const Vector2d pv0  = (v0 - p).cast<double>();

    Vector3d l1{ v0v1.x(), v0v2.x(), pv0.x() };
    Vector3d l2{ v0v1.y(), v0v2.y(), pv0.y() };

    Vector3d u = l1.cross(l2);

    // Handle degenerate case
    if (std::abs(u.z()) < 1) return Vector3d{ -1, 1, 1 };

    return Vector3d{ 1 - (u.x() + u.y()) / u.z(), u.y() / u.z(), u.x() / u.z() };
}

auto Rasterizer::compute_bounding_box(const Vector2i& v0, const Vector2i& v1, const Vector2i& v2)
-> std::pair<Vector2i, Vector2i>
{
    const auto max_x = std::max(0, std::max(v0.x(), std::max(v1.x(), v2.x())));
    const auto max_y = std::max(0, std::max(v0.y(), std::max(v1.y(), v2.y())));
    const auto min_x = std::min(v0.x(), std::min(v1.x(), v2.x()));
    const auto min_y = std::min(v0.y(), std::min(v1.y(), v2.y()));

    return {
        { clamp_to_canvas({ min_x, min_y }) },
        { clamp_to_canvas({ max_x, max_y }) }
    };
}

// Draw triangles based on barycentric coordinates
void Rasterizer::draw_triangle(Vector2i v0, Vector2i v1, Vector2i v2, Color color)
{
    auto [bounding_box_min, bounding_box_max] = compute_bounding_box(v0, v1, v2);

    for (int32_t x = bounding_box_min.x(); x <= bounding_box_max.x(); ++x)
    {
        for (int32_t y = bounding_box_min.y(); y <= bounding_box_max.y(); ++y)
        {
            const auto p = Vector2i{ x, y };
            const auto bc = compute_barycentric_coords(v0, v1, v2, p);

            if (bc.x() < 0 || bc.y() < 0 || bc.z() < 0) continue;

            canvas_set(x, y, color);
        }
    }
}

// Failed attempt, using a line sweep from the "top vertex"
// Unfortunately, it produces artefacts
/* void Rasterizer::draw_triangle(Vector2i v0, Vector2i v1, Vector2i v2, Color color)
{
    bool steep = false;

    if (!(is_in_bounds(v0) || is_in_bounds(v1) || is_in_bounds(v2))) return;

    std::array<Vector2i, 3> vertices = { 
        clamp_to_canvas(v0), 
        clamp_to_canvas(v1), 
        clamp_to_canvas(v2)
    };

    std::sort(vertices.begin(), vertices.end(), [](const Vector2i& lhs, const Vector2i& rhs)
    {
        return lhs.y() < rhs.y();
    });
    
    const auto& top_v = vertices[0];

    auto x0 = vertices[1].x();
    auto y0 = vertices[1].y();

    auto x1 = vertices[2].x();
    auto y1 = vertices[2].y();

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

    const int32_t dx_dy = dy * 2;
    const int32_t sdx = x1 - x0;
    const int8_t delta = y1 > y0 ? 1 : -1;

    if (steep)
    {
        for (int32_t x = x0; x <= x1; ++x)
        {
            draw_line(top_v.x(), top_v.y(), x, y);

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
        for (int32_t x = x0; x <= x1; ++x)
        {
            draw_line(top_v.x(), top_v.y(), x, y, color);

            error += dx_dy;
            if (error > sdx)
            {
                y += delta;
                error -= sdx * 2;
            }
        }
    }
} */

void Rasterizer::draw(const Mesh& mesh)
{
    Vector3d light_dir = Vector3d::UnitZ();

    for (int i = 0; i < mesh.get_num_faces(); ++i)
    {
        Vector3i face = mesh.get_face(i);
        Vector3d normal = (mesh.get_vertex(face[2]) - mesh.get_vertex(face[0])).cross(mesh.get_vertex(face[1]) - mesh.get_vertex(face[0]));
        normal.normalize();
        auto light_intensity = normal.dot(light_dir);

        if (light_intensity > 0)
        {
            std::array<Vector2i, 3> screen_coords{};
            for (int j = 0; j < screen_coords.size(); ++j)
            {
                const auto& vertex = mesh.get_vertex(face[j]);
                screen_coords[j] = world_to_screen(vertex);
            }

            Color color = { static_cast<uint8_t>(light_intensity * 255), static_cast<uint8_t>(light_intensity * 255), static_cast<uint8_t>(light_intensity * 255) };
            draw_triangle(screen_coords[0], screen_coords[1], screen_coords[2], color);
        }
    }
}

void Rasterizer::draw_wireframe(const Mesh& mesh)
{
    for (int i = 0; i < mesh.get_num_faces(); ++i) 
    {
        Vector3i face = mesh.get_face(i);

        for (int j = 0; j < 3; ++j) 
        {
            const Vector3d& wv0 = mesh.get_vertex(face[j]);
            const Vector3d& wv1 = mesh.get_vertex(face[(j + 1) % 3]);

            auto sv0 = world_to_screen(wv0);
            auto sv1 = world_to_screen(wv1);
            draw_line(sv0.x(), sv0.y(), sv1.x(), sv1.y(), Color{0, 255, 0});
        }
    }
}

void Rasterizer::draw_overlay(const resource::SurfaceHandle& surface)
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

bool Rasterizer::is_in_bounds(const Vector2i& pos)
{
    return is_in_bounds(pos[0], pos[1]);
}

auto Rasterizer::clamp_to_canvas(const Vector2i& pos)
-> Vector2i
{
    return {
        std::min(pos.x(), static_cast<int32_t>(window_dimensions_.width) - 1),
        std::min(pos.y(), static_cast<int32_t>(window_dimensions_.height) - 1)
    };
}

// For now, we assume that the "world" coordinate is the clip coordinate ([-1, 1], disregarding the z component)
auto Rasterizer::world_to_screen(const Vector3d& v)
-> Vector2i
{
    return {
        static_cast<uint32_t>((v(0) + 1.) * window_dimensions_.width / 2.),
        static_cast<uint32_t>((v(1) + 1.) * window_dimensions_.height / 2.)
    };
}

uint32_t Rasterizer::color_to_colorpoint(const Color& color)
{
    return 0xFF << 24 | color.r << 16 | color.g << 8 | color.b;
}

}
