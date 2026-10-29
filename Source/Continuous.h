#pragma once
#include <juce_dsp/juce_dsp.h>
#include <complex>
#include <array>
// Deterministic periodic multisine. Every non-DC/Nyquist bin is excited.
// RMS is approximately -36.12 dBFS in each profile. Never initialise on audio thread.
struct TestSignal {
 int order,size;
 std::vector<float> samples,frequency;
 explicit TestSignal(int exponent):order(exponent),size(1<<exponent),samples((size_t)size),frequency((size_t)size*2,0){
  juce::dsp::FFT fft(order);uint32_t seed=0x4e464350;
  float amplitude=0.015625f*std::sqrt(float(size));
  for(int k=1;k<size/2;++k){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;double phi=juce::MathConstants<double>::twoPi*double(seed)/4294967296.0;frequency[(size_t)2*k]=amplitude*float(std::cos(phi));frequency[(size_t)2*k+1]=amplitude*float(std::sin(phi));}
  auto inverse=frequency;fft.performRealOnlyInverseTransform(inverse.data());std::copy_n(inverse.begin(),size,samples.begin());
  // Use the actual rounded sample sequence as reference, not its ideal spectrum.
  std::fill(frequency.begin(),frequency.end(),0);std::copy(samples.begin(),samples.end(),frequency.begin());fft.performRealOnlyForwardTransform(frequency.data());
 }
 static const TestSignal& get(int profile,double rate=48000){static const TestSignal fast(13),balanced(16),precise(18),fastHi(14),balancedHi(17),preciseHi(19);if(rate>64000)return profile==0?fastHi:profile==2?preciseHi:balancedHi;return profile==0?fast:profile==2?precise:balanced;}
};
struct ContinuousResponse {
 static constexpr int order=16,size=1<<order;
 int profile=-1,length=size;
 const TestSignal* source=nullptr;
 std::unique_ptr<juce::dsp::FFT> fft;
 std::vector<float> data,magnitude,phase;
 std::vector<std::complex<float>> transfer;
 ContinuousResponse(){setProfile(1);}
 void setProfile(int index,double rate=48000){index=juce::jlimit(0,2,index);const auto& reference=TestSignal::get(index,rate);if(index==profile && length==reference.size)return;profile=index;source=&reference;length=reference.size;fft=std::make_unique<juce::dsp::FFT>(reference.order);data.assign((size_t)length*2,0);transfer.resize((size_t)length/2);magnitude.clear();phase.clear();}
 bool analyse(const float* samples){
  float peak=0;for(int i=0;i<length;++i){if(!std::isfinite(samples[i]))return false;peak=juce::jmax(peak,std::abs(samples[i]));}if(peak<1.e-8f)return false;
  std::fill(data.begin(),data.end(),0);std::copy_n(samples,length,data.begin());fft->performRealOnlyForwardTransform(data.data());
  const auto& ref=source->frequency;
  for(int k=1;k<length/2;++k){std::complex<float> y(data[(size_t)2*k],data[(size_t)2*k+1]),x(ref[(size_t)2*k],ref[(size_t)2*k+1]);transfer[(size_t)k]=y/x;if(!std::isfinite(transfer[(size_t)k].real()) || !std::isfinite(transfer[(size_t)k].imag()))return false;}
  std::fill(data.begin(),data.end(),0);
  for(int k=1;k<length/2;++k){data[(size_t)2*k]=transfer[(size_t)k].real();data[(size_t)2*k+1]=transfer[(size_t)k].imag();}
  fft->performRealOnlyInverseTransform(data.data());int origin=0;
  for(int i=1;i<length;++i)if(std::abs(data[(size_t)i])>std::abs(data[(size_t)origin]))origin=i;
  magnitude.resize((size_t)length/2);phase.resize((size_t)length/2);
  for(int k=1;k<length/2;++k){auto h=transfer[(size_t)k];magnitude[(size_t)k]=juce::Decibels::gainToDecibels(std::abs(h),-160.0f);phase[(size_t)k]=float(std::remainder(std::arg(h)+juce::MathConstants<double>::twoPi*k*origin/length,juce::MathConstants<double>::twoPi)*180.0/juce::MathConstants<double>::pi);}
  magnitude[0]=magnitude[1];phase[0]=phase[1];return true;
 }
};
