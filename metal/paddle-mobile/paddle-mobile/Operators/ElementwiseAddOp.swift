///* Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License. */

import Foundation

class ElementwiseAddParam<P: PrecisionType>: OpParam {
    typealias ParamPrecisionType = P
    required init(opDesc: OpDesc, inScope: Scope) throws {
        do {
            input = try ElementwiseAddParam.inputX(inputs: opDesc.inputs, from: inScope)
            inputY = try ElementwiseAddParam.inputY(inputs: opDesc.paraInputs, from: inScope)
            
            output = try ElementwiseAddParam.outputOut(outputs: opDesc.outputs, from: inScope)
            axis = try ElementwiseAddParam.getAttr(key: "axis", attrs: opDesc.attrs)
        } catch let error {
            throw error
        }
    }
    let input: Texture<P>
    let inputY: Tensor<P>
    var output: Texture<P>
    let axis: Int
}

class ElementwiseAddOp<P: PrecisionType>: Operator<ElementwiseAddKernel<P>, ElementwiseAddParam<P>>, Runable, Creator, InferShaperable{
    
    func inferShape() {
        para.output.dim = para.input.dim
    }
    
    typealias OpType = ElementwiseAddOp<P>
    func runImpl(device: MTLDevice, buffer: MTLCommandBuffer) throws {
    }
}






