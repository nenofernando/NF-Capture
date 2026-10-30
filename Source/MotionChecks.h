#pragma once
// Exercise actual parameter changes, not just static frequency-response fixtures.
struct MotionChecks {
 struct Band {
  double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
  void set(double sr,double f,double q,double gain){double A=std::pow(10.,gain/40),w=2*MathConstants<double>::pi*f/sr,alpha=std::sin(w)/(2*q),a0=1+alpha/A;b0=(1+alpha*A)/a0;b1=-2*std::cos(w)/a0;b2=(1-alpha*A)/a0;a1=b1;a2=(1-alpha/A)/a0;}
  double process(double x){double y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return y;}
  double db(int k,int size)const{auto z=std::exp(std::complex<double>(0,-2*MathConstants<double>::pi*k/size));return 20*std::log10(std::abs((b0+b1*z+b2*z*z)/(1.+a1*z+a2*z*z)));}
 };
 static bool run(){
  for(int profile:{0,1,2}){
   double sr=44100;const auto& input=TestSignal::get(profile,sr);ContinuousResponse reader;reader.setProfile(profile,sr);StableResponseGate gate;Band band;band.set(sr,4180,.9,26.8);
   std::vector<float> output((size_t)input.size),confirmed;
   auto acquire=[&](){for(int i=0;i<input.size;++i)output[(size_t)i]=(float)band.process(input.samples[(size_t)i]);if(!reader.analyse(output.data()))return -1;bool valid=gate.submit(reader.magnitude,reader.phase,sr,input.size);if(valid)confirmed=reader.magnitude;return valid?1:0;};
   for(int i=0;i<5;++i)if(acquire()<0)return false;
   if(confirmed.empty())return false;
   int rejected=0,previews=0;
   for(double hz:{1150.,3200.,600.,4100.,900.,2300.}){
    auto previous=confirmed;band.set(sr,hz,14.72,30);
    int result=acquire();if(result<0)return false;if(reader.shortResponse)++previews;if(result==0){++rejected;if(confirmed!=previous)return false;}
   }
   band.set(sr,1150,14.72,30);int accepted=0;
   for(int i=0;i<8;++i){int result=acquire();if(result<0)return false;accepted+=result;}
   if(rejected<5 || accepted==0 || previews==0)return false;
   double error=0;for(int k=1;k<input.size/2;++k){double hz=k*sr/input.size;if(hz>=20 && hz<=20000)error=jmax(error,std::abs(double(confirmed[(size_t)k])-band.db(k,input.size)));}
   if(error>0.02)return false;
   std::cout<<"Motion profile="<<profile<<" rejected="<<rejected<<" previews="<<previews<<" final-error="<<error<<" dB\n";
   band.set(sr,103.816,24,30);for(int i=0;i<jmax(5,(int)std::ceil(sr*20/input.size));++i)if(acquire()<0)return false;
   auto before=confirmed;
   for(int i=0;i<input.size;++i){if(i==(int)(sr*0.03))band.set(sr,4180,.9,26.8);output[(size_t)i]=(float)band.process(input.samples[(size_t)i]);}
   if(!reader.analyse(output.data()))return false;
   if(gate.submit(reader.magnitude,reader.phase,sr,input.size))return false;
   if(confirmed!=before)return false;
   int recovered=0;for(int i=0;i<5;++i){int result=acquire();if(result<0)return false;recovered+=result;}
   if(recovered==0)return false;
   std::cout<<"Mid-period parameter change profile="<<profile<<" rejected and recovered\n";

  }
  return true;
 }
};
