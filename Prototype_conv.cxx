#include "RModel.hxx"
#include "RModelParser_ONNX.hxx"

using namespace TMVA::Experimental::SOFIE;

int main(){
   RModelParser_ONNX parser;
   RModel model = parser.Parse("./Conv.onnx");

   //std::cout << "===" << std::endl;
   //std::cout << "Genrate output" << std::endl;
   model.Generate();

   //std::cout << "\n\n===" << std::endl;
   //std::cout << "Print Generated" << std::endl;
   model.PrintGenerated();
}
