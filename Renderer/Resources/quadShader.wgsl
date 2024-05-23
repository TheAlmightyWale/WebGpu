struct VertexInput {
    @location(0) position: vec3f,
    @location(1) texCoord: vec2f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) texCoord: vec2f,
    @location(1) @interpolate(flat) texIndex: u32,
    @location(2) @interpolate(flat) instance : u32,
};

struct Camera {
    //x,y are camera center, z,w are camera extents (width, height)
    posExtent: vec4f,
}

struct Animation {
    startCoord: vec2f,
    frameDim: vec2f,
    currFrame: u32,
    _padding0: f32,
    _padding: vec2f, //can't use vec3f here as it actully aligns on 16bytes, rather than 12
}

struct Transform {
    position: vec3f,
    texIndex: u32,
    scale: vec2f,
    _padding: vec2f,
}

const k_maxInstancesPerDraw = 100;

@group(0) @binding(0) var<uniform> uTransforms: array<Transform, k_maxInstancesPerDraw>;
@group(0) @binding(1) var textures: texture_2d_array<f32>;
@group(0) @binding(2) var txSampler: sampler;
@group(0) @binding(3) var<uniform> uCamera: Camera;
@group(0) @binding(4) var<uniform> uAnimations: array<Animation, k_maxInstancesPerDraw>;

@vertex
fn vs_main(in: VertexInput, @builtin(instance_index) instance: u32) -> VertexOutput {
    var out: VertexOutput;
    var transform = uTransforms[instance];
    //Model transform
    out.position = vec4f((transform.position.xy + transform.scale.xy * in.position.xy).xy, transform.position.z + in.position.z, 1.0f);
    //View space
    out.position = vec4f(out.position.xy - uCamera.posExtent.xy, out.position.z, out.position.w);
    //NDC projection 
    out.position = vec4f(out.position.x / uCamera.posExtent.z,
                         out.position.y / uCamera.posExtent.w,
                         out.position.zw
        );
    out.texCoord = in.texCoord;
    out.texIndex = transform.texIndex;
    out.instance = instance;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let anim: Animation = uAnimations[in.instance];
    let dims: vec2f = vec2f(textureDimensions(textures));
    let offset: vec2f = vec2f(f32(anim.currFrame) * anim.frameDim.x + anim.startCoord.x, anim.startCoord.y);
    let normOffset: vec2f = offset / dims; //offset in texture space
    let spriteDim: vec2f = anim.frameDim / dims; //dimensions in texture space
    let spriteTexCoords: vec2f = in.texCoord * spriteDim + normOffset;

    let color = textureSample(textures, txSampler, spriteTexCoords, in.texIndex);
    //gamma-correction
    return vec4f(pow(color.xyz, vec3f(2.2)).xyz, color.a);
}
