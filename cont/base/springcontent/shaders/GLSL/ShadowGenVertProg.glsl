#if (GL_FRAGMENT_PRECISION_HIGH == 1)
// ancient GL3 ATI drivers confuse GLSL for GLSL-ES and require this
precision highp float;
#else
precision mediump float;
#endif

void main() {
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	gl_ClipVertex  = gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
}