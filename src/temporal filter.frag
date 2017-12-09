
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
