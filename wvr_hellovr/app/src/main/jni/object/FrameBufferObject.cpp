// "WaveVR SDK 
// © 2017 HTC Corporation. All Rights Reserved.
//
// Unless otherwise required by copyright law and practice,
// upon the execution of HTC SDK license agreement,
// HTC grants you access to and use of the WaveVR SDK(s).
// You shall fully comply with all of HTC’s SDK license agreement terms and
// conditions signed by you and all SDK and API requirements,
// specifications, and documentation provided by HTC to You."

#include <FrameBufferObject.h>
#include <Object.h>

#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <log.h>

#define NUM_MULTIVIEW_SLICES    2

// Functions that have to be queried from EGL
typedef void(*PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVR)(GLenum, GLenum, GLuint, GLint, GLint, GLsizei);
PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVR glFramebufferTextureMultiviewOVR = NULL;

#ifndef GL_TEXTURE_2D_MULTISAMPLE_ARRAY
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY        0x9102
#endif

#ifndef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE       0x8D56
#endif

// Functions that have to be queried from EGL
typedef void(*PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVR)(GLenum, GLenum, GLuint, GLint, GLsizei, GLint, GLsizei);

PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVR glFramebufferTextureMultisampleMultiviewOVR = NULL;

typedef void(*PFNGLTEXSTORAGE3DMULTISAMPLEOES)(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei,GLboolean);

PFNGLTEXSTORAGE3DMULTISAMPLEOES glTexStorage3DMultisampleOES = NULL;

void FrameBufferObject::InitializeMultiView() {

    if(glFramebufferTextureMultiviewOVR == NULL)
    glFramebufferTextureMultiviewOVR = (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVR)eglGetProcAddress("glFramebufferTextureMultiviewOVR");
    if (!glFramebufferTextureMultiviewOVR)
    {
        LOGE("Failed to  get proc address for glFramebufferTextureMultiviewOVR!!");
        exit(EXIT_FAILURE);
    }

    if(glFramebufferTextureMultisampleMultiviewOVR == NULL)
    glFramebufferTextureMultisampleMultiviewOVR = (PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVR)eglGetProcAddress("glFramebufferTextureMultisampleMultiviewOVR");
    if (!glFramebufferTextureMultisampleMultiviewOVR)
    {
        LOGE("Failed to  get proc address for glFramebufferTextureMultisampleMultiviewOVR!!");
        exit(EXIT_FAILURE);
    }

    if(glTexStorage3DMultisampleOES == NULL)
    glTexStorage3DMultisampleOES = (PFNGLTEXSTORAGE3DMULTISAMPLEOES)eglGetProcAddress("glTexStorage3DMultisampleOES");
    if (!glTexStorage3DMultisampleOES)
    {
        LOGE("Failed to  get proc address for glTexStorage3DMultisampleOES!!");
        exit(EXIT_FAILURE);
    }
}

FrameBufferObject::FrameBufferObject(int textureId, int width, int height, bool msaa, bool multiview) :
        mMSAA(msaa),
        mWidth(width),
        mHeight(height),
        mFrameBufferId(0),
        mDepthBufferId(0),
        mTextureId(textureId) {
    if (msaa) {
        int samples;
        glGetIntegerv(GL_MAX_SAMPLES_EXT, &samples);
        if (samples < 4 || !Object::hasGlExtension("GL_EXT_multisampled_render_to_texture"))
            msaa = false;
    }

    if (multiview) {
        InitializeMultiView();
        if (msaa) {
            msaa = false;
             initMultiviewMSAA();
            return;
        } else {
            msaa = false;
            initMultiview();
        }
    }
    else {
    	if (msaa) {
     		initMSAA();
    	} else {
        	msaa = false;
        	init();
		}
	}

    // check FBO status
    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        mHasError = true;
        clear();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FrameBufferObject::~FrameBufferObject() {
    clear();
}

void FrameBufferObject::initMSAA() {
    // Reference to https://www.khronos.org/registry/gles/extensions/EXT/EXT_multisampled_render_to_texture.txt
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT =
        (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)eglGetProcAddress( "glRenderbufferStorageMultisampleEXT" );

    PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT =
        (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)eglGetProcAddress( "glFramebufferTexture2DMultisampleEXT" );

    if (glRenderbufferStorageMultisampleEXT == NULL ||
        glFramebufferTexture2DMultisampleEXT == NULL) {
        mHasError = true;
        return;
    }

    glGenRenderbuffers(1, &mDepthBufferId);
    glBindRenderbuffer(GL_RENDERBUFFER, mDepthBufferId);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT24, mWidth, mHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &mFrameBufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferId);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBufferId);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTextureId, 0, 4);
}

void FrameBufferObject::init() {
    glGenFramebuffers(1, &mFrameBufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferId);

    glGenRenderbuffers(1, &mDepthBufferId);
    glBindRenderbuffer(GL_RENDERBUFFER, mDepthBufferId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mWidth, mHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBufferId);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTextureId, 0);
}

void FrameBufferObject::initMultiviewMSAA() {
    const GLsizei NumSamples = 2;

    glGenTextures(1, &mDepthBufferId);
    glBindTexture(GL_TEXTURE_2D_ARRAY, mDepthBufferId);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT24, mWidth, mHeight, 2);

    glGenFramebuffers(1, &mFrameBufferId);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFrameBufferId);

    glFramebufferTextureMultisampleMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, mDepthBufferId, 0, NumSamples, 0, 2);

    glFramebufferTextureMultisampleMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTextureId, 0, NumSamples, 0, 2);
}

void FrameBufferObject::initMultiview() {
    glGenTextures(1, &mDepthBufferId);
    glBindTexture(GL_TEXTURE_2D_ARRAY, mDepthBufferId);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT24, mWidth, mHeight, 2);

    glGenFramebuffers(1, &mFrameBufferId);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFrameBufferId);

    glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTextureId, 0, 0, 2);

    glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, mDepthBufferId, 0, 0, 2);
}

void FrameBufferObject::clear() {
    if (mDepthBufferId != 0)
        glDeleteRenderbuffers(1, &mDepthBufferId);
    if (mFrameBufferId != 0)
        glDeleteFramebuffers(1, &mFrameBufferId);
    mFrameBufferId = mDepthBufferId = mTextureId = 0;
}

void FrameBufferObject::bindFrameBuffer(bool multiview) {
    if(multiview){
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFrameBufferId);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferId);
    }
}
    
void FrameBufferObject::unbindFrameBuffer(bool multiview) {
    if(multiview)
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    else
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
