struct VertexInput {
    @location(0) position: vec3f,
    @location(1) texCoord: vec2f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) texCoord: vec2f,
};

struct Transform {
    position: vec3f,
    scale: vec2f,
}

//TODO  map to screen space transform uniforms


@group(0) @binding(0) var<uniform> uTransform: Transform;
@group(0) @binding(1) var texture: texture_2d<f32>;
@group(0) @binding(2) var txSampler: sampler;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4f((uTransform.position.xy + uTransform.scale.xy * in.position.xy).xy, uTransform.position.z + in.position.z, 1.0f);
    out.texCoord = in.texCoord;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let color = textureSample(texture, txSampler, in.texCoord);
    //gamma-correction
    return vec4f(pow(color.xyz, vec3f(2.2)).xyz, color.a);

}
