#include <mesh.hxx>
#include <rasterizer.hxx>
#include <resource_handler.hxx>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <Eigen/Dense>
#include <Eigen/Geometry>

#include <plog/Log.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Initializers/ConsoleInitializer.h>

#include <snowhouse/snowhouse.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <format>
#include <vector>

constexpr int INITIAL_WINDOW_WIDTH = 800;
constexpr int INITIAL_WINDOW_HEIGHT = 600;
constexpr double PI = M_PI;

namespace tinyrenderer
{

void init_log()
{
    static plog::RollingFileAppender<plog::CsvFormatter> file_appender("tinyrenderer.log", 8000, 3);
    static plog::ConsoleAppender<plog::TxtFormatter> console_appender;

    plog::init(plog::debug, &file_appender).addAppender(&console_appender);
}

struct WindowDimensions
{
    uint32_t width;
    uint32_t height;
};

class FrameInfoReporter
{
private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using FontHandle = resource::FontHandle;

public:
    FrameInfoReporter()
    : current_tick_{}
    , font_ {}
    , start_frame_time_(Clock::now())
    , end_frame_time_(Clock::now())
    {
        using snowhouse::IsNull;

        const char* font_path = "assets/font/FiraCode-Retina.ttf";
        font_ = TTF_OpenFont(font_path, 14);
        if (font_.get() == nullptr)
        {
            PLOG(plog::fatal) << "Can not find font '" << font_path << "'! SDL_Error: " << SDL_GetError() << std::endl;
            AssertThat(font_.get(), !IsNull());
        }
    }

    FrameInfoReporter(const FrameInfoReporter&) = delete;
    FrameInfoReporter(FrameInfoReporter&&) = delete;
    FrameInfoReporter& operator=(const FrameInfoReporter&) = delete;
    FrameInfoReporter& operator=(FrameInfoReporter&&) = delete;

    void start_frame()
    {
        ++current_tick_;

        if (current_tick_ == frame_report_tick)
        {
            start_frame_time_ = Clock::now();
        }
    }

    void end_frame()
    {
        if (current_tick_ == frame_report_tick)
        {
            end_frame_time_ = Clock::now();
            current_tick_ = 0;
        }
    }

    auto get_frame_time()
    {
        return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end_frame_time_ - start_frame_time_).count());
    }

    resource::SurfaceHandle get_frame_info_surface()
    {
        auto frame_time = get_frame_time();
        auto info = std::format("Frame time : {:.2f}ms            FPS : {:.0f}",
            frame_time / 1000.,
            1000000. / frame_time
        );
        SDL_Surface* msg_surface = TTF_RenderText_Blended_Wrapped(font_.get(), info.c_str(), {255, 255, 255}, 300);

        return resource::SurfaceHandle{ msg_surface };
    }

private:
    static constexpr size_t frame_report_tick = 15;
    size_t current_tick_;
    TimePoint start_frame_time_;
    TimePoint end_frame_time_;
    FontHandle font_;
};

void rotate_mesh(Mesh& mesh, double delta_time)
{
    if (delta_time == 0.) return;
    double angle = 2 * PI * (delta_time / 2000000.);

    auto rotation_matrix = Eigen::AngleAxisd(angle, Eigen::Vector3d::UnitY()).toRotationMatrix();
    // Eigen::Matrix4f rotation_matrix = Eigen::Matrix4f::Identity();

    mesh.transform([&rotation_matrix](const Eigen::Vector3d& vertex)
    {
        return rotation_matrix * vertex;
    });
}

void main_loop()
{
    init_log();
    bool running = true;

    WindowDimensions window_dimensions{ INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT };
    Mesh mesh = *Mesh::load("assets/mesh/mumbaka.obj");

    TTF_Init();
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        PLOG(plog::fatal) << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
    }
    else
    {

        FrameInfoReporter frame_reporter{};
        PLOG(plog::debug) << "Creating window..." << std::endl;

        resource::WindowHandle window{ SDL_CreateWindow
        (
            "Tiny Render",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        ) };

        Rasterizer rasterizer{ std::move(window) };

        while (running)
        {
            frame_reporter.start_frame();
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                case SDL_WINDOWEVENT: 
                {
                    switch (event.window.event)
                    {
                    case SDL_WINDOWEVENT_RESIZED:
                    {
                        uint32_t width = event.window.data1;
                        uint32_t height = event.window.data2;

                        rasterizer.resize_canvas(width, height);
                    }
                    break;
                    case SDL_WINDOWEVENT_CLOSE:
                    {
                        PLOG(plog::debug) << "Shutting down" << std::endl;
                        running = false;
                    } 
                    break;
                    }
                }
                case SDL_KEYDOWN:
                {
                    switch (event.key.keysym.sym)
                    {
                    case SDLK_ESCAPE:
                    case SDLK_q:
                    {
                        PLOG(plog::debug) << "Shutting down" << std::endl;
                        running = false;
                    }
                    }
                }
                }
            }

            rasterizer.draw(mesh);
            rasterizer.render();

            frame_reporter.end_frame();
            rasterizer.draw_text(frame_reporter.get_frame_info_surface());
            rasterizer.render_overlay();
            rotate_mesh(mesh, frame_reporter.get_frame_time());
        }
    }
}

}

int main(int arc, char* argv[])
{
    tinyrenderer::main_loop();
    return 0;
}