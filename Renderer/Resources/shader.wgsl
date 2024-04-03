struct VertexInput {
	@location(0) position: vec3f,
	@location(1) color: vec3f,
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) color: vec3f,
};

struct Uniforms {
	color: vec4f,
	time: f32,
};

//variable sits within the uniform address space
@group(0) @binding(0) var<uniform> uUniforms: Uniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	let ratio = 800.0 / 600.0; //target surface dimensions

	var offset = vec2f(-0.6875, -0.463);
	offset += 0.3 * vec2f(cos(uUniforms.time), sin(uUniforms.time));
	//Do nothing with offset for now

	let angle = uUniforms.time;
	let alpha: f32 = cos(angle);
	let beta: f32 = sin(angle);
	var position = vec3f(
		in.position.x,
		alpha * in.position.y + beta * in.position.z,
		alpha * in.position.z - beta * in.position.y,
	);

	var out: VertexOutput;
    out.position = vec4f(position.x, (position.y) * ratio, 0.0, 1.0);
	out.color = in.color;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    // We apply a gamma-correction to the color
	// We need to convert our input sRGB color into linear before the target
	// surface converts it back to sRGB.
	let linear_color = pow(in.color * uUniforms.color.rgb, vec3f(2.2));
	return vec4f(linear_color, uUniforms.color.a);
}
