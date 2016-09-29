/*
 * Buffer.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef __LLGL_BUFFER_H__
#define __LLGL_BUFFER_H__


#include "Export.h"
#include "BufferFlags.h"


namespace LLGL
{


//! Hardware buffer interface.
class LLGL_EXPORT Buffer
{

    public:

        virtual ~Buffer();

        //! Returns the type of this buffer.
        inline BufferType GetType() const
        {
            return type_;
        }

    protected:

        Buffer(const BufferType type);

    private:

        BufferType type_;

};


} // /namespace LLGL


#endif



// ================================================================================