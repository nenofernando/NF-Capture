#include "Plugin.cpp"
#include <iostream>
#include "ResizeChecks.h"
struct CaptureTest {
 static bool restored(Editor& e){return e.liveActive && e.live.getButtonText()=="LIVE: parar";}
 static bool run(Processor& p,Editor& e){
  e.live.onClick();
  auto feed=[&](float gain){
   MidiBuffer midi;
   for(int offset=0;offset<ContinuousResponse::size;offset+=256){AudioBuffer<float>b(2,256);b.clear();for(int i=0;i<256;++i){int t=offset+i;float v=t==127?0.25f*gain:t==128?0.125f*gain:0.0f;for(int c=0;c<2;++c)b.setSample(c,i,v);}p.processBlock(b,midi);}
   e.timerCallback();
  };
  feed(0.5f);if(!e.captured || e.responseSize!=ContinuousResponse::size)return false;
  auto a=e.magnitude;
  feed(1.0f);for(size_t k=1;k<a.size();++k)if(std::abs((e.magnitude[k]-a[k])-6.0205999f)>0.0001f)return false;
  e.compare.onClick();auto held=e.magnitude;
  e.hold.onClick();feed(0.25f);if(e.magnitude!=held)return false;
  e.hold.onClick();feed(0.25f);for(size_t k=1;k<held.size();++k)if(std::abs((e.magnitude[k]-held[k])+12.0411998f)>0.0001f)return false;
  return true;
 }
};
int main(){
 ScopedJuceInitialiser_GUI gui;
 for(double sr:{44100.,48000.,96000.})for(int block:{64,511,1024}){
  Processor p;p.prepareToPlay(sr,block);p.state=1;MidiBuffer midi;int t=0;
  for(int j=0;j<count/block+20;++j){AudioBuffer<float>b(2,block);b.clear();for(int i=0;i<block;++i,++t){float v=t==127?0.125f:t==128?0.0625f:0.0f;b.setSample(0,i,v);b.setSample(1,i,v);} auto copy=b; p.processBlock(b,midi);for(int c=0;c<2;++c)for(int i=0;i<block;++i)if(copy.getSample(c,i)!=b.getSample(c,i))return 1;}
  if(p.state!=3)return 2;
  std::vector<float>d(2*count);std::copy(p.impulse.begin(),p.impulse.end(),d.begin());dsp::FFT fft(order);fft.performRealOnlyForwardTransform(d.data());
  for(int k=1;k<count/2;++k){double w=MathConstants<double>::twoPi*k/count;std::complex<double> expected=0.5+0.25*std::exp(std::complex<double>(0,-w));std::complex<double> actual(d[2*k]/0.25,d[2*k+1]/0.25);if(std::abs(actual-expected)>0.00001)return 3;}
 }
 for(int offset:{0,127,8191}){
  std::array<float,ContinuousResponse::size> samples{};
  samples[(size_t)offset]=0.125f;samples[(size_t)((offset+1)%ContinuousResponse::size)]=0.0625f;
  ContinuousResponse live;if(!live.analyse(samples.data()))return 5;
  for(int k=1;k<ContinuousResponse::size/2;++k){double w=MathConstants<double>::twoPi*k/ContinuousResponse::size;auto z=0.5+0.25*std::exp(std::complex<double>(0,-w));if(std::abs(live.magnitude[(size_t)k]-20*std::log10(std::abs(z)))>0.001 || std::abs(live.phase[(size_t)k]-std::arg(z)*180/MathConstants<double>::pi)>0.001)return 6;}
 }
 Processor preview; preview.prepareToPlay(48000,512); std::unique_ptr<AudioProcessorEditor> editor(preview.createEditor()); if(!CaptureTest::run(preview,*static_cast<Editor*>(editor.get())))return 4; if(!ResizeChecks::run(*static_cast<Editor*>(editor.get()),"Capture"))return 7; MemoryBlock saved;preview.getStateInformation(saved);Processor restored;restored.setStateInformation(saved.getData(),(int)saved.getSize());restored.prepareToPlay(48000,512);std::unique_ptr<AudioProcessorEditor> reopened(restored.createEditor());if(!restored.liveEnabled.load() || !CaptureTest::restored(*static_cast<Editor*>(reopened.get())))return 8;
 auto image=editor->createComponentSnapshot(editor->getLocalBounds()); FileOutputStream out(File::getCurrentWorkingDirectory().getChildFile("NF-Capture-preview.png")); out.setPosition(0); out.truncate(); PNGImageFormat png; png.writeImageToStream(image,out);
 std::cout<<"PASS: 9 configurations; FIR magnitude/phase within 1e-5 complex gain; audio passthrough exact; LIVE follows gain changes, Hold freezes and resumes; resizing, grip, hit testing and logo reset and LIVE restoration pass.\n";
}
