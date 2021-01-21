#include "DebugUI.h"

void DebugUI::Init(SDL_Window* window, SDL_Renderer* renderer)
{
    terminal_log.set_min_log_level(ImTerm::message::severity::info);
    Logger::GetClientLogger()->sinks().push_back(terminal_log.get_terminal_helper());
    ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForSDL(window);
	ImGuiSDL::Initialize(renderer, 800, 600);
    chip8_ram_editor.Cols = 32;
    chip8_vram_editor.Cols = 64;

	return;
}


void DebugUI::Deinit()
{
	ImGuiSDL::Deinitialize();
	ImGui::DestroyContext();
}

static void HelpMarker(const char* desc)
{
    //ImGui::TextDisabled("(?)   ");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void DebugUI::Draw(SDL_Window* window, SDL_Renderer* renderer, SDL_Texture* tex, Chip8* core, unsigned int res_w, unsigned int res_h, unsigned int* speed, SDL_Color (&colors)[16], bool* debug_grid_lines, float* volume)
{
	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();

    if (open_file && open_file->ready())
    {
        auto result = open_file->result();
        if (result.size())
        {
            last_file = result[0];
            core->Reset();
            core->Load(last_file);
        }
        open_file = nullptr;
    }

	//ImGui::ShowDemoWindow();

    if (show_menu_bar)
        ShowMenuBar(core, speed, &m_Zoom, colors, debug_grid_lines, volume);

    if (show_display)
        ShowDisplayWindow(&show_display, tex, m_Zoom, res_w, res_h);

    if (show_vram)
        ShowVRAMWindow(&show_vram, core, res_w, res_h);

    if (show_ram)
        ShowRAMWindow(&show_ram, core, res_w, res_h);

    if(show_regs)
        ShowCPUEditor(&show_regs, core);

    if (show_stack)
        ShowStackWindow(&show_stack, core);
    if(show_log)
        show_log = terminal_log.show();

	SDL_Rect windowRect = { 0, 0, 1, 1 };
	SDL_RenderSetClipRect(renderer, &windowRect); //fixes an SDL bug for D3D backend
	SDL_SetRenderDrawColor(renderer, 114, 144, 154, 255);
	SDL_RenderClear(renderer);

	ImGui::Render();
	ImGuiSDL::Render(ImGui::GetDrawData());
}

void DebugUI::HandleInput(SDL_Event* event)
{
	ImGui_ImplSDL2_ProcessEvent(event);
}

bool DebugUI::WantCaptureKB()
{
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	captureKB = io.WantCaptureKeyboard;
	return captureKB;
}

bool DebugUI::WantCaptureMouse()
{
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	captureMouse = io.WantCaptureMouse;
	return captureMouse;
}

void DebugUI::ShowRegisterObject(const char* prefix, int uid, Chip8* core)
{
    // Use object uid as identifier. Most commonly you could also use the object pointer as a base ID.
    ImGui::PushID(uid);

    // Text and Tree nodes are less high than framed widgets, using AlignTextToFramePadding() we add vertical spacing to make the tree lines equal high.
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Registers");
    ImGui::Separator();
    ImGui::Columns(4);
    //static float placeholder_members[8] = { 0.0f, 0.0f, 1.0f, 3.1416f, 100.0f, 999.0f };
    for (int i = 0; i < 16; i++)
    {
        ImGui::PushID(i); // Use field index as identifier.
        // Here we use a TreeNode to highlight on hover (we could use e.g. Selectable as well)
        ImGui::AlignTextToFramePadding();
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet;
        //ImGui::TreeNodeEx("Reg", flags, "V%X", i);
        ImGui::Text("V%X:", i);
        ImGui::SameLine();
        //ImGui::NextColumn();
        ImGui::SetNextItemWidth(30);
        ImGui::InputScalar("##value", ImGuiDataType_U8, core->GetRegV(i), NULL, NULL, "%02X");
        ImGui::NextColumn();
        ImGui::PopID();
    }
    ImGui::PopID();
}

// Demonstrate create a simple property editor.
void DebugUI::ShowCPUEditor(bool* p_open, Chip8* core)
{
    ImGui::SetNextWindowSize(ImVec2(440, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("CPU:", p_open))
    {
        ImGui::End();
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
    ImGui::Columns(1);
    ImGui::Separator();

    // Iterate placeholder objects (all the same data)
    ShowRegisterObject("Regs", 0, core);

    ImGui::Columns(1);
    ImGui::Separator();
    ImGui::PopStyleVar();

    ImGui::PushID(16); // Use field index as identifier.
    // Here we use a TreeNode to highlight on hover (we could use e.g. Selectable as well)
    ImGui::AlignTextToFramePadding();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet;
    //ImGui::TreeNodeEx("Reg", flags, "I");
    ImGui::TextUnformatted("I:  ");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(40);
    ImGui::InputScalar("##value", ImGuiDataType_U16, core->GetRegI(), NULL, NULL, "%04X");
    ImGui::NextColumn();
    ImGui::PopID();
    ImGui::PushID(17);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("PC: ");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(40);
    ImGui::InputScalar("##value", ImGuiDataType_U16, core->GetPC(), NULL, NULL, "%04X");
    ImGui::NextColumn();
    ImGui::PopID();
    ImGui::Columns(1);
    ImGui::Separator();
    if (ImGui::Button("Step"))
    {
        if (!core->GetDebugStepping())
            core->ToggleDebugStepping();
        core->Step();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
    {
        core->Reset();
    }
    ImGui::SameLine();
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !core->GetDebugStepping());
    if (ImGui::Button("Run"))
    {
        core->ToggleDebugStepping();
    }
    ImGui::PopItemFlag();
    ImGui::End();
}


void DebugUI::ShowDisplayWindow(bool* p_open, SDL_Texture* tex, unsigned int zoom, unsigned int res_w, unsigned int res_h)
{
    //ImGui::ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::SetNextWindowSize(ImVec2((res_w * zoom) + res_w + 1, (res_h * zoom) + res_h + 1), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Display", p_open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize))
    {
        ImGui::End();
        return;
    }
    ImGui::Image(tex, ImVec2((res_w * zoom) + res_w + 1, (res_h * zoom) + res_h + 1));
    ImGui::End();

}

void DebugUI::ShowRAMWindow(bool* p_open, Chip8* core, unsigned int res_w, unsigned int res_h)
{
    ImGui::SetNextWindowSize(ImVec2(1000, 650), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("RAM", p_open))
    {
        ImGui::End();
        return;
    }
    chip8_ram_editor.DrawContents(core->GetRAM(), sizeof(uint8_t) * 0x10000, 0); //TODO: don't hard code ram size
    ImGui::End();
}

void DebugUI::ShowVRAMWindow(bool* p_open, Chip8* core, unsigned int res_w, unsigned int res_h)
{
    ImGui::SetNextWindowSize(ImVec2(1385, 490), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("VRAM", p_open))
    {
        ImGui::End();
        return;
    }
    chip8_vram_editor.DrawContents(core->GetVRAM(), sizeof(uint8_t) * res_w * res_h, 0);
    ImGui::End();
}

void DebugUI::ShowStackWindow(bool* p_open, Chip8* core)
{
    ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Stack", p_open))
    {
        ImGui::End();
        return;
    }
    ImGui::TextUnformatted("SP: ");
    ImGui::SameLine();
    ImGui::InputScalar("##value", ImGuiDataType_S8, (core->GetSP()), NULL, NULL, "%04d");
    ImGui::Separator();

    ImGui::PushID(-1); // Use field index as identifier.
        // Here we use a TreeNode to highlight on hover (we could use e.g. Selectable as well)
    ImGui::AlignTextToFramePadding();
    if (*core->GetSP() == -1)
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.1f, 1.0f), "Empty");
    else
        ImGui::TextDisabled("-----");
    ImGui::PopID();
    ImGui::Columns(2);
    for (int i = 0; i < core->GetStackSize() / 2; i++)
    {
        ImGui::PushID(i); // Use field index as identifier.
        // Here we use a TreeNode to highlight on hover (we could use e.g. Selectable as well)
        ImGui::AlignTextToFramePadding();
        if(i == *core->GetSP())
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.1f, 1.0f), "%02u:", i);
        else
            ImGui::Text("%02u:", i);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::InputScalar("##value", ImGuiDataType_U16, (core->GetStack()+i), NULL, NULL, "%04X");
        ImGui::PopID();
    }
    ImGui::NextColumn();
    for (int i = core->GetStackSize() / 2; i < core->GetStackSize(); i++)
    {
        ImGui::PushID(i); // Use field index as identifier.
        // Here we use a TreeNode to highlight on hover (we could use e.g. Selectable as well)
        ImGui::AlignTextToFramePadding();
        if (i == *core->GetSP())
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.1f, 1.0f), "%02u:", i);
        else
            ImGui::Text("%02u:", i);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::InputScalar("##value", ImGuiDataType_U16, (core->GetStack() + i), NULL, NULL, "%04X");
        ImGui::PopID();
    }
    ImGui::End();
}

void DebugUI::ShowMenuBar(Chip8* core, unsigned int* speed, unsigned int* zoom, SDL_Color (&colors)[16], bool* debug_grid_lines, float* volume)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ShowMenuFile(core);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Windows"))
        {
            ShowMenuWindows();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Emulation"))
        {
            ShowMenuEmulation(core, speed);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options"))
        {
            ShowMenuOptions(core, zoom, colors, debug_grid_lines, volume);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void DebugUI::ShowMenuFile(Chip8* core)
{
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, (bool)open_file);
    if (ImGui::MenuItem("Load", ""))
    {
        open_file = std::make_shared<pfd::open_file>("Choose file", "C:\\");
    }
    ImGui::PopItemFlag();
    if (ImGui::MenuItem("Reload", ""))
    {
        core->Reset();
        if (last_file != "")
        {
            core->Load(last_file);
        }
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Quit", ""))
    {
        *keep_running = false;
    }
}

void DebugUI::ShowMenuEmulation(Chip8* core, unsigned int* speed)
{


    const ImU32 u32_one = 1;
    ImGui::TextUnformatted("CPU Cycles:");
    ImGui::SameLine();
    ImGui::InputScalar("", ImGuiDataType_U32, speed, &u32_one, NULL, "%u");

    if (ImGui::BeginMenu("System Mode"))
    {
        if (ImGui::MenuItem("COSMAC VIP (CHIP-8)", NULL, core->GetSystemMode() == Chip8::SYSTEM_MODE::CHIP_8 ? true : false))
        {
            if (core->GetSystemMode() != Chip8::SYSTEM_MODE::CHIP_8)
                m_Zoom = m_Zoom * 2;
            core->SetSystemMode(Chip8::SYSTEM_MODE::CHIP_8);
        }
        HelpMarker("Original COSMAC VIP CHIP-8 interpreter.");
        
        if (ImGui::MenuItem("DREAM 6800", NULL, false, false)) {}
        
        if (ImGui::MenuItem("ETI-660", NULL, false, false)) {}
        
        if (ImGui::MenuItem("HP-48 (SUPER-CHIP)", NULL, core->GetSystemMode() == Chip8::SYSTEM_MODE::SUPER_CHIP ? true : false))
        {
            if(core->GetSystemMode() == Chip8::SYSTEM_MODE::CHIP_8)
                m_Zoom = m_Zoom / 2;
            core->SetSystemMode(Chip8::SYSTEM_MODE::SUPER_CHIP);
        }
        HelpMarker("SUPER-CHIP 1.1 for HP-48 series calculators.");

        if (ImGui::MenuItem("Octo (XO-CHIP)", NULL, core->GetSystemMode() == Chip8::SYSTEM_MODE::XO_CHIP ? true : false))
        {
            if (core->GetSystemMode() == Chip8::SYSTEM_MODE::CHIP_8)
                m_Zoom = m_Zoom / 2;
            core->SetSystemMode(Chip8::SYSTEM_MODE::XO_CHIP);
        }
        
        HelpMarker("Octo IDE XO-CHIP compatibility.");
        
        //if (ImGui::MenuItem("XO-Chip", NULL, core->GetSystemMode() == Chip8::SYSTEM_MODE::XO_CHIP ? true : false)) { core->SetSystemMode(Chip8::SYSTEM_MODE::XO_CHIP); }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Quirks"))
    {
        ImGui::MenuItem("VIP Jumps (NNN+V0)", NULL, &core->quirks.vip_jump );
        ImGui::MenuItem("Copy VY to VX Before Bit Shifts", NULL, &core->quirks.vip_shifts );
        ImGui::MenuItem("Register Store/Load Increments I", NULL, &core->quirks.vip_regs_read_write );
        ImGui::MenuItem("Logic Ops Reset VF", NULL, &core->quirks.logic_flag_reset);
        ImGui::MenuItem("Draw Ops Wrap", NULL, &core->quirks.draw_wrap );
        ImGui::MenuItem("Draw Ops Wait for Vblank", NULL, &core->quirks.draw_vblank );
        ImGui::MenuItem("S-CHIP 1.0 Large Fonts", NULL, &core->quirks.schip_10_fonts);
        ImGui::EndMenu();
    }

}

void DebugUI::ShowMenuWindows()
{
    ImGui::MenuItem("Display", NULL, &show_display);
    ImGui::MenuItem("CPU", NULL, &show_regs);
    ImGui::MenuItem("Stack", NULL, &show_stack);
    ImGui::MenuItem("RAM", NULL, &show_ram);
    ImGui::MenuItem("VRAM", NULL, &show_vram);
    ImGui::MenuItem("Log", NULL, &show_log);
}

void DebugUI::ShowMenuOptions(Chip8* core, unsigned int* zoom, SDL_Color (&colors)[16], bool* debug_grid_lines, float* volume)
{
    if (ImGui::BeginMenu("Display"))
    {
        if (ImGui::MenuItem("Draw Pixel Grid", "", debug_grid_lines, true))
        {
            /*std::fill_n(core->GetPrevVRAM(), 128 * 64, 0);
            core->SetScreenDirty();
            core->SetWipeScreen();*/
            grid_toggled = true;
        }

        const ImU32 u32_one = 1;
        ImGui::TextUnformatted("Zoom:");
        ImGui::SameLine();
        ImGui::InputScalar("", ImGuiDataType_U32, zoom, &u32_one, NULL, "%u");
        if (*zoom < 1) { *zoom = 1; }

        if (ImGui::BeginMenu("Colors"))
        {
            ImGui::Text("Display Colors:");
            static bool alpha_preview = true;
            static bool alpha_half_preview = false;
            static bool drag_and_drop = true;
            static bool options_menu = true;
            static bool hdr = false;
            static ImVec4 background_color   = ImVec4((float)colors[0].r / 255.0f, (float)colors[0].g / 255.0f, (float)colors[0].b / 255.0f, 255.0f / 255.0f);
            static ImVec4 foreground_color_1 = ImVec4((float)colors[1].r / 255.0f, (float)colors[1].g / 255.0f, (float)colors[1].b / 255.0f, 255.0f / 255.0f);
            static ImVec4 foreground_color_2 = ImVec4((float)colors[2].r / 255.0f, (float)colors[2].g / 255.0f, (float)colors[2].b / 255.0f, 255.0f / 255.0f);
            static ImVec4 overlap_color      = ImVec4((float)colors[3].r / 255.0f, (float)colors[3].g / 255.0f, (float)colors[3].b / 255.0f, 255.0f / 255.0f);
            static ImVec4 backup_color;
            ImGuiColorEditFlags misc_flags = (hdr ? ImGuiColorEditFlags_HDR : 0) | (drag_and_drop ? 0 : ImGuiColorEditFlags_NoDragDrop) | (alpha_half_preview ? ImGuiColorEditFlags_AlphaPreviewHalf : (alpha_preview ? ImGuiColorEditFlags_AlphaPreview : 0)) | (options_menu ? 0 : ImGuiColorEditFlags_NoOptions);
            // Generate a default palette. The palette will persist and can be edited.
            static bool saved_palette_init = true;
            static ImVec4 saved_palette[32] = {};
            if (saved_palette_init)
            {
                for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
                {
                    ImGui::ColorConvertHSVtoRGB(n / 31.0f, 0.8f, 0.8f,
                        saved_palette[n].x, saved_palette[n].y, saved_palette[n].z);
                    saved_palette[n].w = 1.0f; // Alpha
                }
                saved_palette_init = false;
            }

            bool open_popup_bg = ImGui::ColorButton("Background##3b", background_color, misc_flags);
            ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
            //open_popup_bg |= ImGui::Button("Background");
            if (open_popup_bg)
            {
                ImGui::OpenPopup("mypicker_bg");
                backup_color = background_color;
            }
            if (ImGui::BeginPopup("mypicker_bg"))
            {
                ImGui::Text("Background Color");
                ImGui::Separator();
                ImGui::ColorPicker4("##picker", (float*)&background_color, misc_flags | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
                ImGui::SameLine();

                ImGui::BeginGroup(); // Lock X position
                ImGui::Text("Current");
                ImGui::ColorButton("##current", background_color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40));
                ImGui::Text("Previous");
                if (ImGui::ColorButton("##previous", backup_color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40)))
                    background_color = backup_color;
                ImGui::Separator();
                ImGui::Text("Palette");
                for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
                {
                    ImGui::PushID(n);
                    if ((n % 8) != 0)
                        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.y);

                    ImGuiColorEditFlags palette_button_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
                    if (ImGui::ColorButton("##palette", saved_palette[n], palette_button_flags, ImVec2(20, 20)))
                        background_color = ImVec4(saved_palette[n].x, saved_palette[n].y, saved_palette[n].z, background_color.w); // Preserve alpha!

                    // Allow user to drop colors into each palette entry. Note that ColorButton() is already a
                    // drag source by default, unless specifying the ImGuiColorEditFlags_NoDragDrop flag.
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
                            memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 3);
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
                            memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 4);
                        ImGui::EndDragDropTarget();
                    }

                    ImGui::PopID();
                }
                ImGui::EndGroup();
                ImGui::EndPopup();

                colors[0].r = (Uint8)(background_color.x * 255.0);
                colors[0].g = (Uint8)(background_color.y * 255.0);
                colors[0].b = (Uint8)(background_color.z * 255.0);
                colors[0].a = (Uint8)(background_color.w * 255.0);

                std::fill_n(core->GetPrevVRAM(), 128 * 64, 0);
                core->SetScreenDirty();
                core->SetWipeScreen();
            }

            bool open_popup_fg_1 = ImGui::ColorButton("Foreground 1##3b", foreground_color_1, misc_flags);
            ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
            //open_popup_fg_1 |= ImGui::Button("Foreground 1");
            if (open_popup_fg_1)
            {
                ImGui::OpenPopup("mypicker_fg1");
                backup_color = foreground_color_1;
            }
            if (ImGui::BeginPopup("mypicker_fg1"))
            {
                ImGui::Text("Foreground 1 Color");
                ImGui::Separator();
                ImGui::ColorPicker4("##picker", (float*)&foreground_color_1, misc_flags | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
                ImGui::SameLine();

                ImGui::BeginGroup(); // Lock X position
                ImGui::Text("Current");
                ImGui::ColorButton("##current", foreground_color_1, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40));
                ImGui::Text("Previous");
                if (ImGui::ColorButton("##previous", backup_color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40)))
                    foreground_color_1 = backup_color;
                ImGui::Separator();
                ImGui::Text("Palette");
                for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
                {
                    ImGui::PushID(n);
                    if ((n % 8) != 0)
                        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.y);

                    ImGuiColorEditFlags palette_button_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
                    if (ImGui::ColorButton("##palette", saved_palette[n], palette_button_flags, ImVec2(20, 20)))
                        foreground_color_1 = ImVec4(saved_palette[n].x, saved_palette[n].y, saved_palette[n].z, foreground_color_1.w); // Preserve alpha!

                    // Allow user to drop colors into each palette entry. Note that ColorButton() is already a
                    // drag source by default, unless specifying the ImGuiColorEditFlags_NoDragDrop flag.
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
                            memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 3);
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
                            memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 4);
                        ImGui::EndDragDropTarget();
                    }

                    ImGui::PopID();
                }
                ImGui::EndGroup();
                ImGui::EndPopup();

                colors[1].r = (Uint8)(foreground_color_1.x * 255.0);
                colors[1].g = (Uint8)(foreground_color_1.y * 255.0);
                colors[1].b = (Uint8)(foreground_color_1.z * 255.0);
                colors[1].a = (Uint8)(foreground_color_1.w * 255.0);

                std::fill_n(core->GetPrevVRAM(), 128 * 64, 0);
                core->SetScreenDirty();
                core->SetWipeScreen();

            }

            bool open_popup_fg_2 = ImGui::ColorButton("Foreground 2##3b", foreground_color_2, misc_flags);
            ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
            //open_popup_fg_2 |= ImGui::Button("Foreground 2");
            if (open_popup_fg_2)
            {
                ImGui::OpenPopup("mypicker_fg2");
                backup_color = foreground_color_2;
            }
            if (ImGui::BeginPopup("mypicker_fg2"))
            {
                ImGui::Text("Foreground 2 Color");
                ImGui::Separator();
                ImGui::ColorPicker4("##picker", (float*)&foreground_color_2, misc_flags | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
                ImGui::SameLine();

                ImGui::BeginGroup(); // Lock X position
                ImGui::Text("Current");
                ImGui::ColorButton("##current", foreground_color_2, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40));
                ImGui::Text("Previous");
                if (ImGui::ColorButton("##previous", backup_color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40)))
                    foreground_color_2 = backup_color;
                ImGui::Separator();
                ImGui::Text("Palette");
                for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
                {
                    ImGui::PushID(n);
                    if ((n % 8) != 0)
                        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.y);

                    ImGuiColorEditFlags palette_button_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
                    if (ImGui::ColorButton("##palette", saved_palette[n], palette_button_flags, ImVec2(20, 20)))
                        foreground_color_2 = ImVec4(saved_palette[n].x, saved_palette[n].y, saved_palette[n].z, foreground_color_2.w); // Preserve alpha!

                    // Allow user to drop colors into each palette entry. Note that ColorButton() is already a
                    // drag source by default, unless specifying the ImGuiColorEditFlags_NoDragDrop flag.
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
                            memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 3);
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
                            memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 4);
                        ImGui::EndDragDropTarget();
                    }

                    ImGui::PopID();
                }
                ImGui::EndGroup();
                ImGui::EndPopup();

                colors[2].r = (Uint8)(foreground_color_2.x * 255.0);
                colors[2].g = (Uint8)(foreground_color_2.y * 255.0);
                colors[2].b = (Uint8)(foreground_color_2.z * 255.0);
                colors[2].a = (Uint8)(foreground_color_2.w * 255.0);

                std::fill_n(core->GetPrevVRAM(), 128 * 64, 0);
                core->SetScreenDirty();
                core->SetWipeScreen();

            }

            bool open_popup_ol = ImGui::ColorButton("Overlap##3b", overlap_color, misc_flags);
            //ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
            //open_popup_ol |= ImGui::Button("Overlap");
            if (open_popup_ol)
            {
                ImGui::OpenPopup("mypicker_ol");
                backup_color = overlap_color;
            }
            if (ImGui::BeginPopup("mypicker_ol"))
            {
                ImGui::Text("Overlap Color");
                ImGui::Separator();
                ImGui::ColorPicker4("##picker", (float*)&overlap_color, misc_flags | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
                ImGui::SameLine();

                ImGui::BeginGroup(); // Lock X position
                ImGui::Text("Current");
                ImGui::ColorButton("##current", overlap_color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40));
                ImGui::Text("Previous");
                if (ImGui::ColorButton("##previous", backup_color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40)))
                    overlap_color = backup_color;
                ImGui::Separator();
                ImGui::Text("Palette");
                for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
                {
                    ImGui::PushID(n);
                    if ((n % 8) != 0)
                        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.y);

                    ImGuiColorEditFlags palette_button_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
                    if (ImGui::ColorButton("##palette", saved_palette[n], palette_button_flags, ImVec2(20, 20)))
                        overlap_color = ImVec4(saved_palette[n].x, saved_palette[n].y, saved_palette[n].z, overlap_color.w); // Preserve alpha!

                    // Allow user to drop colors into each palette entry. Note that ColorButton() is already a
                    // drag source by default, unless specifying the ImGuiColorEditFlags_NoDragDrop flag.
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
                            memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 3);
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
                            memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 4);
                        ImGui::EndDragDropTarget();
                    }

                    ImGui::PopID();
                }
                ImGui::EndGroup();
                ImGui::EndPopup();

                colors[3].r = (Uint8)(overlap_color.x * 255.0);
                colors[3].g = (Uint8)(overlap_color.y * 255.0);
                colors[3].b = (Uint8)(overlap_color.z * 255.0);
                colors[3].a = (Uint8)(overlap_color.w * 255.0);

                std::fill_n(core->GetPrevVRAM(), 128 * 64, 0);
                core->SetScreenDirty();
                core->SetWipeScreen();

            }

            static ImVec4 xeno_chip_colors[12];
            static bool popups[12];
            static std::string popup_id;
            static std::string popup_label;
            for (int it = 0; it < 12; it++)
            {
                xeno_chip_colors[it] = ImVec4((float)colors[it + 4].r / 255.0f, (float)colors[it + 4].g / 255.0f, (float)colors[it + 4].b / 255.0f, 255.0f / 255.0f);
                popup_id = "popup" + std::to_string(it+4);
                popup_label = "Color " + std::to_string(it+4);

                popups[it] = ImGui::ColorButton(popup_label.c_str(), xeno_chip_colors[it], misc_flags);
                if((it % 4) != 3)
                    ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
                //open_popup_ol |= ImGui::Button("Overlap");
                if (popups[it])
                {
                    ImGui::OpenPopup(popup_id.c_str());
                    backup_color = xeno_chip_colors[it];
                }
                if (ImGui::BeginPopup(popup_id.c_str()))
                {
                    ImGui::Text("Color %d", it + 4);
                    ImGui::Separator();
                    ImGui::ColorPicker4("##picker", (float*)&xeno_chip_colors[it], misc_flags | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
                    ImGui::SameLine();

                    ImGui::BeginGroup(); // Lock X position
                    ImGui::Text("Current");
                    ImGui::ColorButton("##current", xeno_chip_colors[it], ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40));
                    ImGui::Text("Previous");
                    if (ImGui::ColorButton("##previous", backup_color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40)))
                        xeno_chip_colors[it] = backup_color;
                    ImGui::Separator();
                    ImGui::Text("Palette");
                    for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
                    {
                        ImGui::PushID(n);
                        if ((n % 8) != 0)
                            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.y);

                        ImGuiColorEditFlags palette_button_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
                        if (ImGui::ColorButton("##palette", saved_palette[n], palette_button_flags, ImVec2(20, 20)))
                            xeno_chip_colors[it] = ImVec4(saved_palette[n].x, saved_palette[n].y, saved_palette[n].z, xeno_chip_colors[it].w); // Preserve alpha!

                        // Allow user to drop colors into each palette entry. Note that ColorButton() is already a
                        // drag source by default, unless specifying the ImGuiColorEditFlags_NoDragDrop flag.
                        if (ImGui::BeginDragDropTarget())
                        {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
                                memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 3);
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
                                memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 4);
                            ImGui::EndDragDropTarget();
                        }

                        ImGui::PopID();
                    }
                    ImGui::EndGroup();
                    ImGui::EndPopup();

                    colors[it+4].r = (Uint8)(xeno_chip_colors[it].x * 255.0);
                    colors[it+4].g = (Uint8)(xeno_chip_colors[it].y * 255.0);
                    colors[it+4].b = (Uint8)(xeno_chip_colors[it].z * 255.0);
                    colors[it+4].a = (Uint8)(xeno_chip_colors[it].w * 255.0);

                    std::fill_n(core->GetPrevVRAM(), 128 * 64, 0);
                    core->SetScreenDirty();
                    core->SetWipeScreen();

                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Audio"))
    {
        static bool mute;
        static float prev_volume;
        if (ImGui::Checkbox("Mute", &mute))
        {
            if (mute)
            {
                prev_volume = *volume;
                *volume = 0;
            }
            else
            {
                *volume = prev_volume;
            }
        }

        ImGui::SliderFloat("Volume", volume, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::EndMenu();
    }


}


