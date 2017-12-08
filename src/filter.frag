#version 330 core
out vec4 gl_FragData[0];

in vec3 gl_TexCoord[0];

//wavelet
uniform sampler2D colorMap, normalMap, posMap;
uniform float c_phi, n_phi, p_phi, stepwidth;
uniform float kernel[25];
uniform vec2 offset[25];
uniform vec2 offset2by2[4];
uniform vec2 offset3by3[9];


//temporal
uniform sampler2D vectorMap;

void main(void) {
vec4 sum = vec4(0.0);
vec2 step = vec2(1./512., 1./512.); // resolution
vec4 cval = texture2D(colorMap, gl_TexCoord[0].st);
vec4 nval = texture2D(normalMap, gl_TexCoord[0].st);
vec4 pval = texture2D(posMap, gl_TexCoord[0].st);

float cum_w = 0.0;
for(int i = 0; i < 25; i++) {

vec2 uv = gl_TexCoord[0].st + offset[i]*step*stepwidth;
vec4 ctmp = texture2D(colorMap, uv);
vec4 t = cval - ctmp;
float dist2 = dot(t,t);
float c_w = min(exp(-(dist2)/c_phi), 1.0);

vec4 ntmp = texture2D(normalMap, uv);
t = nval - ntmp;
dist2 = max(dot(t,t)/(stepwidth*stepwidth),0.0);
float n_w = min(exp(-(dist2)/n_phi), 1.0);

vec4 ptmp = texture2D(posMap, uv);
t = pval - ptmp;
dist2 = dot(t,t);
float p_w = min(exp(-(dist2)/p_phi),1.0);

float weight = c_w * n_w * p_w;
sum += ctmp * weight * kernel[i];
cum_w += weight*kernel[i];
}
gl_FragData[0] = sum/cum_w;

vec4 result=vec4(0,0);
vec2 motionvector=vec2(texture2D(vectorMap,gl_TexCoord[0].st));
vec2 startpoint=gl_Texcoord[0].st+motionvector;

float alpha=0.2;
float epsilon=1.0*e-5;
int judgevalue=0;
int count=0;

for(int i=0;i<4;i++){  //2by2

vec2 uv =startpoint+offset2by2[i]*step*stepwidth;
vec4 ctmp = texture2D(colorMap, uv);
vec4 ntmp = texture2D(normalMap, uv);
vec4 ptmp = texture2D(posMap, uv);
vec4 tc = cval - ctmp;
vec4 tn = nval - ntmp;
vec4 tp = pval - ptmp;
if(dot((cval-ctmp),(cval-ctmp))>epsilon)judgevalue+=1;
if(dot((nval-ntmp),(nval-ntmp))>epsilon)judgevalue+=1;
if(dot((pval-ctmp),(cval-ptmp))>epsilon)judgevalue+=1;

if(judgevalue==0){
result+=ctmp;
count++;
}
judgevalue=0;

}// end of for
if(result!=vec4(0,0)){
result/=count;
count=0;
}

else{    // 3by3
for(int i=0;i<9;i++){

vec2 uv =startpoint+offset3by3[i]*step*stepwidth;
vec4 ctmp = texture2D(colorMap, uv);
vec4 ntmp = texture2D(normalMap, uv);
vec4 ptmp = texture2D(posMap, uv);
vec4 tc = cval - ctmp;
vec4 tn = nval - ntmp;
vec4 tp = pval - ptmp;
if(dot((cval-ctmp),(cval-ctmp))>epsilon)judgevalue+=1;
if(dot((nval-ntmp),(nval-ntmp))>epsilon)judgevalue+=1;
if(dot((pval-ctmp),(cval-ptmp))>epsilon)judgevalue+=1;

if(judgevalue==0){
result+=ctmp;
count++;
}
}// end of for

if(result!=vec4(0,0)){
result/=count;
count=0;
}
else result=cval;

}
gl_FragData[0]=result*(1-alpha)+cval*alpha;

}





