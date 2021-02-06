#include "DebugUI.h"
#include "imguial_button.h"

void DebugUI::Init()
{
    ImGui::CreateContext();
	ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForD3D(fe_State->window); //this is a badly named function. it's not specific to D3D really
	//ImGui_ImplSDL2_InitForSDL(fe_State->window);
    int win_w, win_h;
    SDL_GetWindowSize(fe_State->window, &win_w, &win_h);
	ImGuiSDL::Initialize(fe_State->renderer, win_w, win_h);
    chip8_ram_editor.Cols = 32;
    chip8_vram_editor.Cols = 64;
    auto imgui_logger = std::make_shared<imgui_log_sink_mt>(log);
    log->setFilterHeaderLabel("Filter");
    Logger::GetClientLogger()->sinks().push_back(imgui_logger);
    Logger::GetClientLogger()->set_level(spdlog::level::info);

    SDL_RendererInfo r_info;
    SDL_GetRendererInfo(fe_State->renderer, &r_info);

    LOG_INFO("Renderer: {}", r_info.name);

	return;
}


void DebugUI::Deinit()
{
    Logger::GetClientLogger()->set_level(spdlog::level::warn);
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

void DebugUI::Draw()
{
	ImGui_ImplSDL2_NewFrame(fe_State->window);
	ImGui::NewFrame();

	//ImGui::ShowDemoWindow();

    if (show_menu_bar)
        ShowMenuBar();

    if (show_display)
        ShowDisplayWindow(&show_display);

    if (show_audio)
        ShowAudioWindow(&show_audio);

    if (show_vram)
        ShowVRAMWindow(&show_vram);

    if (show_ram)
        ShowRAMWindow(&show_ram);

    if(show_regs)
        ShowCPUEditor(&show_regs);

    if (show_stack)
        ShowStackWindow(&show_stack);

    if (show_log)
        ShowLogWindow(&show_log);
    //    show_log = terminal_log->show();

    if(show_key_remap)
        ShowKeyRemapWindow(&show_key_remap);

	SDL_Rect windowRect = { 0, 0, 1, 1 };
	SDL_RenderSetClipRect(fe_State->renderer, &windowRect); //fixes an SDL bug for D3D backend
	SDL_SetRenderDrawColor(fe_State->renderer, 114, 144, 154, 255);
	SDL_RenderClear(fe_State->renderer);

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
	fe_State->capture_KB = io.WantCaptureKeyboard;
	return fe_State->capture_KB;
}

bool DebugUI::WantCaptureMouse()
{
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	fe_State->capture_Mouse = io.WantCaptureMouse;
	return fe_State->capture_Mouse;
}

// Demonstrate create a simple property editor.
void DebugUI::ShowCPUEditor(bool* p_open)
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
        ImGui::InputScalar("##value", ImGuiDataType_U8, fe_State->core->GetRegV(i), NULL, NULL, "%02X");
        ImGui::NextColumn();
        ImGui::PopID();
    }

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
    ImGui::InputScalar("##value", ImGuiDataType_U16, fe_State->core->GetRegI(), NULL, NULL, "%04X");
    ImGui::NextColumn();
    ImGui::PopID();
    ImGui::PushID(17);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("PC: ");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(40);
    ImGui::InputScalar("##value", ImGuiDataType_U16, fe_State->core->GetPC(), NULL, NULL, "%04X");
    ImGui::NextColumn();
    ImGui::PopID();
    ImGui::Columns(1);
    ImGui::Separator();
    if (ImGui::Button("Step"))
    {
        if (!fe_State->core->GetDebugStepping())
            fe_State->core->ToggleDebugStepping();
        fe_State->core->Step();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
    {
        fe_State->core->Reset();
    }
    ImGui::SameLine();
    //ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !fe_State->core->GetDebugStepping());
    if (ImGuiAl::Button("Run", fe_State->core->GetDebugStepping()))
    {
        fe_State->core->ToggleDebugStepping();
    }
    //ImGui::PopItemFlag();
    ImGui::End();
}


void DebugUI::ShowDisplayWindow(bool* p_open)
{
    unsigned int zoom = fe_State->resolution_Zoom;
    unsigned int res_w = fe_State->core->res.base_width;
    unsigned int res_h = fe_State->core->res.base_height;
    //ImGui::ImGuiWindowFlags_AlwaysAutoResize;
    //ImGui::SetNextWindowSize(ImVec2((float)((res_w * zoom) + res_w + 1), (float)((res_h * zoom) + res_h + 1)), ImGuiCond_Always);
    ImGui::SetNextWindowContentSize(ImVec2((float)((res_w * zoom) + res_w + 1), (float)((res_h * zoom) + res_h + 1)));
    if (!ImGui::Begin("Display", p_open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize))
    {
        ImGui::End();
        return;
    }
    //ImGui::Image(fe_State->screen_Texture, ImVec2((float)((res_w * zoom) + res_w + 1),(float)((res_h * zoom) + res_h + 1)));
    ImGui::Image(fe_State->screen_Texture, ImGui::GetContentRegionAvail());
    ImGui::End();

}

void DebugUI::ShowLogWindow(bool* p_open)
{
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    if(!ImGui::Begin("Log", p_open))
    {
        ImGui::End();
        return;
    }
    log->draw();
    ImGui::End();
}

void DebugUI::ShowRAMWindow(bool* p_open)
{
    ImGui::SetNextWindowSize(ImVec2(1000, 650), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("RAM", p_open))
    {
        ImGui::End();
        return;
    }
    chip8_ram_editor.DrawContents(fe_State->core->GetRAM(), sizeof(uint8_t) * (((unsigned long long)fe_State->core->GetRAMLimit())+1), 0); //TODO: don't hard code ram size
    ImGui::End();
}

void DebugUI::ShowVRAMWindow(bool* p_open)
{
    ImGui::SetNextWindowSize(ImVec2(1385, 490), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("VRAM", p_open))
    {
        ImGui::End();
        return;
    }
    static unsigned int res_w = fe_State->core->res.base_width;
    static unsigned int res_h = fe_State->core->res.base_height;
    chip8_vram_editor.DrawContents(fe_State->core->GetVRAM(), sizeof(uint8_t) * res_w * res_h, 0);
    ImGui::End();
}

void DebugUI::ShowStackWindow(bool* p_open)
{
    ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Stack", p_open))
    {
        ImGui::End();
        return;
    }
    ImGui::TextUnformatted("SP: ");
    ImGui::SameLine();
    ImGui::InputScalar("##value", ImGuiDataType_S8, (fe_State->core->GetSP()), NULL, NULL, "%04d");
    ImGui::Separator();

    ImGui::PushID(-1); // Use field index as identifier.
        // Here we use a TreeNode to highlight on hover (we could use e.g. Selectable as well)
    ImGui::AlignTextToFramePadding();
    if (*(fe_State->core->GetSP()) == -1)
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.1f, 1.0f), "Empty");
    else
        ImGui::TextDisabled("-----");
    ImGui::PopID();
    ImGui::Columns(2);
    for (int i = 0; i < fe_State->core->GetStackSize() / 2; i++)
    {
        ImGui::PushID(i); // Use field index as identifier.
        // Here we use a TreeNode to highlight on hover (we could use e.g. Selectable as well)
        ImGui::AlignTextToFramePadding();
        if(i == *(fe_State->core->GetSP()))
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.1f, 1.0f), "%02u:", i);
        else
            ImGui::Text("%02u:", i);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::InputScalar("##value", ImGuiDataType_U16, (fe_State->core->GetStack()+i), NULL, NULL, "%04X");
        ImGui::PopID();
    }
    ImGui::NextColumn();
    for (int i = fe_State->core->GetStackSize() / 2; i < fe_State->core->GetStackSize(); i++)
    {
        ImGui::PushID(i); // Use field index as identifier.
        // Here we use a TreeNode to highlight on hover (we could use e.g. Selectable as well)
        ImGui::AlignTextToFramePadding();
        if (i == *(fe_State->core->GetSP()))
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.1f, 1.0f), "%02u:", i);
        else
            ImGui::Text("%02u:", i);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::InputScalar("##value", ImGuiDataType_U16, (fe_State->core->GetStack() + i), NULL, NULL, "%04X");
        ImGui::PopID();
    }
    ImGui::End();
}

void DebugUI::ShowAudioWindow(bool* p_open)
{
    static float audio_h_zoom = 0, audio_v_zoom = 0;

    ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Audio", p_open))
    {
        ImGui::End();
        return;
    }
    if (!audio_h_zoom)
        audio_h_zoom = 1;
    if (!audio_v_zoom)
        audio_v_zoom = 1;
    static ImGui::PlotConfig conf;
    static float y_positions[4096] = { 0 };
    for (int i = 0; i < 4096; i++) 
        y_positions[i] = fe_State->audio_Output[(4096 - i + fe_State->audio_Output_Pos) % 4096];
    //static float x_positions[4096];
    //for (int i = 0; i < pos; i++)
    //    x_positions[i] = (i + 4096 - pos);
    //for (int i = pos; i < 4096; i++)
    //    x_positions[i] = (i - pos);
    //conf.values.xs = x_positions; // this line is optional
    conf.values.ys = y_positions;
    conf.values.offset = 0;
    conf.values.count = (int)std::roundf(4096.0f / audio_h_zoom);
    conf.scale.min = -1.0f / audio_v_zoom;
    conf.scale.max = 1.0f / audio_v_zoom;
    conf.scale.type = conf.scale.Linear;
    conf.tooltip.show = true;
    conf.tooltip.format = "x=%.2f, y=%.2f";
    conf.grid_x.show = false;
    conf.grid_y.show = false;
    conf.frame_size = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() - (3 * ImGui::GetTextLineHeightWithSpacing()));// ImVec2(600, 300);
    conf.values.color = ImColor(255, 255, 0);
    conf.line_thickness = 2.0f;

    ImGui::TextDisabled("Zoom");
    ImGui::SliderFloat("Horizontal", &audio_h_zoom, 1.0f, 256.0f, NULL, ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("Vertical", &audio_v_zoom, 0.1f, 10.0f, NULL, ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
    ImGui::Plot("Channel 1", conf);

    ImGui::End();
}

void DebugUI::ShowKeyRemapWindow(bool* p_open)
{
    static ImVec2 button_size(80, 80);

    ImGui::SetNextWindowSize(ImVec2(440, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Remap Keys:", p_open))
    {
        ImGui::End();
        return;
    }
    
    for (int i = 0; i < 16; i++)
    {
        ImGui::PushID(i);
        if (ImGuiAl::Button(fe_State->keyNames[i].c_str(), !fe_State->wait_for_remap_input, button_size))
        {
            fe_State->wait_for_remap_input = true;
            fe_State->remap_key_index = i;
        }
        ImGui::PopID();
        if (i % 4 != 3)
            ImGui::SameLine();
    }

    
    ImGui::End();
}

void DebugUI::ShowMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ShowMenuFile();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Windows"))
        {
            ShowMenuWindows();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Emulation"))
        {
            ShowMenuEmulation();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options"))
        {
            ShowMenuOptions();
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void DebugUI::ShowMenuFile()
{
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, (bool)fe_State->open_File);
    if (ImGui::MenuItem("Load", ""))
    {
        fe_State->open_File = std::make_shared<pfd::open_file>("Choose file", "C:\\");
    }
    ImGui::PopItemFlag();
    if (ImGui::MenuItem("Reload", ""))
    {
        fe_State->core->Reset();
        if (fe_State->last_File != "")
        {
            fe_State->core->Load(fe_State->file_data);
        }
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Quit", ""))
    {
        fe_State->running = false;
    }
}

void DebugUI::ShowMenuEmulation()
{
    const ImU32 u32_one = 1;
    ImGui::TextUnformatted("CPU Cycles:");
    ImGui::SameLine();
    ImGui::InputScalar("", ImGuiDataType_U32, &(fe_State->run_Cycles), &u32_one, NULL, "%u");

    if (ImGui::BeginMenu("System Mode"))
    {
        if (ImGui::MenuItem("COSMAC VIP (CHIP-8)", NULL, fe_State->core->GetSystemMode() == Chip8::SYSTEM_MODE::CHIP_8 ? true : false))
        {
            if (fe_State->core->GetSystemMode() != Chip8::SYSTEM_MODE::CHIP_8)
            {
                fe_State->resolution_Zoom = fe_State->resolution_Zoom * 2;
                fe_State->zoom_Changed = true;
            }
            fe_State->core->SetSystemMode(Chip8::SYSTEM_MODE::CHIP_8);
        }
        HelpMarker("Original COSMAC VIP CHIP-8 interpreter.");
        
        if (ImGui::MenuItem("HP-48 (SUPER-CHIP)", NULL, fe_State->core->GetSystemMode() == Chip8::SYSTEM_MODE::SUPER_CHIP ? true : false))
        {
            if (fe_State->core->GetSystemMode() == Chip8::SYSTEM_MODE::CHIP_8)
            {
                fe_State->resolution_Zoom = fe_State->resolution_Zoom / 2;
                fe_State->zoom_Changed = true;
            }
            fe_State->core->SetSystemMode(Chip8::SYSTEM_MODE::SUPER_CHIP);
        }
        HelpMarker("SUPER-CHIP 1.1 for HP-48 series calculators.");

        if (ImGui::MenuItem("Octo (XO-CHIP)", NULL, fe_State->core->GetSystemMode() == Chip8::SYSTEM_MODE::XO_CHIP ? true : false))
        {
            if (fe_State->core->GetSystemMode() == Chip8::SYSTEM_MODE::CHIP_8)
            {
                fe_State->resolution_Zoom = fe_State->resolution_Zoom / 2;
                fe_State->zoom_Changed = true;
            }
            fe_State->core->SetSystemMode(Chip8::SYSTEM_MODE::XO_CHIP);
        }
        HelpMarker("Octo IDE XO-CHIP compatibility.");

        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Quirks"))
    {
        ImGui::MenuItem("VIP Jumps (NNN+V0)", NULL, &(fe_State->core->quirks.vip_jump) );
        ImGui::MenuItem("Copy VY to VX Before Bit Shifts", NULL, &(fe_State->core->quirks.vip_shifts) );
        ImGui::MenuItem("Register Store/Load Increments I", NULL, &(fe_State->core->quirks.vip_regs_read_write) );
        ImGui::MenuItem("Logic Ops Reset VF", NULL, &(fe_State->core->quirks.logic_flag_reset) );
        ImGui::MenuItem("Draw Ops Wrap", NULL, &(fe_State->core->quirks.draw_wrap) );
        ImGui::MenuItem("Draw Ops Wait for Vblank", NULL, &(fe_State->core->quirks.draw_vblank) );
        ImGui::MenuItem("S-CHIP 1.0 Large Fonts", NULL, &(fe_State->core->quirks.schip_10_fonts) );
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
    ImGui::MenuItem("Audio Visualizer", NULL, &show_audio);
}

void DebugUI::ShowMenuOptions()
{
    if (ImGui::BeginMenu("Display"))
    {
        if (ImGui::MenuItem("Draw Pixel Grid", "", &(fe_State->debug_Grid_Lines), true))
        {
            fe_State->grid_Toggled = true;
        }

        const ImU32 u32_one = 1;
        ImGui::TextUnformatted("Zoom:");
        ImGui::SameLine();
        if (ImGui::InputScalar("", ImGuiDataType_U32, &(fe_State->resolution_Zoom), &u32_one, NULL, "%u")) { fe_State->zoom_Changed = true; }
        if (fe_State->resolution_Zoom < 1) { fe_State->resolution_Zoom = 1; }

        if (ImGui::BeginMenu("Colors"))
        {
            ImGui::Text("Display Colors:");
            static bool alpha_preview = true;
            static bool alpha_half_preview = false;
            static bool drag_and_drop = true;
            static bool options_menu = true;
            static bool hdr = false;
            static ImVec4 background_color   = ImVec4((float)fe_State->screen_Colors[0].r / 255.0f, (float)fe_State->screen_Colors[0].g / 255.0f, (float)fe_State->screen_Colors[0].b / 255.0f, 255.0f / 255.0f);
            static ImVec4 foreground_color_1 = ImVec4((float)fe_State->screen_Colors[1].r / 255.0f, (float)fe_State->screen_Colors[1].g / 255.0f, (float)fe_State->screen_Colors[1].b / 255.0f, 255.0f / 255.0f);
            static ImVec4 foreground_color_2 = ImVec4((float)fe_State->screen_Colors[2].r / 255.0f, (float)fe_State->screen_Colors[2].g / 255.0f, (float)fe_State->screen_Colors[2].b / 255.0f, 255.0f / 255.0f);
            static ImVec4 overlap_color      = ImVec4((float)fe_State->screen_Colors[3].r / 255.0f, (float)fe_State->screen_Colors[3].g / 255.0f, (float)fe_State->screen_Colors[3].b / 255.0f, 255.0f / 255.0f);
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

                fe_State->screen_Colors[0].r = (Uint8)(background_color.x * 255.0);
                fe_State->screen_Colors[0].g = (Uint8)(background_color.y * 255.0);
                fe_State->screen_Colors[0].b = (Uint8)(background_color.z * 255.0);
                fe_State->screen_Colors[0].a = (Uint8)(background_color.w * 255.0);

                std::fill_n(fe_State->core->GetPrevVRAM(), 128 * 64, 0);
                fe_State->core->SetScreenDirty();
                fe_State->core->SetWipeScreen();
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

                fe_State->screen_Colors[1].r = (Uint8)(foreground_color_1.x * 255.0);
                fe_State->screen_Colors[1].g = (Uint8)(foreground_color_1.y * 255.0);
                fe_State->screen_Colors[1].b = (Uint8)(foreground_color_1.z * 255.0);
                fe_State->screen_Colors[1].a = (Uint8)(foreground_color_1.w * 255.0);

                std::fill_n(fe_State->core->GetPrevVRAM(), 128 * 64, 0);
                fe_State->core->SetScreenDirty();
                fe_State->core->SetWipeScreen();

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

                fe_State->screen_Colors[2].r = (Uint8)(foreground_color_2.x * 255.0);
                fe_State->screen_Colors[2].g = (Uint8)(foreground_color_2.y * 255.0);
                fe_State->screen_Colors[2].b = (Uint8)(foreground_color_2.z * 255.0);
                fe_State->screen_Colors[2].a = (Uint8)(foreground_color_2.w * 255.0);

                std::fill_n(fe_State->core->GetPrevVRAM(), 128 * 64, 0);
                fe_State->core->SetScreenDirty();
                fe_State->core->SetWipeScreen();

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

                fe_State->screen_Colors[3].r = (Uint8)(overlap_color.x * 255.0);
                fe_State->screen_Colors[3].g = (Uint8)(overlap_color.y * 255.0);
                fe_State->screen_Colors[3].b = (Uint8)(overlap_color.z * 255.0);
                fe_State->screen_Colors[3].a = (Uint8)(overlap_color.w * 255.0);

                std::fill_n(fe_State->core->GetPrevVRAM(), 128 * 64, 0);
                fe_State->core->SetScreenDirty();
                fe_State->core->SetWipeScreen();

            }

            static ImVec4 xeno_chip_colors[12];
            static bool popups[12] = { false };
            static std::string popup_id;
            static std::string popup_label;
            for (int it = 0; it < 12; it++)
            {
                xeno_chip_colors[it] = ImVec4((float)fe_State->screen_Colors[it + 4].r / 255.0f, (float)fe_State->screen_Colors[it + 4].g / 255.0f, (float)fe_State->screen_Colors[it + 4].b / 255.0f, 255.0f / 255.0f);
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

                    fe_State->screen_Colors[it+4].r = (Uint8)(xeno_chip_colors[it].x * 255.0);
                    fe_State->screen_Colors[it+4].g = (Uint8)(xeno_chip_colors[it].y * 255.0);
                    fe_State->screen_Colors[it+4].b = (Uint8)(xeno_chip_colors[it].z * 255.0);
                    fe_State->screen_Colors[it+4].a = (Uint8)(xeno_chip_colors[it].w * 255.0);

                    std::fill_n(fe_State->core->GetPrevVRAM(), 128 * 64, 0);
                    fe_State->core->SetScreenDirty();
                    fe_State->core->SetWipeScreen();

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
                prev_volume = fe_State->volume;
                fe_State->volume = 0;
            }
            else
            {
                fe_State->volume = prev_volume;
            }
        }

        ImGui::SliderFloat("Volume", &(fe_State->volume), 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Input"))
    {
        if (ImGui::BeginMenu("Keypad Style"))
        {
            bool vip_selected = (fe_State->selected_Key_Layout == UIState::KeyLayout::VIP ? true : false);
            bool dream_selected = (fe_State->selected_Key_Layout == UIState::KeyLayout::DREAM ? true : false);
            bool digi_selected = (fe_State->selected_Key_Layout == UIState::KeyLayout::DIGITRAN ? true : false);

            if (ImGui::MenuItem("VIP", NULL, &vip_selected))
            {
                if (fe_State->selected_Key_Layout != UIState::KeyLayout::VIP)
                {
                    fe_State->selected_Key_Layout = UIState::KeyLayout::VIP;
                    fe_State->key_Layout_Changed = true;
                }
            }
            HelpMarker("1 2 3 C\n4 5 6 D\n7 8 9 E\nA 0 B F");

            if (ImGui::MenuItem("DREAM 6800", NULL, &dream_selected))
            {
                if (fe_State->selected_Key_Layout != UIState::KeyLayout::DREAM)
                {
                    fe_State->selected_Key_Layout = UIState::KeyLayout::DREAM;
                    fe_State->key_Layout_Changed = true;
                }
            }
            HelpMarker("C D E F\n8 9 A B\n4 5 6 7\n0 1 2 3");

            if (ImGui::MenuItem("Digitran", NULL, &digi_selected))
            {
                if (fe_State->selected_Key_Layout != UIState::KeyLayout::DIGITRAN)
                {
                    fe_State->selected_Key_Layout = UIState::KeyLayout::DIGITRAN;
                    fe_State->key_Layout_Changed = true;
                }
            }
            HelpMarker("0 1 2 3\n4 5 6 7\n8 9 A B\nC D E F");

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Map Keys"))
        {
            show_key_remap = true;
        }

        ImGui::EndMenu();



    }

}


