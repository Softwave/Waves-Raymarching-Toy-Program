// Raylib + ImGui Starter
// (c) Softwave 2026
// https://s0ftwave.net/

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "rlImGui.h"
#include "imgui.h"
#include "build_config.h"
#include <vector>
#include <cmath>
#include <random>
#include <string>
#include <fstream>
#include <sstream>
#include <ctime>
#include <chrono>
#include <iomanip>


constexpr const char* VERSION = APP_VERSION;
constexpr const char* PROJNAME = APP_PROJECT;
const char* TESTBUF = "One of my many little tools for making pretty pictures with code. \nCopyright (c) Softwave 2026 \nhttps://s0ftwave.net/";
bool open = true;
const float screenshotHintDuration = 6.0f;
// Setup info
std::string gameNameString = PROJNAME;
std::string versionString = VERSION;

//
float colMod1[3] = { 5.0f, 2.5f, 3.0f };

// --- ShaderToy wrapper ---
// Prepended to every .fs file so shaders only need to write mainImage().
static const char* SHADERTOY_PREAMBLE = R"glsl(
#version 330

uniform vec3      iResolution;
uniform float     iTime;
uniform float     iTimeDelta;
uniform float     iFrameRate;
uniform int       iFrame;
uniform float     iChannelTime[4];
uniform vec3      iChannelResolution[4];
uniform vec4      iMouse;
uniform sampler2D iChannel0;
uniform sampler2D iChannel1;
uniform sampler2D iChannel2;
uniform sampler2D iChannel3;

out vec4 fragColor;
)glsl";

static const char* SHADERTOY_POSTAMBLE = R"glsl(
void main() {
    mainImage(fragColor, gl_FragCoord.xy);
}
)glsl";

static void TakeScreenshot(Shader shader, int resLoc, int scale)
{
    int ssW = GetScreenWidth()  * scale;
    int ssH = GetScreenHeight() * scale;
    RenderTexture2D ssTarget = LoadRenderTexture(ssW, ssH);

    float ssResolution[3] = { (float)ssW, (float)ssH, 1.0f };
    SetShaderValue(shader, resLoc, ssResolution, SHADER_UNIFORM_VEC3);

    BeginTextureMode(ssTarget);
        ClearBackground(BLACK);
        BeginShaderMode(shader);
        DrawRectangle(0, 0, ssW, ssH, WHITE);
        EndShaderMode();
    EndTextureMode();

    // Restore screen resolution uniform for next frame.
    float screenRes[3] = { (float)GetScreenWidth(), (float)GetScreenHeight(), 1.0f };
    SetShaderValue(shader, resLoc, screenRes, SHADER_UNIFORM_VEC3);

    Image frame = LoadImageFromTexture(ssTarget.texture);
    ImageFlipVertical(&frame); // RenderTexture is stored upside-down
    std::time_t now = std::time(nullptr);
    std::tm* t = std::localtime(&now);
    char filename[64];
    std::strftime(filename, sizeof(filename), "screens/screenshot_%Y%m%d_%H%M%S.png", t);
    ExportImage(frame, filename);
    UnloadImage(frame);
    UnloadRenderTexture(ssTarget);
}

static std::string ReadFileToString(const char* path)
{
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// Load a ShaderToy-format fragment shader by wrapping it with the
// standard preamble (uniforms + output) and a void main() postamble.
static Shader LoadShaderToy(const char* path)
{
    std::string userCode = ReadFileToString(path);
    if (userCode.empty()) return Shader{ 0, nullptr };
    std::string fullCode = std::string(SHADERTOY_PREAMBLE) + userCode + SHADERTOY_POSTAMBLE;
    return LoadShaderFromMemory(nullptr, fullCode.c_str());
}


void SetupImGuiStyle()
{
    // 'Sage' theme
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.DisabledAlpha = 0.5f;
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.WindowMinSize = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(4.0f, 3.0f);
    style.FrameRounding = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.IndentSpacing = 21.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize = 14.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabMinSize = 10.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.TabBorderSize = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.FontSizeBase = 22;
    style.FontScaleMain = 0.500f;
    style.FontScaleDpi = 2.500f;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.55f, 0.55f, 0.52f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.93f, 0.93f, 0.93f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_Border]                 = ImVec4(0.72f, 0.72f, 0.72f, 0.65f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.82f, 0.82f, 0.80f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.80f, 0.85f, 0.79f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.77f, 0.83f, 0.76f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.72f, 0.72f, 0.72f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.60f, 0.64f, 0.64f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.82f, 0.82f, 0.80f, 0.80f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.85f, 0.85f, 0.83f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.88f, 0.88f, 0.87f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.72f, 0.72f, 0.72f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.60f, 0.62f, 0.56f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.42f, 0.42f, 0.35f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.42f, 0.42f, 0.35f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.55f, 0.58f, 0.50f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.42f, 0.42f, 0.35f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.77f, 0.83f, 0.76f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.68f, 0.75f, 0.67f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.55f, 0.62f, 0.54f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.77f, 0.83f, 0.76f, 0.60f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.72f, 0.79f, 0.71f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.65f, 0.73f, 0.64f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.72f, 0.72f, 0.72f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.55f, 0.58f, 0.50f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.42f, 0.42f, 0.35f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.72f, 0.72f, 0.72f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.55f, 0.58f, 0.50f, 0.60f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.42f, 0.42f, 0.35f, 0.90f);
    colors[ImGuiCol_InputTextCursor]        = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.77f, 0.83f, 0.76f, 1.00f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.82f, 0.82f, 0.80f, 1.00f);
    colors[ImGuiCol_TabSelected]            = ImVec4(0.93f, 0.93f, 0.93f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline]    = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TabDimmed]              = ImVec4(0.85f, 0.85f, 0.83f, 0.90f);
    colors[ImGuiCol_TabDimmedSelected]      = ImVec4(0.90f, 0.90f, 0.89f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline]  = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.42f, 0.42f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.65f, 0.55f, 0.30f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.55f, 0.62f, 0.54f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(0.65f, 0.55f, 0.30f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.82f, 0.85f, 0.81f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.72f, 0.72f, 0.72f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.82f, 0.82f, 0.80f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(0.00f, 0.00f, 0.00f, 0.03f);
    colors[ImGuiCol_TextLink]               = ImVec4(0.35f, 0.40f, 0.30f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.77f, 0.83f, 0.76f, 0.45f);
    colors[ImGuiCol_TreeLines]              = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(0.42f, 0.42f, 0.35f, 0.90f);
    colors[ImGuiCol_DragDropTargetBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_UnsavedMarker]          = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_NavCursor]              = ImVec4(0.50f, 0.55f, 0.46f, 0.80f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.50f, 0.50f, 0.50f, 0.15f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.40f, 0.40f, 0.38f, 0.35f);

}


int main(void)
{
    // Init Window
    const int screenWidth  = 1600;
    const int screenHeight = 900;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    //SetConfigFlags(FLAG_VSYNC_HINT); // This also sets the FPS to the same as display
    InitWindow(screenWidth, screenHeight, PROJNAME);
    SetTargetFPS(60); // Important, we can cap at 60 (or whatever)
    // This majorly lowers GPU load

    // Setup Raylib font
    Font dmFont = LoadFontEx("DM.ttf", 32, nullptr, 0);
    //SetTextureFilter(dmFont.texture, TEXTURE_FILTER_BILINEAR);

    double appStartTime = GetTime();

    // Basic ImGui setup
    rlImGuiSetup(true);
    SetupImGuiStyle();
    // ImGui Font
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig fontConfig;
    fontConfig.OversampleH = 4;
    fontConfig.OversampleV = 4;
    fontConfig.PixelSnapH  = true;
    ImFont* myFont = io.Fonts->AddFontFromFileTTF("DM.ttf", 11.0f, &fontConfig);
    io.FontDefault = myFont;
    ImFont* secondaryFont = io.Fonts->AddFontFromFileTTF("DM.ttf", 18.0f, &fontConfig);
    io.Fonts->Build();
    ImGui::PushFont(myFont);



    // Setup our shader (ShaderToy format)
    const char* shaderPath = "wave.fs";
    Shader waveShader      = LoadShaderToy(shaderPath);
    int timeLoc            = GetShaderLocation(waveShader, "iTime");
    int timeDeltaLoc       = GetShaderLocation(waveShader, "iTimeDelta");
    int frameRateLoc       = GetShaderLocation(waveShader, "iFrameRate");
    int resolutionLoc      = GetShaderLocation(waveShader, "iResolution");
    int mouseLoc           = GetShaderLocation(waveShader, "iMouse");
    int frameLoc           = GetShaderLocation(waveShader, "iFrame");
    int dThingLoc          = GetShaderLocation(waveShader, "dThing");
    int mod1Loc            = GetShaderLocation(waveShader, "mod1");
    int mod2Loc            = GetShaderLocation(waveShader, "mod2");
    int mod3Loc            = GetShaderLocation(waveShader, "mod3");
    int mod4Loc            = GetShaderLocation(waveShader, "mod4");
    int col1Loc            = GetShaderLocation(waveShader, "colourMod1");
    long shaderModTime     = GetFileModTime(shaderPath);

    // Our state
    float shaderTime   = 0.0f;
    float timeSpeed    = 1.0f;
    int   frameCount   = 0;
    float lastDt       = 0.0f;
    int   screenshotScale = 2; // multiplier over screen resolution
    bool  showUI = true;
    // wave.fs uniforms
    float dThing = 2.5f;
    float wMod1  = 2.0f;
    float wMod2  = 4.0f;
    float wMod3  = 2.5f;
    float wMod4  = 0.5f;
    float colMod1[3] = { 5.0f, 2.5f, 3.0f };
    // iMouse: xy = current pos (while held), zw = last click pos
    float mouseClickX  = 0.0f, mouseClickY = 0.0f;

    // Main loop
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        lastDt = dt;
        shaderTime += dt * timeSpeed;
        frameCount++;

        // Live shader reload
        long currentModTime = GetFileModTime(shaderPath);
        if (currentModTime != shaderModTime)
        {
            Shader newShader = LoadShaderToy(shaderPath);
            if (newShader.id > 0)
            {
                UnloadShader(waveShader);
                waveShader    = newShader;
                timeLoc       = GetShaderLocation(waveShader, "iTime");
                timeDeltaLoc  = GetShaderLocation(waveShader, "iTimeDelta");
                frameRateLoc  = GetShaderLocation(waveShader, "iFrameRate");
                resolutionLoc = GetShaderLocation(waveShader, "iResolution");
                mouseLoc      = GetShaderLocation(waveShader, "iMouse");
                frameLoc      = GetShaderLocation(waveShader, "iFrame");
                dThingLoc     = GetShaderLocation(waveShader, "dThing");
                mod1Loc       = GetShaderLocation(waveShader, "mod1");
                mod2Loc       = GetShaderLocation(waveShader, "mod2");
                mod3Loc       = GetShaderLocation(waveShader, "mod3");
                mod4Loc       = GetShaderLocation(waveShader, "mod4");
                col1Loc       = GetShaderLocation(waveShader, "colourMod1");
            }
            shaderModTime = currentModTime;
        }
        // Screenshot stuff
        bool ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        bool screenshotShortcutPressed = ctrlDown && shiftDown && IsKeyPressed(KEY_P);
        bool shouldSaveScreenshot = screenshotShortcutPressed && !ImGui::GetIO().WantCaptureKeyboard;
        if (IsKeyPressed(KEY_F1) && !ImGui::GetIO().WantCaptureKeyboard)
            showUI = !showUI;

        // iMouse: xy = current pos while button held, zw = last click pos
        float mx = (float)GetMouseX();
        float my = (float)GetScreenHeight() - (float)GetMouseY(); // flip Y to match ShaderToy
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { mouseClickX = mx; mouseClickY = my; }
        float mouseBtnSign = IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? 1.0f : -1.0f;
        float resolution[3] = { (float)GetScreenWidth(), (float)GetScreenHeight(), 1.0f };
        float mouseUniform[4] = {
            IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? mx : 0.0f,
            IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? my : 0.0f,
            mouseBtnSign * mouseClickX,
            mouseBtnSign * mouseClickY
        };
        float fps = lastDt > 0.0f ? 1.0f / lastDt : 0.0f;
        SetShaderValue(waveShader, resolutionLoc, resolution,    SHADER_UNIFORM_VEC3);
        SetShaderValue(waveShader, timeLoc,       &shaderTime,   SHADER_UNIFORM_FLOAT);
        SetShaderValue(waveShader, timeDeltaLoc,  &lastDt,       SHADER_UNIFORM_FLOAT);
        SetShaderValue(waveShader, frameRateLoc,  &fps,          SHADER_UNIFORM_FLOAT);
        SetShaderValue(waveShader, mouseLoc,      mouseUniform,  SHADER_UNIFORM_VEC4);
        SetShaderValue(waveShader, frameLoc,      &frameCount,   SHADER_UNIFORM_INT);
        SetShaderValue(waveShader, dThingLoc,     &dThing,       SHADER_UNIFORM_FLOAT);
        SetShaderValue(waveShader, mod1Loc,       &wMod1,        SHADER_UNIFORM_FLOAT);
        SetShaderValue(waveShader, mod2Loc,       &wMod2,        SHADER_UNIFORM_FLOAT);
        SetShaderValue(waveShader, mod3Loc,       &wMod3,        SHADER_UNIFORM_FLOAT);
        SetShaderValue(waveShader, mod4Loc,       &wMod4,        SHADER_UNIFORM_FLOAT);
        SetShaderValue(waveShader, col1Loc,       &colMod1,      SHADER_UNIFORM_VEC3);

        BeginDrawing();
        //
        // Draw things
        BeginShaderMode(waveShader);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
        EndShaderMode();




        // Screenshot stuff
        float hintElapsed = (float)(GetTime() - appStartTime);
        if (hintElapsed < screenshotHintDuration)
        {
            float hintAlpha = 1.0f;
            if (hintElapsed > screenshotHintDuration - 1.5f)
            {
                hintAlpha = (screenshotHintDuration - hintElapsed) / 1.5f;
            }
            // Draw some text with our font
            //DrawTextEx(dmFont, labelText.c_str(), { 40.0f, 40.0f }, 32.0f, 2.0f, Fade(WHITE, hintAlpha));
            //DrawTextEx(dmFont, "Press CTRL+SHIFT+P (or press button) to take a screenshot\nNot necessary to toggle UI (F1) to save a screenshot", {42, 82}, 24, 2, Fade(WHITE, hintAlpha));
        }

        if (shouldSaveScreenshot)
        {
            TakeScreenshot(waveShader, resolutionLoc, screenshotScale);
        }

        // Draw the actual ImGui control window
        rlImGuiBegin();
        if (showUI)
        {
        ImGui::SetNextWindowSize(ImVec2(432, 400), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(91, 165), ImGuiCond_Once);
        ImGui::Begin("Controls");
        
        ImGui::PushFont(secondaryFont);
        ImGui::Text("%s v. %s", gameNameString.c_str(), versionString.c_str());
        ImGui::PopFont();
        if (ImGui::Button("Reset Shader"))
        {
            shaderTime = 0.0f;
            frameCount = 0;
            // Set the shader uniforms to their default values as well
            shaderTime   = 0.0f;
            timeSpeed    = 1.0f;
            frameCount   = 0;
            lastDt       = 0.0f;
            screenshotScale = 2; // multiplier over screen resolution

            dThing = 2.5f;
            wMod1  = 2.0f;
            wMod2  = 4.0f;
            wMod3  = 2.5f;
            wMod4  = 0.5f;
            colMod1[0] = 5.0f; colMod1[1] = 2.5f; colMod1[2] = 3.0f;
        }
        ImGui::SameLine();
        ImGui::Button("Take Screenshot");
        if (ImGui::IsItemClicked())
        {
            TakeScreenshot(waveShader, resolutionLoc, screenshotScale);
        }
        ImGui::Separator();
        ImGui::SliderFloat("Time Speed", &timeSpeed, 0.0f, 5.0f);
        ImGui::Separator();
        ImGui::SliderInt("Screenshot Scale", &screenshotScale, 1, 4);
        ImGui::Separator();
        ImGui::Text("Shader Uniforms");
        ImGui::SliderFloat("dThing",  &dThing, 0.0f, 10.0f);
        ImGui::SliderFloat("mod1",    &wMod1,  0.0f, 10.0f);
        ImGui::SliderFloat("mod2",    &wMod2,  0.0f, 10.0f);
        ImGui::SliderFloat("mod3",    &wMod3,  0.0f, 10.0f);
        ImGui::SliderFloat("mod4",    &wMod4,  0.0f, 2.0f);
        ImGui::SliderFloat3("Colour Mod RGB", colMod1, 0.0f, 10.0f);
        ImGui::Text("FPS: %d", GetFPS());

        static char settingsFile[128] = "wave_settings.ini";
        if (ImGui::Button("Save Settings"))
        {
            FILE* f = fopen(settingsFile, "w");
            if (f)
            {
                fprintf(f,"%f %d %f %f %f %f %f %f \n", timeSpeed, screenshotScale, dThing, wMod1, wMod2, colMod1[0], colMod1[1], colMod1[2]);
                fclose(f);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Settings"))
        {
            FILE* f = fopen(settingsFile, "r");
            if (f)
            {
                char settingsLine[512] = {0};
                if (fgets(settingsLine, sizeof(settingsLine), f))
                {
                    // load
                    int loaded = sscanf(settingsLine, "%f %d %f %f %f %f %f %f", &timeSpeed, &screenshotScale, &dThing, &wMod1, &wMod2, &colMod1[0], &colMod1[1], &colMod1[2]);
                    fclose(f);
                }
            }
            



        }

        // Settings file
        ImGui::InputText("Settings File", settingsFile, sizeof(settingsFile));

        ImGui::Separator();

        static bool showAbout = false;
        if (ImGui::Button("About")) showAbout = true;
        if (showAbout)
        {
            ImGui::OpenPopup("About");
        }
        if (ImGui::BeginPopupModal("About", &showAbout, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("%s %s\n© Softwave 2026\n", PROJNAME, VERSION);
            ImGui::TextLinkOpenURL("https://s0ftwave.net/");
            ImGui::Separator();
            ImGui::Text("'Waves' is a raymarched graphical toy that features an undulating, \nrippling, and ever amorphous 3D shape. It can be very nice to look \nat, when you play around with it sufficiently. It is particularly \ngood for making abstract art. I've used it often to make desktop \nwallpapers.\n\n");
            ImGui::Text("I hope you like the program!\n\nIt is made with C++, GLSL, Raylib, and ImGui.\n");
            ImGui::Separator();
            ImGui::Text("Check out my YouTube channel for more stuff!\nAnd do consider tipping me on Ko-fi if you like what I do :)\nI give away a lot of free software.\n\n");
            ImGui::TextLinkOpenURL("https://www.youtube.com/@softwave1662");
            ImGui::TextLinkOpenURL("https://ko-fi.com/s0ftwave");
            ImGui::TextLinkOpenURL("https://buymeacoffee.com/s0ftwave92");
            ImGui::TextLinkOpenURL("https://softwave.itch.io/");
            if (ImGui::Button("OK"))
            {
                showAbout = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        ImGui::Button("Quit");
        if (ImGui::IsItemClicked())
        {
            goto endLabel; // I felt cheeky and like using a goto, ha
        }

        ImGui::End();
        } // showUI

        rlImGuiEnd();
        //
        EndDrawing();
    }

endLabel:
    // Cleaning stuff
    UnloadShader(waveShader);
    UnloadFont(dmFont);
    rlImGuiShutdown();
    CloseWindow();
    return 0;
}
