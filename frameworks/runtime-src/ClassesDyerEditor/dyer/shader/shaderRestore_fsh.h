#ifndef __DYER_SHDER_RESTORE_H__
#define __DYER_SHDER_RESTORE_H__
const char* cc_dyer_restore_fsh = STRINGIFY(
																	\n\
#ifdef GL_ES														\n\
precision mediump float;											\n\
#endif																\n\
varying vec4 v_fragmentColor;										\n\
varying vec2 v_texCoord;											\n\
void main(void){													\n\
	gl_FragColor = texture2D(CC_Texture0, v_texCoord)*v_fragmentColor;	\n\
}																	\n\
);

#endif //__DYER_SHDER_RESTORE_H__