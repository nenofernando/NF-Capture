#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "BinaryData.h"
#include <complex>
#include "Continuous.h"
using namespace juce;
constexpr int order=16, count=1<<order;
class Processor;
class CaptureCanvas : public Component {
public:
 std::function<void(Graphics&)> draw;
 std::function<void(Point<float>)> doubleClick;
 void paint(Graphics& g) override {if(draw)draw(g);}
 void mouseDoubleClick(const MouseEvent& e) override {if(doubleClick)doubleClick(e.position);}
};
class Editor : public AudioProcessorEditor, private Timer {
 friend struct CaptureTest;
 friend struct ResizeChecks;
public:
 Editor(Processor&);
 void paint(Graphics&) override;
 void resized() override;
private:
 void timerCallback() override;
 void analyse();
 void paintCanvas(Graphics&);
 static constexpr int defaultWidth=1000, defaultHeight=NF_PROBE?340:660;
 CaptureCanvas canvas;
 Processor& p;
 Image logo;
 TextButton live{"LIVE: iniciar"}, arm{"Captura unica"}, hold{"Hold"}, compare{"Guardar A"}, save{"Exportar CSV"};
 Slider frequency;
 Label freqLabel;
 String status{"Pronto. Ative LIVE no Probe e aqui para acompanhar os knobs."};
 std::vector<float> magnitude, phase, reference, spectrum;
 dsp::FFT fft{order};
 std::vector<float> fftData=std::vector<float>(2*count,0.0f);
 bool frozen=false, captured=false, liveActive=false;
 ContinuousResponse continuous;
 std::array<float,ContinuousResponse::size> liveSamples{};
 int responseSize=count, referenceSize=count;
 double responseRate=48000, referenceRate=48000;
 std::unique_ptr<FileChooser> chooser;
};
class Processor : public AudioProcessor {
public:
 Processor():AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)){}
 const String getName() const override {return NF_PROBE?"NF Capture Probe":"NF Capture";}
 void prepareToPlay(double sr,int) override {rate=sr; pos=0; state=0; probePosition=0; previousMode=0; angle=0; pulse=false; fifo.reset();}
 void releaseResources() override {}
 bool isBusesLayoutSupported(const BusesLayout& b) const override {return (b.getMainOutputChannelSet()==AudioChannelSet::mono() || b.getMainOutputChannelSet()==AudioChannelSet::stereo()) && b.getMainInputChannelSet()==b.getMainOutputChannelSet();}
 void processBlock(AudioBuffer<float>& b,MidiBuffer&) override {
  ScopedNoDenormals guard;
  if(NF_PROBE){
   int request=fire.exchange(0);
   if(request==1) pulse=true;
   auto mode=signal.load(); auto hz=tone.load();
   if(mode!=previousMode){probePosition=0;previousMode=mode;}
   for(int i=0;i<b.getNumSamples();++i){
    float v=0;
    if(mode==1){v=0.125f*std::sin(angle); angle+=MathConstants<double>::twoPi*hz/rate; if(angle>=MathConstants<double>::twoPi)angle-=MathConstants<double>::twoPi;}
    if(mode==2){v=probePosition==0?0.25f:0.0f;probePosition=(probePosition+1)%ContinuousResponse::size;}
    if(pulse){v=0.25f;pulse=false;}
    if(mode!=0 || request==1) for(int c=0;c<b.getNumChannels();++c)b.setSample(c,i,v);
   }
   return;
  }
  for(int i=0;i<b.getNumSamples();++i){
   float v=b.getSample(0,i);
   if(state.load()==1 && std::abs(v)>0.000001f){pos=0;state.store(2);}
   if(state.load()==2){impulse[(size_t)pos++]=v;if(pos==count)state.store(3,std::memory_order_release);}
  }
  int a,n,c,m;fifo.prepareToWrite(b.getNumSamples(),a,n,c,m);
  for(int i=0;i<n;++i)stream[(size_t)(a+i)]=b.getSample(0,i);
  for(int i=0;i<m;++i)stream[(size_t)(c+i)]=b.getSample(0,n+i);
  fifo.finishedWrite(n+m);
 }
 AudioProcessorEditor* createEditor() override {return new Editor(*this);}
 bool hasEditor() const override {return true;}
 bool acceptsMidi() const override {return false;} bool producesMidi() const override {return false;} double getTailLengthSeconds() const override {return 0;}
 int getNumPrograms() override {return 1;} int getCurrentProgram() override {return 0;} void setCurrentProgram(int) override {} const String getProgramName(int) override {return {}; } void changeProgramName(int,const String&) override {}
 void getStateInformation(MemoryBlock& d) override {MemoryOutputStream out(d,false);out.writeInt(2);out.writeFloat(tone.load());out.writeBool(signal.load()==2);out.writeBool(liveEnabled.load());}
 void setStateInformation(const void* d,int n) override {MemoryInputStream in(d,(size_t)n,false);auto version=in.readInt();if(version==1 || version==2)tone.store(jlimit(20.0f,20000.0f,in.readFloat()));signal=0;liveEnabled=false;if(version==2){signal=in.readBool()?2:0;liveEnabled=in.readBool();}}
 std::atomic<bool> liveEnabled{false};
 std::atomic<int> state{0},fire{0},signal{0};std::atomic<float> tone{1000};
 double rate=48000,angle=0;int pos=0,probePosition=0,previousMode=0; bool pulse=false;
 std::array<float,count> impulse{};
 AbstractFifo fifo{count*4};std::array<float,count*4> stream{};
};
Editor::Editor(Processor& proc):AudioProcessorEditor(proc),p(proc){
 logo=ImageCache::getFromMemory(BinaryData::Logo_png,BinaryData::Logo_pngSize);
 canvas.draw=[this](Graphics& g){paintCanvas(g);};
 canvas.doubleClick=[this](Point<float> point){if(Rectangle<float>(20,8,175,82).contains(point))setSize(defaultWidth,defaultHeight);};
 addAndMakeVisible(canvas);
 setResizable(true,true);
 setResizeLimits(700,defaultHeight*7/10,1600,defaultHeight*16/10);
 getConstrainer()->setFixedAspectRatio(double(defaultWidth)/defaultHeight);
 setSize(defaultWidth,defaultHeight);
 liveActive=p.liveEnabled.load();
 for(auto* b:{&live,&arm,&hold,&compare,&save}){canvas.addAndMakeVisible(b);b->setColour(TextButton::buttonColourId,Colour(0xff824aa4));b->setColour(TextButton::textColourOffId,Colours::white);}
 if(NF_PROBE){
  live.setButtonText(p.signal.load()==2?"LIVE: parar":"LIVE: iniciar");
  status="Ative LIVE para enviar impulsos periodicos ao plugin em teste.";
  live.onClick=[this]{p.signal=p.signal.load()==2?0:2;live.setButtonText(p.signal.load()==2?"LIVE: parar":"LIVE: iniciar");status=p.signal.load()==2?"LIVE ativo: abra o NF Capture e ative LIVE para ver os knobs.":"LIVE parado: passagem de audio.";repaint();};
  arm.setButtonText("Enviar 1 impulso (-12 dBFS)");hold.setButtonText("Seno / parar");compare.setVisible(false);save.setVisible(false);
  arm.onClick=[this]{p.signal=0;live.setButtonText("LIVE: iniciar");p.fire=1;status="Impulso solicitado. O transporte da DAW precisa estar ativo.";repaint();};
  hold.onClick=[this]{p.signal=p.signal.load()==1?0:1;live.setButtonText("LIVE: iniciar");status=p.signal.load()?"Seno ativo: -18.06 dBFS de pico. Pare antes de capturar impulso.":"Gerador parado: passagem de audio.";repaint();};
  canvas.addAndMakeVisible(frequency);frequency.setColour(Slider::thumbColourId,Colour(0xffcf94fa));frequency.setColour(Slider::trackColourId,Colour(0xffad70d2));frequency.setColour(Slider::textBoxBackgroundColourId,Colour(0xff392346));frequency.setColour(Slider::textBoxTextColourId,Colours::white);frequency.setRange(20,20000,1);frequency.setSkewFactorFromMidPoint(1000);frequency.setValue(p.tone.load());frequency.setTextValueSuffix(" Hz");frequency.onValueChange=[this]{p.tone=(float)frequency.getValue();};
 }else{
  live.setButtonText(liveActive?"LIVE: parar":"LIVE: iniciar");
  live.onClick=[this]{if(!liveActive && p.state.load()==2){status="Aguarde a captura unica terminar antes de iniciar LIVE.";repaint();return;}liveActive=!liveActive;p.liveEnabled=liveActive;if(liveActive)p.state.store(0);frozen=false;hold.setButtonText("Hold");live.setButtonText(liveActive?"LIVE: parar":"LIVE: iniciar");if(liveActive){int a,n,c,m;p.fifo.prepareToRead(p.fifo.getNumReady(),a,n,c,m);p.fifo.finishedRead(n+m);}status=liveActive?"LIVE: ative tambem o LIVE do Probe. Medicao do canal esquerdo / mono.":"LIVE parado. Ultima curva mantida.";repaint();};
  arm.onClick=[this]{if(p.state.load()==2)return;liveActive=false;p.liveEnabled=false;frozen=false;live.setButtonText("LIVE: iniciar");hold.setButtonText("Hold"); p.state.store(1);status="Aguardando impulso do Probe. Canal medido: esquerdo / mono.";repaint();};
  hold.onClick=[this]{frozen=!frozen;hold.setButtonText(frozen?"Retomar":"Hold");};
  compare.onClick=[this]{if(captured){reference=magnitude;referenceSize=responseSize;referenceRate=responseRate;status="Curva A guardada nesta janela.";repaint();}};
  save.onClick=[this]{if(!captured)return;chooser=std::make_unique<FileChooser>("Exportar medicao",File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("NF-Capture.csv"),"*.csv");chooser->launchAsync(FileBrowserComponent::saveMode|FileBrowserComponent::canSelectFiles|FileBrowserComponent::warnAboutOverwriting,[this](const FileChooser& fc){auto file=fc.getResult();if(file==File())return;String csv="frequency_hz,magnitude_db,phase_relative_deg\n";for(size_t i=1;i<magnitude.size();++i)csv+=String(i*responseRate/responseSize,6)+","+String(magnitude[i],6)+","+String(phase[i],6)+"\n";status=file.replaceWithText(csv)?"CSV exportado.":"Nao foi possivel salvar o CSV.";repaint();});};
 }
 startTimerHz(15);
}
void Editor::resized(){
 float scale=jmin(float(getWidth())/defaultWidth,float(getHeight())/defaultHeight);
 canvas.setBounds(0,0,defaultWidth,defaultHeight);
 canvas.setTransform(AffineTransform::scale(scale).translated((getWidth()-defaultWidth*scale)*0.5f,(getHeight()-defaultHeight*scale)*0.5f));
live.setBounds(24,115,175,34);arm.setBounds(209,115,245,34);hold.setBounds(464,115,152,34);compare.setBounds(626,115,155,34);save.setBounds(791,115,185,34);frequency.setBounds(32,178,920,42);}
void Editor::analyse(){
 std::fill(fftData.begin(),fftData.end(),0.0f);
 std::copy(p.impulse.begin(),p.impulse.end(),fftData.begin());
 fft.performRealOnlyForwardTransform(fftData.data());
 magnitude.resize(count/2);phase.resize(count/2);
 for(int k=0;k<count/2;++k){std::complex<float> z{fftData[(size_t)2*k],fftData[(size_t)2*k+1]};magnitude[(size_t)k]=Decibels::gainToDecibels(std::abs(z)/0.25f,-160.0f);phase[(size_t)k]=std::arg(z)*180.0f/MathConstants<float>::pi;}
 captured=true;responseSize=count;responseRate=p.rate;p.state.store(0);status="Captura concluida | FFT 65536 | "+String(p.rate/count,3)+" Hz/bin | fase relativa ao inicio detectado";
}
void Editor::timerCallback(){
 if(NF_PROBE)live.setButtonText(p.signal.load()==2?"LIVE: parar":"LIVE: iniciar");
 if(!NF_PROBE){if(liveActive!=p.liveEnabled.load()){liveActive=p.liveEnabled.load();frozen=false;hold.setButtonText("Hold");live.setButtonText(liveActive?"LIVE: parar":"LIVE: iniciar");}
if(!frozen && p.state.load(std::memory_order_acquire)==3)analyse();
  if(liveActive){
   constexpr int frame=ContinuousResponse::size;
   while(p.fifo.getNumReady()>=frame*2){int a,n,c,m;p.fifo.prepareToRead(frame,a,n,c,m);p.fifo.finishedRead(n+m);}
   if(p.fifo.getNumReady()>=frame){int a,n,c,m;p.fifo.prepareToRead(frame,a,n,c,m);
    for(int i=0;i<n;++i)liveSamples[(size_t)i]=p.stream[(size_t)(a+i)];
    for(int i=0;i<m;++i)liveSamples[(size_t)(n+i)]=p.stream[(size_t)(c+i)];
    p.fifo.finishedRead(n+m);
    if(!frozen){
     if(continuous.analyse(liveSamples.data())){magnitude=continuous.magnitude;phase=continuous.phase;responseSize=frame;responseRate=p.rate;captured=true;spectrum.clear();status="LIVE | "+String(p.rate/frame,1)+" atualizacoes/s | "+String(p.rate/frame,2)+" Hz/bin | fase relativa ao pico";}
     else status="LIVE: sem sinal. Ative LIVE no Probe e o transporte da DAW.";
    }
   }
   repaint();return;
  }
  if(p.fifo.getNumReady()>=count){int a,n,c,m;p.fifo.prepareToRead(count,a,n,c,m);
   if(!frozen){std::fill(fftData.begin(),fftData.end(),0.0f);for(int i=0;i<n;++i)fftData[(size_t)i]=p.stream[(size_t)(a+i)];for(int i=0;i<m;++i)fftData[(size_t)(n+i)]=p.stream[(size_t)(c+i)];
    for(int i=0;i<count;++i)fftData[(size_t)i]*=0.5f-0.5f*std::cos(MathConstants<float>::twoPi*i/(count-1));
    fft.performFrequencyOnlyForwardTransform(fftData.data());spectrum.resize(count/2);for(int k=0;k<count/2;++k)spectrum[(size_t)k]=Decibels::gainToDecibels(fftData[(size_t)k]*4.0f/count,-160.0f);
   }p.fifo.finishedRead(n+m);
  }
 }repaint();
}
void Editor::paint(Graphics& g){g.fillAll(Colour(0xff17101d));}
void Editor::paintCanvas(Graphics& g){
 g.fillAll(Colour(0xff211729));g.setColour(Colour(0xff392346));g.fillRect(0,0,defaultWidth,100);
 g.drawImageWithin(logo,20,8,175,82,RectanglePlacement::centred);
 g.setColour(Colours::white);g.setFont(32);g.drawText(NF_PROBE?"NF CAPTURE  /  PROBE":"NF CAPTURE",200,20,600,40,Justification::centred);
 g.setColour(Colour(0xffebc5ff));g.setFont(13);g.drawText("NF AUDIO TOOLS     /     LAB BETA 0.2 / LIVE / R2",200,63,600,22,Justification::centred);
 g.setFont(13);g.setColour(Colours::lightgrey);g.drawText(status,24,defaultHeight-52,defaultWidth-48,24,Justification::centredLeft);
 g.setColour(Colour(0xffc1adcc));g.drawText(NF_PROBE?"Insira antes do plugin em teste. Ative LIVE no Probe e no Capture. Use pista sem audio.":"Violeta: resposta (dB)    Turquesa: curva A    Cinza: espectro (-120 a 0 dBFS)    Rosa: fase (-180 a +180 graus)",24,defaultHeight-28,defaultWidth-48,20,Justification::centredLeft);
 if(NF_PROBE){g.setColour(Colours::white);g.drawText("Fluxo: NF Capture Probe  >  seu plugin  >  NF Capture",32,245,900,26,Justification::centredLeft);return;}
 Rectangle<float> r(65,185,880,365);g.setColour(Colour(0xff291c34));g.fillRoundedRectangle(r,8);
 auto x=[&](double hz){return r.getX()+float(std::log(hz/20.0)/std::log(1000.0))*r.getWidth();};
 for(double hz:{20.,50.,100.,200.,500.,1000.,2000.,5000.,10000.,20000.}){g.setColour(Colour(0xff523960));g.drawVerticalLine((int)x(hz),r.getY(),r.getBottom());g.setColour(Colour(0xffc1adcc));g.drawText(hz>=1000?String(hz/1000,0)+"k":String(hz,0),(int)x(hz)-20,553,40,20,Justification::centred);}
 for(int db=-36;db<=36;db+=12){float y=r.getCentreY()-db*r.getHeight()/72;g.setColour(Colour(db==0?0xff9878a9:0xff523960));g.drawHorizontalLine((int)y,r.getX(),r.getRight());g.setColour(Colour(0xffc1adcc));g.drawText(String(db),14,(int)y-9,44,18,Justification::centredRight);}
 auto line=[&](const std::vector<float>& data,Colour color,int type,int bins,double sr){if(data.empty())return;Path path;bool began=false;for(int px=0;px<(int)r.getWidth();++px){double hz=20*std::pow(1000.0,px/r.getWidth());int k=jlimit(1,(int)data.size()-1,(int)std::round(hz*bins/sr));float value=data[(size_t)k], y=type==1?r.getBottom()-(value+120)*r.getHeight()/120:type==2?r.getCentreY()-value*r.getHeight()/360:r.getCentreY()-value*r.getHeight()/72;y=jlimit(r.getY(),r.getBottom(),y);if(!began){path.startNewSubPath(r.getX()+px,y);began=true;}else path.lineTo(r.getX()+px,y);}g.setColour(color);g.strokePath(path,PathStrokeType(1.5f));};
 line(spectrum,Colour(0xff526575),1,count,p.rate);line(phase,Colour(0xffc175e8),2,responseSize,responseRate);line(reference,Colour(0xff55d6d0),0,referenceSize,referenceRate);line(magnitude,Colour(0xffcf94fa),0,responseSize,responseRate);
}
AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor();}
