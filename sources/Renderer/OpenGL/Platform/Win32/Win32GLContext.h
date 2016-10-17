/*
 * Win32GLContext.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef __LLGL_WIN32_GL_CONTEXT_H__
#define __LLGL_WIN32_GL_CONTEXT_H__


#include "../GLContext.h"
#include <LLGL/Window.h>
#include <LLGL/RenderContextDescriptor.h>
#include "../../OpenGL.h"
#include "Win32GLPlatformContext.h"
#include <LLGL/Platform/NativeHandle.h>


namespace LLGL
{


class Win32GLContext : public GLContext
{

    public:

        Win32GLContext(RenderContextDescriptor& desc, Window& window, GLContext* sharedContext);
        ~Win32GLContext();

        bool Activate(bool activate) override;

        bool SetSwapInterval(int interval) override;

        bool SwapBuffers() override;

    private:

        void CreateContext(Win32GLContext* sharedContext);
        void DeleteContext();

        void GetNativeContextHandle(NativeContextHandle& windowContext);

        void DeleteGLContext(HGLRC& renderContext);

        HGLRC CreateGLContext(bool useExtProfile, Win32GLContext* sharedContext = nullptr);
        HGLRC CreateStdContextProfile();
        HGLRC CreateExtContextProfile(HGLRC sharedGLRC = nullptr);

        void SetupDeviceContextAndPixelFormat();

        void SelectPixelFormat();
        bool SetupAntiAliasing();
        void CopyPixelFormat(Win32GLContext& sourceContext);

        void RecreateWindow();

        RenderContextDescriptor&    desc_;
        Window&                     window_;

        GLPlatformContext           context_;

        bool                        hasSharedContext_   = false;

};


} // /namespace LLGL


#endif



// ================================================================================