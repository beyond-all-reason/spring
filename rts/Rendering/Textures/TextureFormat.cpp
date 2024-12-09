#include "TextureFormat.h"

GLenum GL::GetInternalFormatDataFormat(GLenum internalFormat) {
	GLenum dataFormat;
	switch (internalFormat) {
		case GL_R8UI:
		case GL_R16UI:
		case GL_R32UI: {
			dataFormat = GL_RED_INTEGER;
		} break;
		case GL_RG8UI:
		case GL_RG16UI:
		case GL_RG32UI: {
			dataFormat = GL_RG_INTEGER;
		} break;
		case GL_RGB10_A2UI: {
			dataFormat = GL_RGB_INTEGER;
		} break;
		case GL_RGBA8UI:
		case GL_RGBA16UI:
		case GL_RGBA32UI: {
			dataFormat = GL_RGBA_INTEGER;
		} break;
		case GL_R8:
		case GL_R16:
		case GL_R16F:
		case GL_R32F: {
			dataFormat = GL_RED;
		} break;
		case GL_DEPTH_COMPONENT:
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32:
		case GL_DEPTH_COMPONENT32F: {
			dataFormat = GL_DEPTH_COMPONENT;
		} break;
		case GL_RG8:
		case GL_RG16:
		case GL_RG16F:
		case GL_RG32F: {
			dataFormat = GL_RG;
		} break;
		case GL_RGB10_A2:
		case GL_R11F_G11F_B10F: {
			dataFormat = GL_RGB;
		} break;
		case GL_RGBA8:
		case GL_RGBA16:
		case GL_RGBA16F:
		case GL_RGBA32F:
		default: {
			dataFormat = GL_RGBA;
		} break;
	}
	return dataFormat;
}

GLenum GL::GetInternalFormatDataType(GLenum internalFormat) {
	GLenum dataType;
	switch (internalFormat) {
		case GL_RGBA16F:
		case GL_RG16F:
		case GL_R16F: {
			dataType = GL_HALF_FLOAT;
		} break;
		case GL_RGBA32F:
		case GL_RG32F:
		case GL_R32F:
		case GL_DEPTH_COMPONENT32F: {
			dataType = GL_FLOAT;
		} break;
		case GL_RGBA32UI:
		case GL_RG32UI:
		case GL_R32UI:
		case GL_DEPTH_COMPONENT32: {
			dataType = GL_UNSIGNED_INT;
		} break;
		case GL_DEPTH_COMPONENT24: {
			dataType = GL_UNSIGNED_INT;
		} break;
		case GL_RGBA16:
		case GL_RGBA16UI:
		case GL_RG16:
		case GL_RG16UI:
		case GL_R16:
		case GL_R16UI:
		case GL_DEPTH_COMPONENT16: {
			dataType = GL_UNSIGNED_SHORT;
		} break;
		case GL_R11F_G11F_B10F: {
			dataType = GL_UNSIGNED_INT_10F_11F_11F_REV;
		} break;
		case GL_RGB10_A2:
		case GL_RGB10_A2UI: {
			dataType = GL_UNSIGNED_INT_2_10_10_10_REV;
		} break;
		case GL_RGBA8:
		case GL_RGBA8UI:
		case GL_RG8:
		case GL_RG8UI:
		case GL_R8:
		case GL_R8UI:
		case GL_DEPTH_COMPONENT:
		default: {
			dataType = GL_UNSIGNED_BYTE;
		} break;
	}
	return dataType;
}

GLenum GL::GetInternalFormatUserType(GLenum internalFormat) {
	GLenum userType;
	switch (internalFormat) {
		case GL_R8UI:
		case GL_RG8UI:
		case GL_RGBA8UI: {
			userType = GL_UNSIGNED_BYTE;
		} break;
		case GL_R16UI:
		case GL_RG16UI:
		case GL_RGBA16UI:
		case GL_RGB10_A2UI: {
			userType = GL_UNSIGNED_SHORT;
		} break;
		case GL_R32UI:
		case GL_RG32UI:
		case GL_RGBA32UI: {
			userType = GL_UNSIGNED_INT;
		} break;
		default: {
			userType = GL_FLOAT;
		} break;
	}
	return userType;
}

GLenum GL::GetBindingQueryFromTarget(GLenum target) {
	switch (target) {
		case GL_TEXTURE_1D:                   return GL_TEXTURE_BINDING_1D;
		case GL_TEXTURE_2D:                   return GL_TEXTURE_BINDING_2D;
		case GL_TEXTURE_3D:                   return GL_TEXTURE_BINDING_3D;
		case GL_TEXTURE_1D_ARRAY:             return GL_TEXTURE_BINDING_1D_ARRAY;
		case GL_TEXTURE_2D_ARRAY:             return GL_TEXTURE_BINDING_2D_ARRAY;
		case GL_TEXTURE_RECTANGLE:            return GL_TEXTURE_BINDING_RECTANGLE;
		case GL_TEXTURE_CUBE_MAP:             return GL_TEXTURE_BINDING_CUBE_MAP;
		case GL_TEXTURE_BUFFER:               return GL_TEXTURE_BINDING_BUFFER;
		case GL_TEXTURE_2D_MULTISAMPLE:       return GL_TEXTURE_BINDING_2D_MULTISAMPLE;
		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY: return GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY;
		default: break;
	}
	return 0;
}

GLsizei GL::GetPixelFormatSize(GLenum format) {
	GLsizei size;
	switch (format) {
		case GL_RED:
		case GL_RED_INTEGER:
		case GL_GREEN:
		case GL_GREEN_INTEGER:
		case GL_BLUE:
		case GL_BLUE_INTEGER:
		case GL_ALPHA:
		case GL_ALPHA_INTEGER:
		case GL_LUMINANCE:
		case GL_COLOR_INDEX:
		case GL_STENCIL_INDEX:
		case GL_DEPTH_COMPONENT: {
			size = 1;
		} break;
		case GL_RG:
		case GL_RG_INTEGER:
		case GL_LUMINANCE_ALPHA: {
			size = 2;
		} break;
		case GL_RGB:
		case GL_RGB_INTEGER:
		case GL_BGR:
		case GL_BGR_INTEGER: {
			size = 3;
		} break;
		case GL_RGBA:
		case GL_RGBA_INTEGER:
		case GL_BGRA:
		case GL_BGRA_INTEGER: {
			size = 4;
		} break;
		default: {
			size = 0;
		} break;
	}
	return size;
}
