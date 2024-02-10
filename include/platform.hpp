#include <SDL2/SDL.h>
class Platform
{
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

public:
    Platform(char const* title, int windowWidth, int windowHeight,
             int textureWidth, int textureHeight);
    ~Platform()
    {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
    void Update(void const* buffer, int pitch);
    bool ProcessInput(uint8_t* keys);
};