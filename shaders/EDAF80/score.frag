#version 410
uniform vec2 iResolution;
uniform int score;
in vec2 fragCoord;
out vec4 fragColor;
void join(inout float a, float b)
{
    a = min(a, b);
}

void subt(inout float a, float b)
{
    a = max(a, -b);
}

float box(vec2 p, vec2 s)
{
    return max((abs(p)-s).x,(abs(p)-s).y);
}

float digit(vec2 p, int d)
{
    #define join(a) join(r, a)
    #define subt(a) subt(r, a)
    float r = 100000000.0;
    switch(d)
    {
        case(0):
            join(box(p, vec2(0.1, 0.15)));
            subt(box(p, vec2(0.05, 0.1)));
            break;
        case(1):
            join(box(p, vec2(0.025, 0.15)));
            break;
        case(2):
            join(box(p, vec2(0.1, 0.15)));
            subt(box(p-vec2(0.15, -0.06), vec2(0.2, 0.035)));
            subt(box(p-vec2(-0.15, 0.06), vec2(0.2, 0.035)));
            break;
        case(3):
            join(box(p, vec2(0.1, 0.15)));
            subt(box(p-vec2(-0.15, -0.06), vec2(0.2, 0.035)));
            subt(box(p-vec2(-0.15, 0.06), vec2(0.2, 0.035)));
            break;
        case(4):
            join(box(p-vec2(0.075, 0.0), vec2(0.025, 0.15)));
            join(box(p-vec2(0.0, 0.05), vec2(0.1, 0.1)));
            subt(box(p-vec2(0.0, 0.09), vec2(0.05, 0.085)));
            break;
        case(5):
            join(box(p, vec2(0.1, 0.15)));
            subt(box(p-vec2(0.15, 0.06), vec2(0.2, 0.035)));
            subt(box(p-vec2(-0.15,-0.06), vec2(0.2, 0.035)));
            break;
        case(6):
            p = -p;
            join(box(p-vec2(0.075, 0.0), vec2(0.025, 0.15)));
            join(box(p-vec2(0.0, 0.05), vec2(0.1, 0.1)));
            subt(box(p-vec2(0.0, 0.05), vec2(0.05, 0.05)));
            break;
        case(7):
            join(box(p-vec2(0.0), vec2(0.1, 0.15)));
            subt(box(p-vec2(-0.05), vec2(0.1, 0.15)));
            break;
        case(8):
            join(box(p-vec2(0.0), vec2(0.1, 0.15)));
            subt(box(p-vec2(0.0, 0.05), vec2(0.05, 0.05)));
            subt(box(p-vec2(0.0, -0.05), vec2(0.05, 0.05)));
            join(box(p-vec2(0.0), vec2(0.1, 0.025)));
            break;
        case(9):
            join(box(p-vec2(0.075, 0.0), vec2(0.025, 0.15)));
            join(box(p-vec2(0.0, 0.05), vec2(0.1, 0.1)));
            subt(box(p-vec2(0.0, 0.05), vec2(0.05, 0.05)));
            break;
    }
    return r;
}

float getDig(float num, float d)
{
    float a = floor(num/pow(10., d));
    return a - floor(a/10.)*10.;
}

float print(vec2 p, float num, float spacing)
{
    float r = 10000.0;
    int fig = int(ceil(log2(num) / 3.32192809489)); // log base 10 of x equals log base 2 of x divided by log base 2 of 10
	if (num <= 10){
		for(int i = 0; i <= fig; i++)
		{
        int di = int(getDig(num, float(i)));
        join(digit(p+vec2(i, 0)*spacing, di));
		}
	}
	else {
    for(int i = 0; i < fig; i++)
    {
        int di = int(getDig(num, float(i)));
        join(digit(p+vec2(i, 0)*spacing, di));
    }
	}
    return r;
}

vec3 printFR(vec2 p)
{
    return 1.-vec3(step(0., print((p-vec2(0.5, 0.5))*9., float(10), 0.3)));
}
void main()
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord;

    // Time varying pixel color
    //int d = int(getDig(143., 0.0));
    //vec3 col = vec3(step(0., digit(uv, d))-0.1*step(0., box(uv, vec2(0.1, 0.15))));
    vec3 col = printFR(uv);

    // Output to screen
    fragColor = vec4(col,1.0);
}
