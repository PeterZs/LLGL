/*
 * GLRenderTarget.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef __LLGL_GL_RENDER_TARGET_H__
#define __LLGL_GL_RENDER_TARGET_H__


#include <LLGL/RenderTarget.h>
#include "GLFrameBuffer.h"
#include "GLRenderBuffer.h"
#include "GLTexture.h"
#include <functional>
#include <vector>
#include <memory>


namespace LLGL
{


class GLRenderTarget : public RenderTarget
{

    public:

        GLRenderTarget(unsigned int multiSamples);

        void AttachDepthBuffer(const Gs::Vector2i& size) override;
        void AttachStencilBuffer(const Gs::Vector2i& size) override;
        void AttachDepthStencilBuffer(const Gs::Vector2i& size) override;

        void AttachTexture1D(Texture& texture, int mipLevel = 0) override;
        void AttachTexture2D(Texture& texture, int mipLevel = 0) override;
        void AttachTexture3D(Texture& texture, int layer, int mipLevel = 0) override;
        void AttachTextureCube(Texture& texture, const AxisDirection cubeFace, int mipLevel = 0) override;
        void AttachTexture1DArray(Texture& texture, int layer, int mipLevel = 0) override;
        void AttachTexture2DArray(Texture& texture, int layer, int mipLevel = 0) override;
        void AttachTextureCubeArray(Texture& texture, int layer, const AxisDirection cubeFace, int mipLevel = 0) override;

        void DetachTextures() override;

        /* ----- Extended Internal Functions ----- */

        //! Blits the multi-sample frame buffer onto the default frame buffer.
        void BlitOntoFrameBuffer();

        //! Blits the specified color attachment from the frame buffer onto the screen.
        void BlitOntoScreen(std::size_t colorAttachmentIndex);

        const GLFrameBuffer& GetFrameBuffer() const;

        /*inline const GLRenderBuffer& GetRenderBuffer() const
        {
            return renderBuffer_;
        }*/

    private:

        using AttachTextureCallback = std::function<void(GLenum attachment, GLTexture& textureGL)>;

        void ApplyMipResolution(Texture& texture, int mipLevel);

        void InitRenderBufferStorage(GLRenderBuffer& renderBuffer, GLenum internalFormat);
        GLenum AttachDefaultRenderBuffer(GLFrameBuffer& frameBuffer, GLenum attachment);

        void AttachRenderBuffer(const Gs::Vector2i& size, GLenum internalFormat, GLenum attachment);
        void AttachTexture(Texture& texture, int mipLevel, const AttachTextureCallback& attachmentProc);

        GLenum MakeColorAttachment();

        //! Sets the draw buffers for the currently bound FBO.
        void SetDrawBuffers();

        void CheckFrameBufferStatus(GLenum status, const std::string& info);

        GLFrameBuffer                                   frameBuffer_;

        std::unique_ptr<GLRenderBuffer>                 renderBuffer_;

        /**
        Multi-sampled frame buffer; required since we cannot
        directly draw into a texture when using multi-sampling.
        */
        std::unique_ptr<GLFrameBuffer>                  frameBufferMS_;

        /**
        For multi-sampled render targets we also need a render buffer for each attached texture.
        Otherwise we would need multi-sampled textures (e.g. "glTexImage2DMultisample")
        which is only available since OpenGL 3.2+, but render buffers are available since OpenGL 3.0+.
        */
        std::vector<std::unique_ptr<GLRenderBuffer>>    renderBuffersMS_;

        std::vector<GLenum>                             colorAttachments_;

        GLsizei                                         multiSamples_           = 0;
        GLbitfield                                      blitMask_               = 0;

};


} // /namespace LLGL


#endif



// ================================================================================
