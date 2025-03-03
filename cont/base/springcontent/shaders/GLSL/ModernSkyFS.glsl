#version 130

in vec3 dir;

uniform float time;

uniform vec4 cloudInfo;
uniform vec3 skyColor;
uniform vec3 fogColor;
uniform vec4 planeColor; // .w signals if enabled
uniform vec3 sunDir;
uniform vec4 sunColor;

const float cirrus1  = 0.9;
const float cumulus1 = 1.8;

out vec4 fragColor;

//  https://github.com/BrianSharpe/Wombat/blob/master/Value3D.glsl
float Value3D( vec3 P )
{
    // establish our grid cell and unit position
    vec3 Pi = floor(P);
    vec3 Pf = P - Pi;
    vec3 Pf_min1 = Pf - 1.0;

    // clamp the domain
    Pi.xyz = Pi.xyz - floor(Pi.xyz * ( 1.0 / 69.0 )) * 69.0;
    vec3 Pi_inc1 = step( Pi, vec3( 69.0 - 1.5 ) ) * ( Pi + 1.0 );

    // calculate the hash
    vec4 Pt = vec4( Pi.xy, Pi_inc1.xy ) + vec2( 50.0, 161.0 ).xyxy;
    Pt *= Pt;
    Pt = Pt.xzxz * Pt.yyww;
    vec2 hash_mod = vec2( 1.0 / ( 635.298681 + vec2( Pi.z, Pi_inc1.z ) * 48.500388 ) );
    vec4 hash_lowz = fract( Pt * hash_mod.xxxx );
    vec4 hash_highz = fract( Pt * hash_mod.yyyy );

    //	blend the results and return
    vec3 blend = Pf * Pf * Pf * (Pf * (Pf * 6.0 - 15.0) + 10.0);
    vec4 res0 = mix( hash_lowz, hash_highz, blend.z );
    vec4 blend2 = vec4( blend.xy, vec2( 1.0 - blend.xy ) );
    return dot( res0, blend2.zxzx * blend2.wwyy );
}

#define noise(x) (Value3D(x))

const mat3 m = mat3(0.0, 1.60,  1.20, -1.6, 0.72, -0.96, -1.2, -0.96, 1.28);
#if SIMPLIFIED_RENDERING == 0
float fbm(vec3 p)
{
	float f = 0.0;
	f += noise(p) / 2 ; p = m * p * 1.1;
	f += noise(p) / 4 ; p = m * p * 1.2;
	f += noise(p) / 6 ; p = m * p * 1.3;
	f += noise(p) / 12; p = m * p * 1.4;
	f += noise(p) / 24;
	return f;
}
#else
float fbm(vec3 p)
{
	float f = 0.0;
	f += noise(p) / 2 ; p = m * p * 1.1;
	f += noise(p) / 4 ; p = m * p * 2.2;
	f += noise(p) / 12 ;
	f *= 1.25;
	return f;
}
#endif

float csstep(float m0, float m1, float n0, float n1, float v)
{
	return smoothstep(m0, m1, v) * (1.0 - smoothstep(n0, n1, v));
}

void main()
{
	vec3 pos = normalize(dir);

	float cirrus  = cloudInfo.w * cirrus1;
	float cumulus = cloudInfo.w * cumulus1;

	float sunContrib = pow(max(0.0, dot(pos, normalize(sunDir))), 64.0);

	float wpContrib = (1.0 - smoothstep(-0.5, -0.2, pos.y)) * planeColor.w;
	fragColor.rgb = mix(skyColor, planeColor.rgb, wpContrib);
	fragColor.rgb = mix(fragColor.rgb, sunColor.rgb * sunColor.w * 1.3, sunContrib);

    vec3 day_extinction = vec3(1.0);
    vec3 night_extinction = vec3(1.0 - exp(sunDir.y)) * 0.2;
    vec3 extinction = mix(day_extinction, night_extinction, -sunDir.y * 0.2 + 0.5);

	// Cirrus Clouds
	float density = smoothstep(1.0 - cirrus, 1.0, fbm(pos.xyz / pos.y * 2.0 + time * 0.05)) * 0.3;
	fragColor.rgb = mix(fragColor.rgb, cloudInfo.rgb * extinction * 4.0, density * max(pos.y, 0.0));

	// Cumulus Clouds
	#if SIMPLIFIED_RENDERING == 0
	for (int i = 0; i < 2; i++)
	{
		//vec3 cpos = pos; cpos.y = smoothstep(-0.5, 1.5, pos.y); cpos.xz /= cpos.y; cpos *= 2.0;
		vec3 cpos = pos; cpos.y = smoothstep(-0.5, 1.5, pos.y); cpos.xz /= cpos.y;
		float density = smoothstep(1.0 - cumulus, 1.0, fbm((0.7 + float(i) * 0.01) * cpos + time * 0.3));
		fragColor.rgb = mix(fragColor.rgb, cloudInfo.rgb * extinction * density * 5.0, min(density, 1.0) * (max(pos.y, 0.0)));
	}
	#else
	{
		//vec3 cpos = pos; cpos.y = smoothstep(-0.5, 1.5, pos.y); cpos.xz /= cpos.y; cpos *= 2.0;
		vec3 cpos = pos; cpos.y = smoothstep(-0.5, 1.5, pos.y); cpos.xz /= cpos.y;
		float density = smoothstep(1.0 - cumulus, 1.0, fbm((0.7) * cpos + time * 0.3));
		fragColor.rgb = mix(fragColor.rgb, cloudInfo.rgb * extinction * density * 5.0, min(density, 1.0) * (max(pos.y, 0.0)));
	}
	#endif

	fragColor.a = (0.5 - csstep(-0.8, -0.0, -0.5, 0.3, pos.y));
}