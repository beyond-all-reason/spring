return {
	MainCode =
[[
	float currTime = frameInfo.x + frameInfo.y - createFrame;

	float life = currTime * invttl;
	float rotVal  = GetCurrentRotation(rotParams, currTime);
	float animVal = GetCurrentAnimation(animParams, currTime);

	float igrowth = sizeGrowth * (1.0 - (1.0 - life) * (1.0 - life));

	float isize = size * (igrowth + 1.0);
	float ilength = len * (igrowth + 1.0);

	vec4 currColor = GetCurrentColor(color0, color1, edge0, edge1, life);

	vec3 frontPos = pos + dir * frontOffset * ilength;

	vec3 zdir = (abs(dot(dir, vec3(0, 1, 0)/*UpVector*/)) >= 0.99) ? vec3(0, 0, 1)/*FwdVector*/: vec3(0, 1, 0)/*UpVector*/;
	vec3 xdir = normalize(cross(dir, zdir));
	vec3 ydir = normalize(cross(dir, xdir));

	vec2 sc = vec2(sin(rotVal), cos(rotVal));

	vec3 bounds[12] = vec3[12](
		  ydir * isize                ,
		  ydir * isize + dir * ilength,
		 -ydir * isize + dir * ilength,
		 -ydir * isize                ,

		  xdir * isize                ,
		  xdir * isize + dir * ilength,
		 -xdir * isize + dir * ilength,
		 -xdir * isize                ,

		 -xdir * isize + ydir * isize,
		  xdir * isize + ydir * isize,
		  xdir * isize - ydir * isize,
		 -xdir * isize - ydir * isize
	);

	for (uint i = 0u; i < uint(bounds.length()); ++i) {
		bounds[i] = Rotate(sc, dir, bounds[i]);
	}

	if (validTextures.x) {
		AddEffectsQuad(
			drawOrder,
			vec3(animParams.xy, animVal),
			pos + bounds[0],
			pos + bounds[1],
			pos + bounds[2],
			pos + bounds[3],
			sideTexture,
			currColor
		);

		AddEffectsQuad(
			drawOrder,
			vec3(animParams.xy, animVal),
			pos + bounds[4],
			pos + bounds[5],
			pos + bounds[6],
			pos + bounds[7],
			sideTexture,
			currColor
		);
	}

	if (validTextures.y) {
		AddEffectsQuad(
			drawOrder,
			vec3(animParams.xy, animVal),
			frontPos + bounds[ 8],
			frontPos + bounds[ 9],
			frontPos + bounds[10],
			frontPos + bounds[11],
			frontTexture,
			currColor
		);
	}
]]
}