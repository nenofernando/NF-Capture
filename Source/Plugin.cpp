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
 std::function<void(Point<float>)> doubleClick,move;
 void mouseMove(const MouseEvent& e) override {if(move)move(e.position);}
 void mouseExit(const MouseEvent&) override {if(move)move({-1,-1});}
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
 ComboBox quality,view,scaleChoice;
 double cursorHz=0;
 float inputPeak=0;
 uint32_t lastAudioTime=0;
 int stableFrames=0;
 Slider frequency;
 Label freqLabel;
 String status{"Pronto. Ative LIVE no Probe e aqui para acompanhar os knobs."};
 std::vector<float> magnitude, phase, reference, spectrum;
 dsp::FFT fft{order};
 std::vector<float> fftData=std::vector<float>(2*count,0.0f);
 bool frozen=false, captured=false, liveActive=false;
 ContinuousResponse continuous;
 std::vector<float> liveSamples,liveHistory;
 size_t historyWrite=0,historyFill=0;
 uint64_t seenDrops=0,seenGeneration=0;
 void resetLive();
 int responseSize=count, referenceSize=count;
 double responseRate=48000, referenceRate=48000;
 std::unique_ptr<FileChooser> chooser;
};
class Processor : public AudioProcessor {
public:
 Processor():AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)){(void)TestSignal::get(1);}
 const String getName() const override {return NF_PROBE?"NF Capture Probe":"NF Capture";}
 void prepareToPlay(double sr,int) override {rate=sr; pos=0; state=0; probePosition=0; previousMode=0;previousProfile=-1; angle=0; pulse=false; fifo.reset();generation.fetch_add(1);}
 void releaseResources() override {}
 bool isBusesLayoutSupported(const BusesLayout& b) const override {return (b.getMainOutputChannelSet()==AudioChannelSet::mono() || b.getMainOutputChannelSet()==AudioChannelSet::stereo()) && b.getMainInputChannelSet()==b.getMainOutputChannelSet();}
 void processBlock(AudioBuffer<float>& b,MidiBuffer&) override {
  ScopedNoDenormals guard;
  if(NF_PROBE){
   int request=fire.exchange(0);
   if(request==1) pulse=true;
   auto mode=signal.load(); auto hz=tone.load();auto profile=quality.load();const auto& excitation=TestSignal::get(profile,rate);
   if(mode!=previousMode || profile!=previousProfile){probePosition=0;previousMode=mode;previousProfile=profile;}
   for(int i=0;i<b.getNumSamples();++i){
    float v=0;
    if(mode==1){v=0.125f*std::sin(angle); angle+=MathConstants<double>::twoPi*hz/rate; if(angle>=MathConstants<double>::twoPi)angle-=MathConstants<double>::twoPi;}
    if(mode==2){v=excitation.samples[(size_t)probePosition];probePosition=(probePosition+1)%excitation.size;}
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
  fifo.finishedWrite(n+m);if(n+m<b.getNumSamples())dropped.fetch_add((uint64_t)(b.getNumSamples()-n-m));
 }
 AudioProcessorEditor* createEditor() override {return new Editor(*this);}
 bool hasEditor() const override {return true;}
 bool acceptsMidi() const override {return false;} bool producesMidi() const override {return false;} double getTailLengthSeconds() const override {return 0;}
 int getNumPrograms() override {return 1;} int getCurrentProgram() override {return 0;} void setCurrentProgram(int) override {} const String getProgramName(int) override {return {}; } void changeProgramName(int,const String&) override {}
 void getStateInformation(MemoryBlock& d) override {MemoryOutputStream out(d,false);out.writeInt(3);out.writeFloat(tone.load());out.writeBool(signal.load()==2);out.writeBool(liveEnabled.load());out.writeInt(quality.load());}
 void setStateInformation(const void* d,int n) override {MemoryInputStream in(d,(size_t)n,false);auto version=in.readInt();if(version>=1 && version<=3)tone.store(jlimit(20.0f,20000.0f,in.readFloat()));signal=0;liveEnabled=false;if(version>=2 && version<=3){signal=in.readBool()?2:0;liveEnabled=in.readBool();}quality=version==3?jlimit(0,2,in.readInt()):1;}
 std::atomic<int> quality{1};std::atomic<uint64_t> generation{0},dropped{0};
 std::atomic<bool> liveEnabled{false};
 std::atomic<int> state{0},fire{0},signal{0};std::atomic<float> tone{1000};
 double rate=48000,angle=0;int pos=0,probePosition=0,previousMode=0,previousProfile=-1; bool pulse=false;
 std::array<float,count> impulse{};
 AbstractFifo fifo{1<<20};std::vector<float> stream=std::vector<float>(1<<20,0);
};
Editor::Editor(Processor& proc):AudioProcessorEditor(proc),p(proc){
 logo=ImageCache::getFromMemory(BinaryData::Logo_png,BinaryData::Logo_pngSize);
 canvas.move=[this](Point<float> point){cursorHz=point.x>=65 && point.x<=945 && point.y>=205 && point.y<=550?20.0*std::pow(1000.0,(point.x-65.0)/880.0):0;repaint();};
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
  live.onClick=[this]{if(!liveActive && p.state.load()==2){status="Aguarde a captura unica terminar antes de iniciar LIVE.";repaint();return;}liveActive=!liveActive;p.liveEnabled=liveActive;if(liveActive)p.state.store(0);frozen=false;hold.setButtonText("Hold");live.setButtonText(liveActive?"LIVE: parar":"LIVE: iniciar");if(liveActive){resetLive();int a,n,c,m;p.fifo.prepareToRead(p.fifo.getNumReady(),a,n,c,m);p.fifo.finishedRead(n+m);}status=liveActive?"LIVE: ative tambem o LIVE do Probe. Medicao do canal esquerdo / mono.":"LIVE parado. Ultima curva mantida.";repaint();};
  arm.onClick=[this]{if(p.state.load()==2)return;liveActive=false;p.liveEnabled=false;frozen=false;live.setButtonText("LIVE: iniciar");hold.setButtonText("Hold"); p.state.store(1);status="Aguardando impulso do Probe. Canal medido: esquerdo / mono.";repaint();};
  hold.onClick=[this]{frozen=!frozen;hold.setButtonText(frozen?"Retomar":"Hold");};
  compare.onClick=[this]{if(captured){reference=magnitude;referenceSize=responseSize;referenceRate=responseRate;status="Curva A guardada nesta janela.";repaint();}};
  save.onClick=[this]{if(!captured)return;chooser=std::make_unique<FileChooser>("Exportar medicao",File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("NF-Capture.csv"),"*.csv");chooser->launchAsync(FileBrowserComponent::saveMode|FileBrowserComponent::canSelectFiles|FileBrowserComponent::warnAboutOverwriting,[this](const FileChooser& fc){auto file=fc.getResult();if(file==File())return;String csv="frequency_hz,magnitude_db,phase_relative_deg\n";for(size_t i=1;i<magnitude.size();++i)csv+=String(i*responseRate/responseSize,6)+","+String(magnitude[i],6)+","+String(phase[i],6)+"\n";status=file.replaceWithText(csv)?"CSV exportado.":"Nao foi possivel salvar o CSV.";repaint();});};
 }
 for(auto* combo:{&quality,&view,&scaleChoice}){canvas.addAndMakeVisible(combo);combo->setColour(ComboBox::backgroundColourId,Colour(0xff392346));combo->setColour(ComboBox::textColourId,Colours::white);}
 quality.addItem("Rapido",1);quality.addItem("Equilibrado",2);quality.addItem("Preciso",3);quality.setSelectedId(p.quality.load()+1,dontSendNotification);
 quality.onChange=[this]{p.quality=quality.getSelectedId()-1;if(!NF_PROBE)resetLive();};
 view.addItem("Ganho",1);view.addItem("Ganho + fase",2);view.addItem("Espectro",3);view.setSelectedId(1,dontSendNotification);view.onChange=[this]{repaint();};
 scaleChoice.addItem("Escala auto",1);scaleChoice.addItem("+/- 12 dB",2);scaleChoice.addItem("+/- 24 dB",3);scaleChoice.addItem("+/- 48 dB",4);scaleChoice.addItem("+/- 96 dB",5);scaleChoice.setSelectedId(1,dontSendNotification);scaleChoice.onChange=[this]{repaint();};
 if(NF_PROBE){view.setVisible(false);scaleChoice.setVisible(false);}else resetLive();
 resized();startTimerHz(15);
}
void Editor::resized(){
 float scale=jmin(float(getWidth())/defaultWidth,float(getHeight())/defaultHeight);
 canvas.setBounds(0,0,defaultWidth,defaultHeight);
 canvas.setTransform(AffineTransform::scale(scale).translated((getWidth()-defaultWidth*scale)*0.5f,(getHeight()-defaultHeight*scale)*0.5f));
live.setBounds(24,115,175,34);arm.setBounds(209,115,245,34);hold.setBounds(464,115,152,34);compare.setBounds(626,115,155,34);save.setBounds(791,115,185,34);frequency.setBounds(32,206,920,42);quality.setBounds(24,160,165,26);view.setBounds(199,160,150,26);scaleChoice.setBounds(359,160,145,26);}
void Editor::analyse(){
 std::fill(fftData.begin(),fftData.end(),0.0f);
 std::copy(p.impulse.begin(),p.impulse.end(),fftData.begin());
 fft.performRealOnlyForwardTransform(fftData.data());
 magnitude.resize(count/2);phase.resize(count/2);
 for(int k=0;k<count/2;++k){std::complex<float> z{fftData[(size_t)2*k],fftData[(size_t)2*k+1]};magnitude[(size_t)k]=Decibels::gainToDecibels(std::abs(z)/0.25f,-160.0f);phase[(size_t)k]=std::arg(z)*180.0f/MathConstants<float>::pi;}
 captured=true;responseSize=count;responseRate=p.rate;p.state.store(0);status="Captura concluida | FFT 65536 | "+String(p.rate/count,3)+" Hz/bin | fase relativa ao inicio detectado";
}
void Editor::resetLive(){
 int a,n,c,m;p.fifo.prepareToRead(p.fifo.getNumReady(),a,n,c,m);p.fifo.finishedRead(n+m);
 continuous.setProfile(p.quality.load(),p.rate);liveSamples.assign((size_t)continuous.length,0);liveHistory.assign((size_t)continuous.length,0);historyWrite=historyFill=0;stableFrames=0;lastAudioTime=Time::getMillisecondCounter();seenGeneration=p.generation.load();seenDrops=p.dropped.load();magnitude.clear();phase.clear();spectrum.clear();captured=false;
}
void Editor::timerCallback(){
 if(NF_PROBE)live.setButtonText(p.signal.load()==2?"LIVE: parar":"LIVE: iniciar");
 if(!NF_PROBE){if(liveActive!=p.liveEnabled.load()){liveActive=p.liveEnabled.load();frozen=false;hold.setButtonText("Hold");live.setButtonText(liveActive?"LIVE: parar":"LIVE: iniciar");}
if(!frozen && p.state.load(std::memory_order_acquire)==3)analyse();
  if(liveActive){
   if(continuous.profile!=p.quality.load() || seenGeneration!=p.generation.load() || seenDrops!=p.dropped.load())resetLive();
   int a,n,c,m;int ready=p.fifo.getNumReady();p.fifo.prepareToRead(ready,a,n,c,m);
   auto append=[&](int index,int length){for(int i=0;i<length;++i){float sample=p.stream[(size_t)(index+i)];liveHistory[historyWrite]=sample;historyWrite=(historyWrite+1)%liveHistory.size();historyFill=jmin(historyFill+1,liveHistory.size());}};
   append(a,n);append(c,m);p.fifo.finishedRead(n+m);if(n+m>0)lastAudioTime=Time::getMillisecondCounter();
   if(!frozen && n+m>0 && historyFill==liveHistory.size()){
    for(size_t i=0;i<liveSamples.size();++i)liveSamples[i]=liveHistory[(historyWrite+i)%liveHistory.size()];
    if(continuous.analyse(liveSamples.data())){float change=1000;if(magnitude.size()==continuous.magnitude.size()){change=0;for(size_t k=1;k<magnitude.size();++k){double hz=k*p.rate/continuous.length;if(hz>=20 && hz<=20000)change=jmax(change,std::abs(magnitude[k]-continuous.magnitude[k]));}}stableFrames=change<0.08f?stableFrames+1:0;magnitude=continuous.magnitude;phase=continuous.phase;responseSize=continuous.length;responseRate=p.rate;captured=true;spectrum.clear();inputPeak=0;for(float sample:liveSamples)inputPeak=jmax(inputPeak,std::abs(sample));status="LIVE | "+String(p.rate/responseSize,3)+" Hz/bin | janela "+String(responseSize/p.rate,2)+" s | "+(inputPeak>=1?"NIVEL ACIMA DE 0 dBFS":(stableFrames>=3?"estavel entre janelas":"em transicao / aguarde estabilizar"));}
    else{status="Sem sinal valido: confira LIVE, perfil igual no Probe e transporte da DAW.";captured=false;magnitude.clear();phase.clear();}
   }else if(!frozen && historyFill<liveHistory.size())status="Preparando LIVE: "+String(int(100*historyFill/liveHistory.size()))+"% | mesmo perfil no Probe e Capture";
   if(!frozen && n+m==0 && Time::getMillisecondCounter()-lastAudioTime>750)status="Audio parado: verifique transporte e roteamento. Ultima curva mantida.";
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
 g.setColour(Colour(0xffebc5ff));g.setFont(13);g.drawText("NF AUDIO TOOLS     /     LAB BETA 0.2 / LIVE / R3",200,63,600,22,Justification::centred);
 g.setFont(13);g.setColour(Colours::lightgrey);g.drawText(frozen?"HOLD | "+status:status,24,defaultHeight-52,defaultWidth-48,24,Justification::centredLeft);
 g.setColour(Colour(0xffc1adcc));g.drawText(NF_PROBE?"Mesmo perfil nos dois componentes. LIVE usa multisseno de aproximadamente -36 dBFS RMS.":"Ganho: violeta / dB   |   Referencia A: turquesa   |   Fase: painel separado / graus   |   Perfil igual no Probe",24,defaultHeight-28,defaultWidth-48,20,Justification::centredLeft);
 if(NF_PROBE){g.setColour(Colours::white);g.drawText("Fluxo: NF Capture Probe  >  seu plugin  >  NF Capture",32,255,900,26,Justification::centredLeft);return;}
 const bool showPhase=view.getSelectedId()==2, showSpectrum=view.getSelectedId()==3;
 Rectangle<float> r(65,205,880,showPhase?220.0f:345.0f),pr(65,455,880,95);
 auto x=[&](double hz){return 65.0f+float(std::log(hz/20.0)/std::log(1000.0))*880;};
 float range=12;for(size_t k=1;k<magnitude.size();++k){double hz=k*responseRate/responseSize;if(hz>=20 && hz<=jmin(20000.0,responseRate/2))range=jmax(range,std::abs(magnitude[k]));}for(float value:reference)if(std::isfinite(value))range=jmax(range,std::abs(value));range=jlimit(12.0f,96.0f,12*std::ceil(range/12));if(scaleChoice.getSelectedId()>1)range=std::array<float,4>{12,24,48,96}[(size_t)scaleChoice.getSelectedId()-2];
 auto panel=[&](Rectangle<float> rect){g.setColour(Colour(0xff291c34));g.fillRoundedRectangle(rect,6);for(double hz:{20.,50.,100.,200.,500.,1000.,2000.,5000.,10000.,20000.}){g.setColour(Colour(0xff523960));g.drawVerticalLine((int)x(hz),rect.getY(),rect.getBottom());}};panel(r);if(showPhase)panel(pr);
 for(int i=0;i<=6;++i){float y=r.getY()+i*r.getHeight()/6;float value=showSpectrum?-20.0f*i:range-2*range*i/6;g.setColour(Colour(i==3?0xff9878a9:0xff523960));g.drawHorizontalLine((int)y,r.getX(),r.getRight());g.setColour(Colour(0xffc1adcc));g.drawText(String(value,0),12,(int)y-8,45,18,Justification::centredRight);}
 g.setColour(Colour(0xffebc5ff));g.drawText(showSpectrum?"ESPECTRO / dBFS":"GANHO / dB",65,188,200,16,Justification::centredLeft);
 if(showPhase){g.drawText("FASE RELATIVA / graus",65,434,250,18,Justification::centredLeft);for(int v:{-180,0,180}){float y=pr.getCentreY()-v*pr.getHeight()/360;g.setColour(Colour(0xffc1adcc));g.drawText(String(v),8,(int)y-8,48,16,Justification::centredRight);}}
 for(double hz:{20.,50.,100.,200.,500.,1000.,2000.,5000.,10000.,20000.}){g.setColour(Colour(0xffc1adcc));g.drawText(hz>=1000?String(hz/1000,0)+"k":String(hz,0),(int)x(hz)-20,553,40,20,Justification::centred);}
 auto sampleAt=[](const std::vector<float>& values,double bin){int k=jlimit(1,(int)values.size()-1,(int)bin),next=jmin(k+1,(int)values.size()-1);float fraction=(float)jlimit(0.0,1.0,bin-k);return values[(size_t)k]*(1-fraction)+values[(size_t)next]*fraction;};
 auto line=[&](const std::vector<float>& values,Colour color,int type,int bins,double sr){if(values.size()<2)return;auto rect=type==2?pr:r;Path path;bool began=false;float last=0;for(int px=0;px<=880;++px){double hz=20*std::pow(1000.0,px/880.0);if(hz>=sr/2)break;double bin=hz*bins/sr;float value=sampleAt(values,bin);if(type==2){int k=jlimit(1,(int)values.size()-1,(int)bin),next=jmin(k+1,(int)values.size()-1);float delta=std::remainder(values[(size_t)next]-values[(size_t)k],360.0f);value=std::remainder(values[(size_t)k]+float(jlimit(0.0,1.0,bin-k))*delta,360.0f);}float y=type==1?rect.getBottom()-(value+120)*rect.getHeight()/120:type==2?rect.getCentreY()-value*rect.getHeight()/360:rect.getCentreY()-value*rect.getHeight()/(2*range);y=jlimit(rect.getY(),rect.getBottom(),y);if(!began || (type==2 && std::abs(value-last)>180)){path.startNewSubPath(65.0f+px,y);began=true;}else path.lineTo(65.0f+px,y);last=value;
 if(type==0 && px<880){double upperHz=20*std::pow(1000.0,(px+1)/880.0);int lo=jmax(1,(int)std::ceil(bin)),hi=jmin((int)values.size()-1,(int)std::floor(upperHz*bins/sr));if(lo<=hi){int minimum=lo,maximum=lo;for(int k=lo+1;k<=hi;++k){if(values[(size_t)k]<values[(size_t)minimum])minimum=k;if(values[(size_t)k]>values[(size_t)maximum])maximum=k;}int first=jmin(minimum,maximum),second=jmax(minimum,maximum);for(int k:{first,second}){float xx=x(k*sr/bins),yy=jlimit(rect.getY(),rect.getBottom(),rect.getCentreY()-values[(size_t)k]*rect.getHeight()/(2*range));path.lineTo(xx,yy);}}}
 }g.setColour(color);g.strokePath(path,PathStrokeType(1.7f));};
 if(showSpectrum){line(spectrum,Colour(0xffb5a0cd),1,count,p.rate);if(liveActive){g.setColour(Colours::white);g.drawText("Espectro disponivel com LIVE parado. Use Ganho para medir a resposta.",90,350,820,30,Justification::centred);}}
 else{line(reference,Colour(0xff55d6d0),0,referenceSize,referenceRate);line(magnitude,Colour(0xffcf94fa),0,responseSize,responseRate);if(showPhase)line(phase,Colour(0xffffba75),2,responseSize,responseRate);}
 if(cursorHz>0 && !showSpectrum && magnitude.size()>1 && cursorHz<responseRate/2){int k=jlimit(1,(int)magnitude.size()-1,(int)std::round(cursorHz*responseSize/responseRate));double hz=k*responseRate/responseSize;g.setColour(Colour(0xffebc5ff));g.drawText(String(hz,2)+" Hz | "+String(magnitude[(size_t)k],2)+" dB"+(showPhase?" | "+String(phase[(size_t)k],1)+" graus":" (bin medido)"),520,160,450,26,Justification::centredRight);g.setColour(Colour(0x668888aa));g.drawVerticalLine((int)x(hz),r.getY(),550);}
}
AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor();}
