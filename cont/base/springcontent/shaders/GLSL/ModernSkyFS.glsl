#version 130

in vec3 vPos;

uniform float time    = 0.0;

uniform vec4 cloudInfo = vec4(1.0, 1.0, 1.0, 0.5);
uniform vec4 planeColor; // .w signals if enabled
uniform vec3 scatterInfo = vec3(0.0020, 0.0009, 0.9200);
uniform vec3 sunDir;

const float cirrus1  = 0.8;
const float cumulus1 = 1.6;


#define Br (scatterInfo.x)
#define Bm (scatterInfo.y)
#define g  (scatterInfo.z)

/*
const float Br = 0.0320; // Rayleigh coefficient
const float Bm = 0.0005; // Mie coefficient
const float g =  0.9200; // Mie scattering direction. Should be ALMOST 1.0f
*/

const vec3 nitrogen = vec3(0.650, 0.570, 0.475);

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

void main()
{
	vec3 pos = normalize(vPos);
	pos.y = max(pos.y, -0.3);

	float cirrus  = cloudInfo.w * cirrus1;
	float cumulus = cloudInfo.w * cumulus1;

	vec3 Kr = Br / pow(nitrogen, vec3(4.0));
	vec3 Km = Bm / pow(nitrogen, vec3(0.84));

	// Atmosphere Scattering
	float mu = dot(normalize(vPos), normalize(sunDir));
	float rayleigh = 3.0 / (8.0 * 3.14) * (1.0 + mu * mu);
	vec3 mie = (Kr + Km * (1.0 - g * g) / (2.0 + g * g) / pow(1.0 + g * g - 2.0 * g * mu, 1.5)) / (Br + Bm);

	vec3 day_extinction = exp(-exp(-((pos.y + sunDir.y * 4.0) * (exp(-pos.y * 16.0) + 0.1) / 80.0) / Br) * (exp(-pos.y * 16.0) + 0.1) * Kr / Br) * exp(-pos.y * exp(-pos.y * 8.0 ) * 4.0) * exp(-pos.y * 2.0) * 4.0;
	vec3 night_extinction = vec3(1.0 - exp(sunDir.y)) * 0.2;
	vec3 extinction = mix(day_extinction, night_extinction, -sunDir.y * 0.2 + 0.5);
	const vec3 sunColor = vec3(0.992, 0.985, 0.827);
	fragColor.rgb = rayleigh * mie * extinction * sunColor;

	// Cirrus Clouds
	float density = smoothstep(1.0 - cirrus, 1.0, fbm(pos.xyz / pos.y * 2.0 + time * 0.05)) * 0.3;
	fragColor.rgb = mix(fragColor.rgb, cloudInfo.rgb * extinction * 4.0, density * max(pos.y, 0.0));

	// Cumulus Clouds
	for (int i = 0; i < 2; i++)
	{
		float density = smoothstep(1.0 - cumulus, 1.0, fbm((0.7 + float(i) * 0.01) * pos.xyz / pos.y + time * 0.3));
		fragColor.rgb = mix(fragColor.rgb, cloudInfo.rgb * extinction * density * 5.0, min(density, 1.0) * (max(pos.y, 0.0)));
	}


	fragColor.rgb = pow(1.0 - exp(-1.3 * fragColor.rgb), vec3(1.3));

	//fragColor = mix(fragColor, vec4(planeColor.rgb, 1.0), smoothstep(0.0, 0.8, -pos.y) * planeColor.w);

	fragColor = mix(vec4(planeColor.rgb, 1.0), fragColor, smoothstep(-0.7, 0.0, pos.y) * planeColor.w);
}