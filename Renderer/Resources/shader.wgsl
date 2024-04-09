struct VertexInput {
	@location(0) position: vec3f,
	@location(1) color: vec3f,
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) color: vec3f,
};

struct Uniforms {
	projection: mat4x4f,
	view: mat4x4f,
	model: mat4x4f,
	color: vec4f,
	time: f32,
};

//variable sits within the uniform address space
@group(0) @binding(0) var<uniform> uUniforms: Uniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
    out.position = uUniforms.projection * uUniforms.view * uUniforms.model * vec4f(in.position, 1.0);
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
