/*
 * GLVertexArrayObject.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015-2018 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef LLGL_GL_VERTEX_ARRAY_OBJECT_H
#define LLGL_GL_VERTEX_ARRAY_OBJECT_H


#include <LLGL/VertexFormat.h>
#include "../OpenGL.h"


namespace LLGL
{


// Wrapper class for an OpenGL Vertex-Array-Object (VAO), for GL 3.0+.
class GLVertexArrayObject
{

    public:

        GLVertexArrayObject();
        ~GLVertexArrayObject();

        // Builds the specified attribute using a 'glVertexAttrib*Pointer' function.
        void BuildVertexAttribute(const VertexAttribute& attribute, GLsizei stride, GLuint index);

        // Returns the ID of the hardware vertex-array-object (VAO)
        inline GLuint GetID() const
        {
            return id_;
        }

    private:

        GLuint id_ = 0; //!< Vertex array object ID.

};


} // /namespace LLGL


#endif



// ================================================================================
