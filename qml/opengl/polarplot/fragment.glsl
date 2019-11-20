//The source texture that will be transformed
uniform sampler2D src;
uniform float angle;

//The texture coordinate passed in from the vertex shader (the default luxe vertex shader is suffice)
varying vec2 coord;

//x: width of the texture divided by the radius that represents the top of the image (normally screen width / radius)
//y: height of the texture divided by the radius representing the top of the image (normally screen height / radius)
//uniform vec2 sizeOverRadius;

//This is a constant to determine how far left/right on the source image we look for additional samples
//to counteract thin lines disappearing towards the center (a somewhat adjusted texel width in uv-coordinates)
//This value is 1 / (texture width * 2 PI)
//uniform float sampleOffset;

//Linear blend factor that swaps the direction the y-axis of the source is mapped onto the radius
//if the value is 1, y = 0 is at the outer edge of the circle,
//if the value is 0, y = 0 is the center of the circle
//uniform float polarFactor;

void main() {
    vec2 sizeOverRadius = vec2(2.0, 2.0);
    float sampleOffset = 0;
    float polarFactor = 1.0;
	//Make relative to center
	vec2 relPos = coord - vec2(0.5 ,0.5);

	//Adjust for screen ratio
	relPos *= sizeOverRadius;

	//Normalised polar coordinates.
	//y: radius from center
	//x: angle
	vec2 polar;

	polar.y = sqrt(relPos.x * relPos.x + relPos.y * relPos.y);

	//Any radius over 1 would go beyond the source texture size, this simply outputs black for those fragments
	if(polar.y > 1.0){
		gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
		return;
	}

	polar.x = atan(relPos.y, relPos.x);

	//Normally, the angle starts from the axis going right and is counter-clockwise
	//I want the left edge of the screen image to be the line upwards,
	//so I rotate the angle half pi clockwise
	polar.x += 3.1415/2.0;

	//Normalise from angle to 0-1 range
	polar.x /= 6.28318530718;

	//Make clockwise
	//polar.x = -polar.x;

	polar.x = mod(polar.x, 1.0);

	//The xOffset fixes lines disappearing towards the center of the coordinate system
	//This happens because there's only a few pixels trying to display the whole width of the source image
	//so they 'miss' the lines. To fix this, we sample at the transformed position
	//and a bit to the left and right of it to catch anything we might miss.
	//Using 1 / radius gives us minimal offset far out from the circle,
	//and a wide offset for pixels close to the center

	float xOffset = 0.0;
	if(polar.y != 0.0){
		xOffset = 1.0 / polar.y;
	}

	//Adjusts for texture resolution
	xOffset *= sampleOffset;

	//This inverts the radius variable depending on the polarFactor
	polar.y = polar.y * polarFactor + (1.0 - polar.y) * (1.0 - polarFactor);

	//Sample at positions with a slight offset
	vec4 one = texture2D(src, vec2(polar.x - xOffset, polar.y));

	vec4 two = texture2D(src, polar);

	vec4 three = texture2D(src, vec2(polar.x + xOffset, polar.y));

	//Take the maximum of the three samples. This is not ideal, but the quickest way to choose a coloured sample over the background colour.
    /*
    vec2 dista = abs(coord - vec2(0.5, 0.5));
    if(distance(dista, vec2(0.5, 0.5)) < 0.4) {
        gl_FragColor = vec4(1, 0, 0, 1);
        return;
    }*/
	gl_FragColor = max(max(one, two), three);
}
