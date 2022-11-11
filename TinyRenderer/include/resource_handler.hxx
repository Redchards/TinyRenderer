#ifndef TINYRENDERER_RESOURCE_HANDLER_HXX
#define TINYRENDERER_RESOURCE_HANDLER_HXX

#include <config.hxx>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <type_traits>

namespace tinyrenderer::utils
{
namespace details
{

template<class ResourceHandleTraits, class = void>
struct has_resource_type : std::false_type 
{};

template<class ResourceHandleTraits>
struct has_resource_type<ResourceHandleTraits, std::void_t<typename ResourceHandleTraits::resource_type>> : std::true_type
{};

template<class ResourceHandleTraits>
constexpr bool has_resource_type_v = has_resource_type<ResourceHandleTraits>::value;

template<class ResourceHandleTraits, class = void>
struct has_resource_deleter : std::false_type 
{};

template<class ResourceHandleTraits>
struct has_resource_deleter<ResourceHandleTraits, std::void_t<decltype(ResourceHandleTraits::close(std::declval<typename ResourceHandleTraits::resource_type>()))>> : std::true_type
{};

template<class ResourceHandleTraits>
constexpr bool has_resource_deleter_v = has_resource_deleter<ResourceHandleTraits>::value;

}

namespace concepts
{

template<class TResourceHandleTraits>
concept ResourceHandleTraits = details::has_resource_type_v<TResourceHandleTraits> && details::has_resource_deleter_v<TResourceHandleTraits>;

}

template<concepts::ResourceHandleTraits TResourceHandleTraits>
class ResourceHandle
{
private:
    using resource_type = typename TResourceHandleTraits::resource_type;

public:
    ResourceHandle() = default;
    explicit ResourceHandle(resource_type raw_resource_handle) noexcept 
    : raw_resource_handle_(raw_resource_handle) 
    {}

    ResourceHandle(ResourceHandle&& other) noexcept 
    : raw_resource_handle_(other.raw_resource_handle_)
    {
        other.raw_resource_handle_ = nullptr;
    }

    ResourceHandle(const ResourceHandle&) = delete;
    ResourceHandle& operator=(const ResourceHandle&) = delete;

    ResourceHandle& operator=(resource_type raw_resource_handler) noexcept
    {
        *this = ResourceHandle{ raw_resource_handler };

        return *this;
    }

    ResourceHandle& operator=(ResourceHandle&& other) noexcept
    {
        if (this != &other)
        {
            close();
            raw_resource_handle_ = other.raw_resource_handle_;
            other.raw_resource_handle_ = nullptr;
        }

        return *this;
    }

    ~ResourceHandle()
    {
        close();
    }

    resource_type get() const noexcept
    {
        return raw_resource_handle_;
    }

private:
    void close() noexcept
    {
        if (raw_resource_handle_ != nullptr)
        {
            TResourceHandleTraits::close(raw_resource_handle_);
            raw_resource_handle_ = nullptr;
        }
    }

private:
    resource_type raw_resource_handle_;
};


struct DLL_API WindowHandleTraits
{
    using resource_type = SDL_Window*;

    static void close(resource_type window_handle) noexcept;
};

struct DLL_API RendererHandleTraits
{
    using resource_type = SDL_Renderer*;

    static void close(resource_type render_handle) noexcept;
};

struct DLL_API TextureHandleTraits
{
    using resource_type = SDL_Texture*;

    static void close(resource_type texture_handle) noexcept;
};

struct DLL_API SurfaceHandleTraits
{
    using resource_type = SDL_Surface*;

    static void close(resource_type surface_handle) noexcept;
};

struct DLL_API FontHandleTraits
{
    using resource_type = TTF_Font*;

    static void close(resource_type font_handle) noexcept;
};

}

namespace tinyrenderer::resource
{

using WindowHandle = utils::ResourceHandle<utils::WindowHandleTraits>;
using RendererHandle = utils::ResourceHandle<utils::RendererHandleTraits>;
using TextureHandle = utils::ResourceHandle<utils::TextureHandleTraits>;
using SurfaceHandle = utils::ResourceHandle<utils::SurfaceHandleTraits>;
using FontHandle = utils::ResourceHandle<utils::FontHandleTraits>;

}

#endif // TINYRENDERER_RESOURCE_HANDLER_HXX
