#version 450

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0; // wave
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

uniform float time;
uniform float speed;
uniform sampler2D noiseTexture;

vec4 lerp(vec4 start, vec4 end, float t) {
    return (1.0-t)*start + t*end;
}

float easeInOutCubic(float x) {
    float x3 = -2.0 * x + 2.0;
    float xxx = x3 * x3 * x3;
    return x < 0.5 ? 4.0 * x * x * x : 1.0 - xxx / 2.0;
}

float easeOutQuint(float x) {
    float x_1 = 1.0 - x;
    return 1.0 - (x_1*x_1*x_1*x_1*x_1);
}

float easeOutCirc(float x) {
    return sqrt(1.0 - (x - 1.0)*(x - 1.0));
}

float easeOutCubic(float x) {
    return 1.0 - (1.0 - x)*(1.0 - x)*(1.0 - x);
}

float scale_func(float x) {
    float slope = 1.0/4.0;
    slope = 1.0/3.0;
    return x * slope;
}

float uv_y_prime(vec2 uv) {
    float start = scale_func(uv.x);
    float cover = 1.0 - 2.0*start;
    
    return start + uv.y * cover;
}

void main() {
    float timeSpeed = time * speed;

    //finalColor = texture(noiseTexture, fragTexCoord);
    //finalColor = texture(texture0, fragTexCoord + vec2(timeSpeed, 0));
    //finalColor = fragColor;
    // finalColor = vec4(0.0, fragTexCoord.g, 0.0, 1.0);
    // finalColor = vec4(texture(texture0, fragTexCoord.xy).xyz, 1.0);

    vec2 uv = fragTexCoord;

    float grad = uv.x;
    vec4 c1 = vec4(0.2, 0.2, 0.0, 1.0);
    vec4 c2 = vec4(1.0, 0.0, 0.0, 1.0);
    vec4 gradColor = lerp(c1, c2, grad);
    
    vec4 text_static = texture(texture0, vec2(uv.x, uv_y_prime(uv)));
    vec4 tex = texture(texture0, vec2(uv.x + timeSpeed, uv_y_prime(uv)));
    vec4 noise_tex = vec4(texture(noiseTexture, uv + vec2(timeSpeed, 0)).xxx, 1.0);
    
    // vec4 tex_color = lerp(vec4(0.0, 0.0, 0.0, 1.0), tex, tex.w);
    vec4 tex_color = tex;
    float noise = noise_tex.x;
    
    float noise_fade = 1.5 * easeOutCubic(grad) - noise;

    // Output to screen
    finalColor = tex_color * (gradColor * noise_fade * 20.0);

    // -- Tests

    // finalColor = tex_color * noise_fade * 1.0;
    
    // finalColor = text_static;
    // finalColor = texture(iChannel0, vec2(uv.x + iTime, uv.x/4.0 + uv.y*(1.0 - uv.x/2.0)));
    
    // finalColor = lerp(vec4(0.0, 0.0, 0.0, 1.0), tex, tex.w);
    
    // finalColor = vec4(noise_fade, noise_fade, noise_fade, 1.0);
    // finalColor = vec4(noise, noise, noise, 1.0);
    // finalColor = c2;
    
    // grad = grad * grad;
    // grad = easeOutCubic(grad);
    // finalColor = vec4(grad, grad, grad, 1.0);
    
    // finalColor = gradColor * 10.0;
}
