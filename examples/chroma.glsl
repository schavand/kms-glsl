  precision mediump float;
  
  uniform sampler2D maTexture;
  uniform sampler2D maVideo;
 
  vec3 keyColor = vec3(0.,1.,0.) ;
  float similarity = 0.4;
  float smoothness = 0.08;
  float spill =0.01;

  // From https://github.com/libretro/glsl-shaders/blob/master/nnedi3/shaders/rgb-to-yuv.glsl
  vec2 RGBtoUV(vec3 rgb) {
    return vec2(
      rgb.r * -0.169 + rgb.g * -0.331 + rgb.b *  0.5    + 0.5,
      rgb.r *  0.5   + rgb.g * -0.419 + rgb.b * -0.081  + 0.5
    );
  }

  vec4 ProcessChromaKey(vec2 texCoord) {
    vec4 rgba = texture2D(maTexture, texCoord);
    float chromaDist = distance(RGBtoUV(texture2D(maTexture, texCoord).rgb), RGBtoUV(keyColor));

    float baseMask = chromaDist - similarity;
    float fullMask = pow(clamp(baseMask / smoothness, 0., 1.), 1.5);
    rgba.a = fullMask;

    float spillVal = pow(clamp(baseMask / spill, 0., 1.), 1.5);
    float desat = clamp(rgba.r * 0.2126 + rgba.g * 0.7152 + rgba.b * 0.0722, 0., 1.);
    rgba.rgb = mix(vec3(desat, desat, desat), rgba.rgb, spillVal);

    return rgba;
  }


void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
 
   vec2 uv = fragCoord.xy;
   uv.x=2.0*uv.x/(iResolution.x+iResolution.y);
   uv.y=2.0*uv.y/(iResolution.x+iResolution.y);

    
   float sin_factor = sin(iTime);
   float cos_factor = cos(iTime);
   uv = (uv - vec2(iResolution.x/(iResolution.x+iResolution.y),iResolution.y/(iResolution.x+iResolution.y))) * mat2(cos_factor, sin_factor, -sin_factor, cos_factor) + vec2(iResolution.x/(iResolution.x+iResolution.y),iResolution.y/(iResolution.x+iResolution.y));


    vec4 testColor = ProcessChromaKey(fragCoord.xy/iResolution.xy);
    fragColor = mix(testColor,texture2D(maVideo,uv),1.0-testColor.a );
    //fragColor = texture2D(maTexture,fragCoord.xy/iResolution.xy);
  }