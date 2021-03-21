import torch
from torch import nn
from torch.nn import Conv2d

model = Conv2d(1, 1,kernel_size = (3, 3), padding = 1, stride = 1)

# set the weights
model.weight.data = torch.ones(9, dtype=torch.float32).reshape((1, 1, 3, 3))
# set the bias
model.bias.data.fill_(0.)
# set the model to inference mode
model.eval()
# input
x = torch.randn(1, 1, 5, 5, requires_grad=True, dtype=torch.float32)
# output
y = model(x)
# export the model
torch.onnx.export(
    model,
    x,
    'Conv.onnx',
    export_params = True,
    opset_version=10,
    input_names=['x'],
    output_names=['y']
)
