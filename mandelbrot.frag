#version 420 core

precision highp float;
precision highp int;

in vec2 uv;
out vec4 fragcolor;

uniform float time;
uniform vec2 relative;
uniform vec2 position;
uniform float radius;
uniform vec2 resolution;
uniform int swapped;
uniform int iterations;
uniform int palleteSize;
uniform float gradientMult;
uniform int burningShip;
uniform float power;
uniform float gradientOffset;

const int NUM_COLORS=20;

layout(std140,binding=0)uniform pallete{
	vec4 colors[NUM_COLORS];
};

vec3 gradient(vec4 colors[NUM_COLORS], int size, float t){
    for(int i = 0;i<size-1;i++){
        if( t <= float(i+1)/float(size-1))
            return mix(colors[i],colors[i+1],(t-i/float(size-1))*float(size-1)).xyz;
    }
}

vec2 complex_pow(vec2 z, float n){
    float theta = atan(z.y/z.x);
    float rn = pow(length(z),n);

    return vec2(rn*cos(n*theta), rn*sin(n*theta));
}

void z2pc(inout vec2 z, inout vec2 c, float n){
    vec2 zn = complex_pow(z,n);
    z.x=(zn.x+c.x);
    z.y=(zn.y+c.y);
}

void z2pc_abs(inout vec2 z, inout vec2 c, float n){
    vec2 zn = complex_pow(z,n);
    z.x=abs(zn.x+c.x);
    z.y=abs(zn.y+c.y);
}

void main(){
    vec2 st = uv*2-.5 ;
    st.x *= resolution.x / resolution.y;

	vec2 z = vec2(0);
    vec2 c=position+((st*4.)-2.)*radius;
    int ind = 0;
    const int mx=iterations;
    for(int i=0;i<mx;i++){
        ind=i;
        if(z.x*z.x+z.y*z.y>=4.)break;

        if(burningShip == 1)
            z2pc_abs(z,c,power);
        else z2pc(z,c,power);

        if(swapped == 1){
		    float tmp=z.x;
		    z.x=z.y;
		    z.y=tmp;
        }
    }
    float zmag=length(z);
    float lvl=( float(ind) + 1. - log(log(abs(zmag)) / log(2.)));
    vec3 color = ind>mx-10?
			vec3(0):
    		gradient(colors,palleteSize,mod(gradientOffset+gradientMult*lvl/float(mx),1));

    fragcolor = vec4(color,1.0);
}


//        float t=c.x*exp(z.x)*cos(z.y)-c.y*exp(z.x)*sin(z.y);
//        z.y    =c.x*exp(z.x)*sin(z.y)-c.y*exp(z.x)*cos(z.y);z.x=t;

//    float lvl=( float(ind) + 1. - log(log(abs(zmag)) / log(30.)));
//	float grad=log(float(ind))/log(zmag);
//ind>mx-10?
//			mix(mix(vec3(1,0,0), vec3(0,1,0), fract(grad)),vec3(0,0,1),fract((grad/2))):
//    		abs(vec3(sin(.2*lvl),
//					 sin(.3*lvl),
//					 sin(.4*lvl)));



