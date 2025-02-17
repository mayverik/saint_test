#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_image/SDL_image.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>
#include <filesystem>
#include <glad/glad.h>

constexpr uint32_t windowStartWidth = 400;
constexpr uint32_t windowStartHeight = 400;

const float vertices[] = {0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                          0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
                          -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
                          -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f};
const unsigned int indices[] = {0, 1, 2,
                                2, 3, 0};
int shaderSuccess;
char infoLog[1024];

GLuint shaderProgram, vao, vbo, ebo, imgTex;

struct AppContext
{
    SDL_Window *window;
    // SDL_Renderer *renderer;
    SDL_GLContext context;
    // SDL_Texture *messageTex, *imageTex;
    SDL_FRect messageDest;
    SDL_AudioDeviceID audioDevice;
    Mix_Music *music;
    SDL_AppResult app_quit = SDL_APP_CONTINUE;
};

SDL_AppResult SDL_Fail()
{
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    // init the library, here we make a window so we only need the Video capabilities.
    if (not SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        return SDL_Fail();
    }

    // init TTF
    if (not TTF_Init())
    {
        return SDL_Fail();
    }

    // opengl is different version for emscripten
    if (SDL_strcmp(SDL_GetPlatform(), "Emscripten") == 0)
    {
        if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3))
        {
            return SDL_Fail();
        }
        if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0))
        {
            return SDL_Fail();
        }
    }

    else
    {
        if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3))
        {
            return SDL_Fail();
        }
        if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3))
        {
            return SDL_Fail();
        }
    }
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE))
    {
        return SDL_Fail();
    }
    if (!SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24))
    {
        return SDL_Fail();
    }
    if (!SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8))
    {
        return SDL_Fail();
    }

    // create a window

    SDL_Window *window = SDL_CreateWindow("SDL Minimal Sample", windowStartWidth, windowStartHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    // SDL_Window *window = SDL_CreateWindow("SDL Minimal Sample", windowStartWidth, windowStartHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (not window)
    {
        return SDL_Fail();
    }

    // // create a renderer
    // SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    // if (not renderer)
    // {
    //     return SDL_Fail();
    // }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context)
    {
        return SDL_Fail();
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        return SDL_APP_FAILURE;
    }

    // load the font
#if __ANDROID__
    std::filesystem::path basePath = ""; // on Android we do not want to use basepath. Instead, assets are available at the root directory.
#else
    auto basePathPtr = SDL_GetBasePath();
    if (not basePathPtr)
    {
        return SDL_Fail();
    }
    const std::filesystem::path basePath = basePathPtr;
#endif

    std::string vertexCodeFileName = "vertex_default.vs";
    std::string fragmentCodeFileName = "fragment_default.fs";
    if (SDL_strcmp(SDL_GetPlatform(), "Emscripten") == 0)
    {
        vertexCodeFileName = "vertex_web.vs";
        fragmentCodeFileName = "fragment_web.fs";
    }

    const char *vertexCode;
    std::string vertexCodeString;
    std::stringstream vertexCodeStream;
    std::ifstream vertexCodeFile(basePath / vertexCodeFileName);
    if (!vertexCodeFile.is_open())
    {
        std::cerr << "Cannot open vertex shader file!" << std::endl;
        return SDL_APP_FAILURE;
    }

    vertexCodeStream << vertexCodeFile.rdbuf();
    vertexCodeFile.close();
    vertexCodeString = vertexCodeStream.str();
    vertexCode = vertexCodeString.c_str();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexCode, NULL);
    glCompileShader(vertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &shaderSuccess);
    if (!shaderSuccess)
    {
        glGetShaderInfoLog(vertexShader, 1024, NULL, infoLog);
        std::cerr << "Error compiling vertex shader!\n"
                  << infoLog << std::endl;
        return SDL_APP_FAILURE;
    }

    const char *fragmentCode;
    std::string fragmentCodeString;
    std::stringstream fragmentCodeStream;
    std::ifstream fragmentCodeFile(basePath / fragmentCodeFileName);

    if (!fragmentCodeFile.is_open())
    {
        std::cerr << "Cannot open fragment code file!" << std::endl;
        return SDL_APP_FAILURE;
    }

    fragmentCodeStream << fragmentCodeFile.rdbuf();
    fragmentCodeFile.close();
    fragmentCodeString = fragmentCodeStream.str();
    fragmentCode = fragmentCodeString.c_str();

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentCode, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &shaderSuccess);
    if (!shaderSuccess)
    {
        glGetShaderInfoLog(fragmentShader, 1024, NULL, infoLog);
        std::cerr << "Error compiling fragment shader!\n"
                  << infoLog << std::endl;
        return SDL_APP_FAILURE;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &shaderSuccess);
    if (!shaderSuccess)
    {
        glGetProgramInfoLog(shaderProgram, 1024, NULL, infoLog);
        std::cerr << "Error linking shader program!\n"
                  << infoLog << std::endl;
        return SDL_APP_FAILURE;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);

    const auto fontPath = basePath / "Inter-VariableFont.ttf";
    TTF_Font *font = TTF_OpenFont(fontPath.string().c_str(), 36);
    if (not font)
    {
        return SDL_Fail();
    }

    // render the font to a surface
    const std::string_view text = "Hello SDL!";
    SDL_Surface *surfaceMessage = TTF_RenderText_Solid(font, text.data(), text.length(), {255, 255, 255});

    // make a texture from the surface
    // SDL_Texture *messageTex = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

    // we no longer need the font or the surface, so we can destroy those now.
    TTF_CloseFont(font);
    SDL_DestroySurface(surfaceMessage);

    glGenTextures(1, &imgTex);
    glBindTexture(GL_TEXTURE_2D, imgTex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // load the SVG
    auto svg_surface = IMG_Load((basePath / "logo.png").string().c_str());
    // SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, svg_surface);
    auto svg_surface_rgba = SDL_ConvertSurface(svg_surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(svg_surface);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, svg_surface->w, svg_surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, svg_surface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    SDL_DestroySurface(svg_surface_rgba);

    // get the on-screen dimensions of the text. this is necessary for rendering it
    // auto messageTexProps = SDL_GetTextureProperties(messageTex);
    // SDL_FRect text_rect{
    //     .x = 0,
    //     .y = 0,
    //     .w = float(SDL_GetNumberProperty(messageTexProps, SDL_PROP_TEXTURE_WIDTH_NUMBER, 0)),
    //     .h = float(SDL_GetNumberProperty(messageTexProps, SDL_PROP_TEXTURE_HEIGHT_NUMBER, 0))};

    // init SDL Mixer
    auto audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (not audioDevice)
    {
        return SDL_Fail();
    }
    if (not Mix_OpenAudio(audioDevice, NULL))
    {
        return SDL_Fail();
    }

    // load the music
    auto musicPath = basePath / "the_entertainer.ogg";
    auto music = Mix_LoadMUS(musicPath.string().c_str());
    if (not music)
    {
        return SDL_Fail();
    }

    // play the music (does not loop)
    Mix_PlayMusic(music, 0);

    // print some information about the window
    SDL_ShowWindow(window);
    {
        int width, height, bbwidth, bbheight;
        SDL_GetWindowSize(window, &width, &height);
        SDL_GetWindowSizeInPixels(window, &bbwidth, &bbheight);
        SDL_Log("Window size: %ix%i", width, height);
        SDL_Log("Backbuffer size: %ix%i", bbwidth, bbheight);
        if (width != bbwidth)
        {
            SDL_Log("This is a highdpi environment.");
        }
    }

    // set up the application data
    *appstate = new AppContext{
        .window = window,
        // .renderer = renderer,
        .context = context,
        // .messageTex = messageTex,
        // .imageTex = tex,
        // .messageDest = text_rect,
        .audioDevice = audioDevice,
        .music = music,
    };

    // SDL_SetRenderVSync(renderer, -1); // enable vysnc

    SDL_Log("Application started successfully!");

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    auto *app = (AppContext *)appstate;

    if (event->type == SDL_EVENT_QUIT)
    {
        app->app_quit = SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    auto *app = (AppContext *)appstate;

    int windowWidth, windowHeight;
    SDL_GetWindowSize(app->window, &windowWidth, &windowHeight);

    // draw a color
    auto time = SDL_GetTicks() / 1000.f;
    auto red = (std::sin(time) + 1) / 2.0 * 255;
    auto green = (std::sin(time / 2) + 1) / 2.0 * 255;
    auto blue = (std::sin(time) * 2 + 1) / 2.0 * 255;

    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(red / 255.0f, blue / 255.0f, green / 255.0f, SDL_ALPHA_OPAQUE_FLOAT);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindTexture(GL_TEXTURE_2D, imgTex);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
    // glDrawArrays(GL_TRIANGLES, 0, 3);

    // SDL_SetRenderDrawColor(app->renderer, red, green, blue, SDL_ALPHA_OPAQUE);
    // SDL_RenderClear(app->renderer);

    // // Renderer uses the painter's algorithm to make the text appear above the image, we must render the image first.
    // SDL_RenderTexture(app->renderer, app->imageTex, NULL, NULL);
    // SDL_RenderTexture(app->renderer, app->messageTex, NULL, &app->messageDest);

    // SDL_RenderPresent(app->renderer);

    SDL_GL_SwapWindow(app->window);

    return app->app_quit;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    auto *app = (AppContext *)appstate;
    if (app)
    {
        // SDL_DestroyRenderer(app->renderer);
        // SDL_DestroyWindow(app->window);

        glDeleteTextures(1, &imgTex);
        glDeleteBuffers(1, &ebo);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);

        glDeleteProgram(shaderProgram);

        SDL_GL_DestroyContext(app->context);
        SDL_DestroyWindow(app->window);

        Mix_FadeOutMusic(1000);    // prevent the music from abruptly ending.
        Mix_FreeMusic(app->music); // this call blocks until the music has finished fading
        Mix_CloseAudio();
        SDL_CloseAudioDevice(app->audioDevice);

        delete app;
    }
    TTF_Quit();
    Mix_Quit();

    SDL_Log("Application quit successfully!");
    SDL_Quit();
}
