/* pmd/edge.vsh */
uniform mat4 modelViewProjectionMatrix;
uniform vec4 color;
attribute vec3 inPosition;
attribute vec3 inNormal; // unused
varying vec4 outColor;

void main() {
    outColor = color;
    gl_Position = modelViewProjectionMatrix * vec4(inPosition, 1);
}
