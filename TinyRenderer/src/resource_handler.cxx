#include <resource_handler.hxx>

namespace tinyrenderer::utils
{

void WindowHandleTraits::close(resource_type window_handle) noexcept
{
    SDL_DestroyWindow(window_handle);
}

void RendererHandleTraits::close(resource_type render_handle) noexcept
{
    SDL_DestroyRenderer(render_handle);
}

void TextureHandleTraits::close(resource_type window_handle) noexcept
{
    SDL_DestroyTexture(window_handle);
}

void SurfaceHandleTraits::close(resource_type surface_handle) noexcept
{
    SDL_FreeSurface(surface_handle);
}

void FontHandleTraits::close(resource_type font_handle) noexcept
{
    TTF_CloseFont(font_handle);
}

}