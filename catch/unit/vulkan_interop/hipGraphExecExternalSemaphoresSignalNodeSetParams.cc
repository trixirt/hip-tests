/*
Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "vulkan_test.hh"
#include "signal_semaphore_common.hh"

/**
 * @addtogroup hipGraphExecExternalSemaphoresSignalNodeSetParams
 * hipGraphExecExternalSemaphoresSignalNodeSetParams
 * @{
 * @ingroup GraphTest
 * `hipGraphExecExternalSemaphoresSignalNodeSetParams(hipGraphExec_t hGraphExec, hipGraphNode_t
 * hNode, const hipExternalSemaphoreSignalNodeParams* nodeParams)` - Updates node parameters in the
 * external semaphore signal node in the given graphExec.
 */

static hipError_t GraphExecSemaphoreSetParamsSignalWrapper(
    hipExternalSemaphore_t* extSemArray, hipExternalSemaphoreSignalParams* paramsArray,
    unsigned int numExtSems, hipStream_t stream) {
  hipGraph_t graph = nullptr;
  HIP_CHECK(hipGraphCreate(&graph, 0));
  hipGraphNode_t node = nullptr;

  hipExternalSemaphoreSignalNodeParams node_params = {};
  node_params.extSemArray = extSemArray;
  node_params.paramsArray = paramsArray;
  node_params.numExtSems = numExtSems;

  hipExternalSemaphoreSignalParams* signal_params =
      new hipExternalSemaphoreSignalParams[numExtSems];
  for (unsigned int i = 0; i < numExtSems; i++) {
    signal_params[i].params.fence.value = 10 + i;
  }

  hipExternalSemaphoreSignalNodeParams initial_params = {};
  initial_params.extSemArray = extSemArray;
  initial_params.paramsArray = signal_params;
  initial_params.numExtSems = numExtSems;

  HIP_CHECK(hipGraphAddExternalSemaphoresSignalNode(&node, graph, nullptr, 0, &initial_params));

  hipGraphExec_t graph_exec = nullptr;
  HIP_CHECK(hipGraphInstantiate(&graph_exec, graph, nullptr, nullptr, 0));

  HIP_CHECK(hipGraphExecExternalSemaphoresSignalNodeSetParams(graph_exec, node, &node_params));

  hipExternalSemaphoreSignalNodeParams retrieved_params{};
  memset(&retrieved_params, 0, sizeof(hipExternalSemaphoreSignalNodeParams));
  HIP_CHECK(hipGraphExternalSemaphoresSignalNodeGetParams(node, &retrieved_params));
  REQUIRE(initial_params == retrieved_params);

  HIP_CHECK(hipGraphLaunch(graph_exec, stream));
  HIP_CHECK(hipStreamSynchronize(stream));

  HIP_CHECK(hipGraphExecDestroy(graph_exec));
  HIP_CHECK(hipGraphDestroy(graph));
  delete[] signal_params;

  return hipSuccess;
}

/**
 * Test Description
 * ------------------------
 *    - Verify that node parameters get updated correctly by creating a node with valid but
 * incorrect parameters, and then setting them to the correct values in the executable graph. The
 * graph is run and it is verified that the graph node signals the external binary semaphore and
 * operation finishes successfully.
 * Test source
 * ------------------------
 *    - unit/vulkan_interop/hipGraphExecExternalSemaphoresSignalNodeSetParams.cc
 * Test requirements
 * ------------------------
 *    - HIP_VERSION >= 6.0
 */
TEST_CASE("Unit_hipGraphExecExternalSemaphoresSignalNodeSetParams_Positive_Basic") {
  SignalExternalSemaphoreCommon(GraphExecSemaphoreSetParamsSignalWrapper);
}

// Timeline semaphores unsupported on AMD
#if HT_NVIDIA

/**
 * Test Description
 * ------------------------
 *    - Verify that node parameters get updated correctly by creating a node with valid but
 * incorrect parameters, and then setting them to the correct values in the executable graph. The
 * graph is run and it is verified that the graph node signals the external timeline semaphore and
 * operation finishes successfully.
 * Test source
 * ------------------------
 *    - unit/vulkan_interop/hipGraphExecExternalSemaphoresSignalNodeSetParams.cc
 * Test requirements
 * ------------------------
 *    - HIP_VERSION >= 6.0
 */
TEST_CASE(
    "Unit_hipGraphExecExternalSemaphoresSignalNodeSetParams_Vulkan_Positive_Timeline_Semaphore") {
  SignalExternalTimelineSemaphoreCommon(GraphExecSemaphoreSetParamsSignalWrapper);
}

/**
 * Test Description
 * ------------------------
 *    - Verify that node parameters get updated correctly by creating a node with valid but
 * incorrect parameters, and then setting them to the correct values in the executable graph. The
 * graph is run and it is verified that the graph node signals the external binary semaphores and
 * operation finishes successfully.
 * Test source
 * ------------------------
 *    - unit/vulkan_interop/hipGraphExecExternalSemaphoresSignalNodeSetParams.cc
 * Test requirements
 * ------------------------
 *    - HIP_VERSION >= 6.0
 */
TEST_CASE(
    "Unit_hipGraphExecExternalSemaphoresSignalNodeSetParams_Vulkan_Positive_Multiple_Semaphores") {
  SignalExternalMultipleSemaphoresCommon(GraphExecSemaphoreSetParamsSignalWrapper);
}
#endif

/**
 * Test Description
 * ------------------------
 *  - Test to verify hipGraphExecExternalSemaphoresSignalNodeSetParams behavior with invalid
 * arguments:
 *    -# Nullptr graphexec
 *    -# Nullptr graph node
 *    -# Nullptr params
 * Test source
 * ------------------------
 *  - /unit/vulkan_interop/hipGraphExecExternalSemaphoresSignalNodeSetParams.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 6.0
 */
TEST_CASE("Unit_hipGraphExecExternalSemaphoresSignalNodeSetParams_Vulkan_Negative_Parameters") {
  hipGraph_t graph = nullptr;
  HIP_CHECK(hipGraphCreate(&graph, 0));

  VulkanTest vkt(enable_validation);
  hipExternalSemaphoreSignalParams signal_params = {};
  signal_params.params.fence.value = 1;
  auto hip_ext_semaphore = ImportBinarySemaphore(vkt);

  hipExternalSemaphoreSignalNodeParams node_params = {};
  node_params.extSemArray = &hip_ext_semaphore;
  node_params.paramsArray = &signal_params;
  node_params.numExtSems = 1;

  hipGraphNode_t node = nullptr;
  HIP_CHECK(hipGraphAddExternalSemaphoresSignalNode(&node, graph, nullptr, 0, &node_params));

  hipGraphExec_t graph_exec = nullptr;
  HIP_CHECK(hipGraphInstantiate(&graph_exec, graph, nullptr, nullptr, 0));

  SECTION("pGraphExec == nullptr") {
    HIP_CHECK_ERROR(hipGraphExecExternalSemaphoresSignalNodeSetParams(nullptr, node, &node_params),
                    hipErrorInvalidValue);
  }

  SECTION("node == nullptr") {
    HIP_CHECK_ERROR(
        hipGraphExecExternalSemaphoresSignalNodeSetParams(graph_exec, nullptr, &node_params),
        hipErrorInvalidValue);
  }

  SECTION("params == nullptr") {
    HIP_CHECK_ERROR(hipGraphExecExternalSemaphoresSignalNodeSetParams(graph_exec, node, nullptr),
                    hipErrorInvalidValue);
  }

  HIP_CHECK(hipDestroyExternalSemaphore(hip_ext_semaphore));
  HIP_CHECK(hipGraphExecDestroy(graph_exec));
  HIP_CHECK(hipGraphDestroy(graph));
}
