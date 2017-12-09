#version 430 core
layout(std430,binding=3)buffer outputvbo
{
  vec4 final[];
};
out vec4 sum;
//wavelet
//colorMap,historycolorMap,normalMap,positionMap,
uniform sampler2D  colorMap,normalMap,historycolorMap,positionMap;
float c_phi=4, n_phi=128, p_phi=1, stepwidth=1;
const float kernel[5] = float[5](1.0 /16.0 ,1.0 / 4.0,3.0 / 8.0,1.0 / 4.0,1.0 / 16.0);
uniform vec2 offset[25];
uniform vec2 offset2by2[4];
uniform vec2 offset3by3[9];
uniform int screenSize[2];
uniform int width;
uniform int height;

in vec2 Texcoord;
//temporal
uniform sampler2D vectorMap;

void main() {
  sum=vec4(0.0);
  // sum=texture2D(positionMap,Texcoord);
  // sum.a=1.0;
  vec2 step = vec2(1./width, 1./height); // resolution
  vec4 cval = texture2D(colorMap, Texcoord.xy);
  vec4 nval = texture2D(normalMap, Texcoord.st);
  vec4 pval = texture2D(positionMap, Texcoord.st);
  vec4 hval = texture2D(historycolorMap, Texcoord.st);

  // final = (hval + cval) /2;
  float cum_w = 0.0;
  for(int i = 0; i < 25; i++) {

    vec2 uv = Texcoord.st + vec2(i%5-2,(i/5)-2)*step*stepwidth;
    //*stepwidth;
    vec4 ctmp = texture2D(colorMap, uv);
    vec4 t = cval - ctmp;
    float dist2 = dot(t,t);
    float c_w = min(exp(-(dist2)/c_phi), 1.0);

    vec4 ntmp = texture2D(normalMap, uv);
    t = nval - ntmp;
    dist2 = max(dot(t,t)/(stepwidth*stepwidth),0.0);
    float n_w = min(exp(-(dist2)/n_phi), 1.0);

    vec4 ptmp = texture2D(positionMap, uv);
    t = pval - ptmp;
    dist2 = dot(t,t);
    float p_w = min(exp(-(dist2)/p_phi),1.0);

    float weight = c_w * n_w * p_w;
    sum += ctmp * weight * kernel[i%5]*kernel[i/5];
    cum_w += weight* kernel[i%5]*kernel[i/5];
  }
  sum = sum/cum_w;
  uint position=uint(Texcoord.x*width+Texcoord.y*height);
  final[position]=sum;
  //final[0]=sum;



  vec4 history=vec4(0.0);
  vec2 motionvector=vec2(texture2D(vectorMap,Texcoord.st));
  vec2 startpoint=Texcoord.st+motionvector;

  float alpha=0.2;
  float epsilon=0.0001;
  int judgevalue=0;
  int count=0;


  for(int i=0;i<4;i++){  //2by2

    vec2 uv =startpoint+offset2by2[i]*step*stepwidth;
    vec4 ctmp = texture2D(colorMap, uv);
    vec4 ntmp = texture2D(normalMap, uv);
    vec4 ptmp = texture2D(positionMap, uv);
    vec4 tc = cval - ctmp;
    vec4 tn = nval - ntmp;
    vec4 tp = pval - ptmp;
    if(dot((cval-ctmp),(cval-ctmp))>epsilon)judgevalue+=1;
    if(dot((nval-ntmp),(nval-ntmp))>epsilon)judgevalue+=1;
    if(dot((pval-ctmp),(cval-ptmp))>epsilon)judgevalue+=1;

    if(judgevalue==0){
      history+=ctmp;
      count++;
    }
    judgevalue=0;

  }// end of for
  if(history!=vec4(0.0)){
    history/=count;
  }
  else{    // 3by3
    count=0;
    for(int i=0;i<9;i++){

      vec2 uv =startpoint+offset3by3[i]*step*stepwidth;
      vec4 ctmp = texture2D(colorMap, uv);
      vec4 ntmp = texture2D(normalMap, uv);
      vec4 ptmp = texture2D(positionMap, uv);
      vec4 tc = cval - ctmp;
      vec4 tn = nval - ntmp;
      vec4 tp = pval - ptmp;
      if(dot((cval-ctmp),(cval-ctmp))>epsilon)judgevalue+=1;
      if(dot((nval-ntmp),(nval-ntmp))>epsilon)judgevalue+=1;
      if(dot((pval-ctmp),(cval-ptmp))>epsilon)judgevalue+=1;

      if(judgevalue==0){
        history+=ctmp;
        count++;
      }
    }// end of for
    if(history!=vec4(0.0)){
      history/=count;
      count=0;
    }
    else history=cval;
  }
  final[position]=final[position]*(1-alpha)+history*alpha;

}
