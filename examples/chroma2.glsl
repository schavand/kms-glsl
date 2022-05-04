  precision mediump float;
  
  uniform sampler2D maTexture0;
  uniform sampler2D maTexture1;
  uniform sampler2D maTexture2;
  uniform sampler2D maTexture3; 

  vec3 keyColor = vec3(0.,0.9,0.) ;
  float similarity = 0.4;
  float smoothness = 0.04;
  float spill =0.02;

  // From https://github.com/libretro/glsl-shaders/blob/master/nnedi3/shaders/rgb-to-yuv.glsl
  vec2 RGBtoUV(vec3 rgb) {
    return vec2(
      rgb.r * -0.169 + rgb.g * -0.331 + rgb.b *  0.5    + 0.5,
      rgb.r *  0.5   + rgb.g * -0.419 + rgb.b * -0.081  + 0.5
    );
  }

  vec4 ProcessChromaKey(vec2 texCoord) {
    vec4 rgba = texture2D(maTexture0, texCoord);
    float chromaDist = distance(RGBtoUV(texture2D(maTexture0, texCoord).rgb), RGBtoUV(keyColor));

    float baseMask = chromaDist - similarity;
    float fullMask = pow(clamp(baseMask / smoothness, 0., 1.), 1.5);
    rgba.a = fullMask;

    float spillVal = pow(clamp(baseMask / spill, 0., 1.), 1.5);
    float desat = clamp(rgba.r * 0.2126 + rgba.g * 0.7152 + rgba.b * 0.0722, 0., 1.);
    rgba.rgb = mix(vec3(desat, desat, desat), rgba.rgb, spillVal);

    return rgba;
  }


 vec4 ProcessChromaKey2(vec2 texCoord) {
    float chromaDist = distance(vec2(texture2D(maTexture2,texCoord).r,texture2D(maTexture3,texCoord).r), RGBtoUV(keyColor));
    vec4 rgba;

    float baseMask = chromaDist - similarity;
    float fullMask = pow(clamp(baseMask / smoothness, 0., 1.), 1.5);
    rgba.a = fullMask;

    float spillVal = pow(clamp(baseMask / spill, 0., 1.), 1.5);
    float desat = clamp(rgba.r * 0.2126 + rgba.g * 0.7152 + rgba.b * 0.0722, 0., 1.);
    vec3 yuv;                                           
    yuv.x = texture2D(maTexture1, texCoord).r;             
    yuv.y = texture2D(maTexture2, texCoord).r - 0.5f;      
    yuv.z = texture2D(maTexture3, texCoord).r - 0.5f;      
    mat3 trans = mat3(1, 1 ,1,                          
                      0, -0.34414, 1.772,               
                      1.402, -0.71414, 0                
                      );       
    rgba.rgb =  mix(vec3(desat, desat, desat), trans*yuv, spillVal);

    return rgba;
  }

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
 

   vec2 uv = fragCoord.xy;
   //uv.x=(1.65*uv.x-700.)/iResolution.x;
   //uv.y=1.0-uv.y/iResolution.y;
   uv.x=0.98557*uv.x / iResolution.x;
   uv.y=1.0-uv.y / iResolution.y;
    
    //float sin_factor = sin(iTime);
    //float cos_factor = cos(iTime);
    //uv = (uv - vec2(iResolution.x/(iResolution.x+iResolution.y),iResolution.y/(iResolution.x+iResolution.y))) * mat2(cos_factor, sin_factor, -sin_factor, cos_factor) + vec2(iResolution.x/(iResolution.x+iResolution.y),iResolution.y/(iResolution.x+iResolution.y));
    //uv.x= uv.x + 0.5*cos_factor;
    //uv.y = uv.y * sin_factor;

    //if (uv.x> 0.98557) uv.x=1.0-uv.x;

    //vec3 yuv;                                           
    //yuv.x = texture2D(maTexture1, uv).r;             
    //yuv.y = texture2D(maTexture2, uv).r - 0.5f;      
    //yuv.z = texture2D(maTexture3, uv).r - 0.5f;      
    //mat3 trans = mat3(1, 1 ,1,                          
    //                  0, -0.34414, 1.772,               
    //                  1.402, -0.71414, 0                
    //                  );                                

    vec4 testColor = ProcessChromaKey2(uv);
    fragColor = mix(testColor,texture2D(maTexture0,fragCoord.xy/iResolution.xy),1.0-testColor.a );

    //fragColor = mix(testColor,texture2D(maTexture0,uv),1.0-testColor.a );
    //fragColor = vec4 (trans*yuv,1.0);
   }
