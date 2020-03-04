uniform sampler2D tex;
in vec4 colorOut;
in vec2 texUV_out;

out vec4 color;

float smoothing = 0.1f;

void main (void) {

	vec4 texColor = texture(tex, texUV_out);
	float alpha = texColor.w;
	if(alpha == 0) discard; 

	vec4 c = colorOut;
	c.a = 1.0f - smoothstep(0.5f - smoothing, 0.5f + smoothing, alpha);
	
    color = c;
}