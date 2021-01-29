#include "BasicUI.h"

void BasicUI::Init()
{
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDL(fe_State->window);
    int win_w, win_h;
    SDL_GetWindowSize(fe_State->window, &win_w, &win_h);
    ImGuiSDL::Initialize(fe_State->renderer, win_w, win_h);
    return;
}


void BasicUI::Deinit()
{
    ImGuiSDL::Deinitialize();
    ImGui::DestroyContext();
}

static void HelpMarker(const char* desc)
{
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void BasicUI::Draw()
{
    ImGui_ImplSDL2_NewFrame(fe_State->window);
    ImGui::NewFrame();

    if (SDL_GetMouseFocus())
        ShowMenuBar();

    ImGui::Render();
    ImGuiSDL::Render(ImGui::GetDrawData());
}

void BasicUI::HandleInput(SDL_Event* event)
{
    ImGui_ImplSDL2_ProcessEvent(event);
}

bool BasicUI::WantCaptureKB()
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    fe_State->capture_KB = io.WantCaptureKeyboard;
    return fe_State->capture_KB;
    return fe_State->capture_KB;
}

bool BasicUI::WantCaptureMouse()
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    fe_State->capture_Mouse = io.WantCaptureMouse;
    return fe_State->capture_Mouse;
}

void BasicUI::ShowMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ShowMenuFile();
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

void BasicUI::ShowMenuFile()
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

void BasicUI::ShowMenuEmulation()
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
        ImGui::MenuItem("VIP Jumps (NNN+V0)", NULL, &(fe_State->core->quirks.vip_jump));
        ImGui::MenuItem("Copy VY to VX Before Bit Shifts", NULL, &(fe_State->core->quirks.vip_shifts));
        ImGui::MenuItem("Register Store/Load Increments I", NULL, &(fe_State->core->quirks.vip_regs_read_write));
        ImGui::MenuItem("Logic Ops Reset VF", NULL, &(fe_State->core->quirks.logic_flag_reset));
        ImGui::MenuItem("Draw Ops Wrap", NULL, &(fe_State->core->quirks.draw_wrap));
        ImGui::MenuItem("Draw Ops Wait for Vblank", NULL, &(fe_State->core->quirks.draw_vblank));
        ImGui::MenuItem("S-CHIP 1.0 Large Fonts", NULL, &(fe_State->core->quirks.schip_10_fonts));
        ImGui::EndMenu();
    }

}

void BasicUI::ShowMenuOptions()
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
            static ImVec4 background_color = ImVec4((float)fe_State->screen_Colors[0].r / 255.0f, (float)fe_State->screen_Colors[0].g / 255.0f, (float)fe_State->screen_Colors[0].b / 255.0f, 255.0f / 255.0f);
            static ImVec4 foreground_color_1 = ImVec4((float)fe_State->screen_Colors[1].r / 255.0f, (float)fe_State->screen_Colors[1].g / 255.0f, (float)fe_State->screen_Colors[1].b / 255.0f, 255.0f / 255.0f);
            static ImVec4 foreground_color_2 = ImVec4((float)fe_State->screen_Colors[2].r / 255.0f, (float)fe_State->screen_Colors[2].g / 255.0f, (float)fe_State->screen_Colors[2].b / 255.0f, 255.0f / 255.0f);
            static ImVec4 overlap_color = ImVec4((float)fe_State->screen_Colors[3].r / 255.0f, (float)fe_State->screen_Colors[3].g / 255.0f, (float)fe_State->screen_Colors[3].b / 255.0f, 255.0f / 255.0f);
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
                popup_id = "popup" + std::to_string(it + 4);
                popup_label = "Color " + std::to_string(it + 4);

                popups[it] = ImGui::ColorButton(popup_label.c_str(), xeno_chip_colors[it], misc_flags);
                if ((it % 4) != 3)
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

                    fe_State->screen_Colors[it + 4].r = (Uint8)(xeno_chip_colors[it].x * 255.0);
                    fe_State->screen_Colors[it + 4].g = (Uint8)(xeno_chip_colors[it].y * 255.0);
                    fe_State->screen_Colors[it + 4].b = (Uint8)(xeno_chip_colors[it].z * 255.0);
                    fe_State->screen_Colors[it + 4].a = (Uint8)(xeno_chip_colors[it].w * 255.0);

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


}