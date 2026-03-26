// A very cool shader that makes very nice effects
// Alas, I don't know who made the original, but I've played a lot with it and
// made it my own, so, ha

uniform float dThing = 2.5;
uniform float mod1 = 2.0;
uniform float mod2 = 4.0;

uniform vec3 colourMod1 = vec3(5.0, 2.5, 3.0);

uniform float mod3 = 2.5;
uniform float mod4 = 0.5;

mat2 m(float a)
{
    float c=cos(a), s=sin(a);
    return mat2(c,-s,s,c);
}

float map(vec3 p)
{
    p.xz *= m(iTime * 0.4);p.xy*= m(iTime * 0.1);
    vec3 q = p * mod1 + iTime;
    return length(p+vec3(sin(iTime * 0.7))) * log(length(p) + 1.0) + sin(q.x + sin(q.z + sin(q.y))) * 0.5 - 1.0;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 a = gl_FragCoord.xy / iResolution.y - vec2(0.9, 0.5);
    vec3 cl = vec3(0.0);
    float d = dThing;

    for (int i = 0; i <= 5.0; i++)
    {
        vec3 p = vec3(0, 0, mod2) + normalize(vec3(a, -1.0)) * d;
        float rz = map(p);
        float f =  clamp((rz - map(p + 0.1)) * mod4, -0.1, 1.0);
        vec3 l = vec3(0.1, 0.3, 0.4) + vec3(colourMod1) * f;
        cl = cl * l + smoothstep(mod3, 0.0, rz) * 0.6 * l;
        d += min(rz, 1.0);
    }
    fragColor = vec4(cl, 1.0);
}
