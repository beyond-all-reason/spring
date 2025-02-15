/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <glad/glad.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int GLAD_GL_VERSION_1_0 = 0;
int GLAD_GL_VERSION_1_1 = 0;
int GLAD_GL_VERSION_1_2 = 0;
int GLAD_GL_VERSION_1_3 = 0;
int GLAD_GL_VERSION_1_4 = 0;
int GLAD_GL_VERSION_1_5 = 0;
int GLAD_GL_VERSION_2_0 = 0;
int GLAD_GL_VERSION_2_1 = 0;
int GLAD_GL_VERSION_3_0 = 0;
int GLAD_GL_VERSION_3_1 = 0;
int GLAD_GL_VERSION_3_2 = 0;

int GLAD_GL_NV_primitive_restart = 0;
int GLAD_GL_EXT_texture_filter_anisotropic = 0;
int GLAD_GL_EXT_texture_array = 0;
int GLAD_GL_EXT_stencil_two_side = 0;
int GLAD_GL_EXT_pixel_buffer_object = 0;
int GLAD_GL_EXT_framebuffer_object = 0;
int GLAD_GL_EXT_framebuffer_multisample = 0;
int GLAD_GL_EXT_framebuffer_blit = 0;
int GLAD_GL_EXT_blend_func_separate = 0;
int GLAD_GL_EXT_blend_equation_separate = 0;
int GLAD_GL_ARB_vertex_shader = 0;
int GLAD_GL_ARB_vertex_program = 0;
int GLAD_GL_ARB_vertex_buffer_object = 0;
int GLAD_GL_ARB_vertex_array_object = 0;
int GLAD_GL_ARB_uniform_buffer_object = 0;
int GLAD_GL_ARB_timer_query = 0;
int GLAD_GL_ARB_texture_storage = 0;
int GLAD_GL_ARB_texture_rectangle = 0;
int GLAD_GL_ARB_texture_query_lod = 0;
int GLAD_GL_ARB_texture_non_power_of_two = 0;
int GLAD_GL_ARB_texture_float = 0;
int GLAD_GL_ARB_texture_env_combine = 0;
int GLAD_GL_ARB_texture_compression = 0;
int GLAD_GL_ARB_sync = 0;
int GLAD_GL_ARB_shading_language_420pack = 0;
int GLAD_GL_ARB_shading_language_100 = 0;
int GLAD_GL_ARB_shader_storage_buffer_object = 0;
int GLAD_GL_ARB_seamless_cube_map = 0;
int GLAD_GL_ARB_occlusion_query = 0;
int GLAD_GL_ARB_multi_draw_indirect = 0;
int GLAD_GL_ARB_multitexture = 0;
int GLAD_GL_ARB_multisample = 0;
int GLAD_GL_ARB_map_buffer_range = 0;
int GLAD_GL_ARB_invalidate_subdata = 0;
int GLAD_GL_ARB_instanced_arrays = 0;
int GLAD_GL_ARB_imaging = 0;
int GLAD_GL_ARB_framebuffer_object = 0;
int GLAD_GL_ARB_fragment_shader = 0;
int GLAD_GL_ARB_fragment_program = 0;
int GLAD_GL_ARB_explicit_attrib_location = 0;
int GLAD_GL_ARB_draw_elements_base_vertex = 0;
int GLAD_GL_ARB_draw_buffers = 0;
int GLAD_GL_ARB_depth_clamp = 0;
int GLAD_GL_ARB_copy_image = 0;
int GLAD_GL_ARB_copy_buffer = 0;
int GLAD_GL_ARB_conservative_depth = 0;
int GLAD_GL_ARB_clip_control = 0;
int GLAD_GL_ARB_buffer_storage = 0;

void APIENTRY impl_glClientActiveTextureARB(GLenum texture) {}
void APIENTRY impl_glClientActiveTexture(GLenum texture) {}
void APIENTRY impl_glActiveTexture(GLenum texture) {}
void APIENTRY impl_glActiveTextureARB(GLenum texture) {}
void APIENTRY impl_glDeleteFramebuffersEXT(GLsizei n, const GLuint* framebuffers) {}
void APIENTRY impl_glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers) {}
void APIENTRY impl_glBindFramebufferEXT(GLenum target, GLuint framebuffer) {}
void APIENTRY impl_glBindFramebuffer(GLenum target, GLuint framebuffer) {}
void APIENTRY impl_glFramebufferTexture2DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
void APIENTRY impl_glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
void APIENTRY impl_glFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {}
void APIENTRY impl_glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {}
void APIENTRY impl_glFramebufferTextureLayerEXT(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {}
void APIENTRY impl_glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint* params) {}


void APIENTRY impl_glDrawBuffers(GLsizei n, const GLenum* bufs) {}

void APIENTRY impl_glDrawBuffersARB(GLsizei n, const GLenum* bufs) {}
void APIENTRY impl_glDeleteBuffersARB(GLsizei n, const GLuint* buffers) {}
void APIENTRY impl_glGenBuffersARB(GLsizei n, GLuint* buffers) {}
void APIENTRY impl_glBindBufferARB(GLenum target, GLuint buffer) {}
void APIENTRY impl_glBufferDataARB(GLenum target, GLsizeiptrARB size, const GLvoid* data, GLenum usage) {}

void APIENTRY impl_glGenFramebuffersEXT(GLsizei n, GLuint* framebuffers) {}
void APIENTRY impl_glGenFramebuffers(GLsizei n, GLuint* framebuffers) {}
GLenum APIENTRY impl_glCheckFramebufferStatusEXT(GLenum target) {
    return 0;
}
GLenum APIENTRY impl_glCheckFramebufferStatus(GLenum target) {
    return 0;
}
void APIENTRY impl_glBlitFramebufferEXT(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {}
void APIENTRY impl_glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {}
void APIENTRY impl_glDeleteQueries(GLsizei n, const GLuint* ids) {}

void APIENTRY impl_glUseProgram(GLuint program) {}
GLuint APIENTRY impl_glCreateProgram() { return 0; }
//glCreateProgram = (PFNGLCREATEPROGRAMPROC) nullptr;
void APIENTRY impl_glDeleteProgram(GLuint program) {}
void APIENTRY impl_glProgramParameteri(GLuint program, GLenum pname, GLint value) {}
void APIENTRY impl_glProgramParameteriEXT(GLuint program, GLenum pname, GLint value) {}
void APIENTRY impl_glLinkProgram(GLuint program) {}
void APIENTRY impl_glGetProgramiv(GLuint program, GLenum pname, GLint* params) {}
void APIENTRY impl_glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {}

void APIENTRY impl_glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t) {}
void APIENTRY impl_glPointParameterf(GLenum pname, GLfloat param) {}
void APIENTRY impl_glPointParameterfv(GLenum pname, const GLfloat* params) {}
void APIENTRY impl_glMultiTexCoord3f(GLenum target, GLfloat s, GLfloat t, GLfloat r) {}
void APIENTRY impl_glFogCoordf(GLfloat coord) {}
void APIENTRY impl_glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {}
void APIENTRY impl_glBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {}
void APIENTRY impl_glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {}
void APIENTRY impl_glStencilFuncSeparate(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask) {}
void APIENTRY impl_glStencilMaskSeparate(GLenum face, GLuint mask) {}
void APIENTRY impl_glGenerateMipmapEXT(GLenum target) {}
void APIENTRY impl_glGenQueries(GLsizei n, GLuint* ids) {}
void APIENTRY impl_glBeginQuery(GLenum target, GLuint id) {}
void APIENTRY impl_glEndQuery(GLenum target) {}
void APIENTRY impl_glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params) {}
void APIENTRY impl_glGetQueryObjectiv(GLuint id, GLenum pname, GLint* params) {}
void APIENTRY impl_glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64* params) {}
void APIENTRY impl_glQueryCounter(GLuint id, GLenum target) {}
GLint APIENTRY impl_glGetUniformLocation(GLuint program, const GLchar* name) {
    return 0;
}
void APIENTRY impl_glUniform1f(GLint location, GLfloat v0) {}
void APIENTRY impl_glUniform2f(GLint location, GLfloat v0, GLfloat v1) {}
void APIENTRY impl_glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {}
void APIENTRY impl_glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {}
void APIENTRY impl_glUniform1i(GLint location, GLint v0) {}
void APIENTRY impl_glUniform2i(GLint location, GLint v0, GLint v1) {}
void APIENTRY impl_glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {}
void APIENTRY impl_glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {}
void APIENTRY impl_glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {}
void APIENTRY impl_glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {}
void APIENTRY impl_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {}
void APIENTRY impl_glUniform1fv(GLint location, GLsizei count, const GLfloat* value) {}
void APIENTRY impl_glUniform2fv(GLint location, GLsizei count, const GLfloat* value) {}
void APIENTRY impl_glUniform3fv(GLint location, GLsizei count, const GLfloat* value) {}
void APIENTRY impl_glUniform4fv(GLint location, GLsizei count, const GLfloat* value) {}
void APIENTRY impl_glUniform1iv(GLint location, GLsizei count, const GLint* value) {}
void APIENTRY impl_glUniform2iv(GLint location, GLsizei count, const GLint* value) {}
void APIENTRY impl_glUniform3iv(GLint location, GLsizei count, const GLint* value) {}
void APIENTRY impl_glUniform4iv(GLint location, GLsizei count, const GLint* value) {}

//Subroutines
void APIENTRY impl_glUniformSubroutinesuiv(GLenum shadertype, GLsizei count, const GLuint* indices) {}
GLuint APIENTRY impl_glGetSubroutineIndex(GLuint program, GLenum shadertype, const GLchar* name) { return -1; }
//Tesselation
void APIENTRY impl_glPatchParameteri(GLenum pname, GLint value) {}
void APIENTRY impl_glPatchParameterfv(GLenum pname, const GLfloat* values) {}

void APIENTRY impl_glBindRenderbufferEXT(GLenum target, GLuint renderbuffer) {}
void APIENTRY impl_glBindRenderbuffer(GLenum target, GLuint renderbuffer) {}
void APIENTRY impl_glDeleteRenderbuffersEXT(GLsizei n, const GLuint* renderbuffers) {}
void APIENTRY impl_glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers) {}
void APIENTRY impl_glGenRenderbuffersEXT(GLsizei n, GLuint* renderbuffers) {}
void APIENTRY impl_glGenRenderbuffers(GLsizei n, GLuint* renderbuffers) {}
void APIENTRY impl_glRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {}
void APIENTRY impl_glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {}
void APIENTRY impl_glRenderbufferStorageMultisampleEXT(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {}
void APIENTRY impl_glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {}
void APIENTRY impl_glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {}
GLboolean APIENTRY impl_glIsRenderbufferEXT(GLuint renderbuffer) {
    return GL_FALSE;
}
GLboolean APIENTRY impl_glIsRenderbuffer(GLuint renderbuffer) {
    return GL_FALSE;
}
void APIENTRY impl_glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) {}
void APIENTRY impl_glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params) {
    for (GLsizei i = 0; i < uniformCount; ++i)
        params[i] = 0;
}
void APIENTRY impl_glGetQueryiv(GLenum target, GLenum pname, GLint* params) {}
void APIENTRY impl_glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {}

void APIENTRY impl_glGenerateMipmap(GLenum target) {}
void APIENTRY impl_glGetUniformfv(GLuint program, GLint location, GLfloat* params) {}
void APIENTRY impl_glGetUniformiv(GLuint program, GLint location, GLint* params) {}
void APIENTRY impl_glGetUniformuiv(GLuint program, GLint location, GLuint* params) {}
void APIENTRY impl_glUniform1uiv(GLint location, GLsizei count, const GLuint* value) {}
void APIENTRY impl_glUniform2uiv(GLint location, GLsizei count, const GLuint* value) {}
void APIENTRY impl_glUniform3uiv(GLint location, GLsizei count, const GLuint* value) {}
void APIENTRY impl_glUniform4uiv(GLint location, GLsizei count, const GLuint* value) {}
void APIENTRY impl_glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) {}
GLint APIENTRY impl_glGetAttribLocation(GLuint program, const GLchar* name) { return -1; }
void APIENTRY impl_glBindAttribLocation(GLuint program, GLuint index, const GLchar* name) {}
GLboolean APIENTRY impl_glIsShader(GLuint shader) { return GL_FALSE; }
GLboolean APIENTRY impl_glIsProgram(GLuint shader) { return GL_FALSE; }

void APIENTRY impl_glDetachShader(GLuint program, GLuint shader) {}
void APIENTRY impl_glAttachShader(GLuint program, GLuint shader) {}
void APIENTRY impl_glDeleteShader(GLuint shader) {}
GLuint APIENTRY impl_glCreateShader(GLenum type) {
    return 0;
}
//glCreateShader = (PFNGLCREATESHADERPROC) nullptr;
void APIENTRY impl_glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* source) {}
void APIENTRY impl_glCompileShader(GLuint shader) {}
void APIENTRY impl_glGetShaderiv(GLuint shader, GLenum pname, GLint* params) {}
void APIENTRY impl_glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {}
void APIENTRY impl_glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) {}
void APIENTRY impl_glShaderSourceARB(GLhandleARB shaderObj, GLsizei count, const GLcharARB** string, const GLint* length) {}

void APIENTRY impl_glUniform1fARB(GLint location, GLfloat v0) {}
void APIENTRY impl_glUniform2fARB(GLint location, GLfloat v0, GLfloat v1) {}
void APIENTRY impl_glUniform3fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {}
void APIENTRY impl_glUniform4fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {}
void APIENTRY impl_glUniform1iARB(GLint location, GLint v0) {}
void APIENTRY impl_glUniform2iARB(GLint location, GLint v0, GLint v1) {}
void APIENTRY impl_glUniform3iARB(GLint location, GLint v0, GLint v1, GLint v2) {}
void APIENTRY impl_glUniform4iARB(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {}
void APIENTRY impl_glUniform1fvARB(GLint location, GLsizei count, const GLfloat* value) {}
void APIENTRY impl_glUniform2fvARB(GLint location, GLsizei count, const GLfloat* value) {}
void APIENTRY impl_glUniform3fvARB(GLint location, GLsizei count, const GLfloat* value) {}
void APIENTRY impl_glUniform4fvARB(GLint location, GLsizei count, const GLfloat* value) {}
void APIENTRY impl_glUniform1ivARB(GLint location, GLsizei count, const GLint* value) {}
void APIENTRY impl_glUniform2ivARB(GLint location, GLsizei count, const GLint* value) {}
void APIENTRY impl_glUniform3ivARB(GLint location, GLsizei count, const GLint* value) {}
void APIENTRY impl_glUniform4ivARB(GLint location, GLsizei count, const GLint* value) {}
void APIENTRY impl_glUniformMatrix2fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {}
void APIENTRY impl_glUniformMatrix3fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {}
void APIENTRY impl_glUniformMatrix4fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {}

void APIENTRY impl_glDeleteObjectARB(GLhandleARB obj) {}
GLhandleARB APIENTRY impl_glGetHandleARB(GLenum pname) {
    return 0;
}
void APIENTRY impl_glDetachObjectARB(GLhandleARB containerObj, GLhandleARB attachedObj) {}
GLhandleARB APIENTRY impl_glCreateShaderObjectARB(GLenum shaderType) {
    return 0;
}

// headless include/GL is behind mingwlibs, avoid signature conflict
// void APIENTRY impl_glShaderSourceARB(GLhandleARB shaderObj, GLsizei count, const GLcharARB**string, const GLint *length) {}
void APIENTRY impl_glCompileShaderARB(GLhandleARB shaderObj) {}
GLhandleARB APIENTRY impl_glCreateProgramObjectARB() {
    return 0;
}
void APIENTRY impl_glAttachObjectARB(GLhandleARB containerObj, GLhandleARB obj) {}

void APIENTRY impl_glLinkProgramARB(GLhandleARB programObj) {}
void APIENTRY impl_glUseProgramObjectARB(GLhandleARB programObj) {}
void APIENTRY impl_glValidateProgramARB(GLhandleARB programObj) {}

void APIENTRY impl_glGetObjectParameterfvARB(GLhandleARB obj, GLenum pname, GLfloat* params) {}
void APIENTRY impl_glGetObjectParameterivARB(GLhandleARB obj, GLenum pname, GLint* params) {}
void APIENTRY impl_glGetInfoLogARB(GLhandleARB obj, GLsizei maxLength, GLsizei* length, GLcharARB* infoLog) {}
void APIENTRY impl_glGetAttachedObjectsARB(GLhandleARB containerObj, GLsizei maxCount, GLsizei* count, GLhandleARB* obj) {}

GLint APIENTRY impl_glGetUniformLocationARB(GLhandleARB programObj, const GLcharARB* name) {
    return 0;
}

void APIENTRY impl_glBindAttribLocationARB(GLhandleARB programObj, GLuint index, const GLcharARB* name) {}
void APIENTRY impl_glGetActiveAttribARB(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei* length, GLint* size, GLenum* type, GLcharARB* name) {}
GLint APIENTRY impl_glGetAttribLocationARB(GLhandleARB programObj, const GLcharARB* name) {
    return 0;
}

void APIENTRY impl_glVertexAttribPointerARB(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer) {}
void APIENTRY impl_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer) {}
void APIENTRY impl_glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) {}
void APIENTRY impl_glVertexAttribDivisor(GLuint index, GLuint divisor) {}
void APIENTRY impl_glEnableVertexAttribArrayARB(GLuint index) {}
void APIENTRY impl_glEnableVertexAttribArray(GLuint index) {}
void APIENTRY impl_glDisableVertexAttribArrayARB(GLuint index) {}
void APIENTRY impl_glDisableVertexAttribArray(GLuint index) {}

void APIENTRY impl_glCompressedTexImage3DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data) {}
void APIENTRY impl_glCompressedTexImage2DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data) {}
void APIENTRY impl_glCompressedTexImage1DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid* data) {}
void APIENTRY impl_glCompressedTexSubImage3DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data) {}
void APIENTRY impl_glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data) {}
void APIENTRY impl_glCompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data) {}
void APIENTRY impl_glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data) {}
void APIENTRY impl_glCompressedTexSubImage1DARB(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid* data) {}
void APIENTRY impl_glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid* data) {}
void APIENTRY impl_glGetCompressedTexImageARB(GLenum target, GLint level, GLvoid* img) {}
void APIENTRY impl_glGetCompressedTexImage(GLenum target, GLint level, GLvoid* img) {}

void APIENTRY impl_glProgramStringARB(GLenum target, GLenum format, GLsizei len, const GLvoid* string) {}
void APIENTRY impl_glBindProgramARB(GLenum target, GLuint program) {}
void APIENTRY impl_glDeleteProgramsARB(GLsizei n, const GLuint* programs) {}
void APIENTRY impl_glGenProgramsARB(GLsizei n, GLuint* programs) {}
//glGenProgramsARB = (PFNGLGENPROGRAMSARBPROC) nullptr;

void APIENTRY impl_glProgramEnvParameter4dARB(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {}
void APIENTRY impl_glProgramEnvParameter4dvARB(GLenum target, GLuint index, const GLdouble* params) {}
void APIENTRY impl_glProgramEnvParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {}
void APIENTRY impl_glProgramEnvParameter4fvARB(GLenum target, GLuint index, const GLfloat* params) {}

void APIENTRY impl_glProgramLocalParameter4dARB(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {}
void APIENTRY impl_glProgramLocalParameter4dvARB(GLenum target, GLuint index, const GLdouble* params) {}
void APIENTRY impl_glProgramLocalParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {}
void APIENTRY impl_glProgramLocalParameter4fvARB(GLenum target, GLuint index, const GLfloat* params) {}

void APIENTRY impl_glGetProgramEnvParameterdvARB(GLenum target, GLuint index, GLdouble* params) {}
void APIENTRY impl_glGetProgramEnvParameterfvARB(GLenum target, GLuint index, GLfloat* params) {}
void APIENTRY impl_glGetProgramLocalParameterdvARB(GLenum target, GLuint index, GLdouble* params) {}
void APIENTRY impl_glGetProgramLocalParameterfvARB(GLenum target, GLuint index, GLfloat* params) {}

void APIENTRY impl_glMultiTexCoord2iARB(GLenum target, GLint s, GLint t) {}
void APIENTRY impl_glMultiTexCoord2ivARB(GLenum target, const GLint* v) {}

void APIENTRY impl_glBindBuffer(GLenum target, GLuint buffer) {}
void APIENTRY impl_glDeleteBuffers(GLsizei n, const GLuint* buffers) {}
void APIENTRY impl_glGenBuffers(GLsizei n, GLuint* buffers) {}
void APIENTRY impl_glBufferStorage(GLenum target, GLsizeiptr size, const void* data, GLbitfield flags) {}
void APIENTRY impl_glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) {}
void APIENTRY impl_glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) {}
void APIENTRY impl_glCopyBufferSubData(GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size) {}
void APIENTRY impl_glClearBufferData(GLenum target, GLenum internalformat, GLenum format, GLenum type, const void* data) {}
void APIENTRY impl_glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {}
void APIENTRY impl_glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {}
GLboolean APIENTRY impl_glIsBuffer(GLuint buffer) { return GL_TRUE; }
void APIENTRY impl_glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params) { *params = 0; }

void APIENTRY impl_glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {}
void APIENTRY impl_glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {}

void APIENTRY impl_glGenVertexArrays(GLsizei n, GLuint* arrays) {}
void APIENTRY impl_glBindVertexArray(GLuint array) {}
void APIENTRY impl_glDeleteVertexArrays(GLsizei n, const GLuint* arrays) {}

GLvoid* APIENTRY impl_glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    return (GLvoid*)NULL;
}
void APIENTRY impl_glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) {}
GLvoid* APIENTRY impl_glMapBuffer(GLenum target, GLenum access) {
    return (GLvoid*)NULL;
}
GLboolean APIENTRY impl_glUnmapBuffer(GLenum target) {
    return GL_FALSE;
}

GLsync APIENTRY impl_glFenceSync(GLenum condition, GLbitfield flags) {
    static GLsync dummy;
    return dummy;
}
GLenum APIENTRY impl_glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) { return GL_ALREADY_SIGNALED; }
void APIENTRY impl_glDeleteSync(GLsync sync) {}
GLboolean APIENTRY impl_glIsSync(GLsync sync) { return GL_TRUE; }

void APIENTRY impl_glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {}
void APIENTRY impl_glMemoryBarrier(GLbitfield barriers) {}

void APIENTRY impl_glMultiTexCoord2i(GLenum target, GLint s, GLint t) {}
void APIENTRY impl_glMultiTexCoord2iv(GLenum target, const GLint* v) {}

void APIENTRY impl_glDeleteFencesNV(GLsizei n, const GLuint* fences) {}
void APIENTRY impl_glGenFencesNV(GLsizei n, GLuint* fences) {}
GLboolean APIENTRY impl_glIsFenceNV(GLuint fence) {
    return GL_FALSE;
}
GLboolean APIENTRY impl_glTestFenceNV(GLuint fence) {
    return GL_FALSE;
}
void APIENTRY impl_glGetFenceivNV(GLuint fence, GLenum pname, GLint* params) {}
void APIENTRY impl_glFinishFenceNV(GLuint fence) {}
void APIENTRY impl_glSetFenceNV(GLuint fence, GLenum condition) {}

void APIENTRY impl_glGetRenderbufferParameterivEXT(GLenum target, GLenum pname, GLint* params) {}
void APIENTRY impl_glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params) {}
GLboolean APIENTRY impl_glIsFramebufferEXT(GLuint framebuffer) {
    return GL_FALSE;
}
void APIENTRY impl_glFramebufferTextureEXT(GLenum target, GLenum attachment, GLuint texture, GLint level) {}
void APIENTRY impl_glFramebufferTexture1DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
void APIENTRY impl_glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
void APIENTRY impl_glFramebufferTexture3DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) {}
void APIENTRY impl_glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) {}
void APIENTRY impl_glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level) {}
void APIENTRY impl_glGetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment, GLenum pname, GLint* params) {}
void APIENTRY impl_glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params) {}

void APIENTRY impl_glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid* indices) {}
void APIENTRY impl_glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels) {}
void APIENTRY impl_glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* pixels) {}
void APIENTRY impl_glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {}

void APIENTRY impl_glGetProgramivARB(GLenum target, GLenum pname, GLint* params) {}
void APIENTRY impl_glValidateProgram(GLuint program) {}

void APIENTRY impl_glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {}
void APIENTRY impl_glEnable(GLenum i) {}
GLboolean APIENTRY impl_glIsEnabled(GLenum cap) { return GL_FALSE; }
void APIENTRY impl_glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {}
void APIENTRY impl_glColor3fv(const GLfloat* v) {}
void APIENTRY impl_glColor3ub(GLubyte red, GLubyte green, GLubyte blue) {}
void APIENTRY impl_glColor3ubv(const GLubyte* v) {}
void APIENTRY impl_glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {}
void APIENTRY impl_glColor4ubv(const GLubyte* v) {}

void APIENTRY impl_glCopyTexImage2D(GLenum target, GLint level,
    GLenum internalformat,
    GLint x, GLint y,
    GLsizei width, GLsizei height,
    GLint border) {}

void APIENTRY impl_glCopyTexSubImage2D(GLenum target, GLint level,
    GLint xoffset, GLint yoffset,
    GLint x, GLint y,
    GLsizei width, GLsizei height) {}

void APIENTRY impl_glCopyImageSubData(GLuint srcName, GLenum srcTarget,
    GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ,
    GLuint dstName, GLenum dstTarget, GLint dstLevel,
    GLint dstX, GLint dstY, GLint dstZ,
    GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) {}

void APIENTRY impl_glDrawBuffer(GLenum mode) {}
void APIENTRY impl_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) {}
void APIENTRY impl_glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLint basevertex) {}
void APIENTRY impl_glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei primcount, GLuint baseinstance) {}
void APIENTRY impl_glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei primcount) {}
void APIENTRY impl_glDrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei instancecount, GLint basevertex) {}
void APIENTRY impl_glDrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance) {}
void APIENTRY impl_glDrawElementsIndirect(GLenum mode, GLenum type, const void* indirect) {};
void APIENTRY impl_glMultiDrawElementsIndirect(GLenum mode, GLenum type, const void* indirect, GLsizei primcount, GLsizei stride) {};
void APIENTRY impl_glEdgeFlag(GLboolean flag) {}
void APIENTRY impl_glEvalCoord1f(GLfloat u) {}
void APIENTRY impl_glEvalCoord2f(GLfloat u, GLfloat v) {}
void APIENTRY impl_glEvalMesh1(GLenum mode, GLint i1, GLint i2) {}
void APIENTRY impl_glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) {}
void APIENTRY impl_glEvalPoint1(GLint i) {}
void APIENTRY impl_glEvalPoint2(GLint i, GLint j) {}
void APIENTRY impl_glFinish() {}
void APIENTRY impl_glFlush() {}
void APIENTRY impl_glFrontFace(GLenum mode) {}

void APIENTRY impl_glFrustum(GLdouble left, GLdouble right,
    GLdouble bottom, GLdouble top,
    GLdouble near_val, GLdouble far_val) {}

void APIENTRY impl_glGetFloatv(GLenum pname, GLfloat* params) {
    *params = 0;
}

void APIENTRY impl_glGetTexImage(GLenum target, GLint level,
    GLenum format, GLenum type,
    GLvoid* pixels) {}

void APIENTRY impl_glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params) {}

void APIENTRY impl_glInitNames() {}

GLboolean APIENTRY impl_glIsTexture(GLuint texture) {
    return 0;
}

void APIENTRY impl_glLightf(GLenum light, GLenum pname, GLfloat param) {}
void APIENTRY impl_glLightModelfv(GLenum pname, const GLfloat* params) {}
void APIENTRY impl_glLoadMatrixf(const GLfloat* m) {}
void APIENTRY impl_glLoadName(GLuint name) {}

void APIENTRY impl_glMap1f(GLenum target, GLfloat u1, GLfloat u2,
    GLint stride,
    GLint order, const GLfloat* points) {}

void APIENTRY impl_glMap2f(GLenum target,
    GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
    GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
    const GLfloat* points) {}

void APIENTRY impl_glMapGrid1f(GLint un, GLfloat u1, GLfloat u2) {}
void APIENTRY impl_glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2) {}

void APIENTRY impl_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) {}
void APIENTRY impl_glNormal3fv(const GLfloat* v) {}

void APIENTRY impl_glPixelStorei(GLenum pname, GLint param) {}
void APIENTRY impl_glPushName(GLuint name) {}
void APIENTRY impl_glPopName() {}
void APIENTRY impl_glReadBuffer(GLenum mode) {}

void APIENTRY impl_glReadPixels(GLint x, GLint y,
    GLsizei width, GLsizei height,
    GLenum format, GLenum type,
    GLvoid* pixels) {}

void APIENTRY impl_glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {}
GLint APIENTRY impl_glRenderMode(GLenum mode) {
    return 0;
}

void APIENTRY impl_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {}
void APIENTRY impl_glScalef(GLfloat x, GLfloat y, GLfloat z) {}
void APIENTRY impl_glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {}
void APIENTRY impl_glSelectBuffer(GLsizei size, GLuint* buffer) {}
void APIENTRY impl_glTexCoord1f(GLfloat s) {}
void APIENTRY impl_glTexCoord2fv(const GLfloat* v) {}
void APIENTRY impl_glTexCoord3f(GLfloat s, GLfloat t, GLfloat r) {}
void APIENTRY impl_glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q) {}
void APIENTRY impl_glTexEnvfv(GLenum target, GLenum pname, const GLfloat* params) {}
void APIENTRY impl_glTexGenf(GLenum coord, GLenum pname, GLfloat param) {}

void APIENTRY impl_glTexImage1D(GLenum target, GLint level,
    GLint internalFormat,
    GLsizei width, GLint border,
    GLenum format, GLenum type,
    const GLvoid* pixels) {
}

void APIENTRY impl_glVertex3fv(const GLfloat* v) {}
void APIENTRY impl_glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {}
void APIENTRY impl_glClipPlane(GLenum plane, const GLdouble* equation) {}
void APIENTRY impl_glClipPlanef(GLenum plane, const GLfloat* equation) {}
void APIENTRY impl_glMatrixMode(GLenum mode) {}

void APIENTRY impl_glGetBooleanv(GLenum pname, GLboolean* params) {
    *params = 0;
}

void APIENTRY impl_glLoadMatrixd(const GLdouble* m) {}
void APIENTRY impl_glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {}

void APIENTRY impl_glNewList(GLuint list, GLenum mode) {}
void APIENTRY impl_glStencilFunc(GLenum func, GLint ref, GLuint mask) {}
void APIENTRY impl_glStencilMask(GLuint mask) {}
void APIENTRY impl_glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {}
void APIENTRY impl_glBlendEquation(GLenum mode) {}
void APIENTRY impl_glEnableClientState(GLenum cap) {}
void APIENTRY impl_glDisableClientState(GLenum cap) {}
void APIENTRY impl_glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* ptr) {}
void APIENTRY impl_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* ptr) {}
void APIENTRY impl_glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* ptr) {}
void APIENTRY impl_glNormalPointer(GLenum type, GLsizei stride, const GLvoid* ptr) {}
void APIENTRY impl_glDrawArrays(GLenum mode, GLint first, GLsizei count) {}
void APIENTRY impl_glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {}
void APIENTRY impl_glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance) {}

void APIENTRY impl_glTexEnvi(GLenum target, GLenum pname, GLint param) {}

void APIENTRY impl_glMultMatrixd(const GLdouble* m) {}
void APIENTRY impl_glMultMatrixf(const GLfloat* m) {}
void APIENTRY impl_glTexGeni(GLenum coord, GLenum pname, GLint param) {}
void APIENTRY impl_glTexGenfv(GLenum coord, GLenum pname, const GLfloat* params) {}
void APIENTRY impl_glLightModeli(GLenum pname, GLint param) {}
void APIENTRY impl_glMaterialfv(GLenum face, GLenum pname, const GLfloat* params) {}
void APIENTRY impl_glMaterialf(GLenum face, GLenum pname, GLfloat param) {}
void APIENTRY impl_glPointSize(GLfloat size) {}
void APIENTRY impl_glCullFace(GLenum mode) {}
void APIENTRY impl_glLogicOp(GLenum opcode) {}
void APIENTRY impl_glEndList() {}
GLuint APIENTRY impl_glGenLists(GLsizei range) {
    return 0;
}

void APIENTRY impl_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {}
void APIENTRY impl_glLoadIdentity() {}
void APIENTRY impl_glOrtho(GLdouble left, GLdouble right,
    GLdouble bottom, GLdouble top,
    GLdouble near_val, GLdouble far_val) {}

void APIENTRY impl_glGenTextures(GLsizei n, GLuint* textures) {}

void APIENTRY impl_glBindTexture(GLenum target, GLuint texture) {}
void APIENTRY impl_glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data) {}

void APIENTRY impl_glTexParameteri(GLenum target, GLenum pname, GLint param) {}
void APIENTRY impl_glTexParameterf(GLenum target, GLenum pname, GLfloat param) {}
void APIENTRY impl_glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params) {}
void APIENTRY impl_glTexParameteriv(GLenum target, GLenum pname, const GLint* params) {}

void APIENTRY impl_glTexImage2D(GLenum target, GLint level,
    GLint internalFormat,
    GLsizei width, GLsizei height,
    GLint border, GLenum format, GLenum type,
    const GLvoid* pixels) {
    // printf( "glTexImage2D\n" );
}

void APIENTRY impl_glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {}

void APIENTRY impl_glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) {}

void APIENTRY impl_glClear(GLbitfield mask) {}
void APIENTRY impl_glTexCoord2i(GLint s, GLint t) {}
void APIENTRY impl_glVertex2f(GLfloat x, GLfloat y) {}
void APIENTRY impl_glVertex3f(GLfloat x, GLfloat y, GLfloat z) {}
void APIENTRY impl_glBegin(GLenum mode) {}
void APIENTRY impl_glEnd() {}

void APIENTRY impl_glDeleteTextures(GLsizei n, const GLuint* textures) {}

void APIENTRY impl_glGetIntegerv(GLenum pname, GLint* params) {}
void APIENTRY impl_glGetIntegeri_v(GLenum target, GLuint index, GLint* data) {}
void APIENTRY impl_glDepthFunc(GLenum func) {}
void APIENTRY impl_glDepthRangef(GLclampf n, GLclampf f) {}
void APIENTRY impl_glShadeModel(GLenum mode) {}

void APIENTRY impl_glHint(GLenum target, GLenum mode) {}
void APIENTRY impl_glTexEnvf(GLenum target, GLenum pname, GLfloat param) {}

const GLubyte* APIENTRY impl_glGetString(GLenum name) {
    return (const GLubyte*)"";
}

void APIENTRY impl_glClearStencil(GLint s) {}
void APIENTRY impl_glLightfv(GLenum light, GLenum pname, const GLfloat* params) {}

void APIENTRY impl_glDeleteLists(GLuint list, GLsizei range) {}
void APIENTRY impl_glDisable(GLenum i) {}
void APIENTRY impl_glClearDepth(GLclampd depth) {}
void APIENTRY impl_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {}
void APIENTRY impl_glSecondaryColor3f(GLfloat red, GLfloat green, GLfloat blue) {}

void APIENTRY impl_glMultiTexCoord1f(GLenum target, GLfloat s) {}
void APIENTRY impl_glMultiTexCoord2f(GLenum target, GLfloat s, GLfloat t) {}
void APIENTRY impl_glMultiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {}

GLenum APIENTRY impl_glGetError(void) {
    return 0;
}

void APIENTRY impl_glPolygonMode(GLenum face, GLenum mode) {}
void APIENTRY impl_glBlendFunc(GLenum sfactor, GLenum dfactor) {}
void APIENTRY impl_glTranslatef(GLfloat x, GLfloat y, GLfloat z) {}
void APIENTRY impl_glColor4fv(const GLfloat* v) {}
void APIENTRY impl_glLineStipple(GLint factor, GLushort pattern) {}
void APIENTRY impl_glPopAttrib() {}
void APIENTRY impl_glPushAttrib(GLbitfield mask) {}
void APIENTRY impl_glDepthMask(GLboolean flag) {}
void APIENTRY impl_glAlphaFunc(GLenum func, GLclampf ref) {}

void APIENTRY impl_glFogfv(GLenum pname, const GLfloat* params) {}
void APIENTRY impl_glFogf(GLenum pname, GLfloat param) {}
void APIENTRY impl_glFogi(GLenum pname, GLint param) {}
void APIENTRY impl_glPushMatrix() {}
void APIENTRY impl_glPopMatrix() {}
void APIENTRY impl_glCallList(GLuint list) {}

void APIENTRY impl_glTexSubImage2D(GLenum target, GLint level,
    GLint xoffset, GLint yoffset,
    GLsizei width, GLsizei height,
    GLenum format, GLenum type,
    const GLvoid* pixels) {}

void APIENTRY impl_glTexCoord2f(GLfloat s, GLfloat t) {}
void APIENTRY impl_glLineWidth(GLfloat width) {}
void APIENTRY impl_glColor3f(GLfloat red, GLfloat green, GLfloat blue) {}
void APIENTRY impl_glPolygonOffset(GLfloat factor, GLfloat units) {}

void APIENTRY impl_glPrimitiveRestartIndexNV(GLuint index) {}
void APIENTRY impl_glPrimitiveRestartIndex(GLuint index) {}
void APIENTRY impl_glDepthRange(GLdouble nearVal, GLdouble farVal) {}

void APIENTRY impl_glBeginConditionalRender(GLuint id, GLenum mode) {}
void APIENTRY impl_glEndConditionalRender(void) {}

void APIENTRY impl_glInvalidateBufferData(GLuint buffer) {}
void APIENTRY impl_glClipControl(GLenum origin, GLenum depth) {}


PFNGLACTIVETEXTUREPROC glad_glActiveTexture = NULL;
PFNGLALPHAFUNCPROC glad_glAlphaFunc = NULL;
PFNGLATTACHSHADERPROC glad_glAttachShader = NULL;
PFNGLBEGINPROC glad_glBegin = NULL;
PFNGLBEGINCONDITIONALRENDERPROC glad_glBeginConditionalRender = NULL;
PFNGLBEGINQUERYPROC glad_glBeginQuery = NULL;
PFNGLBINDATTRIBLOCATIONPROC glad_glBindAttribLocation = NULL;
PFNGLBINDBUFFERPROC glad_glBindBuffer = NULL;
PFNGLBINDBUFFERBASEPROC glad_glBindBufferBase = NULL;
PFNGLBINDBUFFERRANGEPROC glad_glBindBufferRange = NULL;
PFNGLBINDFRAMEBUFFERPROC glad_glBindFramebuffer = NULL;
PFNGLBINDIMAGETEXTUREPROC glad_glBindImageTexture = NULL;
PFNGLBINDRENDERBUFFERPROC glad_glBindRenderbuffer = NULL;
PFNGLBINDTEXTUREPROC glad_glBindTexture = NULL;
PFNGLBINDVERTEXARRAYPROC glad_glBindVertexArray = NULL;
PFNGLBLENDCOLORPROC glad_glBlendColor = NULL;
PFNGLBLENDEQUATIONPROC glad_glBlendEquation = NULL;
PFNGLBLENDEQUATIONSEPARATEPROC glad_glBlendEquationSeparate = NULL;
PFNGLBLENDFUNCPROC glad_glBlendFunc = NULL;
PFNGLBLENDFUNCSEPARATEPROC glad_glBlendFuncSeparate = NULL;
PFNGLBLENDFUNCSEPARATEIPROC glad_glBlendFuncSeparatei = NULL;
PFNGLBLITFRAMEBUFFERPROC glad_glBlitFramebuffer = NULL;
PFNGLBUFFERDATAPROC glad_glBufferData = NULL;
PFNGLBUFFERSTORAGEPROC glad_glBufferStorage = NULL;
PFNGLBUFFERSUBDATAPROC glad_glBufferSubData = NULL;
PFNGLCALLLISTPROC glad_glCallList = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glad_glCheckFramebufferStatus = NULL;
PFNGLCLEARPROC glad_glClear = NULL;
PFNGLCLEARACCUMPROC glad_glClearAccum = NULL;
PFNGLCLEARBUFFERDATAPROC glad_glClearBufferData = NULL;
PFNGLCLEARCOLORPROC glad_glClearColor = NULL;
PFNGLCLEARDEPTHPROC glad_glClearDepth = NULL;
PFNGLCLEARSTENCILPROC glad_glClearStencil = NULL;
PFNGLCLIENTACTIVETEXTUREPROC glad_glClientActiveTexture = NULL;
PFNGLCLIENTWAITSYNCPROC glad_glClientWaitSync = NULL;
PFNGLCLIPPLANEPROC glad_glClipPlane = NULL;
PFNGLCOLOR3FPROC glad_glColor3f = NULL;
PFNGLCOLOR3FVPROC glad_glColor3fv = NULL;
PFNGLCOLOR3UBPROC glad_glColor3ub = NULL;
PFNGLCOLOR3UBVPROC glad_glColor3ubv = NULL;
PFNGLCOLOR4FPROC glad_glColor4f = NULL;
PFNGLCOLOR4FVPROC glad_glColor4fv = NULL;
PFNGLCOLOR4UBPROC glad_glColor4ub = NULL;
PFNGLCOLOR4UBVPROC glad_glColor4ubv = NULL;
PFNGLCOLORMASKPROC glad_glColorMask = NULL;
PFNGLCOLORPOINTERPROC glad_glColorPointer = NULL;
PFNGLCOMPILESHADERPROC glad_glCompileShader = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DPROC glad_glCompressedTexImage2D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glad_glCompressedTexSubImage1D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glad_glCompressedTexSubImage2D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glad_glCompressedTexSubImage3D = NULL;
PFNGLCOPYBUFFERSUBDATAPROC glad_glCopyBufferSubData = NULL;
PFNGLCOPYIMAGESUBDATAPROC glad_glCopyImageSubData = NULL;
PFNGLCOPYTEXIMAGE2DPROC glad_glCopyTexImage2D = NULL;
PFNGLCOPYTEXSUBIMAGE2DPROC glad_glCopyTexSubImage2D = NULL;
PFNGLCOPYTEXSUBIMAGE3DPROC glad_glCopyTexSubImage3D = NULL;
PFNGLCREATEPROGRAMPROC glad_glCreateProgram = NULL;
PFNGLCREATESHADERPROC glad_glCreateShader = NULL;
PFNGLCULLFACEPROC glad_glCullFace = NULL;
PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers = NULL;
PFNGLDELETEFRAMEBUFFERSPROC glad_glDeleteFramebuffers = NULL;
PFNGLDELETELISTSPROC glad_glDeleteLists = NULL;
PFNGLDELETEPROGRAMPROC glad_glDeleteProgram = NULL;
PFNGLDELETEQUERIESPROC glad_glDeleteQueries = NULL;
PFNGLDELETERENDERBUFFERSPROC glad_glDeleteRenderbuffers = NULL;
PFNGLDELETESHADERPROC glad_glDeleteShader = NULL;
PFNGLDELETESYNCPROC glad_glDeleteSync = NULL;
PFNGLDELETETEXTURESPROC glad_glDeleteTextures = NULL;
PFNGLDELETEVERTEXARRAYSPROC glad_glDeleteVertexArrays = NULL;
PFNGLDEPTHFUNCPROC glad_glDepthFunc = NULL;
PFNGLDEPTHMASKPROC glad_glDepthMask = NULL;
PFNGLDEPTHRANGEPROC glad_glDepthRange = NULL;
PFNGLDEPTHRANGEFPROC glad_glDepthRangef = NULL;
PFNGLDETACHSHADERPROC glad_glDetachShader = NULL;
PFNGLDISABLEPROC glad_glDisable = NULL;
PFNGLDISABLECLIENTSTATEPROC glad_glDisableClientState = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glad_glDisableVertexAttribArray = NULL;
PFNGLDISPATCHCOMPUTEPROC glad_glDispatchCompute = NULL;
PFNGLDRAWARRAYSPROC glad_glDrawArrays = NULL;
PFNGLDRAWARRAYSINSTANCEDPROC glad_glDrawArraysInstanced = NULL;
PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC glad_glDrawArraysInstancedBaseInstance = NULL;
PFNGLDRAWBUFFERPROC glad_glDrawBuffer = NULL;
PFNGLDRAWBUFFERSPROC glad_glDrawBuffers = NULL;
PFNGLDRAWELEMENTSPROC glad_glDrawElements = NULL;
PFNGLDRAWELEMENTSBASEVERTEXPROC glad_glDrawElementsBaseVertex = NULL;
PFNGLDRAWELEMENTSINDIRECTPROC glad_glDrawElementsIndirect = NULL;
PFNGLDRAWELEMENTSINSTANCEDPROC glad_glDrawElementsInstanced = NULL;
PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC glad_glDrawElementsInstancedBaseInstance = NULL;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC glad_glDrawElementsInstancedBaseVertex = NULL;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC glad_glDrawElementsInstancedBaseVertexBaseInstance = NULL;
PFNGLDRAWRANGEELEMENTSPROC glad_glDrawRangeElements = NULL;
PFNGLEDGEFLAGPROC glad_glEdgeFlag = NULL;
PFNGLENABLEPROC glad_glEnable = NULL;
PFNGLENABLECLIENTSTATEPROC glad_glEnableClientState = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray = NULL;
PFNGLENDPROC glad_glEnd = NULL;
PFNGLENDCONDITIONALRENDERPROC glad_glEndConditionalRender = NULL;
PFNGLENDLISTPROC glad_glEndList = NULL;
PFNGLENDQUERYPROC glad_glEndQuery = NULL;
PFNGLEVALCOORD1FPROC glad_glEvalCoord1f = NULL;
PFNGLEVALCOORD2FPROC glad_glEvalCoord2f = NULL;
PFNGLEVALMESH1PROC glad_glEvalMesh1 = NULL;
PFNGLEVALMESH2PROC glad_glEvalMesh2 = NULL;
PFNGLEVALPOINT1PROC glad_glEvalPoint1 = NULL;
PFNGLEVALPOINT2PROC glad_glEvalPoint2 = NULL;
PFNGLFENCESYNCPROC glad_glFenceSync = NULL;
PFNGLFINISHPROC glad_glFinish = NULL;
PFNGLFLUSHPROC glad_glFlush = NULL;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC glad_glFlushMappedBufferRange = NULL;
PFNGLFOGCOORDFPROC glad_glFogCoordf = NULL;
PFNGLFOGFPROC glad_glFogf = NULL;
PFNGLFOGFVPROC glad_glFogfv = NULL;
PFNGLFOGIPROC glad_glFogi = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glad_glFramebufferRenderbuffer = NULL;
PFNGLFRAMEBUFFERTEXTUREPROC glad_glFramebufferTexture = NULL;
PFNGLFRAMEBUFFERTEXTURE1DPROC glad_glFramebufferTexture1D = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC glad_glFramebufferTexture2D = NULL;
PFNGLFRAMEBUFFERTEXTURE3DPROC glad_glFramebufferTexture3D = NULL;
PFNGLFRONTFACEPROC glad_glFrontFace = NULL;
PFNGLFRUSTUMPROC glad_glFrustum = NULL;
PFNGLGENBUFFERSPROC glad_glGenBuffers = NULL;
PFNGLGENFRAMEBUFFERSPROC glad_glGenFramebuffers = NULL;
PFNGLGENLISTSPROC glad_glGenLists = NULL;
PFNGLGENQUERIESPROC glad_glGenQueries = NULL;
PFNGLGENRENDERBUFFERSPROC glad_glGenRenderbuffers = NULL;
PFNGLGENTEXTURESPROC glad_glGenTextures = NULL;
PFNGLGENVERTEXARRAYSPROC glad_glGenVertexArrays = NULL;
PFNGLGENERATEMIPMAPPROC glad_glGenerateMipmap = NULL;
PFNGLGETACTIVEATTRIBPROC glad_glGetActiveAttrib = NULL;
PFNGLGETACTIVEUNIFORMPROC glad_glGetActiveUniform = NULL;
PFNGLGETACTIVEUNIFORMSIVPROC glad_glGetActiveUniformsiv = NULL;
PFNGLGETATTRIBLOCATIONPROC glad_glGetAttribLocation = NULL;
PFNGLGETBOOLEANVPROC glad_glGetBooleanv = NULL;
PFNGLGETBUFFERPARAMETERIVPROC glad_glGetBufferParameteriv = NULL;
PFNGLGETCOMPRESSEDTEXIMAGEPROC glad_glGetCompressedTexImage = NULL;
PFNGLGETERRORPROC glad_glGetError = NULL;
PFNGLGETFLOATVPROC glad_glGetFloatv = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glad_glGetFramebufferAttachmentParameteriv = NULL;
PFNGLGETFRAMEBUFFERPARAMETERIVPROC glad_glGetFramebufferParameteriv = NULL;
PFNGLGETINTEGERI_VPROC glad_glGetIntegeri_v = NULL;
PFNGLGETINTEGERVPROC glad_glGetIntegerv = NULL;
PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog = NULL;
PFNGLGETPROGRAMIVPROC glad_glGetProgramiv = NULL;
PFNGLGETQUERYOBJECTIVPROC glad_glGetQueryObjectiv = NULL;
PFNGLGETQUERYOBJECTUI64VPROC glad_glGetQueryObjectui64v = NULL;
PFNGLGETQUERYOBJECTUIVPROC glad_glGetQueryObjectuiv = NULL;
PFNGLGETQUERYIVPROC glad_glGetQueryiv = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVPROC glad_glGetRenderbufferParameteriv = NULL;
PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog = NULL;
PFNGLGETSHADERSOURCEPROC glad_glGetShaderSource = NULL;
PFNGLGETSHADERIVPROC glad_glGetShaderiv = NULL;
PFNGLGETSTRINGPROC glad_glGetString = NULL;
PFNGLGETSUBROUTINEINDEXPROC glad_glGetSubroutineIndex = NULL;
PFNGLGETTEXIMAGEPROC glad_glGetTexImage = NULL;
PFNGLGETTEXLEVELPARAMETERIVPROC glad_glGetTexLevelParameteriv = NULL;
PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation = NULL;
PFNGLGETUNIFORMFVPROC glad_glGetUniformfv = NULL;
PFNGLGETUNIFORMIVPROC glad_glGetUniformiv = NULL;
PFNGLGETUNIFORMUIVPROC glad_glGetUniformuiv = NULL;
PFNGLHINTPROC glad_glHint = NULL;
PFNGLINITNAMESPROC glad_glInitNames = NULL;
PFNGLISBUFFERPROC glad_glIsBuffer = NULL;
PFNGLISENABLEDPROC glad_glIsEnabled = NULL;
PFNGLISPROGRAMPROC glad_glIsProgram = NULL;
PFNGLISRENDERBUFFERPROC glad_glIsRenderbuffer = NULL;
PFNGLISSHADERPROC glad_glIsShader = NULL;
PFNGLISSYNCPROC glad_glIsSync = NULL;
PFNGLISTEXTUREPROC glad_glIsTexture = NULL;
PFNGLLIGHTMODELFVPROC glad_glLightModelfv = NULL;
PFNGLLIGHTMODELIPROC glad_glLightModeli = NULL;
PFNGLLIGHTFPROC glad_glLightf = NULL;
PFNGLLIGHTFVPROC glad_glLightfv = NULL;
PFNGLLINESTIPPLEPROC glad_glLineStipple = NULL;
PFNGLLINEWIDTHPROC glad_glLineWidth = NULL;
PFNGLLINKPROGRAMPROC glad_glLinkProgram = NULL;
PFNGLLOADIDENTITYPROC glad_glLoadIdentity = NULL;
PFNGLLOADMATRIXDPROC glad_glLoadMatrixd = NULL;
PFNGLLOADMATRIXFPROC glad_glLoadMatrixf = NULL;
PFNGLLOADNAMEPROC glad_glLoadName = NULL;
PFNGLLOGICOPPROC glad_glLogicOp = NULL;
PFNGLMAP1FPROC glad_glMap1f = NULL;
PFNGLMAP2FPROC glad_glMap2f = NULL;
PFNGLMAPBUFFERPROC glad_glMapBuffer = NULL;
PFNGLMAPBUFFERRANGEPROC glad_glMapBufferRange = NULL;
PFNGLMAPGRID1FPROC glad_glMapGrid1f = NULL;
PFNGLMAPGRID2FPROC glad_glMapGrid2f = NULL;
PFNGLMATERIALFPROC glad_glMaterialf = NULL;
PFNGLMATERIALFVPROC glad_glMaterialfv = NULL;
PFNGLMATRIXMODEPROC glad_glMatrixMode = NULL;
PFNGLMEMORYBARRIERPROC glad_glMemoryBarrier = NULL;
PFNGLMULTMATRIXDPROC glad_glMultMatrixd = NULL;
PFNGLMULTMATRIXFPROC glad_glMultMatrixf = NULL;
PFNGLMULTIDRAWELEMENTSINDIRECTPROC glad_glMultiDrawElementsIndirect = NULL;
PFNGLMULTITEXCOORD1FPROC glad_glMultiTexCoord1f = NULL;
PFNGLMULTITEXCOORD2FPROC glad_glMultiTexCoord2f = NULL;
PFNGLMULTITEXCOORD2IPROC glad_glMultiTexCoord2i = NULL;
PFNGLMULTITEXCOORD2IVPROC glad_glMultiTexCoord2iv = NULL;
PFNGLMULTITEXCOORD3FPROC glad_glMultiTexCoord3f = NULL;
PFNGLMULTITEXCOORD4FPROC glad_glMultiTexCoord4f = NULL;
PFNGLNEWLISTPROC glad_glNewList = NULL;
PFNGLNORMAL3FPROC glad_glNormal3f = NULL;
PFNGLNORMAL3FVPROC glad_glNormal3fv = NULL;
PFNGLNORMALPOINTERPROC glad_glNormalPointer = NULL;
PFNGLORTHOPROC glad_glOrtho = NULL;
PFNGLPATCHPARAMETERFVPROC glad_glPatchParameterfv = NULL;
PFNGLPATCHPARAMETERIPROC glad_glPatchParameteri = NULL;
PFNGLPIXELSTOREIPROC glad_glPixelStorei = NULL;
PFNGLPOINTPARAMETERFPROC glad_glPointParameterf = NULL;
PFNGLPOINTPARAMETERFVPROC glad_glPointParameterfv = NULL;
PFNGLPOINTSIZEPROC glad_glPointSize = NULL;
PFNGLPOLYGONMODEPROC glad_glPolygonMode = NULL;
PFNGLPOLYGONOFFSETPROC glad_glPolygonOffset = NULL;
PFNGLPOPATTRIBPROC glad_glPopAttrib = NULL;
PFNGLPOPMATRIXPROC glad_glPopMatrix = NULL;
PFNGLPOPNAMEPROC glad_glPopName = NULL;
PFNGLPRIMITIVERESTARTINDEXPROC glad_glPrimitiveRestartIndex = NULL;
PFNGLPROGRAMPARAMETERIPROC glad_glProgramParameteri = NULL;
PFNGLPUSHATTRIBPROC glad_glPushAttrib = NULL;
PFNGLPUSHMATRIXPROC glad_glPushMatrix = NULL;
PFNGLPUSHNAMEPROC glad_glPushName = NULL;
PFNGLQUERYCOUNTERPROC glad_glQueryCounter = NULL;
PFNGLREADBUFFERPROC glad_glReadBuffer = NULL;
PFNGLREADPIXELSPROC glad_glReadPixels = NULL;
PFNGLRECTFPROC glad_glRectf = NULL;
PFNGLRENDERMODEPROC glad_glRenderMode = NULL;
PFNGLRENDERBUFFERSTORAGEPROC glad_glRenderbufferStorage = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glad_glRenderbufferStorageMultisample = NULL;
PFNGLROTATEFPROC glad_glRotatef = NULL;
PFNGLSCALEFPROC glad_glScalef = NULL;
PFNGLSCISSORPROC glad_glScissor = NULL;
PFNGLSECONDARYCOLOR3FPROC glad_glSecondaryColor3f = NULL;
PFNGLSELECTBUFFERPROC glad_glSelectBuffer = NULL;
PFNGLSHADEMODELPROC glad_glShadeModel = NULL;
PFNGLSHADERSOURCEPROC glad_glShaderSource = NULL;
PFNGLSTENCILFUNCPROC glad_glStencilFunc = NULL;
PFNGLSTENCILFUNCSEPARATEPROC glad_glStencilFuncSeparate = NULL;
PFNGLSTENCILMASKPROC glad_glStencilMask = NULL;
PFNGLSTENCILMASKSEPARATEPROC glad_glStencilMaskSeparate = NULL;
PFNGLSTENCILOPPROC glad_glStencilOp = NULL;
PFNGLSTENCILOPSEPARATEPROC glad_glStencilOpSeparate = NULL;
PFNGLTEXCOORD1FPROC glad_glTexCoord1f = NULL;
PFNGLTEXCOORD2FPROC glad_glTexCoord2f = NULL;
PFNGLTEXCOORD2FVPROC glad_glTexCoord2fv = NULL;
PFNGLTEXCOORD2IPROC glad_glTexCoord2i = NULL;
PFNGLTEXCOORD3FPROC glad_glTexCoord3f = NULL;
PFNGLTEXCOORD4FPROC glad_glTexCoord4f = NULL;
PFNGLTEXCOORDPOINTERPROC glad_glTexCoordPointer = NULL;
PFNGLTEXENVFPROC glad_glTexEnvf = NULL;
PFNGLTEXENVFVPROC glad_glTexEnvfv = NULL;
PFNGLTEXENVIPROC glad_glTexEnvi = NULL;
PFNGLTEXGENFPROC glad_glTexGenf = NULL;
PFNGLTEXGENFVPROC glad_glTexGenfv = NULL;
PFNGLTEXGENIPROC glad_glTexGeni = NULL;
PFNGLTEXIMAGE1DPROC glad_glTexImage1D = NULL;
PFNGLTEXIMAGE2DPROC glad_glTexImage2D = NULL;
PFNGLTEXIMAGE2DMULTISAMPLEPROC glad_glTexImage2DMultisample = NULL;
PFNGLTEXIMAGE3DPROC glad_glTexImage3D = NULL;
PFNGLTEXPARAMETERFPROC glad_glTexParameterf = NULL;
PFNGLTEXPARAMETERFVPROC glad_glTexParameterfv = NULL;
PFNGLTEXPARAMETERIPROC glad_glTexParameteri = NULL;
PFNGLTEXPARAMETERIVPROC glad_glTexParameteriv = NULL;
PFNGLTEXSTORAGE2DPROC glad_glTexStorage2D = NULL;
PFNGLTEXSTORAGE3DPROC glad_glTexStorage3D = NULL;
PFNGLTEXSUBIMAGE2DPROC glad_glTexSubImage2D = NULL;
PFNGLTEXSUBIMAGE3DPROC glad_glTexSubImage3D = NULL;
PFNGLTRANSLATEFPROC glad_glTranslatef = NULL;
PFNGLUNIFORM1FPROC glad_glUniform1f = NULL;
PFNGLUNIFORM1FVPROC glad_glUniform1fv = NULL;
PFNGLUNIFORM1IPROC glad_glUniform1i = NULL;
PFNGLUNIFORM1IVPROC glad_glUniform1iv = NULL;
PFNGLUNIFORM1UIVPROC glad_glUniform1uiv = NULL;
PFNGLUNIFORM2FPROC glad_glUniform2f = NULL;
PFNGLUNIFORM2FVPROC glad_glUniform2fv = NULL;
PFNGLUNIFORM2IPROC glad_glUniform2i = NULL;
PFNGLUNIFORM2IVPROC glad_glUniform2iv = NULL;
PFNGLUNIFORM2UIVPROC glad_glUniform2uiv = NULL;
PFNGLUNIFORM3FPROC glad_glUniform3f = NULL;
PFNGLUNIFORM3FVPROC glad_glUniform3fv = NULL;
PFNGLUNIFORM3IPROC glad_glUniform3i = NULL;
PFNGLUNIFORM3IVPROC glad_glUniform3iv = NULL;
PFNGLUNIFORM3UIVPROC glad_glUniform3uiv = NULL;
PFNGLUNIFORM4FPROC glad_glUniform4f = NULL;
PFNGLUNIFORM4FVPROC glad_glUniform4fv = NULL;
PFNGLUNIFORM4IPROC glad_glUniform4i = NULL;
PFNGLUNIFORM4IVPROC glad_glUniform4iv = NULL;
PFNGLUNIFORM4UIVPROC glad_glUniform4uiv = NULL;
PFNGLUNIFORMMATRIX2FVPROC glad_glUniformMatrix2fv = NULL;
PFNGLUNIFORMMATRIX3FVPROC glad_glUniformMatrix3fv = NULL;
PFNGLUNIFORMMATRIX4FVPROC glad_glUniformMatrix4fv = NULL;
PFNGLUNIFORMSUBROUTINESUIVPROC glad_glUniformSubroutinesuiv = NULL;
PFNGLUNMAPBUFFERPROC glad_glUnmapBuffer = NULL;
PFNGLUSEPROGRAMPROC glad_glUseProgram = NULL;
PFNGLVALIDATEPROGRAMPROC glad_glValidateProgram = NULL;
PFNGLVERTEX2FPROC glad_glVertex2f = NULL;
PFNGLVERTEX3FPROC glad_glVertex3f = NULL;
PFNGLVERTEX3FVPROC glad_glVertex3fv = NULL;
PFNGLVERTEX4FPROC glad_glVertex4f = NULL;
PFNGLVERTEXATTRIBDIVISORPROC glad_glVertexAttribDivisor = NULL;
PFNGLVERTEXATTRIBIPOINTERPROC glad_glVertexAttribIPointer = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer = NULL;
PFNGLVERTEXPOINTERPROC glad_glVertexPointer = NULL;
PFNGLVIEWPORTPROC glad_glViewport = NULL;
PFNGLDRAWBUFFERSARBPROC glad_glDrawBuffersARB = NULL;
PFNGLPROGRAMSTRINGARBPROC glad_glProgramStringARB = NULL;
PFNGLBINDPROGRAMARBPROC glad_glBindProgramARB = NULL;
PFNGLDELETEPROGRAMSARBPROC glad_glDeleteProgramsARB = NULL;
PFNGLGENPROGRAMSARBPROC glad_glGenProgramsARB = NULL;
PFNGLPROGRAMENVPARAMETER4DARBPROC glad_glProgramEnvParameter4dARB = NULL;
PFNGLPROGRAMENVPARAMETER4DVARBPROC glad_glProgramEnvParameter4dvARB = NULL;
PFNGLPROGRAMENVPARAMETER4FARBPROC glad_glProgramEnvParameter4fARB = NULL;
PFNGLPROGRAMENVPARAMETER4FVARBPROC glad_glProgramEnvParameter4fvARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4DARBPROC glad_glProgramLocalParameter4dARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4DVARBPROC glad_glProgramLocalParameter4dvARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4FARBPROC glad_glProgramLocalParameter4fARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4FVARBPROC glad_glProgramLocalParameter4fvARB = NULL;
PFNGLGETPROGRAMENVPARAMETERDVARBPROC glad_glGetProgramEnvParameterdvARB = NULL;
PFNGLGETPROGRAMENVPARAMETERFVARBPROC glad_glGetProgramEnvParameterfvARB = NULL;
PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC glad_glGetProgramLocalParameterdvARB = NULL;
PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC glad_glGetProgramLocalParameterfvARB = NULL;
PFNGLGETPROGRAMIVARBPROC glad_glGetProgramivARB = NULL;
PFNGLACTIVETEXTUREARBPROC glad_glActiveTextureARB = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC glad_glClientActiveTextureARB = NULL;
PFNGLMULTITEXCOORD2FARBPROC glad_glMultiTexCoord2fARB = NULL;
PFNGLMULTITEXCOORD2IARBPROC glad_glMultiTexCoord2iARB = NULL;
PFNGLMULTITEXCOORD2IVARBPROC glad_glMultiTexCoord2ivARB = NULL;
PFNGLDELETEOBJECTARBPROC glad_glDeleteObjectARB = NULL;
PFNGLGETHANDLEARBPROC glad_glGetHandleARB = NULL;
PFNGLDETACHOBJECTARBPROC glad_glDetachObjectARB = NULL;
PFNGLCREATESHADEROBJECTARBPROC glad_glCreateShaderObjectARB = NULL;
PFNGLSHADERSOURCEARBPROC glad_glShaderSourceARB = NULL;
PFNGLCOMPILESHADERARBPROC glad_glCompileShaderARB = NULL;
PFNGLCREATEPROGRAMOBJECTARBPROC glad_glCreateProgramObjectARB = NULL;
PFNGLATTACHOBJECTARBPROC glad_glAttachObjectARB = NULL;
PFNGLLINKPROGRAMARBPROC glad_glLinkProgramARB = NULL;
PFNGLUSEPROGRAMOBJECTARBPROC glad_glUseProgramObjectARB = NULL;
PFNGLVALIDATEPROGRAMARBPROC glad_glValidateProgramARB = NULL;
PFNGLUNIFORM1FARBPROC glad_glUniform1fARB = NULL;
PFNGLUNIFORM2FARBPROC glad_glUniform2fARB = NULL;
PFNGLUNIFORM3FARBPROC glad_glUniform3fARB = NULL;
PFNGLUNIFORM4FARBPROC glad_glUniform4fARB = NULL;
PFNGLUNIFORM1IARBPROC glad_glUniform1iARB = NULL;
PFNGLUNIFORM2IARBPROC glad_glUniform2iARB = NULL;
PFNGLUNIFORM3IARBPROC glad_glUniform3iARB = NULL;
PFNGLUNIFORM4IARBPROC glad_glUniform4iARB = NULL;
PFNGLUNIFORM1FVARBPROC glad_glUniform1fvARB = NULL;
PFNGLUNIFORM2FVARBPROC glad_glUniform2fvARB = NULL;
PFNGLUNIFORM3FVARBPROC glad_glUniform3fvARB = NULL;
PFNGLUNIFORM4FVARBPROC glad_glUniform4fvARB = NULL;
PFNGLUNIFORM1IVARBPROC glad_glUniform1ivARB = NULL;
PFNGLUNIFORM2IVARBPROC glad_glUniform2ivARB = NULL;
PFNGLUNIFORM3IVARBPROC glad_glUniform3ivARB = NULL;
PFNGLUNIFORM4IVARBPROC glad_glUniform4ivARB = NULL;
PFNGLUNIFORMMATRIX2FVARBPROC glad_glUniformMatrix2fvARB = NULL;
PFNGLUNIFORMMATRIX3FVARBPROC glad_glUniformMatrix3fvARB = NULL;
PFNGLUNIFORMMATRIX4FVARBPROC glad_glUniformMatrix4fvARB = NULL;
PFNGLGETOBJECTPARAMETERFVARBPROC glad_glGetObjectParameterfvARB = NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC glad_glGetObjectParameterivARB = NULL;
PFNGLGETINFOLOGARBPROC glad_glGetInfoLogARB = NULL;
PFNGLGETATTACHEDOBJECTSARBPROC glad_glGetAttachedObjectsARB = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC glad_glGetUniformLocationARB = NULL;
PFNGLCOMPRESSEDTEXIMAGE3DARBPROC glad_glCompressedTexImage3DARB = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glad_glCompressedTexImage2DARB = NULL;
PFNGLCOMPRESSEDTEXIMAGE1DARBPROC glad_glCompressedTexImage1DARB = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glad_glCompressedTexSubImage3DARB = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glad_glCompressedTexSubImage2DARB = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glad_glCompressedTexSubImage1DARB = NULL;
PFNGLGETCOMPRESSEDTEXIMAGEARBPROC glad_glGetCompressedTexImageARB = NULL;
PFNGLBINDBUFFERARBPROC glad_glBindBufferARB = NULL;
PFNGLDELETEBUFFERSARBPROC glad_glDeleteBuffersARB = NULL;
PFNGLGENBUFFERSARBPROC glad_glGenBuffersARB = NULL;
PFNGLBUFFERDATAARBPROC glad_glBufferDataARB = NULL;
PFNGLVERTEXATTRIBPOINTERARBPROC glad_glVertexAttribPointerARB = NULL;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC glad_glEnableVertexAttribArrayARB = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glad_glDisableVertexAttribArrayARB = NULL;
PFNGLBINDATTRIBLOCATIONARBPROC glad_glBindAttribLocationARB = NULL;
PFNGLGETACTIVEATTRIBARBPROC glad_glGetActiveAttribARB = NULL;
PFNGLGETATTRIBLOCATIONARBPROC glad_glGetAttribLocationARB = NULL;
PFNGLBLITFRAMEBUFFEREXTPROC glad_glBlitFramebufferEXT = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glad_glRenderbufferStorageMultisampleEXT = NULL;
PFNGLISRENDERBUFFEREXTPROC glad_glIsRenderbufferEXT = NULL;
PFNGLBINDRENDERBUFFEREXTPROC glad_glBindRenderbufferEXT = NULL;
PFNGLDELETERENDERBUFFERSEXTPROC glad_glDeleteRenderbuffersEXT = NULL;
PFNGLGENRENDERBUFFERSEXTPROC glad_glGenRenderbuffersEXT = NULL;
PFNGLRENDERBUFFERSTORAGEEXTPROC glad_glRenderbufferStorageEXT = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glad_glGetRenderbufferParameterivEXT = NULL;
PFNGLISFRAMEBUFFEREXTPROC glad_glIsFramebufferEXT = NULL;
PFNGLBINDFRAMEBUFFEREXTPROC glad_glBindFramebufferEXT = NULL;
PFNGLDELETEFRAMEBUFFERSEXTPROC glad_glDeleteFramebuffersEXT = NULL;
PFNGLGENFRAMEBUFFERSEXTPROC glad_glGenFramebuffersEXT = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glad_glCheckFramebufferStatusEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glad_glFramebufferTexture1DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glad_glFramebufferTexture2DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glad_glFramebufferTexture3DEXT = NULL;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glad_glFramebufferRenderbufferEXT = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glad_glGetFramebufferAttachmentParameterivEXT = NULL;
PFNGLGENERATEMIPMAPEXTPROC glad_glGenerateMipmapEXT = NULL;
PFNGLPROGRAMPARAMETERIEXTPROC glad_glProgramParameteriEXT = NULL;
PFNGLFRAMEBUFFERTEXTURELAYEREXTPROC glad_glFramebufferTextureLayerEXT = NULL;
PFNGLDELETEFENCESNVPROC glad_glDeleteFencesNV = NULL;
PFNGLGENFENCESNVPROC glad_glGenFencesNV = NULL;
PFNGLISFENCENVPROC glad_glIsFenceNV = NULL;
PFNGLTESTFENCENVPROC glad_glTestFenceNV = NULL;
PFNGLGETFENCEIVNVPROC glad_glGetFenceivNV = NULL;
PFNGLFINISHFENCENVPROC glad_glFinishFenceNV = NULL;
PFNGLSETFENCENVPROC glad_glSetFenceNV = NULL;
PFNGLFRAMEBUFFERTEXTUREEXTPROC glad_glFramebufferTextureEXT = NULL;
PFNGLPRIMITIVERESTARTINDEXNVPROC glad_glPrimitiveRestartIndexNV = NULL;
PFNGLINVALIDATEBUFFERDATAPROC glad_glInvalidateBufferData = NULL;
PFNGLCLIPCONTROLPROC glad_glClipControl = NULL;

int gladLoadGL(void) {
    glad_glActiveTexture = impl_glActiveTexture;
    glad_glAlphaFunc = impl_glAlphaFunc;
    glad_glAttachShader = impl_glAttachShader;
    glad_glBegin = impl_glBegin;
    glad_glBeginConditionalRender = impl_glBeginConditionalRender;
    glad_glBeginQuery = impl_glBeginQuery;
    glad_glBindAttribLocation = impl_glBindAttribLocation;
    glad_glBindBuffer = impl_glBindBuffer;
    glad_glBindBufferBase = impl_glBindBufferBase;
    glad_glBindBufferRange = impl_glBindBufferRange;
    glad_glBindFramebuffer = impl_glBindFramebuffer;
    glad_glBindImageTexture = impl_glBindImageTexture;
    glad_glBindRenderbuffer = impl_glBindRenderbuffer;
    glad_glBindTexture = impl_glBindTexture;
    glad_glBindVertexArray = impl_glBindVertexArray;
    glad_glBlendColor = impl_glBlendColor;
    glad_glBlendEquation = impl_glBlendEquation;
    glad_glBlendEquationSeparate = impl_glBlendEquationSeparate;
    glad_glBlendFunc = impl_glBlendFunc;
    glad_glBlendFuncSeparate = impl_glBlendFuncSeparate;
    glad_glBlendFuncSeparatei = impl_glBlendFuncSeparatei;
    glad_glBlitFramebuffer = impl_glBlitFramebuffer;
    glad_glBufferData = impl_glBufferData;
    glad_glBufferStorage = impl_glBufferStorage;
    glad_glBufferSubData = impl_glBufferSubData;
    glad_glCallList = impl_glCallList;
    glad_glCheckFramebufferStatus = impl_glCheckFramebufferStatus;
    glad_glClear = impl_glClear;
    glad_glClearAccum = impl_glClearAccum;
    glad_glClearBufferData = impl_glClearBufferData;
    glad_glClearColor = impl_glClearColor;
    glad_glClearDepth = impl_glClearDepth;
    glad_glClearStencil = impl_glClearStencil;
    glad_glClientActiveTexture = impl_glClientActiveTexture;
    glad_glClientWaitSync = impl_glClientWaitSync;
    glad_glClipPlane = impl_glClipPlane;
    glad_glColor3f = impl_glColor3f;
    glad_glColor3fv = impl_glColor3fv;
    glad_glColor3ub = impl_glColor3ub;
    glad_glColor3ubv = impl_glColor3ubv;
    glad_glColor4f = impl_glColor4f;
    glad_glColor4fv = impl_glColor4fv;
    glad_glColor4ub = impl_glColor4ub;
    glad_glColor4ubv = impl_glColor4ubv;
    glad_glColorMask = impl_glColorMask;
    glad_glColorPointer = impl_glColorPointer;
    glad_glCompileShader = impl_glCompileShader;
    glad_glCompressedTexImage2D = impl_glCompressedTexImage2D;
    glad_glCompressedTexSubImage1D = impl_glCompressedTexSubImage1D;
    glad_glCompressedTexSubImage2D = impl_glCompressedTexSubImage2D;
    glad_glCompressedTexSubImage3D = impl_glCompressedTexSubImage3D;
    glad_glCopyBufferSubData = impl_glCopyBufferSubData;
    glad_glCopyImageSubData = impl_glCopyImageSubData;
    glad_glCopyTexImage2D = impl_glCopyTexImage2D;
    glad_glCopyTexSubImage2D = impl_glCopyTexSubImage2D;
    glad_glCopyTexSubImage3D = impl_glCopyTexSubImage3D;
    glad_glCreateProgram = impl_glCreateProgram;
    glad_glCreateShader = impl_glCreateShader;
    glad_glCullFace = impl_glCullFace;
    glad_glDeleteBuffers = impl_glDeleteBuffers;
    glad_glDeleteFramebuffers = impl_glDeleteFramebuffers;
    glad_glDeleteLists = impl_glDeleteLists;
    glad_glDeleteProgram = impl_glDeleteProgram;
    glad_glDeleteQueries = impl_glDeleteQueries;
    glad_glDeleteRenderbuffers = impl_glDeleteRenderbuffers;
    glad_glDeleteShader = impl_glDeleteShader;
    glad_glDeleteSync = impl_glDeleteSync;
    glad_glDeleteTextures = impl_glDeleteTextures;
    glad_glDeleteVertexArrays = impl_glDeleteVertexArrays;
    glad_glDepthFunc = impl_glDepthFunc;
    glad_glDepthMask = impl_glDepthMask;
    glad_glDepthRange = impl_glDepthRange;
    glad_glDepthRangef = impl_glDepthRangef;
    glad_glDetachShader = impl_glDetachShader;
    glad_glDisable = impl_glDisable;
    glad_glDisableClientState = impl_glDisableClientState;
    glad_glDisableVertexAttribArray = impl_glDisableVertexAttribArray;
    glad_glDispatchCompute = impl_glDispatchCompute;
    glad_glDrawArrays = impl_glDrawArrays;
    glad_glDrawArraysInstanced = impl_glDrawArraysInstanced;
    glad_glDrawArraysInstancedBaseInstance = impl_glDrawArraysInstancedBaseInstance;
    glad_glDrawBuffer = impl_glDrawBuffer;
    glad_glDrawBuffers = impl_glDrawBuffers;
    glad_glDrawElements = impl_glDrawElements;
    glad_glDrawElementsBaseVertex = impl_glDrawElementsBaseVertex;
    glad_glDrawElementsIndirect = impl_glDrawElementsIndirect;
    glad_glDrawElementsInstanced = impl_glDrawElementsInstanced;
    glad_glDrawElementsInstancedBaseInstance = impl_glDrawElementsInstancedBaseInstance;
    glad_glDrawElementsInstancedBaseVertex = impl_glDrawElementsInstancedBaseVertex;
    glad_glDrawElementsInstancedBaseVertexBaseInstance = impl_glDrawElementsInstancedBaseVertexBaseInstance;
    glad_glDrawRangeElements = impl_glDrawRangeElements;
    glad_glEdgeFlag = impl_glEdgeFlag;
    glad_glEnable = impl_glEnable;
    glad_glEnableClientState = impl_glEnableClientState;
    glad_glEnableVertexAttribArray = impl_glEnableVertexAttribArray;
    glad_glEnd = impl_glEnd;
    glad_glEndConditionalRender = impl_glEndConditionalRender;
    glad_glEndList = impl_glEndList;
    glad_glEndQuery = impl_glEndQuery;
    glad_glEvalCoord1f = impl_glEvalCoord1f;
    glad_glEvalCoord2f = impl_glEvalCoord2f;
    glad_glEvalMesh1 = impl_glEvalMesh1;
    glad_glEvalMesh2 = impl_glEvalMesh2;
    glad_glEvalPoint1 = impl_glEvalPoint1;
    glad_glEvalPoint2 = impl_glEvalPoint2;
    glad_glFenceSync = impl_glFenceSync;
    glad_glFinish = impl_glFinish;
    glad_glFlush = impl_glFlush;
    glad_glFlushMappedBufferRange = impl_glFlushMappedBufferRange;
    glad_glFogCoordf = impl_glFogCoordf;
    glad_glFogf = impl_glFogf;
    glad_glFogfv = impl_glFogfv;
    glad_glFogi = impl_glFogi;
    glad_glFramebufferRenderbuffer = impl_glFramebufferRenderbuffer;
    glad_glFramebufferTexture = impl_glFramebufferTexture;
    glad_glFramebufferTexture1D = impl_glFramebufferTexture1D;
    glad_glFramebufferTexture2D = impl_glFramebufferTexture2D;
    glad_glFramebufferTexture3D = impl_glFramebufferTexture3D;
    glad_glFrontFace = impl_glFrontFace;
    glad_glFrustum = impl_glFrustum;
    glad_glGenBuffers = impl_glGenBuffers;
    glad_glGenFramebuffers = impl_glGenFramebuffers;
    glad_glGenLists = impl_glGenLists;
    glad_glGenQueries = impl_glGenQueries;
    glad_glGenRenderbuffers = impl_glGenRenderbuffers;
    glad_glGenTextures = impl_glGenTextures;
    glad_glGenVertexArrays = impl_glGenVertexArrays;
    glad_glGenerateMipmap = impl_glGenerateMipmap;
    glad_glGetActiveAttrib = impl_glGetActiveAttrib;
    glad_glGetActiveUniform = impl_glGetActiveUniform;
    glad_glGetActiveUniformsiv = impl_glGetActiveUniformsiv;
    glad_glGetAttribLocation = impl_glGetAttribLocation;
    glad_glGetBooleanv = impl_glGetBooleanv;
    glad_glGetBufferParameteriv = impl_glGetBufferParameteriv;
    glad_glGetCompressedTexImage = impl_glGetCompressedTexImage;
    glad_glGetError = impl_glGetError;
    glad_glGetFloatv = impl_glGetFloatv;
    glad_glGetFramebufferAttachmentParameteriv = impl_glGetFramebufferAttachmentParameteriv;
    glad_glGetFramebufferParameteriv = impl_glGetFramebufferParameteriv;
    glad_glGetIntegeri_v = impl_glGetIntegeri_v;
    glad_glGetIntegerv = impl_glGetIntegerv;
    glad_glGetProgramInfoLog = impl_glGetProgramInfoLog;
    glad_glGetProgramiv = impl_glGetProgramiv;
    glad_glGetQueryObjectiv = impl_glGetQueryObjectiv;
    glad_glGetQueryObjectui64v = impl_glGetQueryObjectui64v;
    glad_glGetQueryObjectuiv = impl_glGetQueryObjectuiv;
    glad_glGetQueryiv = impl_glGetQueryiv;
    glad_glGetRenderbufferParameteriv = impl_glGetRenderbufferParameteriv;
    glad_glGetShaderInfoLog = impl_glGetShaderInfoLog;
    glad_glGetShaderSource = impl_glGetShaderSource;
    glad_glGetShaderiv = impl_glGetShaderiv;
    glad_glGetString = impl_glGetString;
    glad_glGetSubroutineIndex = impl_glGetSubroutineIndex;
    glad_glGetTexImage = impl_glGetTexImage;
    glad_glGetTexLevelParameteriv = impl_glGetTexLevelParameteriv;
    glad_glGetUniformLocation = impl_glGetUniformLocation;
    glad_glGetUniformfv = impl_glGetUniformfv;
    glad_glGetUniformiv = impl_glGetUniformiv;
    glad_glGetUniformuiv = impl_glGetUniformuiv;
    glad_glHint = impl_glHint;
    glad_glInitNames = impl_glInitNames;
    glad_glIsBuffer = impl_glIsBuffer;
    glad_glIsEnabled = impl_glIsEnabled;
    glad_glIsProgram = impl_glIsProgram;
    glad_glIsRenderbuffer = impl_glIsRenderbuffer;
    glad_glIsShader = impl_glIsShader;
    glad_glIsSync = impl_glIsSync;
    glad_glIsTexture = impl_glIsTexture;
    glad_glLightModelfv = impl_glLightModelfv;
    glad_glLightModeli = impl_glLightModeli;
    glad_glLightf = impl_glLightf;
    glad_glLightfv = impl_glLightfv;
    glad_glLineStipple = impl_glLineStipple;
    glad_glLineWidth = impl_glLineWidth;
    glad_glLinkProgram = impl_glLinkProgram;
    glad_glLoadIdentity = impl_glLoadIdentity;
    glad_glLoadMatrixd = impl_glLoadMatrixd;
    glad_glLoadMatrixf = impl_glLoadMatrixf;
    glad_glLoadName = impl_glLoadName;
    glad_glLogicOp = impl_glLogicOp;
    glad_glMap1f = impl_glMap1f;
    glad_glMap2f = impl_glMap2f;
    glad_glMapBuffer = impl_glMapBuffer;
    glad_glMapBufferRange = impl_glMapBufferRange;
    glad_glMapGrid1f = impl_glMapGrid1f;
    glad_glMapGrid2f = impl_glMapGrid2f;
    glad_glMaterialf = impl_glMaterialf;
    glad_glMaterialfv = impl_glMaterialfv;
    glad_glMatrixMode = impl_glMatrixMode;
    glad_glMemoryBarrier = impl_glMemoryBarrier;
    glad_glMultMatrixd = impl_glMultMatrixd;
    glad_glMultMatrixf = impl_glMultMatrixf;
    glad_glMultiDrawElementsIndirect = impl_glMultiDrawElementsIndirect;
    glad_glMultiTexCoord1f = impl_glMultiTexCoord1f;
    glad_glMultiTexCoord2f = impl_glMultiTexCoord2f;
    glad_glMultiTexCoord2i = impl_glMultiTexCoord2i;
    glad_glMultiTexCoord2iv = impl_glMultiTexCoord2iv;
    glad_glMultiTexCoord3f = impl_glMultiTexCoord3f;
    glad_glMultiTexCoord4f = impl_glMultiTexCoord4f;
    glad_glNewList = impl_glNewList;
    glad_glNormal3f = impl_glNormal3f;
    glad_glNormal3fv = impl_glNormal3fv;
    glad_glNormalPointer = impl_glNormalPointer;
    glad_glOrtho = impl_glOrtho;
    glad_glPatchParameterfv = impl_glPatchParameterfv;
    glad_glPatchParameteri = impl_glPatchParameteri;
    glad_glPixelStorei = impl_glPixelStorei;
    glad_glPointParameterf = impl_glPointParameterf;
    glad_glPointParameterfv = impl_glPointParameterfv;
    glad_glPointSize = impl_glPointSize;
    glad_glPolygonMode = impl_glPolygonMode;
    glad_glPolygonOffset = impl_glPolygonOffset;
    glad_glPopAttrib = impl_glPopAttrib;
    glad_glPopMatrix = impl_glPopMatrix;
    glad_glPopName = impl_glPopName;
    glad_glPrimitiveRestartIndex = impl_glPrimitiveRestartIndex;
    glad_glProgramParameteri = impl_glProgramParameteri;
    glad_glPushAttrib = impl_glPushAttrib;
    glad_glPushMatrix = impl_glPushMatrix;
    glad_glPushName = impl_glPushName;
    glad_glQueryCounter = impl_glQueryCounter;
    glad_glReadBuffer = impl_glReadBuffer;
    glad_glReadPixels = impl_glReadPixels;
    glad_glRectf = impl_glRectf;
    glad_glRenderMode = impl_glRenderMode;
    glad_glRenderbufferStorage = impl_glRenderbufferStorage;
    glad_glRenderbufferStorageMultisample = impl_glRenderbufferStorageMultisample;
    glad_glRotatef = impl_glRotatef;
    glad_glScalef = impl_glScalef;
    glad_glScissor = impl_glScissor;
    glad_glSecondaryColor3f = impl_glSecondaryColor3f;
    glad_glSelectBuffer = impl_glSelectBuffer;
    glad_glShadeModel = impl_glShadeModel;
    glad_glShaderSource = impl_glShaderSource;
    glad_glStencilFunc = impl_glStencilFunc;
    glad_glStencilFuncSeparate = impl_glStencilFuncSeparate;
    glad_glStencilMask = impl_glStencilMask;
    glad_glStencilMaskSeparate = impl_glStencilMaskSeparate;
    glad_glStencilOp = impl_glStencilOp;
    glad_glStencilOpSeparate = impl_glStencilOpSeparate;
    glad_glTexCoord1f = impl_glTexCoord1f;
    glad_glTexCoord2f = impl_glTexCoord2f;
    glad_glTexCoord2fv = impl_glTexCoord2fv;
    glad_glTexCoord2i = impl_glTexCoord2i;
    glad_glTexCoord3f = impl_glTexCoord3f;
    glad_glTexCoord4f = impl_glTexCoord4f;
    glad_glTexCoordPointer = impl_glTexCoordPointer;
    glad_glTexEnvf = impl_glTexEnvf;
    glad_glTexEnvfv = impl_glTexEnvfv;
    glad_glTexEnvi = impl_glTexEnvi;
    glad_glTexGenf = impl_glTexGenf;
    glad_glTexGenfv = impl_glTexGenfv;
    glad_glTexGeni = impl_glTexGeni;
    glad_glTexImage1D = impl_glTexImage1D;
    glad_glTexImage2D = impl_glTexImage2D;
    glad_glTexImage2DMultisample = impl_glTexImage2DMultisample;
    glad_glTexImage3D = impl_glTexImage3D;
    glad_glTexParameterf = impl_glTexParameterf;
    glad_glTexParameterfv = impl_glTexParameterfv;
    glad_glTexParameteri = impl_glTexParameteri;
    glad_glTexParameteriv = impl_glTexParameteriv;
    glad_glTexStorage2D = impl_glTexStorage2D;
    glad_glTexStorage3D = impl_glTexStorage3D;
    glad_glTexSubImage2D = impl_glTexSubImage2D;
    glad_glTexSubImage3D = impl_glTexSubImage3D;
    glad_glTranslatef = impl_glTranslatef;
    glad_glUniform1f = impl_glUniform1f;
    glad_glUniform1fv = impl_glUniform1fv;
    glad_glUniform1i = impl_glUniform1i;
    glad_glUniform1iv = impl_glUniform1iv;
    glad_glUniform1uiv = impl_glUniform1uiv;
    glad_glUniform2f = impl_glUniform2f;
    glad_glUniform2fv = impl_glUniform2fv;
    glad_glUniform2i = impl_glUniform2i;
    glad_glUniform2iv = impl_glUniform2iv;
    glad_glUniform2uiv = impl_glUniform2uiv;
    glad_glUniform3f = impl_glUniform3f;
    glad_glUniform3fv = impl_glUniform3fv;
    glad_glUniform3i = impl_glUniform3i;
    glad_glUniform3iv = impl_glUniform3iv;
    glad_glUniform3uiv = impl_glUniform3uiv;
    glad_glUniform4f = impl_glUniform4f;
    glad_glUniform4fv = impl_glUniform4fv;
    glad_glUniform4i = impl_glUniform4i;
    glad_glUniform4iv = impl_glUniform4iv;
    glad_glUniform4uiv = impl_glUniform4uiv;
    glad_glUniformMatrix2fv = impl_glUniformMatrix2fv;
    glad_glUniformMatrix3fv = impl_glUniformMatrix3fv;
    glad_glUniformMatrix4fv = impl_glUniformMatrix4fv;
    glad_glUniformSubroutinesuiv = impl_glUniformSubroutinesuiv;
    glad_glUnmapBuffer = impl_glUnmapBuffer;
    glad_glUseProgram = impl_glUseProgram;
    glad_glValidateProgram = impl_glValidateProgram;
    glad_glVertex2f = impl_glVertex2f;
    glad_glVertex3f = impl_glVertex3f;
    glad_glVertex3fv = impl_glVertex3fv;
    glad_glVertex4f = impl_glVertex4f;
    glad_glVertexAttribDivisor = impl_glVertexAttribDivisor;
    glad_glVertexAttribIPointer = impl_glVertexAttribIPointer;
    glad_glVertexAttribPointer = impl_glVertexAttribPointer;
    glad_glVertexPointer = impl_glVertexPointer;
    glad_glViewport = impl_glViewport;
    glad_glDrawBuffersARB = impl_glDrawBuffersARB;
    glad_glProgramStringARB = impl_glProgramStringARB;
    glad_glBindProgramARB = impl_glBindProgramARB;
    glad_glDeleteProgramsARB = impl_glDeleteProgramsARB;
    glad_glGenProgramsARB = impl_glGenProgramsARB;
    glad_glProgramEnvParameter4dARB = impl_glProgramEnvParameter4dARB;
    glad_glProgramEnvParameter4dvARB = impl_glProgramEnvParameter4dvARB;
    glad_glProgramEnvParameter4fARB = impl_glProgramEnvParameter4fARB;
    glad_glProgramEnvParameter4fvARB = impl_glProgramEnvParameter4fvARB;
    glad_glProgramLocalParameter4dARB = impl_glProgramLocalParameter4dARB;
    glad_glProgramLocalParameter4dvARB = impl_glProgramLocalParameter4dvARB;
    glad_glProgramLocalParameter4fARB = impl_glProgramLocalParameter4fARB;
    glad_glProgramLocalParameter4fvARB = impl_glProgramLocalParameter4fvARB;
    glad_glGetProgramEnvParameterdvARB = impl_glGetProgramEnvParameterdvARB;
    glad_glGetProgramEnvParameterfvARB = impl_glGetProgramEnvParameterfvARB;
    glad_glGetProgramLocalParameterdvARB = impl_glGetProgramLocalParameterdvARB;
    glad_glGetProgramLocalParameterfvARB = impl_glGetProgramLocalParameterfvARB;
    glad_glGetProgramivARB = impl_glGetProgramivARB;
    glad_glActiveTextureARB = impl_glActiveTextureARB;
    glad_glClientActiveTextureARB = impl_glClientActiveTextureARB;
    glad_glMultiTexCoord2fARB = impl_glMultiTexCoord2fARB;
    glad_glMultiTexCoord2iARB = impl_glMultiTexCoord2iARB;
    glad_glMultiTexCoord2ivARB = impl_glMultiTexCoord2ivARB;
    glad_glDeleteObjectARB = impl_glDeleteObjectARB;
    glad_glGetHandleARB = impl_glGetHandleARB;
    glad_glDetachObjectARB = impl_glDetachObjectARB;
    glad_glCreateShaderObjectARB = impl_glCreateShaderObjectARB;
    glad_glShaderSourceARB = impl_glShaderSourceARB;
    glad_glCompileShaderARB = impl_glCompileShaderARB;
    glad_glCreateProgramObjectARB = impl_glCreateProgramObjectARB;
    glad_glAttachObjectARB = impl_glAttachObjectARB;
    glad_glLinkProgramARB = impl_glLinkProgramARB;
    glad_glUseProgramObjectARB = impl_glUseProgramObjectARB;
    glad_glValidateProgramARB = impl_glValidateProgramARB;
    glad_glUniform1fARB = impl_glUniform1fARB;
    glad_glUniform2fARB = impl_glUniform2fARB;
    glad_glUniform3fARB = impl_glUniform3fARB;
    glad_glUniform4fARB = impl_glUniform4fARB;
    glad_glUniform1iARB = impl_glUniform1iARB;
    glad_glUniform2iARB = impl_glUniform2iARB;
    glad_glUniform3iARB = impl_glUniform3iARB;
    glad_glUniform4iARB = impl_glUniform4iARB;
    glad_glUniform1fvARB = impl_glUniform1fvARB;
    glad_glUniform2fvARB = impl_glUniform2fvARB;
    glad_glUniform3fvARB = impl_glUniform3fvARB;
    glad_glUniform4fvARB = impl_glUniform4fvARB;
    glad_glUniform1ivARB = impl_glUniform1ivARB;
    glad_glUniform2ivARB = impl_glUniform2ivARB;
    glad_glUniform3ivARB = impl_glUniform3ivARB;
    glad_glUniform4ivARB = impl_glUniform4ivARB;
    glad_glUniformMatrix2fvARB = impl_glUniformMatrix2fvARB;
    glad_glUniformMatrix3fvARB = impl_glUniformMatrix3fvARB;
    glad_glUniformMatrix4fvARB = impl_glUniformMatrix4fvARB;
    glad_glGetObjectParameterfvARB = impl_glGetObjectParameterfvARB;
    glad_glGetObjectParameterivARB = impl_glGetObjectParameterivARB;
    glad_glGetInfoLogARB = impl_glGetInfoLogARB;
    glad_glGetAttachedObjectsARB = impl_glGetAttachedObjectsARB;
    glad_glGetUniformLocationARB = impl_glGetUniformLocationARB;
    glad_glCompressedTexImage3DARB = impl_glCompressedTexImage3DARB;
    glad_glCompressedTexImage2DARB = impl_glCompressedTexImage2DARB;
    glad_glCompressedTexImage1DARB = impl_glCompressedTexImage1DARB;
    glad_glCompressedTexSubImage3DARB = impl_glCompressedTexSubImage3DARB;
    glad_glCompressedTexSubImage2DARB = impl_glCompressedTexSubImage2DARB;
    glad_glCompressedTexSubImage1DARB = impl_glCompressedTexSubImage1DARB;
    glad_glGetCompressedTexImageARB = impl_glGetCompressedTexImageARB;
    glad_glBindBufferARB = impl_glBindBufferARB;
    glad_glDeleteBuffersARB = impl_glDeleteBuffersARB;
    glad_glGenBuffersARB = impl_glGenBuffersARB;
    glad_glBufferDataARB = impl_glBufferDataARB;
    glad_glVertexAttribPointerARB = impl_glVertexAttribPointerARB;
    glad_glEnableVertexAttribArrayARB = impl_glEnableVertexAttribArrayARB;
    glad_glDisableVertexAttribArrayARB = impl_glDisableVertexAttribArrayARB;
    glad_glBindAttribLocationARB = impl_glBindAttribLocationARB;
    glad_glGetActiveAttribARB = impl_glGetActiveAttribARB;
    glad_glGetAttribLocationARB = impl_glGetAttribLocationARB;
    glad_glBlitFramebufferEXT = impl_glBlitFramebufferEXT;
    glad_glRenderbufferStorageMultisampleEXT = impl_glRenderbufferStorageMultisampleEXT;
    glad_glIsRenderbufferEXT = impl_glIsRenderbufferEXT;
    glad_glBindRenderbufferEXT = impl_glBindRenderbufferEXT;
    glad_glDeleteRenderbuffersEXT = impl_glDeleteRenderbuffersEXT;
    glad_glGenRenderbuffersEXT = impl_glGenRenderbuffersEXT;
    glad_glRenderbufferStorageEXT = impl_glRenderbufferStorageEXT;
    glad_glGetRenderbufferParameterivEXT = impl_glGetRenderbufferParameterivEXT;
    glad_glIsFramebufferEXT = impl_glIsFramebufferEXT;
    glad_glBindFramebufferEXT = impl_glBindFramebufferEXT;
    glad_glDeleteFramebuffersEXT = impl_glDeleteFramebuffersEXT;
    glad_glGenFramebuffersEXT = impl_glGenFramebuffersEXT;
    glad_glCheckFramebufferStatusEXT = impl_glCheckFramebufferStatusEXT;
    glad_glFramebufferTexture1DEXT = impl_glFramebufferTexture1DEXT;
    glad_glFramebufferTexture2DEXT = impl_glFramebufferTexture2DEXT;
    glad_glFramebufferTexture3DEXT = impl_glFramebufferTexture3DEXT;
    glad_glFramebufferRenderbufferEXT = impl_glFramebufferRenderbufferEXT;
    glad_glGetFramebufferAttachmentParameterivEXT = impl_glGetFramebufferAttachmentParameterivEXT;
    glad_glGenerateMipmapEXT = impl_glGenerateMipmapEXT;
    glad_glProgramParameteriEXT = impl_glProgramParameteriEXT;
    glad_glFramebufferTextureLayerEXT = impl_glFramebufferTextureLayerEXT;
    glad_glDeleteFencesNV = impl_glDeleteFencesNV;
    glad_glGenFencesNV = impl_glGenFencesNV;
    glad_glIsFenceNV = impl_glIsFenceNV;
    glad_glTestFenceNV = impl_glTestFenceNV;
    glad_glGetFenceivNV = impl_glGetFenceivNV;
    glad_glFinishFenceNV = impl_glFinishFenceNV;
    glad_glSetFenceNV = impl_glSetFenceNV;
    glad_glFramebufferTextureEXT = impl_glFramebufferTextureEXT;
    glad_glPrimitiveRestartIndexNV = impl_glPrimitiveRestartIndexNV;
    glad_glInvalidateBufferData = impl_glInvalidateBufferData;
    glad_glClipControl = impl_glClipControl;

    return 0;
}

#ifdef __cplusplus
} //extern "C" {
#endif