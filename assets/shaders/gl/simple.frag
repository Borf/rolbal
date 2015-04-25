uniform sampler2D s_texture;

varying vec3 normal;
varying vec2 texCoord;


void main()
{
	float c = 0.5 + 0.5 * dot(normalize(vec3(1,1,1)), normalize(normal));


	gl_FragColor = texture2D(s_texture, texCoord) * c;
}
