#ifndef __DYER_SHDER_FSH_H__
#define __DYER_SHDER_FSH_H__
const char* cc_dyer_fsh = STRINGIFY(
																		\n\
#ifdef GL_ES																\n\
precision mediump float;													\n\
#endif																		\n\
varying vec2 v_texCoord;													\n\
																			\n\
uniform sampler2D u_dTex;													\n\
uniform float u_dHs[20];													\n\
uniform float u_dSs[20];													\n\
uniform float u_dLs[20];													\n\
																			\n\															
const vec3 c_v3_1 = vec3(241.0 / 255.0, 73.0 / 255.0, 161.0 / 255.0);		\n\ /*变色目标*/
const vec3 c_v3_2 = vec3(217.0 / 255.0, 11.0 / 255.0, 43.0 / 255.0);		\n\
const vec3 c_v3_3 = vec3(254.0 / 255.0, 167.0 / 255.0, 154.0 / 255.0);		\n\
const vec3 c_v3_4 = vec3(26.0 / 255.0, 123.0 / 255.0, 172.0 / 255.0);		\n\
const vec3 c_v3_5 = vec3(168.0 / 255.0, 245.0 / 255.0, 194.0 / 255.0);		\n\
const vec3 c_v3_6 = vec3(14.0 / 255.0, 8.0 / 255.0, 239.0 / 255.0);			\n\
const vec3 c_v3_7 = vec3(138.0 / 255.0, 191.0 / 255.0, 40.0 / 255.0);		\n\
const vec3 c_v3_8 = vec3(91.0 / 255.0, 28.0 / 255.0, 199.0 / 255.0);		\n\
const vec3 c_v3_9 = vec3(113.0 / 255.0, 111.0 / 255.0, 246.0 / 255.0);		\n\
const vec3 c_v3_10 = vec3(139.0 / 255.0, 60.0 / 255.0, 10.0 / 255.0);		\n\
const vec3 c_v3_11 = vec3(27.0 / 255.0, 29.0 / 255.0, 22.0 / 255.0);		\n\
const float disVal = 0.12;													\n\
																			\n\
bool compVec3(vec4 target, vec3 arg)										\n\
{																			\n\
	vec3 p = target.rgb - arg;													\n\
	if (abs(p.r)<disVal && abs(p.g)<disVal && abs(p.b)<disVal)				\n\
		return true;														\n\
	return false;															\n\
}																			\n\
void main(){																\n\
	vec4 texColor = texture2D(CC_Texture0, v_texCoord);						\n\
	if(texColor.a < 0.1){													\n\
		discard;															\n\
	}																		\n\
	vec4 targetColor = texture2D(u_dTex, v_texCoord);						\n\
	vec3 hslVec = vec3(0.0);												\n\
	if (compVec3(targetColor, c_v3_1))										\n\
	{																		\n\
		hslVec = vec3(u_dHs[1], u_dSs[1], u_dLs[1]);						\n\
	}																		\n\
	else if (compVec3(targetColor, c_v3_2))									\n\
	{																		\n\
		hslVec = vec3(u_dHs[2], u_dSs[2], u_dLs[2]);						\n\
	}																		\n\
	else if (compVec3(targetColor, c_v3_3))									\n\
	{																		\n\
		hslVec = vec3(u_dHs[3], u_dSs[3], u_dLs[3]);						\n\
	}																		\n\
	else if (compVec3(targetColor, c_v3_4))									\n\
	{																		\n\
		hslVec = vec3(u_dHs[4], u_dSs[4], u_dLs[4]);						\n\
	}																		\n\
	else if (compVec3(targetColor, c_v3_5))									\n\
	{																		\n\
		hslVec = vec3(u_dHs[5], u_dSs[5], u_dLs[5]);						\n\
	}																		\n\
	else if (compVec3(targetColor, c_v3_6))									\n\
	{																		\n\
		hslVec = vec3(u_dHs[6], u_dSs[6], u_dLs[6]);						\n\
	}																		\n\
	else if (compVec3(targetColor, c_v3_7))									\n\
	{																		\n\
		hslVec = vec3(u_dHs[7], u_dSs[7], u_dLs[7]);						\n\
	}																		\n\
	else if (compVec3(targetColor, c_v3_8))									\n\
	{																		\n\
		hslVec = vec3(u_dHs[8], u_dSs[8], u_dLs[8]);						\n\
	}																		\n\
	else if (compVec3(targetColor, c_v3_9)	)								\n\
	{																		\n\
		hslVec = vec3(u_dHs[9], u_dSs[9], u_dLs[9]);						\n\
	}																		\n\
	else if (compVec3(targetColor, c_v3_10))								\n\
	{																		\n\
		hslVec = vec3(u_dHs[10], u_dSs[10], u_dLs[10]);						\n\
	}																		\n\
	else if (compVec3(targetColor, c_v3_11))								\n\
	{																		\n\
		hslVec = vec3(u_dHs[11], u_dSs[11], u_dLs[11]);						\n\
	}																		\n\
	else																	\n\
	{																		\n\
		gl_FragColor = texColor;											\n\
		return;																\n\
	}																		\n\
	if (abs(hslVec[0])<0.0001 && abs(hslVec[1])<0.0001 && abs(hslVec[2])<0.0001)	\n\
	{																		\n\
		gl_FragColor = texColor;											\n\
		return;																\n\
	}																		\n\
	float colmax = max(max(texColor.r, texColor.g), texColor.b);			\n\
	float colmin = min(min(texColor.r, texColor.g), texColor.b);			\n\
	float delta = colmax - colmin;											\n\
																			\n\
	float h = 0.0;															\n\
	float lv = 0.5*(colmax + colmin);										\n\
	if (colmax == colmin)													\n\
	{																		\n\
		h = 0.0;															\n\
	}																		\n\
	else if (colmax == texColor.r && texColor.b <= texColor.g)				\n\
	{																		\n\
		h = 60.0*(texColor.g - texColor.b) / delta;							\n\
	}																		\n\
	else if (colmax == texColor.r && texColor.g<texColor.b)					\n\
	{																		\n\
		h = 60.0*(texColor.g - texColor.b) / delta + 360.0;					\n\
	}																		\n\
	else if (colmax == texColor.g)											\n\
	{																		\n\
		h = 60.0*(texColor.b - texColor.r) / delta + 120.0;					\n\
	}																		\n\
	else if (colmax == texColor.b)											\n\
	{																		\n\
		h = 60.0*(texColor.r - texColor.g) / delta + 240.0;					\n\
	}																		\n\
	float s = 0.0;															\n\
	if (lv == 0.0 || colmax == colmin)										\n\
	{																		\n\
		s = 0.0;															\n\
	}																		\n\
	else if (0.0 <= lv && lv <= 0.5)										\n\
	{																		\n\
		s = (delta) / (2.0*lv);												\n\
	}																		\n\
	else if (lv>0.5)														\n\
	{																		\n\
		s = (delta) / (2.0 - 2.0*lv);										\n\
	}																		\n\
	h = h + hslVec[0];														\n\
	s = min(1.0, max(0.0, s + hslVec[1]));									\n\
	float q;																\n\
	if (lv<0.5)																\n\
	{																		\n\
		q = lv*(1.0 + s);													\n\
	}																		\n\
	else																	\n\
	{																		\n\
		q = lv + s - lv*s;													\n\
	}																		\n\
	float p = 2.0*lv - q;													\n\
	float hk = h / 360.0;													\n\
	float t[3];																\n\
	t[0] = hk + 1.0 / 3.0; t[1] = hk; t[2] = hk - 1.0 / 3.0;				\n\
	if (t[0]<0.0)															\n\
		t[0] += 1.0;														\n\
	else if (t[0]>1.0)														\n\
		t[0] -= 1.0;														\n\
	if (t[1]<0.0)															\n\
		t[1] += 1.0;														\n\
	else if (t[1]>1.0)														\n\
		t[1] -= 1.0;														\n\
	if (t[2]<0.0)															\n\
		t[2] += 1.0;														\n\
	else if (t[2]>1.0)														\n\
		t[2] -= 1.0;														\n\
																			\n\
	vec3 fc = vec3(p);														\n\
	if (t[0]<1.0 / 6.0)														\n\
	{																		\n\
		fc.r = p + ((q - p)*6.0*t[0]);										\n\
	}																		\n\
	else if (1.0 / 6.0 <= t[0] && t[0]<0.5)									\n\
	{																		\n\
		fc.r = q;															\n\
	}																		\n\
	else if (0.5 <= t[0] && t[0]<2.0 / 3.0)									\n\
	{																		\n\
		fc.r = p + ((q - p)*6.0*(2.0 / 3.0 - t[0]));						\n\
	}																		\n\
																			\n\
	if (t[1]<1.0 / 6.0)														\n\
	{																		\n\
		fc.g = p + ((q - p)*6.0*t[1]);										\n\
	}																		\n\
	else if (1.0 / 6.0 <= t[1] && t[1]<0.5)									\n\
	{																		\n\
		fc.g = q;															\n\
	}																		\n\
	else if (0.5 <= t[1] && t[1]<2.0 / 3.0)									\n\
	{																		\n\
		fc.g = p + ((q - p)*6.0*(2.0 / 3.0 - t[1]));						\n\
	}																		\n\
																			\n\
	if (t[2]<1.0 / 6.0)														\n\
	{																		\n\
		fc.b = p + ((q - p)*6.0*t[2]);										\n\
	}																		\n\
	else if (1.0 / 6.0 <= t[2] && t[2]<0.5)									\n\
	{																		\n\
		fc.b = q;															\n\
	}																		\n\
	else if (0.5 <= t[2] && t[2]<2.0 / 3.0)									\n\
	{																		\n\
		fc.b = p + ((q - p)*6.0*(2.0 / 3.0 - t[2]));						\n\
	}																		\n\
	gl_FragColor = vec4(fc + hslVec[2], texColor.a);						\n\
}

);

#endif	//__DYER_SHDER_FSH_H__