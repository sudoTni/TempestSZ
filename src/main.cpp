#include <raylib.h>
#include "Constants.hpp"
#include "GameState.hpp"
#include "Jukebox.hpp"
#include "Renderer.hpp"
#include "Particles.hpp"
#include "SFX.hpp"
#include "HUD.hpp"
#include "Well.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "Bullet.hpp"
#include "Collision.hpp"
#include "Input.hpp"
#include "IRP.hpp"
#include <exception>
#include <fstream>
#include <string>

// Defined in win_icon.cpp (isolated from <windows.h> / <raylib.h> conflicts).
// Sets the window title-bar and taskbar icon from the PE resource (icon ID 1).
void SetWindowIconFromResource(void* nativeHwnd);

int main(int argc, char** argv) {
    std::ofstream bootLog("sz_launch.log", std::ios::out | std::ios::trunc);
    bootLog << "startup\n";

    bool irpMode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--irp") {
            irpMode = true;
        }
    }

    // 1. Initialization
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(SCREEN_W, SCREEN_H, "SZ - Tempest");
    SetWindowIconFromResource(GetWindowHandle());
    bootLog << "InitWindow done, ready=" << (IsWindowReady() ? 1 : 0) << "\n";
    if (!IsWindowReady()) {
        bootLog << "window init failed\n";
        return 1;
    }

    InitAudioDevice();
    bootLog << "InitAudioDevice done\n";
    SetTargetFPS(TARGET_FPS);

    if (irpMode) {
        SetRandomSeed(1337U);
    }

    try {
        Jukebox::Instance().Init();
        bootLog << "Jukebox init\n";
        SFX::Instance().Init();
        bootLog << "SFX init\n";
        GameState::Instance().Init();
        bootLog << "GameState init\n";
        Particles::Instance().Init();
        bootLog << "Particles init\n";
        Renderer::Instance().Init();
        bootLog << "Renderer init\n";
        HUD::Instance().Init();
        bootLog << "HUD init\n";
        IRP::Init(irpMode);
        bootLog << "IRP init\n";
    } catch (const std::exception& ex) {
        bootLog << "init exception: " << ex.what() << "\n";
        return 1;
    }

    // 2. Main Loop
    while (!WindowShouldClose() && !(IRP::IsEnabled() && !IRP::IsRunning())) {
        float dt = GetFrameTime();
        if (IRP::IsEnabled()) {
            dt = 1.0f / (float)TARGET_FPS;
        }
        Input::BeginFrame();
        IRP::UpdatePre(dt);

        const bool irpFrozen = IRP::IsEnabled() && !IRP::IsRunning();

        // -- Update Phase --
        if (!irpFrozen) {
            GameState::Instance().Update(dt);
            Collision::Update();
            Jukebox::Instance().Update(dt);
            Particles::Instance().Update(dt);
        }
        
        // -- Render Phase --
        Renderer::Instance().BeginFrame();
        
        // Draw scene to internal gameRT
        Renderer::Instance().BeginGameRender();
            if (!IRP::HudOnlyMode()) {
                Renderer::Instance().DrawWell();
            }
            if (!IRP::GeometryOnlyMode() && !IRP::HudOnlyMode()) {
                Renderer::Instance().DrawEnemies();
                Renderer::Instance().DrawPlayer();
                Renderer::Instance().DrawBullets();
                Particles::Instance().Draw();
            }
        Renderer::Instance().EndGameRender();

        // Post-process bloom
        Renderer::Instance().ApplyBloom();

        // Final Blit + HUD
        BeginDrawing();
            ClearBackground(BLACK);
            Renderer::Instance().DrawFinalComposite();
            HUD::Instance().Draw();
            Jukebox::Instance().DrawWidget();
            IRP::DrawOverlay();
            if (IRP::IsEnabled()) {
                DrawFPS(SCREEN_W - 80, 10);
            }
        EndDrawing();

        IRP::UpdatePost();
    }

    // 3. Shutdown
    Renderer::Instance().Shutdown();
    Jukebox::Instance().Shutdown();
    SFX::Instance().Shutdown();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
