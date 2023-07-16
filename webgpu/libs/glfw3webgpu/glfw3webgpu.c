/**
 * This is an extension of GLFW for WebGPU, abstracting away the details of
 * OS-specific operations.
 * 
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://eliemichel.github.io/LearnWebGPU
 * 
 * Most of this code comes from the wgpu-native triangle example:
 *   https://github.com/gfx-rs/wgpu-native/blob/master/examples/triangle/main.c
 * 
 * MIT License
 * Copyright (c) 2022-2023 Elie Michel and the wgpu-native authors
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "glfw3webgpu.h"

#include <webgpu/webgpu.h>

#define WGPU_TARGET_MACOS 1
#define WGPU_TARGET_LINUX_X11 2
#define WGPU_TARGET_WINDOWS 3
#define WGPU_TARGET_LINUX_WAYLAND 4

#if defined(_WIN32)
#define WGPU_TARGET WGPU_TARGET_WINDOWS
#elif defined(__APPLE__)
#define WGPU_TARGET WGPU_TARGET_MACOS
#else
#define WGPU_TARGET WGPU_TARGET_LINUX_X11
#endif

#if WGPU_TARGET == WGPU_TARGET_MACOS
#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>
#endif

#include <GLFW/glfw3.h>
#if WGPU_TARGET == WGPU_TARGET_MACOS
#define GLFW_EXPOSE_NATIVE_COCOA
#elif WGPU_TARGET == WGPU_TARGET_LINUX_X11
#define GLFW_EXPOSE_NATIVE_X11
#elif WGPU_TARGET == WGPU_TARGET_LINUX_WAYLAND
#define GLFW_EXPOSE_NATIVE_WAYLAND
#elif WGPU_TARGET == WGPU_TARGET_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

WGPUSurface glfwGetWGPUSurface(WGPUInstance instance, GLFWwindow* window) {
#if WGPU_TARGET == WGPU_TARGET_MACOS
    {
        id metal_layer = NULL;
        NSWindow* ns_window = glfwGetCocoaWindow(window);
        [ns_window.contentView setWantsLayer : YES] ;
        metal_layer = [CAMetalLayer layer];
        [ns_window.contentView setLayer : metal_layer] ;
        return wgpuInstanceCreateSurface(
            instance,
            &(WGPUSurfaceDescriptor){
            .label = NULL,
                .nextInChain =
                (const WGPUChainedStruct*)&(
                    WGPUSurfaceDescriptorFromMetalLayer) {
                .chain =
                    (WGPUChainedStruct){
                        .next = NULL,
                        .sType = WGPUSType_SurfaceDescriptorFromMetalLayer,
                },
                .layer = metal_layer,
            },
        });
    }
#elif WGPU_TARGET == WGPU_TARGET_LINUX_X11
    {
        Display* x11_display = glfwGetX11Display();
        Window x11_window = glfwGetX11Window(window);
        return wgpuInstanceCreateSurface(
            instance,
            &(WGPUSurfaceDescriptor){
            .label = NULL,
                .nextInChain =
                (const WGPUChainedStruct*)&(
                    WGPUSurfaceDescriptorFromXlibWindow) {
                .chain =
                    (WGPUChainedStruct){
                        .next = NULL,
                        .sType = WGPUSType_SurfaceDescriptorFromXlibWindow,
                },
                .display = x11_display,
                .window = x11_window,
            },
        });
    }
#elif WGPU_TARGET == WGPU_TARGET_LINUX_WAYLAND
    {
        struct wl_display* wayland_display = glfwGetWaylandDisplay();
        struct wl_surface* wayland_surface = glfwGetWaylandWindow(window);
        return wgpuInstanceCreateSurface(
            instance,
            &(WGPUSurfaceDescriptor){
            .label = NULL,
                .nextInChain =
                (const WGPUChainedStruct*)&(
                    WGPUSurfaceDescriptorFromWaylandSurface) {
                .chain =
                    (WGPUChainedStruct){
                        .next = NULL,
                        .sType =
                            WGPUSType_SurfaceDescriptorFromWaylandSurface,
},
.display = wayland_display,
.surface = wayland_surface,
                },
        });
  }
#elif WGPU_TARGET == WGPU_TARGET_WINDOWS
    {
        HWND hwnd = glfwGetWin32Window(window);
        HINSTANCE hinstance = GetModuleHandle(NULL);
        return wgpuInstanceCreateSurface(
            instance,
            &(WGPUSurfaceDescriptor){
            .label = NULL,
                .nextInChain =
                (const WGPUChainedStruct*)&(
                    WGPUSurfaceDescriptorFromWindowsHWND) {
                .chain =
                    (WGPUChainedStruct){
                        .next = NULL,
                        .sType = WGPUSType_SurfaceDescriptorFromWindowsHWND,
            },
            .hinstance = hinstance,
            .hwnd = hwnd,
        },
    });
  }
#else
#error "Unsupported WGPU_TARGET"
#endif
}

