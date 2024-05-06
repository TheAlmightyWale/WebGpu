struct VertexInput {
    @location(0) position: vec3f,
    @location(1) texCoord: vec2f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) texCoord: vec2f,
};

struct Camera {
    //x,y are camera center, z,w are camera extents (width, height)
    posExtent: vec4f,
}

struct Transform {
    position: vec3f,
    scale: vec2f,
}


@group(0) @binding(0) var<uniform> uTransform: Transform;
@group(0) @binding(1) var texture: texture_2d<f32>;
@group(0) @binding(2) var txSampler: sampler;
@group(0) @binding(3) var<uniform> uCamera: Camera;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    //Model transform
    out.position = vec4f((uTransform.position.xy + uTransform.scale.xy * in.position.xy).xy, uTransform.position.z + in.position.z, 1.0f);
    //View space
    out.position = vec4f(out.position.xy - uCamera.posExtent.xy, out.position.z, out.position.w);
    //NDC projection 
    out.position = vec4f(out.position.x / uCamera.posExtent.z,
                         out.position.y / uCamera.posExtent.w,
                         out.position.zw
        );
    out.texCoord = in.texCoord;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let color = textureSample(texture, txSampler, in.texCoord);
    //let color = vec4f(0.4,1.0,0.0,1.0);
    //gamma-correction
    return vec4f(pow(color.xyz, vec3f(2.2)).xyz, color.a);
}
