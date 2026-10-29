#pragma once
struct ResizeChecks {
 static bool failed(int line,Editor& e){std::cout<<"Resize check failed at "<<line<<" size "<<e.getWidth()<<"x"<<e.getHeight()<<" resizable="<<e.isResizable()<<std::endl;return false;}
 static bool run(Editor& e,const String& name){
  e.setVisible(true);
  if(!e.isResizable())return failed(__LINE__,e);
  auto dir=File::getCurrentWorkingDirectory().getChildFile("../../work/visual-qa");dir.createDirectory();
  for(int width:{700,1400,1600}){
   e.getConstrainer()->setBoundsForComponent(&e,{0,0,width,roundToInt(width*double(Editor::defaultHeight)/1000)},false,false,true,true);
   if(e.getWidth()!=width || std::abs(double(e.getWidth())/e.getHeight()-1000.0/Editor::defaultHeight)>0.005)return failed(__LINE__,e);
   ResizableCornerComponent* corner=nullptr;
   for(int i=0;i<e.getNumChildComponents();++i)if(auto* candidate=dynamic_cast<ResizableCornerComponent*>(e.getChildComponent(i)))corner=candidate;
   if(corner==nullptr || !corner->isVisible() || corner->getRight()!=e.getWidth() || corner->getBottom()!=e.getHeight())return failed(__LINE__,e);
   auto point=e.getLocalPoint(&e.live,e.live.getLocalBounds().getCentre());
   if(e.getComponentAt(point)!=&e.live)return failed(__LINE__,e);
   auto image=e.createComponentSnapshot(e.getLocalBounds());FileOutputStream output(dir.getChildFile(name+"-"+String(width)+".png"));output.setPosition(0);output.truncate();PNGImageFormat png;png.writeImageToStream(image,output);
   e.canvas.doubleClick({500,200});if(e.getWidth()!=width)return failed(__LINE__,e);
   e.canvas.doubleClick({100,40});if(e.getWidth()!=1000 || e.getHeight()!=Editor::defaultHeight)return failed(__LINE__,e);
  }
  return true;
 }
};
