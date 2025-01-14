/*
Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANNTY OF ANY KIND, EXPRESS OR
IMPLIED, INNCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANNY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER INN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR INN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/*
This testcase verifies the Basic scenario
1. H2D-KernelLaunch-D2H then D2D-KernelLaunch-D2H in peer GPU
*/

#include <hip_test_common.hh>
#include <hip_test_kernels.hh>
#include <hip_test_checkers.hh>

static constexpr auto NUM_ELM{1024};

/*
This testcase verifies hipMemcpyDtoDAsync API
1.Initializes device variables
2.Launches kernel and performs the sum of device variables
3.Copies the result to host variable and validates the result.
4.Sets the peer device
5.D2D copy from GPU-0 to GPU-1
6.Kernel Launch
7.DtoH copy and validating the result
*/

TEMPLATE_TEST_CASE("Unit_hipMemcpyDtoDAsync_Basic", "",
                   int, float, double) {
  size_t Nbytes = NUM_ELM * sizeof(TestType);
  int numDevices = 0;
  TestType *A_d{nullptr}, *B_d{nullptr}, *C_d{nullptr},
           *X_d{nullptr}, *Y_d{nullptr}, *Z_d{nullptr};
  TestType *A_h{nullptr}, *B_h{nullptr}, *C_h{nullptr};
  hipStream_t stream;

  HIP_CHECK(hipGetDeviceCount(&numDevices));
  if (numDevices > 1) {
    int canAccessPeer = 0;
    HIP_CHECK(hipDeviceCanAccessPeer(&canAccessPeer, 0, 1));
    if (canAccessPeer) {
      HIP_CHECK(hipSetDevice(0));
      HipTest::initArrays<TestType>(&A_d, &B_d, &C_d, &A_h, &B_h, &C_h,
                                    NUM_ELM, false);
      HIP_CHECK(hipSetDevice(1));
      HIP_CHECK(hipMalloc(&X_d, Nbytes));
      HIP_CHECK(hipMalloc(&Y_d, Nbytes));
      HIP_CHECK(hipMalloc(&Z_d, Nbytes));

      HIP_CHECK(hipSetDevice(0));
      HIP_CHECK(hipMemcpy(A_d, A_h, Nbytes, hipMemcpyHostToDevice));
      HIP_CHECK(hipMemcpy(B_d, B_h, Nbytes, hipMemcpyHostToDevice));
      hipLaunchKernelGGL(HipTest::vectorADD, dim3(1),
                         dim3(1), 0, 0,
                         static_cast<const TestType *>(A_d),
                         static_cast<const TestType *>(B_d), C_d, NUM_ELM);
      HIP_CHECK(hipGetLastError());
      HIP_CHECK(hipMemcpy(C_h, C_d, Nbytes, hipMemcpyDeviceToHost));
      HIP_CHECK(hipDeviceSynchronize());
      HipTest::checkVectorADD<TestType>(A_h, B_h, C_h, NUM_ELM);

      HIP_CHECK(hipSetDevice(1));
      HIP_CHECK(hipStreamCreate(&stream));
      HIP_CHECK(hipMemcpyDtoDAsync((hipDeviceptr_t)X_d, (hipDeviceptr_t)A_d,
                                   Nbytes, stream));
      HIP_CHECK(hipMemcpyDtoDAsync((hipDeviceptr_t)Y_d, (hipDeviceptr_t)B_d,
                                   Nbytes, stream));
      HIP_CHECK(hipStreamSynchronize(stream));

      hipLaunchKernelGGL(HipTest::vectorADD, dim3(1),
                         dim3(1), 0, 0,
                         static_cast<const TestType*>(X_d),
                         static_cast<const TestType*>(Y_d), Z_d, NUM_ELM);
      HIP_CHECK(hipGetLastError());
      HIP_CHECK(hipMemcpyDtoHAsync(C_h, (hipDeviceptr_t)Z_d, Nbytes, stream));
      HIP_CHECK(hipStreamSynchronize(stream));
      HIP_CHECK(hipDeviceSynchronize());
      HipTest::checkVectorADD<TestType>(A_h, B_h, C_h, NUM_ELM);

      HipTest::freeArrays<TestType>(A_d, B_d, C_d, A_h, B_h, C_h, false);
      HIP_CHECK(hipFree(X_d));
      HIP_CHECK(hipFree(Y_d));
      HIP_CHECK(hipFree(Z_d));
    } else {
      SUCCEED("Machine does not seem to have P2P Capabilities");
    }
  }
}

/**
* Test Description
* ------------------------
*  - Basic functional testcase to trigger capturehipMemcpyDtoDAsync internal api
*  to improve code coverage.
* Test source
* ------------------------
*  - unit/memory/hipMemcpyDtoDAsync.cc
* Test requirements
* ------------------------
*  - HIP_VERSION >= 6.0
*/
TEST_CASE("Unit_hipMemcpyDtoDAsync_capturehipMemcpyDtoDAsync") {
  int numDevices = 0;
  HIP_CHECK(hipGetDeviceCount(&numDevices));
  if (numDevices > 1) {
    int canAccessPeer = 0;
    HIP_CHECK(hipDeviceCanAccessPeer(&canAccessPeer, 0, 1));
    if (canAccessPeer == 1) {
      hipGraph_t graph{nullptr};
      hipGraphExec_t graphExec{nullptr};
      hipStream_t stream;
      size_t Nbytes = NUM_ELM * sizeof(int);
      HIP_CHECK(hipStreamCreate(&stream));
      int *A_h = reinterpret_cast<int*>(malloc(Nbytes));
      int *B_h = reinterpret_cast<int*>(malloc(Nbytes));
      int *A_d, *B_d;
      for (int i = 0; i < NUM_ELM; i++) {
        A_h[i] = i;
      }
      HIP_CHECK(hipSetDevice(0));
      HIP_CHECK(hipMalloc(reinterpret_cast<void**>(&A_d), Nbytes));
      HIP_CHECK(hipMemcpy(A_d, A_h, Nbytes, hipMemcpyHostToDevice));

      HIP_CHECK(hipSetDevice(1));
      HIP_CHECK(hipMalloc(reinterpret_cast<void**>(&B_d), Nbytes));

      // Start Capturing
      HIP_CHECK(hipStreamBeginCapture(stream, hipStreamCaptureModeGlobal));
      HIP_CHECK(hipMemcpyDtoDAsync((hipDeviceptr_t)B_d, (hipDeviceptr_t)A_d, Nbytes, stream));
      // End Capture
      HIP_CHECK(hipStreamEndCapture(stream, &graph));

      // Create and Launch Executable Graphs
      HIP_CHECK(hipGraphInstantiate(&graphExec, graph, nullptr, nullptr, 0));
      HIP_CHECK(hipGraphLaunch(graphExec, stream));
      HIP_CHECK(hipStreamSynchronize(stream));

      HIP_CHECK(hipMemcpyDtoH(B_h, (hipDeviceptr_t)B_d, Nbytes));
      for (int i = 0; i < NUM_ELM; i++) {
        REQUIRE(B_h[i] == A_h[i]);
      }
      HIP_CHECK(hipGraphExecDestroy(graphExec))
      HIP_CHECK(hipGraphDestroy(graph));
      HIP_CHECK(hipStreamDestroy(stream));
      free(A_h);
      free(B_h);
      HIP_CHECK(hipFree(A_d));
      HIP_CHECK(hipFree(B_d));
    } else {
      SUCCEED("Machine doesnt have P2P support enabled hence skipping test");
    }
  } else {
    SUCCEED("Machine doesnt have multiple gpus hence skipping test");
  }
}
