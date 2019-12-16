varying vec2 coord;
uniform float ratio;
uniform sampler2D src;

void main() {
    // Mod is used to wrap around the 0-1 scaled x axis
    vec2 shiftedCoord = mod(coord + vec2(ratio, 0), 1.0);
    gl_FragColor = texture2D(src, shiftedCoord);
}
