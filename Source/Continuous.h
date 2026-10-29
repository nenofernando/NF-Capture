#pragma once
#include <juce_dsp/juce_dsp.h>
#include <complex>
// Periodic impulses permit exact DFT magnitude measurement for settled LTI systems.
// Phase is relative to the largest absolute sample in the received period.
struct ContinuousResponse {
 static constexpr int order=13, size=1<<order;
 juce::dsp::FFT fft{order};
 std::array<float,size*2> data{};
 std::vector<float> magnitude,phase;
 bool analyse(const float* samples){
  int peak=0;
  for(int i=1;i<size;++i)if(std::abs(samples[i])>std::abs(samples[peak]))peak=i;
  if(std::abs(samples[peak])<1.0e-7f)return false;
  data.fill(0);
  for(int i=0;i<size;++i)data[(size_t)i]=samples[(i+peak)%size];
  fft.performRealOnlyForwardTransform(data.data());
  magnitude.resize(size/2);phase.resize(size/2);
  for(int k=0;k<size/2;++k){std::complex<float> z(data[(size_t)2*k],data[(size_t)2*k+1]);magnitude[(size_t)k]=juce::Decibels::gainToDecibels(std::abs(z)/0.25f,-160.0f);phase[(size_t)k]=std::arg(z)*180.0f/juce::MathConstants<float>::pi;}
  return true;
 }
};
