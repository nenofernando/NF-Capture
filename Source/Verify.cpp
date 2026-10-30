#include "Plugin.cpp"
#include <iostream>
#include "ResizeChecks.h"
#include "PrecisionChecks.h"
#include "MotionChecks.h"
struct CaptureTest {
 static bool restored(Editor& e){return e.liveActive && e.live.getButtonText()=="LIVE: parar";}
 static void phaseSnapshot(Editor& e){e.view.setSelectedId(2,dontSendNotification);auto image=e.createComponentSnapshot(e.getLocalBounds());FileOutputStream output(File::getCurrentWorkingDirectory().getChildFile("NF-Capture-R4-Phase.png"));output.setPosition(0);output.truncate();PNGImageFormat png;png.writeImageToStream(image,output);e.view.setSelectedId(1,dontSendNotification);}
 static bool narrowPreview(Processor& p,Editor& e){
  e.quality.setSelectedId(3,sendNotificationSync);e.reference.clear();
  struct Band{double b0,b1,b2,a1,a2,z1=0,z2=0;Band(double sr,double f,double q,double gain){double A=std::pow(10.,gain/40),w=2*MathConstants<double>::pi*f/sr,alpha=std::sin(w)/(2*q),a0=1+alpha/A;b0=(1+alpha*A)/a0;b1=-2*std::cos(w)/a0;b2=(1-alpha*A)/a0;a1=b1;a2=(1-alpha/A)/a0;}double process(double x){double y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return y;}};
  Band low(p.rate,103.816,24,30),mid(p.rate,517,4.28,-30),high(p.rate,1760,8.91,30);
  const auto& signal=TestSignal::get(2,p.rate);MidiBuffer midi;
  for(int cycle=0;cycle<5;++cycle){for(int offset=0;offset<signal.size;offset+=256){AudioBuffer<float>b(2,256);for(int i=0;i<256;++i){float y=(float)high.process(mid.process(low.process(signal.samples[(size_t)(offset+i)])));b.setSample(0,i,y);b.setSample(1,i,y);}p.processBlock(b,midi);}e.timerCallback();}
  if(!e.captured)return false;
  e.cursorHz=103.816;e.view.setSelectedId(1,dontSendNotification);
  return true;
 }
 static bool run(Processor& p,Editor& e){
  e.quality.setSelectedId(2,sendNotificationSync);e.live.onClick();
  auto feed=[&](float gain){
   MidiBuffer midi;
   for(int offset=0;offset<ContinuousResponse::size;offset+=256){AudioBuffer<float>b(2,256);b.clear();for(int i=0;i<256;++i){int t=offset+i;float v=TestSignal::get(1).samples[(size_t)t]*gain;for(int c=0;c<2;++c)b.setSample(c,i,v);}p.processBlock(b,midi);}
   e.timerCallback();
  };
  feed(0.5f);if(e.captured)return false;feed(0.5f);if(!e.captured || e.responseSize!=ContinuousResponse::size)return false;
  auto a=e.magnitude;
  feed(1.0f);if(e.measurementCurrent || e.magnitude!=a || !e.previewReady || e.save.isEnabled())return false;for(size_t k=1;k<a.size();++k)if(std::abs((e.previewMagnitude[k]-a[k])-6.0205999f)>0.0001f)return false;feed(1.0f);for(size_t k=1;k<a.size();++k)if(std::abs((e.magnitude[k]-a[k])-6.0205999f)>0.0001f)return false;
  e.compare.onClick();auto held=e.magnitude;
  e.hold.onClick();feed(0.25f);if(e.magnitude!=held)return false;
  e.hold.onClick();feed(0.25f);if(e.measurementCurrent || e.magnitude!=held)return false;feed(0.25f);for(size_t k=1;k<held.size();++k)if(std::abs((e.magnitude[k]-held[k])+12.0411998f)>0.0001f)return false;
  AudioBuffer<float> overflow(2,(1<<20)+128);overflow.clear();MidiBuffer midi;p.processBlock(overflow,midi);if(p.dropped.load()==0)return false;e.timerCallback();if(e.captured || e.historyFill!=0)return false;feed(0.25f);feed(0.25f);if(!e.captured)return false;
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
 if(!PrecisionChecks::run())return 6;
 if(!MotionChecks::run())return 10;
 Processor preview; preview.prepareToPlay(48000,512); std::unique_ptr<AudioProcessorEditor> editor(preview.createEditor()); if(!CaptureTest::run(preview,*static_cast<Editor*>(editor.get())))return 4; if(!ResizeChecks::run(*static_cast<Editor*>(editor.get()),"Capture"))return 7; MemoryBlock saved;preview.getStateInformation(saved);Processor restored;restored.setStateInformation(saved.getData(),(int)saved.getSize());restored.prepareToPlay(48000,512);std::unique_ptr<AudioProcessorEditor> reopened(restored.createEditor());if(!restored.liveEnabled.load() || !CaptureTest::restored(*static_cast<Editor*>(reopened.get())))return 8;
 if(!CaptureTest::narrowPreview(preview,*static_cast<Editor*>(editor.get())))return 9; auto image=editor->createComponentSnapshot(editor->getLocalBounds()); FileOutputStream out(File::getCurrentWorkingDirectory().getChildFile("NF-Capture-preview.png")); out.setPosition(0); out.truncate(); PNGImageFormat png; png.writeImageToStream(image,out); auto* panel=static_cast<Editor*>(editor.get());CaptureTest::phaseSnapshot(*panel);
 std::cout<<"PASS: 9 configurations; FIR magnitude/phase within 1e-5 complex gain; audio passthrough exact; LIVE follows gain changes, Hold freezes and resumes; resizing, grip, hit testing and logo reset and LIVE restoration pass; three profiles, narrow EQ reference and dropped-buffer recovery and independent confirmation gate and moving-EQ rejection and responsive labelled preview pass.\n";
}
