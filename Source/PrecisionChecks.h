#pragma once
struct PrecisionChecks {
 static bool run(){
  for(double sr:{44100.,48000.,96000.})for(int profile:{0,1,2}){
   const auto& signal=TestSignal::get(profile,sr);ContinuousResponse reader;reader.setProfile(profile,sr);
   for(double gain:{30.,-30.}){
    double f=103.816,q=24,A=std::pow(10.,gain/40),w=2*MathConstants<double>::pi*f/sr,alpha=std::sin(w)/(2*q),a0=1+alpha/A;
    double b0=(1+alpha*A)/a0,b1=-2*std::cos(w)/a0,b2=(1-alpha*A)/a0,a1=b1,a2=(1-alpha/A)/a0,z1=0,z2=0;
    std::vector<float> result((size_t)signal.size);int cycles=jmax(4,(int)std::ceil(sr*20/signal.size));
    for(int pass=0;pass<cycles;++pass)for(int i=0;i<signal.size;++i){double x=signal.samples[(size_t)((i+127)%signal.size)],y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;result[(size_t)i]=(float)y;}
    if(!reader.analyse(result.data()))return false;
    double error=0,measuredPeak=-160;
    for(int k=1;k<signal.size/2;++k){double hz=k*sr/signal.size;if(hz<20 || hz>20000)continue;auto z=std::exp(std::complex<double>(0,-2*MathConstants<double>::pi*k/signal.size));auto h=(b0+b1*z+b2*z*z)/(1.+a1*z+a2*z*z);error=jmax(error,std::abs(reader.magnitude[(size_t)k]-20*std::log10(std::abs(h))));if(hz>80 && hz<130)measuredPeak=jmax(measuredPeak,double(reader.magnitude[(size_t)k]));}
    std::cout<<"EQ reference "<<sr<<" Hz profile="<<profile<<" gain="<<gain<<" max-bin-error="<<error<<" dB peak="<<measuredPeak<<" dB\n";
    if(error>0.02)return false;
   }
   std::vector<float> flat((size_t)signal.size);
   for(int i=0;i<signal.size;++i)flat[(size_t)i]=0.5f*signal.samples[(size_t)((i+signal.size-1)%signal.size)];
   if(!reader.analyse(flat.data()))return false;
   for(int k=1;k<signal.size/2;++k)if(std::abs(reader.magnitude[(size_t)k]+6.0206f)>0.001f || std::abs(reader.phase[(size_t)k])>0.01f)return false;
   std::fill(flat.begin(),flat.end(),0);if(reader.analyse(flat.data()))return false;flat[0]=std::numeric_limits<float>::quiet_NaN();if(reader.analyse(flat.data()))return false;
  }
  return true;
 }
};
