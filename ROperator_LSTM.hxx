#ifndef TMVA_SOFIE_ROPERATOR_LSTM
#define TMVA_SOFIE_ROPERATOR_LSTM

#include "RModel.hxx"
#include "ROperator.hxx"
#include "SOFIE_common.hxx"
#include <cstring>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace TMVA {
namespace Experimental {
namespace SOFIE {

template <typename T> class ROperator_LSTM final : public ROperator {
 private:
   std::vector<float> fAttrActivationAlpha;
   std::vector<float> fAttrActivationBeta;
   std::vector<std::string> fAttrActivations;
   float fAttrClip;
   std::string fAttrDirection;
   size_t fAttrHiddenSize;
   size_t fAttrInputForget;
   size_t fAttrLayout;

   std::string fNX;
   std::string fNW;
   std::string fNR;
   std::string fNB;
   std::string fNSequence_lens;
   std::string fNInitial_h;
   std::string fNInitial_c;
   std::string fNP;
   std::string fNY;
   std::string fNY_h;
   std::string fNY_c;

   std::vector<size_t> fShapeX;
   std::vector<size_t> fShapeW;
   std::vector<size_t> fShapeR;
   std::vector<size_t> fShapeB;
   std::vector<size_t> fShapeSequence_lens;
   std::vector<size_t> fShapeInitial_h;
   std::vector<size_t> fShapeInitial_c;
   std::vector<size_t> fShapeP;
   std::vector<size_t> fShapeY;
   std::vector<size_t> fShapeY_h;
   std::vector<size_t> fShapeY_c;

   std::string fType;

 public:
   ROperator_LSTM() = delete;

   ROperator_LSTM(std::vector<float> activation_alpha,
                 std::vector<float> activation_beta,
                 std::vector<std::string> activations, float clip,
                 std::string direction, size_t hidden_size,
                 size_t input_forget, size_t layout,
                 std::string nameX, std::string nameW, std::string nameR,
                 std::string nameB, std::string nameSequence_lens,
                 std::string nameInitial_h, std::string nameInitial_c, std::string nameP,
                 std::string nameY, std::string nameY_h, std::string nameY_c)
       : fAttrActivationAlpha(activation_alpha),
         fAttrActivationBeta(activation_beta), fAttrActivations(activations),
         fAttrClip(clip), fAttrDirection(direction), fAttrHiddenSize(hidden_size),
         fAttrInputForget(input_forget), fAttrLayout(layout),
         fNX(UTILITY::Clean_name(nameX)), fNW(UTILITY::Clean_name(nameW)),
         fNR(UTILITY::Clean_name(nameR)), fNB(UTILITY::Clean_name(nameB)),
         fNSequence_lens(UTILITY::Clean_name(nameSequence_lens)),
         fNInitial_h(UTILITY::Clean_name(nameInitial_h)),
         fNInitial_c(UTILITY::Clean_name(nameInitial_c)), fNP(UTILITY::Clean_name(nameP)),
         fNY(UTILITY::Clean_name(nameY)), fNY_h(UTILITY::Clean_name(nameY_h)),
         fNY_c(UTILITY::Clean_name(nameY_c)) {
      if (std::is_same<T, float>::value) {
         fType = "float";
      } else {
         throw std::runtime_error(
             "TMVA SOFIE Encountered unsupported type parsing a LSTM operator");
      }
   }

   std::vector<ETensorType> TypeInference(std::vector<ETensorType> input) {
      ETensorType out = input[0];
      return {out, out};
   }

   std::vector<std::vector<size_t>> ShapeInference(std::vector<std::vector<size_t>> input) {
      size_t num_directions = input[1][0];
      size_t hidden_size = input[1][1];
      if (fAttrLayout == 0) {
         size_t seq_length = input[0][0];
         size_t batch_size = input[0][1];
         std::vector<std::vector<size_t>> ret(
             {{seq_length, num_directions, batch_size, hidden_size},
              {num_directions, batch_size, hidden_size},
              {num_directions, batch_size, hidden_size}});
         return ret;
      } else {
         size_t batch_size = input[0][0];
         size_t seq_length = input[0][1];
         std::vector<std::vector<size_t>> ret(
             {{batch_size, seq_length, num_directions, hidden_size},
              {batch_size, num_directions, hidden_size},
              {batch_size, num_directions, hidden_size}});
         return ret;
      }
   }

   void Initialize(RModel &model) {
      // Check the input and output tensors
      if (!model.CheckIfTensorAlreadyExist(fNX)) {
         throw std::runtime_error("TMVA SOFIE LSTM Op input tensor " + fNX + "  is not found in model.");
      }
      fShapeX = model.GetTensorShape(fNX);
      if (fShapeX.size() != 3) {
         throw std::runtime_error("TMVA SOFIE LSTM Op input tensor " + fNX + " is not of 3 dimensions.");
      }
      if (!model.CheckIfTensorAlreadyExist(fNW)) {
         throw std::runtime_error("TMVA SOFIE LSTM Op input tensor " + fNW + "  is not found in model.");
      }
      fShapeW = model.GetTensorShape(fNW);
      if (fShapeW.size() != 3) {
         throw std::runtime_error("TMVA SOFIE LSTM Op input tensor " + fNW + " is not of 3 dimensions.");
      }
      if (!model.CheckIfTensorAlreadyExist(fNR)) {
         throw std::runtime_error("TMVA SOFIE LSTM Op input tensor " + fNR + "  is not found in model.");
      }
      fShapeR = model.GetTensorShape(fNR);
      if (fShapeR.size() != 3) {
         throw std::runtime_error("TMVA SOFIE LSTM Op input tensor " + fNR + " is not of 3 dimensions.");
      }
      if (!fNB.empty()) {
         if (!model.CheckIfTensorAlreadyExist(fNB)) {
            throw std::runtime_error("TMVA SOFIE LSTM op input tensor " + fNB + " is not  found in model.");
         }
         fShapeB = model.GetTensorShape(fNB);
         if (fShapeB.size() != 2 && fShapeB.size() != 5) {
            throw std::runtime_error("TMVA SOFIE LSTM op input tensor " + fNB + " is not of 2 or 5 dimensions.");
         }
         if (fShapeB.size() == 2) {
            // Broadcasting the bias
            auto original_data = model.GetInitializedTensorData(fNB);
            size_t num_directions = fShapeW[0];
            size_t seq_length = (fAttrLayout == 0)? fShapeX[0] : fShapeX[1];
            size_t batch_size = (fAttrLayout == 0)? fShapeX[1] : fShapeX[0];
            if (fType == "float") {
               float *original_bias = static_cast<float*>(original_data.get());
               float *new_bias = new float[4 * num_directions * seq_length * batch_size * fAttrHiddenSize];
               for (size_t gate = 0; gate < 4; gate++) {
                  float sum[fAttrHiddenSize];
                  for (size_t direction = 0; direction < num_directions; direction++) {
                     size_t offset = direction * 8 * fAttrHiddenSize + gate * fAttrHiddenSize;
                     for (size_t h = 0; h < fAttrHiddenSize; h++) {
                        sum[h] = original_bias[offset + h] + original_bias[offset + h + 4 * fAttrHiddenSize];
                     }
                     for (size_t seq = 0; seq < seq_length; seq++) {
                        for (size_t batch = 0; batch < batch_size; batch++) {
                           size_t bias_offset = gate * num_directions * seq_length * batch_size * fAttrHiddenSize
                              + direction * seq_length * batch_size * fAttrHiddenSize
                              + seq * batch_size * fAttrHiddenSize + batch * fAttrHiddenSize;
                           std::copy(sum, sum + fAttrHiddenSize, new_bias + bias_offset);
                        }
                     }
                  }
               }
               std::vector<size_t> new_bias_shape = {4, num_directions, seq_length, batch_size, fAttrHiddenSize};
               std::shared_ptr<void> new_bias_ptr(new_bias, std::default_delete<float[]>());
               model.UpdateInitializedTensor(fNB, model.GetTensorType(fNB), new_bias_shape, new_bias_ptr);
               fShapeB = model.GetTensorShape(fNB);
            }
         }
      }
      if (!fNSequence_lens.empty()) {
         if (!model.CheckIfTensorAlreadyExist(fNSequence_lens)) {
            throw std::runtime_error("TMVA SOFIE LSTM Op input tensor " +
                                     fNSequence_lens +
                                     "is not found in model.");
         }
         fShapeSequence_lens = model.GetTensorShape(fNSequence_lens);
         if (fShapeSequence_lens.size() != 1) {
            throw std::runtime_error("TMVA SOFIE LSTM Op input tensor " +
                                     fNSequence_lens +
                                     " is not of 1 dimension.");
         }
      }
      if (!fNInitial_h.empty()) {
         if (!model.CheckIfTensorAlreadyExist(fNInitial_h)) {
            throw std::runtime_error("TMVA SOFIE LSTM Op input tensor " +
                                     fNInitial_h + " is not found in model.");
         }
         fShapeInitial_h = model.GetTensorShape(fNInitial_h);
         if (fShapeInitial_h.size() != 3) {
            throw std::runtime_error("TMVA SOFIE LSTM Op input tensor " +
                                     fNInitial_h + " is not of 3 dimensions.");
         }
      }
      if (!fNInitial_c.empty()) {
         if (!model.CheckIfTensorAlreadyExist(fNInitial_c)) {
            throw std::runtime_error("TMVA SOFIE LSTM Op input tensor " +
                                     fNInitial_c + " is not found in model.");
         }
         fShapeInitial_c = model.GetTensorShape(fNInitial_c);
         if (fShapeInitial_c.size() != 3) {
            throw std::runtime_error("TMVA SOFIE LSTM Op input tensor " +
                                     fNInitial_c + " is not of 3 dimensions.");
         }
      }
      if (!fNP.empty()) {
         if (!model.CheckIfTensorAlreadyExist(fNP)) {
            throw std::runtime_error("TMVA SOFIE LSTM op input tensor " + fNP + " is not  found in model.");
         }
         fShapeP = model.GetTensorShape(fNP);
         if (fShapeP.size() != 2 && fShapeP.size() != 4) {
            throw std::runtime_error("TMVA SOFIE LSTM op input tensor " + fNP + " is not of 2 or 4 dimensions.");
         }
         if (fShapeP.size() == 2) {
            // Broadcasting the weight for peepholes
            auto original_data = model.GetInitializedTensorData(fNP);
            size_t num_directions = fShapeW[0];
            size_t batch_size = (fAttrLayout == 0)? fShapeX[1] : fShapeX[0];
            if (fType == "float") {
               float *original_p = static_cast<float*>(original_data.get());
               float *new_p = new float[num_directions * 3 * batch_size * fAttrHiddenSize];
               for (size_t direction = 0; direction < num_directions; direction++) {
                  for (size_t gate = 0; gate < 3; gate++) {
                     size_t p_offset = direction * 3 * fAttrHiddenSize + gate * fAttrHiddenSize;
                     for (size_t batch = 0; batch < batch_size; batch++) {
                        size_t offset = direction * 3 * batch_size * fAttrHiddenSize
                           + gate * batch_size * fAttrHiddenSize + batch * fAttrHiddenSize;
                        std::copy(original_p + p_offset, original_p + p_offset + fAttrHiddenSize,
                           new_p + offset);
                     }
                  }
               }
               std::vector<size_t> new_p_shape = {num_directions, 3, batch_size, fAttrHiddenSize};
               std::shared_ptr<void> new_p_ptr(new_p, std::default_delete<float[]>());
               model.UpdateInitializedTensor(fNP, model.GetTensorType(fNP), new_p_shape, new_p_ptr);
               fShapeP = model.GetTensorShape(fNP);
            }
         }
      }
      if (!fNY.empty()) {
         fShapeY = ShapeInference({fShapeX, fShapeW})[0];
         if (!model.CheckIfTensorAlreadyExist(fNY)) {
            model.AddIntermediateTensor(fNY, model.GetTensorType(fNX), fShapeY);
         }
      }
      if (!fNY_h.empty()) {
         fShapeY_h = ShapeInference({fShapeX, fShapeW})[1];
         if (!model.CheckIfTensorAlreadyExist(fNY_h)) {
            model.AddIntermediateTensor(fNY_h, model.GetTensorType(fNX), fShapeY_h);
         }
      }
      if (!fNY_c.empty()) {
         fShapeY_c = ShapeInference({fShapeX, fShapeW})[2];
         if (!model.CheckIfTensorAlreadyExist(fNY_c)) {
            model.AddIntermediateTensor(fNY_c, model.GetTensorType(fNX), fShapeY_c);
         }
      }
      // Check the attributes
      for (auto &activation : fAttrActivations) {
         if (activation != "Relu" && activation != "Tanh" &&
             activation != "Sigmoid" && activation != "Affine" &&
             activation != "LeakyRelu" && activation != "ThresholdRelu" &&
             activation != "ScaledTanh" && activation != "HardSigmoid" &&
             activation != "Elu" && activation != "Softsign" &&
             activation != "Softplus") {
            throw std::runtime_error("TMVA SOFIE - Activation function " +
                                     activation + " not implemented");
         }
      }
      if (fAttrDirection != "forward" && fAttrDirection != "backward" &&
          fAttrDirection != "bidirectional") {
         throw std::runtime_error(
             "TMVA SOFIE - Invalid LSTM direction fAttrDirection = " +
             fAttrDirection);
      }
      if (4 * fAttrHiddenSize != fShapeW[1]) {
         throw std::runtime_error(
             "TMVA SOFIE - fAttrHiddenSize must be equal to " +
             std::to_string(fShapeW[1] / 4));
      }
      if (fAttrInputForget > 1) {
         throw std::runtime_error(
            "TMVA SOFIE - fAttrInputForget = " + std::to_string(fAttrInputForget)
            + " must be 0 or 1.");
      }
      if (fAttrLayout > 1) {
         throw std::runtime_error("TMVA SOFIE - Layout fAttrLayout = " +
                                  std::to_string(fAttrLayout) +
                                  " must be 0 (timewise) or 1 (batchwise)");
      }
      if (fAttrActivations.empty()) {
         if (fAttrDirection == "bidirectional") {
            fAttrActivations = {"Sigmoid", "Tanh", "Tanh", "Sigmoid", "Tanh", "Tanh"};
         } else {
            fAttrActivations = {"Sigmoid", "Tanh", "Tanh"};
         }
      }
   }

   std::string Generate(std::string OpName) {
      OpName = "op_" + OpName;
      std::stringstream out;

      size_t seq_length = (fAttrLayout == 0) ? fShapeX[0] : fShapeX[1];
      size_t batch_size = (fAttrLayout == 0) ? fShapeX[1] : fShapeX[0];
      size_t input_size = fShapeX[2];
      size_t num_directions = fShapeW[0];

      // set the input
      if (fAttrLayout == 0) {
         if (fType == "float") {
            out << "\t" << "float *" << OpName << "_input = tensor_" << fNX << ";\n";
         }
      } else {
         if (fType == "float") {
            out << "\t" << "float " << OpName << "_input[" << seq_length * batch_size * input_size << "];\n";
         }
         out << "\t" << "for(size_t seq = 0; seq < " << seq_length << "; seq++) {\n";
         out << "\t" << "\t" << "for(size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
         out << "\t" << "\t" << "\t" << "for(size_t i = 0; i < " << input_size << "; i++) {\n";
         out << "\t" << "\t" << "\t" << "\t" << OpName << "_input[seq * " << batch_size * input_size
             << " + batch * " << input_size << " + i] = " << "tensor_" << fNX << "[batch * "
             << seq_length * input_size << " + seq * " << input_size << " + i];\n";
         out << "\t" << "\t" << "\t" << "}\n";
         out << "\t" << "\t" << "}\n";
         out << "\t" << "}\n";
      }

      // Set the initial hidden state
      if (!fNInitial_h.empty()) {
         if (fAttrLayout == 0) {
            if (fType == "float") {
               out << "\t" << "float *" << OpName << "_initial_hidden_state = " << " tensor_"
                   << fNInitial_h << ";\n";
            }
         } else {
            if (fType == "float") {
               out << "\t" << "float " << OpName << "_initial_hidden_state[" << num_directions * batch_size *
                   fAttrHiddenSize << "];\n";
            }
            for (size_t direction = 0; direction < num_directions; direction++) {
               out << "\t" << "for(size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
               out << "\t" << "\t" << "for(size_t h = 0; h < " << fAttrHiddenSize << "; h++) {\n";
               out << "\t" << "\t" << "\t" << OpName << "_initial_hidden_state["
                   << direction * batch_size * fAttrHiddenSize << " + batch * " << fAttrHiddenSize
                   << " + h] = tensor_" << fNInitial_h << "[batch * " << num_directions * fAttrHiddenSize
                   << " + " << direction * fAttrHiddenSize << " + h];\n";
               out << "\t" << "\t" << "}\n";
               out << "\t" << "}\n";
            }
         }
      }

      // Set the initial cell state
      if (!fNInitial_c.empty()) {
         if (fAttrLayout == 0) {
            if (fType == "float") {
               out << "\t" << "float *" << OpName << "_initial_cell_state = " << " tensor_"
                   << fNInitial_c << ";\n";
            }
         } else {
            if (fType == "float") {
               out << "\t" << "float " << OpName << "_initial_cell_state[" << num_directions * batch_size *
                   fAttrHiddenSize << "];\n";
            }
            for (size_t direction = 0; direction < num_directions; direction++) {
               out << "\t" << "for(size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
               out << "\t" << "\t" << "for(size_t h = 0; h < " << fAttrHiddenSize << "; h++) {\n";
               out << "\t" << "\t" << "\t" << OpName << "_initial_cell_state["
                   << direction * batch_size * fAttrHiddenSize << " + batch * " << fAttrHiddenSize
                   << " + h] = tensor_" << fNInitial_c << "[batch * " << num_directions * fAttrHiddenSize
                   << " + " << direction * fAttrHiddenSize << " + h];\n";
               out << "\t" << "\t" << "}\n";
               out << "\t" << "}\n";
            }
         }
      }

      // Set the feedforward
      size_t ff_size = seq_length * batch_size * fAttrHiddenSize;
      if (fType == "float") {
         out << "\t" << "float " << OpName << "_ff_input_gate[" << ff_size << "];\n";
         out << "\t" << "float " << OpName << "_ff_output_gate[" << ff_size << "];\n";
         out << "\t" << "float " << OpName << "_ff_cell_gate[" << ff_size << "];\n";
         if (fAttrInputForget == 0) {
            out << "\t" << "float " << OpName << "_ff_forget_gate[" << ff_size << "];\n";
         }
      }
      // Set the gates
      size_t hidden_state_size = seq_length * num_directions * batch_size * fAttrHiddenSize;
      if (fType == "float") {
         out << "\t" << "float " << OpName << "_input_gate[" << hidden_state_size << "];\n";
         out << "\t" << "float " << OpName << "_output_gate[" << hidden_state_size << "];\n";
         out << "\t" << "float " << OpName << "_cell_gate[" << hidden_state_size << "];\n";
         if (fAttrInputForget == 0) {
            out << "\t" << "float " << OpName << "_forget_gate[" << hidden_state_size << "];\n";
         }
      }
      // Set the cell state and the new cell state = h(cell state)
      if (fType == "float") {
         out << "\t" << "float " << OpName << "_cell_state[" << hidden_state_size << "];\n";
         out << "\t" << "float " << OpName << "_new_cell_state[" << hidden_state_size << "];\n";
      }

      // Set the hidden state
      if (fAttrLayout == 0 && !fNY.empty()) {
         if (fType == "float") {
            out << "\t" << "float *" << OpName << "_hidden_state = tensor_" << fNY << ";\n";
         }
      } else {
         if (fType == "float") {
            out << "\t" << "float " << OpName << "_hidden_state[" << hidden_state_size << "];\n";
         }
      }

      out << "\t" << "char " << OpName << "_transA = 'N';\n";
      out << "\t" << "char " << OpName << "_transB = 'T';\n";
      out << "\t" << "int " << OpName << "_m = " << seq_length * batch_size << ";\n";
      out << "\t" << "int " << OpName << "_n = " << fAttrHiddenSize << ";\n";
      out << "\t" << "int " << OpName << "_k = " << input_size << ";\n";
      if (fType == "float") {
         out << "\t" << "float " << OpName << "_alpha = 1.;\n";
         out << "\t" << "float " << OpName << "_beta = 0.;\n";
      }
      if (!fNB.empty()) {
         out << "\t" << "int " << OpName << "_bias_size = " << seq_length * batch_size * fAttrHiddenSize << ";\n";
         out << "\t" << "int " << OpName << "_incx = 1;\n";
         out << "\t" << "int " << OpName << "_incy = 1;\n";
      }

      for (size_t direction = 0; direction < num_directions; direction++) {
         if (direction == 0) {
            if (fType == "float") {
               // input_gate = input * weight_i^T
               out << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                   << OpName <<"_n, &" << OpName << "_m, &" << OpName << "_k, &" << OpName << "_alpha, tensor_"
                   << fNW << ", &" << OpName << "_k, " << OpName << "_input, &" << OpName << "_k, &"
                  << OpName << "_beta, " << OpName << "_ff_input_gate, &" << OpName << "_n);\n";
               // output_gate = input * weight_o^T
               size_t wo_offset = 2 * fAttrHiddenSize * input_size;
               out << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                   << OpName <<"_n, &" << OpName << "_m, &" << OpName << "_k, &" << OpName << "_alpha, tensor_"
                  << fNW << " + " << wo_offset << ", &" << OpName << "_k, " << OpName << "_input, &"
                  << OpName << "_k, &" << OpName << "_beta, " << OpName << "_ff_output_gate, &" << OpName << "_n);\n";
               // cell_gate = input * weight_c^T
               size_t wc_offset = 3 * fAttrHiddenSize * input_size;
               out << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                   << OpName <<"_n, &" << OpName << "_m, &" << OpName << "_k, &" << OpName << "_alpha, tensor_"
                  << fNW << " + " << wc_offset << ", &" << OpName << "_k, " << OpName << "_input, &"
                  << OpName << "_k, &" << OpName << "_beta, " << OpName << "_ff_cell_gate, &" << OpName << "_n);\n";
            }
         } else {
            if (fType == "float") {
               // input_gate = input * weight_i^T
               size_t wi_offset = 4 * fAttrHiddenSize * input_size;
               out << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                   << OpName <<"_n, &" << OpName << "_m, &" << OpName << "_k, &" << OpName << "_alpha, tensor_"
                  << fNW << " + " << wi_offset << ", &" << OpName << "_k, " << OpName << "_input, &"
                  << OpName << "_k, &" << OpName << "_beta, " << OpName << "_ff_input_gate, &" << OpName << "_n);\n";
               // output_gate = input * weight_o^T
               size_t wo_offset = 4 * fAttrHiddenSize * input_size + 2 * fAttrHiddenSize * input_size;
               out << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                   << OpName <<"_n, &" << OpName << "_m, &" << OpName << "_k, &" << OpName << "_alpha, tensor_"
                  << fNW << " + " << wo_offset << ", &" << OpName << "_k, " << OpName << "_input, &"
                  << OpName << "_k, &" << OpName << "_beta, " << OpName << "_ff_output_gate, &" << OpName << "_n);\n";
               // cell_gate = input * weight_c^T
               size_t wc_offset = 4 * fAttrHiddenSize * input_size + 3 * fAttrHiddenSize * input_size;
               out << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                   << OpName <<"_n, &" << OpName << "_m, &" << OpName << "_k, &" << OpName << "_alpha, tensor_"
                  << fNW << " + " << wc_offset << ", &" << OpName << "_k, " << OpName << "_input, &"
                  << OpName << "_k, &" << OpName << "_beta, " << OpName << "_ff_cell_gate, &" << OpName << "_n);\n";
            }
         }
         if (fAttrInputForget == 0) {
            // forget_gate = input * weight_f^T
            if (direction == 0) {
               if (fType == "float") {
                  size_t wf_offset = fAttrHiddenSize * input_size;
                  out << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                      << OpName <<"_n, &" << OpName << "_m, &" << OpName << "_k, &" << OpName << "_alpha, tensor_"
                      << fNW << " + " << wf_offset << ", &" << OpName << "_k, " << OpName << "_input, &"
                      << OpName << "_k, &" << OpName << "_beta, " << OpName << "_ff_forget_gate, &" << OpName << "_n);\n";
               }
            } else {
               if (fType == "float") {
                  size_t wf_offset = 4 * fAttrHiddenSize * input_size + fAttrHiddenSize * input_size;
                  out << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                      << OpName <<"_n, &" << OpName << "_m, &" << OpName << "_k, &" << OpName << "_alpha, tensor_"
                      << fNW << " + " << wf_offset << ", &" << OpName << "_k, " << OpName << "_input, &"
                      << OpName << "_k, &" << OpName << "_beta, " << OpName << "_ff_forget_gate, &" << OpName << "_n);\n";
               }
            }
         }

         // Add the bias
         if (!fNB.empty()) {
            if (direction == 0) {
               if (fType == "float") {
                  // ff_input_gate += bias_i
                  out << "\t" << "BLAS::saxpy_(&" << OpName << "_bias_size, &" << OpName << "_alpha, tensor_"
                      << fNB << ", &" << OpName << "_incx, " << OpName << "_ff_input_gate, &" << OpName << "_incy);\n";
                  // ff_output_gate += bias_o
                  size_t bo_offset = 2 * num_directions * seq_length * batch_size * fAttrHiddenSize;
                  out << "\t" << "BLAS::saxpy_(&" << OpName << "_bias_size, &" << OpName << "_alpha, tensor_"
                      << fNB << " + " << bo_offset << ", &" << OpName << "_incx, " << OpName << "_ff_output_gate, &"
                      << OpName << "_incy);\n";
                  // ff_cell_gate += bias_c
                  size_t bc_offset = 3 * num_directions * seq_length * batch_size * fAttrHiddenSize;
                  out << "\t" << "BLAS::saxpy_(&" << OpName << "_bias_size, &" << OpName << "_alpha, tensor_"
                      << fNB << " + " << bc_offset << ", &" << OpName << "_incx, " << OpName << "_ff_cell_gate, &"
                      << OpName << "_incy);\n";
               }
            } else {
               if (fType == "float") {
                  // ff_input_gate += bias_i
                  size_t bi_offset = seq_length * batch_size * fAttrHiddenSize;
                  out << "\t" << "BLAS::saxpy_(&" << OpName << "_bias_size, &" << OpName << "_alpha, tensor_"
                      << fNB << " + " << bi_offset << ", &" << OpName << "_incx, " << OpName << "_ff_input_gate, &"
                      << OpName << "_incy);\n";
                  // ff_output_gate += bias_o
                  size_t bo_offset = 2 * num_directions * seq_length * batch_size * fAttrHiddenSize
                     + seq_length * batch_size * fAttrHiddenSize;
                  out << "\t" << "BLAS::saxpy_(&" << OpName << "_bias_size, &" << OpName << "_alpha, tensor_"
                      << fNB << " + " << bo_offset << ", &" << OpName << "_incx, " << OpName << "_ff_output_gate, &"
                      << OpName << "_incy);\n";
                  // ff_cell_gate += bias_c
                  size_t bc_offset = 3 * num_directions * seq_length * batch_size * fAttrHiddenSize
                     + seq_length * batch_size * fAttrHiddenSize;
                  out << "\t" << "BLAS::saxpy_(&" << OpName << "_bias_size, &" << OpName << "_alpha, tensor_"
                      << fNB << " + " << bc_offset << ", &" << OpName << "_incx, " << OpName << "_ff_cell_gate, &"
                      << OpName << "_incy);\n";
               }
            }
            if (fAttrInputForget == 0) {
               // ff_forget_gate += bias_f
               if (direction == 0) {
                  if (fType == "float") {
                     size_t bo_offset = 3 * num_directions * seq_length * batch_size * fAttrHiddenSize;
                     out << "\t" << "BLAS::saxpy_(&" << OpName << "_bias_size, &" << OpName << "_alpha, tensor_"
                         << fNB << " + " << bo_offset << ", &" << OpName << "_incx, " << OpName << "_ff_forget_gate, &"
                         << OpName << "_incy);\n";
                  }
               } else {
                  if (fType == "float") {
                     size_t bo_offset = 3 * num_directions * seq_length * batch_size * fAttrHiddenSize
                        + seq_length * batch_size * fAttrHiddenSize;
                     out << "\t" << "BLAS::saxpy_(&" << OpName << "_bias_size, &" << OpName << "_alpha, tensor_"
                         << fNB << " + " << bo_offset << ", &" << OpName << "_incx, " << OpName << "_ff_forget_gate, &"
                         << OpName << "_incy);\n";
                  }
               }
            }
         }

         // Copy ff_input_gate, ff_output_gate, ff_cell_gate and ff_forget_gate into input_gate, output_gate,
         //   cell_gate and forget_gate
         out << "\t" << "for (size_t seq = 0; seq < " << seq_length << "; seq++) {\n";
         out << "\t" << "\t" << "size_t ff_offset = seq * " << batch_size * fAttrHiddenSize << ";\n";
         if (direction == 0) {
            out << "\t" << "\t" << "size_t gate_offset = seq * " << num_directions * batch_size * fAttrHiddenSize
               << ";\n";
         } else {
            out << "\t" << "\t" << "size_t gate_offset = seq * " << num_directions * batch_size * fAttrHiddenSize
                << " + " << batch_size * fAttrHiddenSize << ";\n";
         }
         size_t ff_seq_size = batch_size * fAttrHiddenSize;
         out << "\t" << "\t" << "std::copy(" << OpName << "_ff_input_gate + ff_offset, " << OpName
             << "_ff_input_gate + ff_offset + " << ff_seq_size << ", " << OpName << "_input_gate + gate_offset);\n";
         out << "\t" << "\t" << "std::copy(" << OpName << "_ff_output_gate + ff_offset, " << OpName
             << "_ff_output_gate + ff_offset + " << ff_seq_size << ", " << OpName << "_output_gate + gate_offset);\n";
         out << "\t" << "\t" << "std::copy(" << OpName << "_ff_cell_gate + ff_offset, " << OpName
             << "_ff_cell_gate + ff_offset + " << ff_seq_size << ", " << OpName << "_cell_gate + gate_offset);\n";
         if (fAttrInputForget == 0) {
            out << "\t" << "\t" << "std::copy(" << OpName << "_ff_forget_gate + ff_offset, " << OpName
                << "_ff_forget_gate + ff_offset + " << ff_seq_size << ", " << OpName << "_forget_gate + gate_offset);\n";
         }
         out << "\t" << "}\n";

         out << "\t" << "for (size_t seq = 0; seq < " << seq_length << "; seq++) {\n";
         if (fAttrDirection == "backward" || direction == 1) {
            out << "\t" << "\t" << "size_t index = " << seq_length - 1 << " - seq;\n";
         } else {
            out << "\t" << "\t" << "size_t index = seq;\n";
         }
         out << "\t" << "\t" << "int m2 = " << batch_size << ";\n";
         if (direction == 0) {
            out << "\t" << "\t" << "size_t offset = index * " << num_directions * batch_size * fAttrHiddenSize
                 << ";\n";
         } else {
            out << "\t" << "\t" << "size_t offset = index * " << num_directions * batch_size * fAttrHiddenSize
                << " + " << batch_size * fAttrHiddenSize << ";\n";
         }
         size_t size = batch_size * fAttrHiddenSize;
         // gate = gate + initial_hidden_state * Recurrence^T
         out << "\t" << "\t" << "if (seq == 0) {\n";
         if (!fNInitial_h.empty()) {
            if (direction == 0) {
               if (fType == "float") {
                  out << "\t" << "\t" << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                      << OpName << "_n, &m2, &" << OpName << "_n, &" << OpName << "_alpha, tensor_" << fNR << ", &"
                      << OpName << "_n, " << OpName << "_initial_hidden_state, &" << OpName << "_n, &" << OpName
                      << "_alpha, " << OpName << "_input_gate + offset, &" << OpName << "_n);\n";
                  size_t ro_offset = 2 * fAttrHiddenSize * fAttrHiddenSize;
                  out << "\t" << "\t" << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                      << OpName << "_n, &m2, &" << OpName << "_n, &" << OpName << "_alpha, tensor_" << fNR << " + "
                      << ro_offset << ", &" << OpName << "_n, " << OpName << "_initial_hidden_state, &" << OpName
                      << "_n, &" << OpName << "_alpha, " << OpName << "_output_gate + offset, &" << OpName << "_n);\n";
                  size_t rc_offset = 3 * fAttrHiddenSize * fAttrHiddenSize;
                  out << "\t" << "\t" << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                      << OpName << "_n, &m2, &" << OpName << "_n, &" << OpName << "_alpha, tensor_" << fNR << " + "
                      << rc_offset << ", &" << OpName << "_n, " << OpName << "_initial_hidden_state, &" << OpName
                      << "_n, &" << OpName << "_alpha, " << OpName << "_cell_gate + offset, &" << OpName << "_n);\n";
                  if (fAttrInputForget == 0) {
                     size_t rf_offset = fAttrHiddenSize * fAttrHiddenSize;
                     out << "\t" << "\t" << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                         << OpName << "_n, &m2, &" << OpName << "_n, &" << OpName << "_alpha, tensor_" << fNR << " + "
                         << rf_offset << ", &" << OpName << "_n, " << OpName << "_initial_hidden_state, &" << OpName
                         << "_n, &" << OpName << "_alpha, " << OpName << "_forget_gate + offset, &" << OpName << "_n);\n";
                  }
               }
            } else { // direction=1
               if (fType == "float") {
                  size_t ri_offset = 4 * fAttrHiddenSize * fAttrHiddenSize;
                  out << "\t" << "\t" << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                      << OpName << "_n, &m2, &" << OpName << "_n, &" << OpName << "_alpha, tensor_" << fNR << " + "
                      << ri_offset << ", &" << OpName << "_n, " << OpName << "_initial_hidden_state, &" << OpName
                      << "_n, &" << OpName << "_alpha, " << OpName << "_input_gate + offset, &" << OpName << "_n);\n";
                  size_t ro_offset = 4 * fAttrHiddenSize * fAttrHiddenSize + 2 * fAttrHiddenSize * fAttrHiddenSize;
                  out << "\t" << "\t" << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                      << OpName << "_n, &m2, &" << OpName << "_n, &" << OpName << "_alpha, tensor_" << fNR << " + "
                      << ro_offset << ", &" << OpName << "_n, " << OpName << "_initial_hidden_state, &" << OpName
                      << "_n, &" << OpName << "_alpha, " << OpName << "_output_gate + offset, &" << OpName << "_n);\n";
                  size_t rc_offset = 4 * fAttrHiddenSize * fAttrHiddenSize + 3 * fAttrHiddenSize * fAttrHiddenSize;
                  out << "\t" << "\t" << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                      << OpName << "_n, &m2, &" << OpName << "_n, &" << OpName << "_alpha, tensor_" << fNR << " + "
                      << rc_offset << ", &" << OpName << "_n, " << OpName << "_initial_hidden_state, &" << OpName
                      << "_n, &" << OpName << "_alpha, " << OpName << "_cell_gate + offset, &" << OpName << "_n);\n";
                  if (fAttrInputForget == 0) {
                     size_t rf_offset = 4 * fAttrHiddenSize * fAttrHiddenSize + fAttrHiddenSize * fAttrHiddenSize;
                     out << "\t" << "\t" << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                         << OpName << "_n, &m2, &" << OpName << "_n, &" << OpName << "_alpha, tensor_" << fNR << " + "
                         << rf_offset << ", &" << OpName << "_n, " << OpName << "_initial_hidden_state, &" << OpName
                         << "_n, &" << OpName << "_alpha, " << OpName << "_forget_gate + offset, &" << OpName << "_n);\n";
                  }
               }
            }
         }
         out << "\t" << "\t" << "} else {\n";
         // gate = gate + previous_hidden_state * Recurrence^T
         if (direction == 0) {
            if (fAttrDirection == "backward") {
               out << "\t" << "\t" << "\t" << "size_t previous_offset = (index + 1) * "
                   << num_directions * batch_size * fAttrHiddenSize << ";\n";
            } else {
               out << "\t" << "\t" << "\t" << "size_t previous_offset = (seq - 1) * "
                   << num_directions * batch_size * fAttrHiddenSize << ";\n";
            }
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                << OpName << "_n, &m2, &" << OpName << "_n, &" << OpName << "_alpha, tensor_" << fNR << ", &"
                << OpName << "_n, " << OpName << "_hidden_state + previous_offset, &" << OpName << "_n, &"
                << OpName << "_alpha, " << OpName << "_input_gate + offset, &" << OpName << "_n);\n";
               size_t ro_offset = 2 * fAttrHiddenSize * fAttrHiddenSize;
               out << "\t" << "\t" << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                << OpName << "_n, &m2, &" << OpName << "_n, &" << OpName << "_alpha, tensor_" << fNR << " + "
                << ro_offset << ", &" << OpName << "_n, " << OpName << "_hidden_state + previous_offset, &"
                << OpName << "_n, &" << OpName << "_alpha, " << OpName << "_output_gate + offset, &"
                << OpName << "_n);\n";
               size_t rc_offset = 3 * fAttrHiddenSize * fAttrHiddenSize;
               out << "\t" << "\t" << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                << OpName << "_n, &m2, &" << OpName << "_n, &" << OpName << "_alpha, tensor_" << fNR << " + "
                << rc_offset << ", &" << OpName << "_n, " << OpName << "_hidden_state + previous_offset, &"
                << OpName << "_n, &" << OpName << "_alpha, " << OpName << "_cell_gate + offset, &"
                << OpName << "_n);\n";
               if (fAttrInputForget == 0) {
                  size_t rf_offset = fAttrHiddenSize * fAttrHiddenSize;
                  out << "\t" << "\t" << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                      << OpName << "_n, &m2, &" << OpName << "_n, &" << OpName << "_alpha, tensor_" << fNR << " + "
                      << rf_offset << ", &" << OpName << "_n, " << OpName << "_hidden_state + previous_offset, &"
                      << OpName << "_n, &" << OpName << "_alpha, " << OpName << "_forget_gate + offset, &"
                      << OpName << "_n);\n";
               }
            }
         } else {
            out << "\t" << "\t" << "\t" << "size_t previous_offset = (index + 1) * "
                << num_directions * batch_size * fAttrHiddenSize + batch_size * fAttrHiddenSize << ";\n";
            if (fType == "float") {
               size_t ri_offset = 4 * fAttrHiddenSize * fAttrHiddenSize;
               out << "\t" << "\t" << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                << OpName << "_n, &m2, &" << OpName << "_n, &" << OpName << "_alpha, tensor_" << fNR << " + "
                << ri_offset << ", &" << OpName << "_n, " << OpName << "_hidden_state + previous_offset, &"
                << OpName << "_n, &" << OpName << "_alpha, " << OpName << "_input_gate + offset, &"
                << OpName << "_n);\n";
               size_t ro_offset = 4 * fAttrHiddenSize * fAttrHiddenSize + 2 * fAttrHiddenSize * fAttrHiddenSize;
               out << "\t" << "\t" << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                << OpName << "_n, &m2, &" << OpName << "_n, &" << OpName << "_alpha, tensor_" << fNR << " + "
                << ro_offset << ", &" << OpName << "_n, " << OpName << "_hidden_state + previous_offset, &"
                << OpName << "_n, &" << OpName << "_alpha, " << OpName << "_output_gate + offset, &"
                << OpName << "_n);\n";
               size_t rc_offset = 4 * fAttrHiddenSize * fAttrHiddenSize + 3 * fAttrHiddenSize * fAttrHiddenSize;
               out << "\t" << "\t" << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                << OpName << "_n, &m2, &" << OpName << "_n, &" << OpName << "_alpha, tensor_" << fNR << " + "
                << rc_offset << ", &" << OpName << "_n, " << OpName << "_hidden_state + previous_offset, &"
                << OpName << "_n, &" << OpName << "_alpha, " << OpName << "_cell_gate + offset, &"
                << OpName << "_n);\n";
               if (fAttrInputForget == 0) {
                  size_t rf_offset = 4 * fAttrHiddenSize * fAttrHiddenSize + fAttrHiddenSize * fAttrHiddenSize;
                  out << "\t" << "\t" << "\t" << "BLAS::sgemm_(&" << OpName << "_transB, &" << OpName << "_transA, &"
                      << OpName << "_n, &m2, &" << OpName << "_n, &" << OpName << "_alpha, tensor_" << fNR << " + "
                      << rf_offset << ", &" << OpName << "_n, " << OpName << "_hidden_state + previous_offset, &"
                      << OpName << "_n, &" << OpName << "_alpha, " << OpName << "_forget_gate + offset, &"
                      << OpName << "_n);\n";
               }
            }
         }
         out << "\t" << "\t" << "}\n";

         // Clip the elements of the cell gate into the range [-fClip, fClip]
         if (fAttrClip > .0) {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "float x = (" << OpName << "_cell_gate[i] > " << -fAttrClip << ") ? "
                   << OpName << "_cell_gate[i] : " << -fAttrClip << ";\n";
            }
            out << "\t" << "\t" << "\t" << OpName << "_cell_gate[i] = (x < " << fAttrClip << ") ? x : "
                << fAttrClip << ";\n";
            out << "\t" << "\t" << "}\n";
         }
         // Apply the activation function to the cell gate, cell_gate = g(cell_gate)
         if (fAttrActivations[direction * 3 + 1] == "Relu") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "if (" << OpName << "_cell_gate[i] < 0.)\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_cell_gate[i] = 0.;\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3 + 1] == "Tanh") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "float ex = exp(-2 * " << OpName << "_cell_gate[i]);\n";
            }
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_cell_gate[i] = (1. - ex) / (1. + ex);\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3 + 1] == "Sigmoid") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_cell_gate[i] = 1. / (1. + exp(-" << OpName
                << "_cell_gate[i]));\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3 + 1] == "Affine") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_cell_gate[i] = "
                << fAttrActivationAlpha[direction * 3 + 1] << " * " << OpName << "_cell_gate[i] + "
                << fAttrActivationBeta[direction * 3 + 1] << ";\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3 + 1] == "ScaledTanh") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "float ex = exp(-2 * " << fAttrActivationBeta[direction * 3 + 1]
                   << " * "<< OpName << "_cell_gate[i]);\n";
               }
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_cell_gate[i] = "
                   << fAttrActivationAlpha[direction * 3 + 1] << " * (1. - ex) / (1. + ex);\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3 + 1] == "HardSigmoid") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "float a = " << fAttrActivationAlpha[direction * 3 + 1] << " * "
                   << OpName << "_cell_gate[i] + " << fAttrActivationBeta[direction * 3 + 1] << ";\n";
               out << "\t" << "\t" << "\t" << "float b = (a > 0.) ? a : 0.;\n";
            }
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_cell_gate[i] = (b < 1.) ? b : 1.;\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3 + 1] == "LeakyRelu") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "if (" << OpName << "_cell_gate[i] < 0.)\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_cell_gate[i] = "
                << fAttrActivationAlpha[direction * 3 + 1] << " * " << OpName << "_cell_gate[i];\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3 + 1] == "ThresholdRelu") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "if (" << OpName << "_cell_gate[i] < "
                << fAttrActivationAlpha[direction * 3 + 1] << ")\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_cell_gate[i] = 0.;\n";
            out << "\t" << "\t" << "}";
         } else if (fAttrActivations[direction * 3 + 1] == "Elu") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "if (" << OpName << "_cell_gate[i] < 0.)\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_cell_gate[i] = "
                << fAttrActivationAlpha[direction * 3 + 1] << " * exp(" << OpName << "_cell_gate[i] - 1.);\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3 + 1] == "Softsign") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_cell_gate[i] = " << OpName
                << "_cell_gate[i] / (1. + abs(" << OpName << "_cell_gate[i]));\n";
            out << "\t" << "\t" << "}\n";
         } else { // fAttrActivations[direction * 3 + 1] = Softplus
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_cell_gate[i] = log(1. + exp("
                << OpName << "_cell_gate[i]));\n";
            out << "\t" << "\t" << "}\n";
         }

         // Peephole connections for the input gate and the forget gate
         if (!fNP.empty()) {
            // gate = 1.0 * gate + previous_cell_state * P^T
            out << "\t" << "\t" << "if (seq == 0) {\n";
            if (!fNInitial_c.empty()) {
               if (direction == 0) {
                  out << "\t" << "\t" << "\t" << "for (size_t i = 0; i < " << size << "; i++) {\n";
                  out << "\t" << "\t" << "\t" << "\t" << OpName << "_input_gate[i + offset] += tensor_" << fNP
                      << "[i] * " << OpName << "_initial_cell_state[i];\n";
                  out << "\t" << "\t" << "\t" << "}\n";
                  if (fAttrInputForget == 0) {
                     size_t pf_offset = batch_size * fAttrHiddenSize;
                     out << "\t" << "\t" << "\t" << "for (size_t i = 0; i < " << size << "; i++) {\n";
                     out << "\t" << "\t" << "\t" << "\t" << OpName << "_forget_gate[i + offset] += tensor_" << fNP
                         << "[i + " << pf_offset << "] * " << OpName << "_initial_cell_state[i];\n";
                     out << "\t" << "\t" << "\t" << "}\n";
                  }
               } else {
                  size_t pi_offset = 3 * batch_size * fAttrHiddenSize;
                  size_t initial_c_offset = batch_size * fAttrHiddenSize;
                  out << "\t" << "\t" << "\t" << "for (size_t i = 0; i < " << size << "; i++) {\n";
                  out << "\t" << "\t" << "\t" << "\t" << OpName << "_input_gate[i + offset] += tensor_" << fNP
                      << "[i + " << pi_offset << "] * " << OpName << "_initial_cell_state[i + " << initial_c_offset
                      << "];\n";
                  out << "\t" << "\t" << "\t" << "}\n";
                  if (fAttrInputForget == 0) {
                     size_t pf_offset = 3 * batch_size * fAttrHiddenSize + batch_size * fAttrHiddenSize;
                     out << "\t" << "\t" << "\t" << "for (size_t i = 0; i < " << size << "; i++) {\n";
                     out << "\t" << "\t" << "\t" << "\t" << OpName << "_forget_gate[i + offset] += tensor_" << fNP
                         << "[i + " << pf_offset << "] * " << OpName << "_initial_cell_state[i + " << initial_c_offset
                         << "];\n";
                     out << "\t" << "\t" << "\t" << "}\n";
                  }
               }
            }
            out << "\t" << "\t" << "} else {\n";
            if (direction == 0) {
               if (fAttrDirection == "backward") {
                  out << "\t" << "\t" << "\t" << "size_t c_offset = (index + 1) * "
                      << num_directions * batch_size * fAttrHiddenSize;
               } else {
                  out << "\t" << "\t" << "\t" << "size_t c_offset = (seq - 1) * "
                      << num_directions * batch_size * fAttrHiddenSize;
               }
               out << "\t" << "\t" << "\t" << "for (size_t i = 0; i < " << size << "; i++) {\n";
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_input_gate[i + offset] += tensor_" << fNP
                   << "[i] * " << OpName << "_initial_cell_state[i + c_offset];\n";
               out << "\t" << "\t" << "\t" << "}\n";
               if (fAttrInputForget == 0) {
                  size_t pf_offset = batch_size * fAttrHiddenSize;
                  out << "\t" << "\t" << "\t" << "for (size_t i = 0; i < " << size << "; i++) {\n";
                  out << "\t" << "\t" << "\t" << "\t" << OpName << "_forget_gate[i + offset] += tensor_" << fNP
                      << "[i + " << pf_offset << "] * " << OpName << "_initial_cell_state[i + c_offset];\n";
                  out << "\t" << "\t" << "\t" << "}\n";
               }
            } else { // direction=1
               size_t pi_offset = 3 * batch_size * fAttrHiddenSize;
               out << "\t" << "\t" << "\t" << "size_t c_offset = (index + 1) * "
                   << num_directions * batch_size * fAttrHiddenSize + batch_size * fAttrHiddenSize;
               out << "\t" << "\t" << "\t" << "for (size_t i = 0; i < " << size << "; i++) {\n";
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_input_gate[i + offset] += tensor_" << fNP
                   << "[i + " << pi_offset << "] * " << OpName << "_initial_cell_state[i + c_offset];\n";
               out << "\t" << "\t" << "\t" << "}\n";
               if (fAttrInputForget == 0) {
                  size_t pf_offset = 3 * batch_size * fAttrHiddenSize + batch_size * fAttrHiddenSize;
                  out << "\t" << "\t" << "\t" << "for (size_t i = 0; i < " << size << "; i++) {\n";
                  out << "\t" << "\t" << "\t" << "\t" << OpName << "_forget_gate[i + offset] += tensor_" << fNP
                      << "[i + " << pf_offset << "] * " << OpName << "_initial_cell_state[i + c_offset];\n";
                  out << "\t" << "\t" << "\t" << "}\n";
               }
            }
            out << "\t" << "\t" << "}\n";
         }

         // Clip the elements of the input gate into the range [-fClip, fClip]
         if (fAttrClip > .0) {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "float x = (" << OpName << "_input_gate[i] > " << -fAttrClip << ") ? "
                   << OpName << "_input_gate[i] : " << -fAttrClip << ";\n";
            }
            out << "\t" << "\t" << "\t" << OpName << "_input_gate[i] = (x < " << fAttrClip << ") ? x : "
                << fAttrClip << ";\n";
            out << "\t" << "\t" << "}\n";
         }
         // Apply the activation function to the input gate
         if (fAttrActivations[direction * 3] == "Relu") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "if (" << OpName << "_input_gate[i] < 0.)\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_input_gate[i] = 0.;\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3] == "Tanh") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "float ex = exp(-2 * " << OpName << "_input_gate[i]);\n";
            }
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_input_gate[i] = (1. - ex) / (1. + ex);\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3] == "Sigmoid") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_input_gate[i] = 1. / (1. + exp(-" << OpName
                << "_input_gate[i]));\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3] == "Affine") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_input_gate[i] = "
                << fAttrActivationAlpha[direction * 3] << " * " << OpName << "_input_gate[i] + "
                << fAttrActivationBeta[direction * 3] << ";\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3] == "ScaledTanh") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "float ex = exp(-2 * " << fAttrActivationBeta[direction * 3]
                   << " * "<< OpName << "_input_gate[i]);\n";
               }
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_input_gate[i] = "
                   << fAttrActivationAlpha[direction * 3] << " * (1. - ex) / (1. + ex);\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3] == "HardSigmoid") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "float a = " << fAttrActivationAlpha[direction * 3] << " * "
                   << OpName << "_input_gate[i] + " << fAttrActivationBeta[direction * 3] << ";\n";
               out << "\t" << "\t" << "\t" << "float b = (a > 0.) ? a : 0.;\n";
            }
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_input_gate[i] = (b < 1.) ? b : 1.;\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3] == "LeakyRelu") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "if (" << OpName << "_input_gate[i] < 0.)\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_input_gate[i] = "
                << fAttrActivationAlpha[direction * 3] << " * " << OpName << "_input_gate[i];\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3] == "ThresholdRelu") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "if (" << OpName << "_input_gate[i] < "
                << fAttrActivationAlpha[direction * 3] << ")\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_input_gate[i] = 0.;\n";
            out << "\t" << "\t" << "}";
         } else if (fAttrActivations[direction * 3] == "Elu") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "if (" << OpName << "_input_gate[i] < 0.)\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_input_gate[i] = "
                << fAttrActivationAlpha[direction * 3] << " * exp(" << OpName << "_input_gate[i] - 1.);\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3] == "Softsign") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_input_gate[i] = " << OpName
                << "_input_gate[i] / (1. + abs(" << OpName << "_input_gate[i]));\n";
            out << "\t" << "\t" << "}\n";
         } else { // fAttrActivations[direction * 3] = Softplus
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_input_gate[i] = log(1. + exp("
                << OpName << "_input_gate[i]));\n";
            out << "\t" << "\t" << "}\n";
         }

         if (fAttrInputForget == 0) {
            // Clip the elements of the forget gate into the range [-fClip, fClip]
            if (fAttrClip > .0) {
               out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
               if (fType == "float") {
                  out << "\t" << "\t" << "\t" << "float x = (" << OpName << "_forget_gate[i] > "
                      << -fAttrClip << ") ? " << OpName << "_forget_gate[i] : " << -fAttrClip << ";\n";
               }
               out << "\t" << "\t" << "\t" << OpName << "_forget_gate[i] = (x < " << fAttrClip
                   << ") ? x : " << fAttrClip << ";\n";
               out << "\t" << "\t" << "}\n";
            }
            // Apply the activation function to the forget gate, cell_gate = g(cell_gate)
            if (fAttrActivations[direction * 3] == "Relu") {
               out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
               out << "\t" << "\t" << "\t" << "if (" << OpName << "_forget_gate[i] < 0.)\n";
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_forget_gate[i] = 0.;\n";
               out << "\t" << "\t" << "}\n";
            } else if (fAttrActivations[direction * 3] == "Tanh") {
               out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
               if (fType == "float") {
                  out << "\t" << "\t" << "\t" << "float ex = exp(-2 * " << OpName << "_forget_gate[i]);\n";
               }
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_forget_gate[i] = (1. - ex) / (1. + ex);\n";
               out << "\t" << "\t" << "}\n";
            } else if (fAttrActivations[direction * 3] == "Sigmoid") {
               out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_forget_gate[i] = 1. / (1. + exp(-"
                   << OpName << "_forget_gate[i]));\n";
               out << "\t" << "\t" << "}\n";
            } else if (fAttrActivations[direction * 3] == "Affine") {
               out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_forget_gate[i] = "
                   << fAttrActivationAlpha[direction * 3] << " * " << OpName << "_forget_gate[i] + "
                  << fAttrActivationBeta[direction * 3] << ";\n";
               out << "\t" << "\t" << "}\n";
            } else if (fAttrActivations[direction * 3] == "ScaledTanh") {
               out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
               if (fType == "float") {
                  out << "\t" << "\t" << "\t" << "float ex = exp(-2 * " << fAttrActivationBeta[direction * 3]
                      << " * "<< OpName << "_forget_gate[i]);\n";
               }
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_forget_gate[i] = "
                   << fAttrActivationAlpha[direction * 3] << " * (1. - ex) / (1. + ex);\n";
               out << "\t" << "\t" << "}\n";
            } else if (fAttrActivations[direction * 3] == "HardSigmoid") {
               out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
               if (fType == "float") {
                  out << "\t" << "\t" << "\t" << "float a = " << fAttrActivationAlpha[direction * 3] << " * "
                      << OpName << "_forget_gate[i] + " << fAttrActivationBeta[direction * 3] << ";\n";
                  out << "\t" << "\t" << "\t" << "float b = (a > 0.) ? a : 0.;\n";
               }
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_forget_gate[i] = (b < 1.) ? b : 1.;\n";
               out << "\t" << "\t" << "}\n";
            } else if (fAttrActivations[direction * 3] == "LeakyRelu") {
               out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
               out << "\t" << "\t" << "\t" << "if (" << OpName << "_forget_gate[i] < 0.)\n";
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_forget_gate[i] = "
                   << fAttrActivationAlpha[direction * 3] << " * " << OpName << "_forget_gate[i];\n";
               out << "\t" << "\t" << "}\n";
            } else if (fAttrActivations[direction * 3] == "ThresholdRelu") {
               out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
               out << "\t" << "\t" << "\t" << "if (" << OpName << "_forget_gate[i] < "
                   << fAttrActivationAlpha[direction * 3] << ")\n";
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_forget_gate[i] = 0.;\n";
               out << "\t" << "\t" << "}";
            } else if (fAttrActivations[direction * 3] == "Elu") {
               out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
               out << "\t" << "\t" << "\t" << "if (" << OpName << "_forget_gate[i] < 0.)\n";
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_forget_gate[i] = "
                   << fAttrActivationAlpha[direction * 3] << " * exp(" << OpName << "_forget_gate[i] - 1.);\n";
               out << "\t" << "\t" << "}\n";
            } else if (fAttrActivations[direction * 3] == "Softsign") {
               out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_forget_gate[i] = " << OpName
                   << "_forget_gate[i] / (1. + abs(" << OpName << "_forget_gate[i]));\n";
               out << "\t" << "\t" << "}\n";
            } else { // fAttrActivations[direction * 3] = Softplus
               out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_forget_gate[i] = log(1. + exp("
                   << OpName << "_forget_gate[i]));\n";
               out << "\t" << "\t" << "}\n";
            }
         }

         // cell_state = input_gate o cell_gate
         out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
         out << "\t" << "\t" << "\t" << OpName << "_cell_state[i] = " << OpName << "_input_gate[i] * "
             << OpName << "_cell_gate[i];\n";
         out << "\t" << "\t" << "}\n";

         if (fAttrInputForget == 0) {
            out << "\t" << "\t" << "if (seq == 0) {\n";
            if (!fNInitial_c.empty()) {
               // cell_state += forget_gate o initial_cell_state
               out << "\t" << "\t" << "\t" << "for (size_t i = 0; i < " << size << "; i++) {\n";
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_cell_state[i + offset] += "
                   << OpName << "_forget_gate[i + offset] * " << OpName << "_initial_cell_state[i];\n";
               out << "\t" << "\t" << "\t" << "}\n";
            }
            out << "\t" << "\t" << "} else {\n";
            // cell_state += forget_gate o previous_cell_state
            if (direction == 0) {
               if (fAttrDirection == "backward") {
                  out << "\t" << "\t" << "\t" << "size_t previous_offset = (index + 1) * "
                      << num_directions * batch_size * fAttrHiddenSize << ";\n";
               } else {
                  out << "\t" << "\t" << "\t" << "size_t previous_offset = (seq - 1) * "
                      << num_directions * batch_size * fAttrHiddenSize << ";\n";
               }
            } else { // direction=1
               out << "\t" << "\t" << "\t" << "size_t previous_offset = (index + 1) * "
                   << num_directions * batch_size * fAttrHiddenSize + batch_size * fAttrHiddenSize << ";\n";
            }
            out << "\t" << "\t" << "\t" << "for (size_t i = 0; i < " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_cell_state[i + offset] += "
                << OpName << "_forget_gate[i + offset] * " << OpName << "_cell_state[i + previous_offset];\n";
            out << "\t" << "\t" << "\t" << "}\n";
            out << "\t" << "\t" << "}\n";
         }

         if (!fNP.empty()) {
            // Peephole connection for the output gate
            if (direction == 0) {
               size_t p_offset = 2 * batch_size * fAttrHiddenSize;
               out << "\t" << "\t" << "\t" << "for (size_t i = 0; i < " << size << "; i++) {\n";
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_output_gate[i + offset] += tensor_"
                   << fNP << "[i + " << p_offset << "] * " << OpName << "_cell_state[i + offset];\n";
               out << "\t" << "\t" << "\t" << "}\n";
            } else { // direction=1
               size_t p_offset = 3 * batch_size * fAttrHiddenSize + 2 * batch_size * fAttrHiddenSize;
               out << "\t" << "\t" << "\t" << "for (size_t i = 0; i < " << size << "; i++) {\n";
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_output_gate[i + offset] += tensor_"
                   << fNP << "[i + " << p_offset << "] * " << OpName << "_cell_state[i + offset];\n";
               out << "\t" << "\t" << "\t" << "}\n";
            }
         }

         // Clip the elements of the output gate into the range [-fClip, fClip]
         if (fAttrClip > .0) {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "float x = (" << OpName << "_output_gate[i] > " << -fAttrClip
                   << ") ? " << OpName << "_output_gate[i] : " << -fAttrClip << ";\n";
            }
            out << "\t" << "\t" << "\t" << OpName << "_output_gate[i] = (x < " << fAttrClip << ") ? x : "
                << fAttrClip << ";\n";
            out << "\t" << "\t" << "}\n";
         }
         // Apply the activation function to the output gate
         if (fAttrActivations[direction * 3] == "Relu") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "if (" << OpName << "_output_gate[i] < 0.)\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_output_gate[i] = 0.;\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3] == "Tanh") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "float ex = exp(-2 * " << OpName << "_output_gate[i]);\n";
            }
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_output_gate[i] = (1. - ex) / (1. + ex);\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3] == "Sigmoid") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_output_gate[i] = 1. / (1. + exp(-" << OpName
                << "_output_gate[i]));\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3] == "Affine") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_output_gate[i] = "
                << fAttrActivationAlpha[direction * 3] << " * " << OpName << "_output_gate[i] + "
                << fAttrActivationBeta[direction * 3] << ";\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3] == "ScaledTanh") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "float ex = exp(-2 * " << fAttrActivationBeta[direction * 3]
                   << " * "<< OpName << "_output_gate[i]);\n";
               }
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_output_gate[i] = "
                   << fAttrActivationAlpha[direction * 3] << " * (1. - ex) / (1. + ex);\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3] == "HardSigmoid") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "float a = " << fAttrActivationAlpha[direction * 3] << " * "
                   << OpName << "_output_gate[i] + " << fAttrActivationBeta[direction * 3] << ";\n";
               out << "\t" << "\t" << "\t" << "float b = (a > 0.) ? a : 0.;\n";
            }
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_output_gate[i] = (b < 1.) ? b : 1.;\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3] == "LeakyRelu") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "if (" << OpName << "_output_gate[i] < 0.)\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_output_gate[i] = "
                << fAttrActivationAlpha[direction * 3] << " * " << OpName << "_output_gate[i];\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3] == "ThresholdRelu") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "if (" << OpName << "_output_gate[i] < "
                << fAttrActivationAlpha[direction * 3] << ")\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_output_gate[i] = 0.;\n";
            out << "\t" << "\t" << "}";
         } else if (fAttrActivations[direction * 3] == "Elu") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "if (" << OpName << "_output_gate[i] < 0.)\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_output_gate[i] = "
                << fAttrActivationAlpha[direction * 3] << " * exp(" << OpName << "_output_gate[i] - 1.);\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3] == "Softsign") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_output_gate[i] = " << OpName
                << "_output_gate[i] / (1. + abs(" << OpName << "_output_gate[i]));\n";
            out << "\t" << "\t" << "}\n";
         } else { // fAttrActivations[direction * 3] = Softplus
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_output_gate[i] = log(1. + exp("
                << OpName << "_output_gate[i]));\n";
            out << "\t" << "\t" << "}\n";
         }

         // copy cell_state into new_cell_state
         out << "\t" << "\t" << "std::copy(" << OpName << "_cell_state + offset, " << OpName
             << "_cell_state + offset + " << size << ", "<< OpName << "_new_cell_state + offset);\n";
         // Clip the elements of the new_cell_state into the range [-fAttrClip, fAttrClip]
         if (fAttrClip > .0) {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "float x = (" << OpName << "_new_cell_state[i] > " << -fAttrClip
                   << ") ? " << OpName << "_new_cell_state[i] : " << -fAttrClip << ";\n";
            }
            out << "\t" << "\t" << "\t" << OpName << "_new_cell_state[i] = (x < " << fAttrClip << ") ? x : "
                << fAttrClip << ";\n";
            out << "\t" << "\t" << "}\n";
         }
         // Apply the activation function to the new cell state
         if (fAttrActivations[direction * 3 + 2] == "Relu") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "if (" << OpName << "_new_cell_state[i] < 0.)\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_new_cell_state[i] = 0.;\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3 + 2] == "Tanh") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "float ex = exp(-2 * " << OpName << "_new_cell_state[i]);\n";
            }
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_new_cell_state[i] = (1. - ex) / (1. + ex);\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3 + 2] == "Sigmoid") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_new_cell_state[i] = 1. / (1. + exp(-" << OpName
                << "_new_cell_state[i]));\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3 + 2] == "Affine") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_new_cell_state[i] = "
                << fAttrActivationAlpha[direction * 3 + 2] << " * " << OpName << "_new_cell_state[i] + "
                << fAttrActivationBeta[direction * 3 + 2] << ";\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3 + 2] == "ScaledTanh") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "float ex = exp(-2 * " << fAttrActivationBeta[direction * 3 + 2]
                   << " * "<< OpName << "_new_cell_state[i]);\n";
               }
               out << "\t" << "\t" << "\t" << "\t" << OpName << "_new_cell_state[i] = "
                   << fAttrActivationAlpha[direction * 3 + 2] << " * (1. - ex) / (1. + ex);\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3 + 2] == "HardSigmoid") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            if (fType == "float") {
               out << "\t" << "\t" << "\t" << "float a = " << fAttrActivationAlpha[direction * 3 + 2] << " * "
                   << OpName << "_new_cell_state[i] + " << fAttrActivationBeta[direction * 3 + 2] << ";\n";
               out << "\t" << "\t" << "\t" << "float b = (a > 0.) ? a : 0.;\n";
            }
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_new_cell_state[i] = (b < 1.) ? b : 1.;\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3 + 2] == "LeakyRelu") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "if (" << OpName << "_new_cell_state[i] < 0.)\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_new_cell_state[i] = "
                << fAttrActivationAlpha[direction * 3 + 2] << " * " << OpName << "_new_cell_state[i];\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3 + 2] == "ThresholdRelu") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "if (" << OpName << "_new_cell_state[i] < "
                << fAttrActivationAlpha[direction * 3 + 2] << ")\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_new_cell_state[i] = 0.;\n";
            out << "\t" << "\t" << "}";
         } else if (fAttrActivations[direction * 3 + 2] == "Elu") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "if (" << OpName << "_new_cell_state[i] < 0.)\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_new_cell_state[i] = "
                << fAttrActivationAlpha[direction * 3 + 2] << " * exp(" << OpName << "_new_cell_state[i] - 1.);\n";
            out << "\t" << "\t" << "}\n";
         } else if (fAttrActivations[direction * 3 + 2] == "Softsign") {
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_new_cell_state[i] = " << OpName
                << "_new_cell_state[i] / (1. + abs(" << OpName << "_new_cell_state[i]));\n";
            out << "\t" << "\t" << "}\n";
         } else { // fAttrActivations[direction * 3 + 2] = Softplus
            out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << OpName << "_new_cell_state[i] = log(1. + exp("
                << OpName << "_new_cell_state[i]));\n";
            out << "\t" << "\t" << "}\n";
         }

         // hidden_state = output_gate o new_cell_state
         out << "\t" << "\t" << "for (size_t i = offset; i < offset + " << size << "; i++) {\n";
         out << "\t" << "\t" << "\t" << OpName << "_hidden_state[i] = " << OpName << "_output_gate[i] * "
             << OpName << "_new_cell_state[i];\n";
         out << "\t" << "\t" << "}\n";
         out << "\t" << "}\n";
      }

      // Padding the hidden state for LSTM with different sequence lengths
      if (!fNSequence_lens.empty()) {
         out << "\t" << "for (size_t seq = 0; seq < " << seq_length << "; seq++) {\n";
         out << "\t" << "\t" << "for (size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
         out << "\t" << "\t" << "\t" << "if (seq >= tensor_" << fNSequence_lens << "[batch]) {\n";
         for (size_t direction = 0; direction < num_directions; direction++) {
            out << "\t" << "\t" << "\t" << "\t" << "\t" << "for (size_t h = 0; h < " << fAttrHiddenSize << "; h++) {\n";
            out << "\t" << "\t" << "\t" << "\t" << "\t" << "\t" << "size_t idx = seq * "
                << num_directions * batch_size * fAttrHiddenSize + direction * batch_size * fAttrHiddenSize
                << " + batch * " << fAttrHiddenSize << " + h;\n";
            out << "\t" << "\t" << "\t" << "\t" << "\t" << "\t" << OpName << "_cell_state[idx] = 0.;\n";
            out << "\t" << "\t" << "\t" << "\t" << "\t" << "\t" << OpName << "_hidden_state[idx] = 0.;\n";
            out << "\t" << "\t" << "\t" << "\t" << "\t" << "}\n";
         }
         out << "\t" << "\t" << "\t" << "}\n";
         out << "\t" << "\t" << "}\n";
         out << "\t" << "}\n";
      }

      // Copy the hidden state into y and y_h and copy cell_state into y_c
      if (fAttrLayout == 0) {
         if (!fNY_h.empty()) {
            // Copy hidden_state into Y_h
            if (fNSequence_lens.empty()) {
               size_t y_h_size = batch_size * fAttrHiddenSize;
               if (fAttrDirection == "backward") {
                  out << "\t" << "std::copy(" << OpName << "_hidden_state, " << OpName << "_hidden_state + "
                      << y_h_size << ", tensor_" << fNY_h << ");\n";
               } else {
                  size_t offset = (seq_length - 1) * num_directions * batch_size * fAttrHiddenSize;
                  out << "\t" << "std::copy(" << OpName << "_hidden_state + " << offset << ", " << OpName
                      << "_hidden_state + " << offset << " + " << y_h_size << ", tensor_" << fNY_h << ");\n";
               }
               if (num_directions == 2) {
                  out << "\t" << "std::copy(" << OpName << "_hidden_state + " << y_h_size << ", " << OpName
                      << "_hidden_state + " << 2 * y_h_size << ", tensor_" << fNY_h << " + " << y_h_size << ");\n";
               }
            } else { // LSTM with different sequence lengths
               if (fAttrDirection == "backward") {
                  out << "\t" << "for (size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
                  out << "\t" << "\t" << "size_t offset = batch * " << fAttrHiddenSize << ";\n";
                  out << "\t" << "\t" << "std::copy(" << OpName << "_hidden_state + offset, " << OpName
                      << "_hidden_state + offset + " << fAttrHiddenSize << ", tensor_" << fNY_h << " + offset);\n";
                  out << "\t" << "}\n";
               } else {
                  out << "\t" << "for (size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
                  out << "\t" << "\t" << "size_t seq = " << "tensor_" << fNSequence_lens << "[batch] - 1;\n";
                  out << "\t" << "\t" << "size_t offset = seq * " << num_directions * batch_size * fAttrHiddenSize
                      << " + batch * " << fAttrHiddenSize << ";\n";
                  out << "\t" << "\t" << "size_t y_h_offset = batch * " << fAttrHiddenSize << ";\n";
                  out << "\t" << "\t" << "std::copy(" << OpName << "_hidden_state + offset, " << OpName
                      << "_hidden_state + offset + " << fAttrHiddenSize << ", tensor_" << fNY_h << " + y_h_offset);\n";
                  out << "\t" << "}\n";
               }
               if (num_directions == 2) {
                  out << "\t" << "for (size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
                  out << "\t" << "\t" << "size_t offset = " << batch_size * fAttrHiddenSize
                      << " + batch * " << fAttrHiddenSize << ";\n";
                  out << "\t" << "\t" << "size_t y_h_offset = " << batch_size * fAttrHiddenSize
                      << " + batch * " << fAttrHiddenSize << ";\n";
                  out << "\t" << "\t" << "std::copy(" << OpName << "_hidden_state + offset, " << OpName
                      << "_hidden_state + offset + " << fAttrHiddenSize << ", tensor_" << fNY_h << " + y_h_offset);\n";
                  out << "\t" << "}\n";
               }
            }
         }
         if (!fNY_c.empty()) {
            // Copy cell_state into Y_c
            if (fNSequence_lens.empty()) {
               size_t y_h_size = batch_size * fAttrHiddenSize;
               if (fAttrDirection == "backward") {
                  out << "\t" << "std::copy(" << OpName << "_cell_state, " << OpName << "_hidden_state + "
                      << y_h_size << ", tensor_" << fNY_c << ");\n";
               } else {
                  size_t offset = (seq_length - 1) * num_directions * batch_size * fAttrHiddenSize;
                  out << "\t" << "std::copy(" << OpName << "_cell_state + " << offset << ", " << OpName
                      << "_cell_state + " << offset << " + " << y_h_size << ", tensor_" << fNY_c << ");\n";
               }
               if (num_directions == 2) {
                  out << "\t" << "std::copy(" << OpName << "_cell_state + " << y_h_size << ", " << OpName
                      << "_cell_state + " << 2 * y_h_size << ", tensor_" << fNY_c << " + " << y_h_size << ");\n";
               }
            } else { // LSTM with different sequence lengths
               if (fAttrDirection == "backward") {
                  out << "\t" << "for (size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
                  out << "\t" << "\t" << "size_t offset = batch * " << fAttrHiddenSize << ";\n";
                  out << "\t" << "\t" << "std::copy(" << OpName << "_cell_state + offset, " << OpName
                      << "_cell_state + offset + " << fAttrHiddenSize << ", tensor_" << fNY_c << " + offset);\n";
                  out << "\t" << "}\n";
               } else {
                  out << "\t" << "for (size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
                  out << "\t" << "\t" << "size_t seq = " << "tensor_" << fNSequence_lens << "[batch] - 1;\n";
                  out << "\t" << "\t" << "size_t offset = seq * " << num_directions * batch_size * fAttrHiddenSize
                      << " + batch * " << fAttrHiddenSize << ";\n";
                  out << "\t" << "\t" << "size_t y_h_offset = batch * " << fAttrHiddenSize << ";\n";
                  out << "\t" << "\t" << "std::copy(" << OpName << "_cell_state + offset, " << OpName
                      << "_cell_state + offset + " << fAttrHiddenSize << ", tensor_" << fNY_c << " + y_h_offset);\n";
                  out << "\t" << "}\n";
               }
               if (num_directions == 2) {
                  out << "\t" << "for (size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
                  out << "\t" << "\t" << "size_t offset = " << batch_size * fAttrHiddenSize
                      << " + batch * " << fAttrHiddenSize << ";\n";
                  out << "\t" << "\t" << "size_t y_h_offset = " << batch_size * fAttrHiddenSize
                      << " + batch * " << fAttrHiddenSize << ";\n";
                  out << "\t" << "\t" << "std::copy(" << OpName << "_cell_state + offset, " << OpName
                      << "_cell_state + offset + " << fAttrHiddenSize << ", tensor_" << fNY_c << " + y_h_offset);\n";
                  out << "\t" << "}\n";
               }
            }
         }
      } else { // fAttrLayout=1
         if (!fNY.empty()) {
            // Copy hidden_state into Y
            for (size_t direction = 0; direction < num_directions; direction++) {
               out << "\t" << "for (size_t seq = 0; seq < " << seq_length << "; seq++) {\n";
               out << "\t" << "\t" << "for (size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
               out << "\t" << "\t" << "\t" << "size_t offset = seq * " << num_directions * batch_size * fAttrHiddenSize
                   << " + " << direction * batch_size * fAttrHiddenSize << " + batch * " << fAttrHiddenSize << ";\n";
               out << "\t" << "\t" << "\t" << "size_t y_offset = batch * " << seq_length * num_directions * fAttrHiddenSize
                   << " + seq * " << num_directions * fAttrHiddenSize << " + " << direction * fAttrHiddenSize << ";\n";
               out << "\t" << "\t" << "\t" << "std::copy(" << OpName << "_hidden_state + offset, " << OpName
                   << "_hidden_state + offset + " << fAttrHiddenSize << ", tensor_" << fNY << " + y_offset);\n";
               out << "\t" << "\t" << "}\n";
               out << "\t" << "}\n";
            }
         }
         if (!fNY_h.empty()) {
            // Copy the hidden_state into Y_h
            if (fAttrDirection == "backward") {
               out << "\t" << "for (size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
               out << "\t" << "\t" << "size_t offset = batch * " << fAttrHiddenSize << ";\n";
               out << "\t" << "\t" << "size_t y_h_offset = batch * " << num_directions * fAttrHiddenSize << ";\n";
               out << "\t" << "\t" << "std::copy(" << OpName << "_hidden_state + offset, " << OpName
                   << "_hidden_state + offset + " << fAttrHiddenSize << ", tensor_" << fNY_h << " + y_h_offset);\n";
               out << "\t" << "}\n";
            } else {
               out << "\t" << "for (size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
               if (fNSequence_lens.empty()) {
                  out << "\t" << "\t" << "size_t seq = " << seq_length - 1 << ";\n";
               } else {
                  out << "\t" << "\t" << "size_t seq = " << "tensor_" << fNSequence_lens << "[batch] - 1;\n";
               }
               out << "\t" << "\t" << "size_t offset = seq * " << num_directions * batch_size * fAttrHiddenSize
                   << " + batch * " << fAttrHiddenSize << ";\n";
               out << "\t" << "\t" << "size_t y_h_offset = batch * " << num_directions * fAttrHiddenSize << ";\n";
               out << "\t" << "\t" << "std::copy(" << OpName << "_hidden_state + offset, " << OpName
                   << "_hidden_state + offset + " << fAttrHiddenSize << ", tensor_" << fNY_h << " + y_h_offset);\n";
               out << "\t" << "}\n";
            }
            if (num_directions == 2) {
               out << "\t" << "for (size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
               out << "\t" << "\t" << "size_t offset = " << batch_size * fAttrHiddenSize << " + batch * "
                   << fAttrHiddenSize << ";\n";
               out << "\t" << "\t" << "size_t y_h_offset = batch * " << num_directions * fAttrHiddenSize << " + "
                   << fAttrHiddenSize << ";\n";
               out << "\t" << "\t" << "std::copy(" << OpName << "_hidden_state + offset, " << OpName
                   << "_hidden_state + offset + " << fAttrHiddenSize << ", tensor_" << fNY_h << " + y_h_offset);\n";
               out << "\t" << "}\n";
            }
         }

         if (!fNY_c.empty()) {
            // copy the cell_state into Y_c
            if (fAttrDirection == "backward") {
               out << "\t" << "for (size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
               out << "\t" << "\t" << "size_t offset = batch * " << fAttrHiddenSize << ";\n";
               out << "\t" << "\t" << "size_t y_h_offset = batch * " << num_directions * fAttrHiddenSize << ";\n";
               out << "\t" << "\t" << "std::copy(" << OpName << "_cell_state + offset, " << OpName
                   << "_cell_state + offset + " << fAttrHiddenSize << ", tensor_" << fNY_c << " + y_h_offset);\n";
               out << "\t" << "}\n";
            } else {
               out << "\t" << "for (size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
               if (fNSequence_lens.empty()) {
                  out << "\t" << "\t" << "size_t seq = " << seq_length - 1 << ";\n";
               } else {
                  out << "\t" << "\t" << "size_t seq = " << "tensor_" << fNSequence_lens << "[batch] - 1;\n";
               }
               out << "\t" << "\t" << "size_t offset = seq * " << num_directions * batch_size * fAttrHiddenSize
                   << " + batch * " << fAttrHiddenSize << ";\n";
               out << "\t" << "\t" << "size_t y_h_offset = batch * " << num_directions * fAttrHiddenSize << ";\n";
               out << "\t" << "\t" << "std::copy(" << OpName << "_cell_state + offset, " << OpName
                   << "_cell_state + offset + " << fAttrHiddenSize << ", tensor_" << fNY_c << " + y_h_offset);\n";
               out << "\t" << "}\n";
            }
            if (num_directions == 2) {
               out << "\t" << "for (size_t batch = 0; batch < " << batch_size << "; batch++) {\n";
               out << "\t" << "\t" << "size_t offset = " << batch_size * fAttrHiddenSize << " + batch * "
                   << fAttrHiddenSize << ";\n";
               out << "\t" << "\t" << "size_t y_h_offset = batch * " << num_directions * fAttrHiddenSize << " + "
                   << fAttrHiddenSize << ";\n";
               out << "\t" << "\t" << "std::copy(" << OpName << "_cell_state + offset, " << OpName
                   << "_cell_state + offset + " << fAttrHiddenSize << ", tensor_" << fNY_c << " + y_h_offset);\n";
               out << "\t" << "}\n";
            }
         }
      }

      return out.str();
   }
};

} // namespace SOFIE
} // namespace Experimental
} // namespace TMVA

#endif