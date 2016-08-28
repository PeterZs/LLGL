/*
 * RenderTarget.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef __LLGL_RENDER_TARGET_H__
#define __LLGL_RENDER_TARGET_H__


#include "Export.h"
#include "TextureFlags.h"
#include <Gauss/Vector2.h>


namespace LLGL
{


class Texture;

//! Render target interface.
class LLGL_EXPORT RenderTarget
{

    public:

        virtual ~RenderTarget();

        /**
        \brief Attaches an internal depth buffer to this render target.
        \param[in] size Specifies the size of the depth buffer. This must be the same as for all other attachemnts.
        \remarks Only a single depth buffer, stencil buffer, or depth-stencil buffer can be attached.
        \see AttachDepthStencilBuffer
        */
        virtual void AttachDepthBuffer(const Gs::Vector2i& size) = 0;

        /**
        \brief Attaches an internal stencil buffer to this render target.
        \remarks Only a single depth buffer, stencil buffer, or depth-stencil buffer can be attached.
        \see AttachDepthBuffer
        */
        virtual void AttachStencilBuffer(const Gs::Vector2i& size) = 0;

        /**
        \brief Attaches an internal depth-stencil buffer to this render target.
        \remarks Only a single depth buffer, stencil buffer, or depth-stencil buffer can be attached.
        \see AttachDepthBuffer
        */
        virtual void AttachDepthStencilBuffer(const Gs::Vector2i& size) = 0;

        virtual void AttachTexture1D(Texture& texture, int mipLevel = 0) = 0;
        virtual void AttachTexture2D(Texture& texture, int mipLevel = 0) = 0;
        virtual void AttachTexture3D(Texture& texture, int layer, int mipLevel = 0) = 0;
        virtual void AttachTextureCube(Texture& texture, const AxisDirection cubeFace, int mipLevel = 0) = 0;
        virtual void AttachTexture1DArray(Texture& texture, int layer, int mipLevel = 0) = 0;
        virtual void AttachTexture2DArray(Texture& texture, int layer, int mipLevel = 0) = 0;
        virtual void AttachTextureCubeArray(Texture& texture, int layer, const AxisDirection cubeFace, int mipLevel = 0) = 0;

        //! Detaches all textures from this render target.
        virtual void DetachTextures() = 0;

        /**
        \brief Returns the frame buffer resolution.
        \remarks This will be determined by the first texture attachment. Every further attachment must have the same size.
        */
        inline const Gs::Vector2i& GetResolution() const
        {
            return resolution_;
        }

    protected:

        void ApplyResolution(const Gs::Vector2i& resolution);
        void ResetResolution();

    private:

        Gs::Vector2i resolution_;

};


} // /namespace LLGL


#endif



// ================================================================================
