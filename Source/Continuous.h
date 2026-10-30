#pragma once
#include <juce_dsp/juce_dsp.h>
#include <complex>
#include <array>
// Periodic isolated impulses avoid the random-phase division artefacts of R3.
// Each complete period is an independent acquisition. LIVE peak: -30.10 dBFS.
struct TestSignal {
 int order,size;
 std::vector<float> samples,frequency;
 explicit TestSignal(int exponent):order(exponent),size(1<<exponent),samples((size_t)size,0),frequency((size_t)size*2,0){
  samples[0]=0.03125f;juce::dsp::FFT fft(order);std::copy(samples.begin(),samples.end(),frequency.begin());fft.performRealOnlyForwardTransform(frequency.data());
 }
 static const TestSignal& get(int profile,double rate=48000){static const TestSignal fast(13),balanced(16),precise(18),fastHi(14),balancedHi(17),preciseHi(19);if(rate>64000)return profile==0?fastHi:profile==2?preciseHi:balancedHi;return profile==0?fast:profile==2?precise:balanced;}
};
struct ContinuousResponse {
 static constexpr int order=16,size=1<<order;
 int profile=-1,length=size;
 double sampleRate=48000;
 bool shortResponse=false;
 const TestSignal* source=nullptr;
 std::unique_ptr<juce::dsp::FFT> fft;
 std::vector<float> data,magnitude,phase;
 std::vector<std::complex<float>> transfer;
 ContinuousResponse(){setProfile(1);}
 void setProfile(int index,double rate=48000){sampleRate=rate;index=juce::jlimit(0,2,index);const auto& reference=TestSignal::get(index,rate);if(index==profile && length==reference.size)return;profile=index;source=&reference;length=reference.size;fft=std::make_unique<juce::dsp::FFT>(reference.order);data.assign((size_t)length*2,0);transfer.resize((size_t)length/2+1);magnitude.clear();phase.clear();}
 bool analyse(const float* samples){
  float peak=0;for(int i=0;i<length;++i){if(!std::isfinite(samples[i]))return false;peak=juce::jmax(peak,std::abs(samples[i]));}if(peak<1.e-8f)return false;
  std::fill(data.begin(),data.end(),0);std::copy_n(samples,length,data.begin());fft->performRealOnlyForwardTransform(data.data());
  const auto& ref=source->frequency;
  for(int k=0;k<=length/2;++k){std::complex<float> y(data[(size_t)2*k],data[(size_t)2*k+1]),x(ref[(size_t)2*k],ref[(size_t)2*k+1]);transfer[(size_t)k]=y/x;if(!std::isfinite(transfer[(size_t)k].real()) || !std::isfinite(transfer[(size_t)k].imag()))return false;}
  std::fill(data.begin(),data.end(),0);
  for(int k=0;k<=length/2;++k){data[(size_t)2*k]=transfer[(size_t)k].real();data[(size_t)2*k+1]=transfer[(size_t)k].imag();}
  fft->performRealOnlyInverseTransform(data.data());int origin=0;
  for(int i=1;i<length;++i)if(std::abs(data[(size_t)i])>std::abs(data[(size_t)origin]))origin=i;
  double totalEnergy=0,outside=0;int radius=juce::jmin(length/3,(int)(sampleRate*0.05));
  for(int i=0;i<length;++i){double energy=double(data[(size_t)i])*data[(size_t)i];totalEnergy+=energy;int distance=(i-origin+length)%length;distance=juce::jmin(distance,length-distance);if(distance>radius)outside+=energy;}
  double tail=juce::jmax(0.,totalEnergy-double(data[(size_t)origin])*data[(size_t)origin]);
  shortResponse=totalEnergy>0 && (tail<totalEnergy*1.e-7 || outside/juce::jmax(tail,totalEnergy*1.e-12)<0.02);
  magnitude.resize((size_t)length/2);phase.resize((size_t)length/2);
  for(int k=1;k<length/2;++k){auto h=transfer[(size_t)k];magnitude[(size_t)k]=juce::Decibels::gainToDecibels(std::abs(h),-160.0f);phase[(size_t)k]=float(std::remainder(std::arg(h)+juce::MathConstants<double>::twoPi*k*origin/length,juce::MathConstants<double>::twoPi)*180.0/juce::MathConstants<double>::pi);}
  magnitude[0]=magnitude[1];phase[0]=phase[1];return true;
 }
};

// Publish only repeatable independent acquisitions. No frequency smoothing and
// no alteration of accepted bins. The previous confirmed curve stays untouched.
struct StableResponseGate {
 std::vector<float> previous,previousPhase;
 int confirmations=0;
 float maximumChange=0,maximumPhaseChange=0;
 void reset(){previous.clear();previousPhase.clear();confirmations=0;maximumChange=maximumPhaseChange=0;}
 bool submit(const std::vector<float>& values,const std::vector<float>& phases,double rate,int length){
  if(values.size()!=size_t(length/2) || phases.size()!=values.size()){reset();return false;}
  for(size_t k=0;k<values.size();++k)if(!std::isfinite(values[k]) || !std::isfinite(phases[k])){reset();return false;}
  if(previous.size()!=values.size()){previous=values;previousPhase=phases;confirmations=0;return false;}
  maximumChange=maximumPhaseChange=0;
  for(size_t k=1;k<values.size();++k){double hz=k*rate/length;if(hz>=20 && hz<=juce::jmin(20000.,rate/2)){maximumChange=juce::jmax(maximumChange,std::abs(values[k]-previous[k]));if(values[k]>-100 && previous[k]>-100)maximumPhaseChange=juce::jmax(maximumPhaseChange,std::abs(std::remainder(phases[k]-previousPhase[k],360.0f)));}}
  previous=values;previousPhase=phases;
  if(maximumChange>0.10f || maximumPhaseChange>0.5f){confirmations=0;return false;}
  ++confirmations;return true;
 }
};
