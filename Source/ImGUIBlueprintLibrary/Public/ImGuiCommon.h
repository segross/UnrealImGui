// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#ifdef IMGUI_API
#define WITH_IMGUI 1
#else
#define WITH_IMGUI 0
#endif // IMGUI_API

#if WITH_IMGUI
#include <ImGuiModule.h>
#include <ImGuiDelegates.h>

#include <imgui.h>
#endif // WITH_IMGUI
