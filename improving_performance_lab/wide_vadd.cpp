/**********
Copyright (c) 2019-2020, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/


#include "event_timer.hpp"

#include <iostream>
#include <memory>
#include <string>

// Xilinx OpenCL and XRT includes
#include "xilinx_ocl.hpp"

void vadd_sw(double *a, double *b, double *c, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        c[i] = a[i] + b[i];
    }
}

int main(int argc, char *argv[])
{
    // Initialize an event timer we'll use for monitoring the application
    EventTimer et;
    // Check if the binary file is passed as argument
    if (argc != 2) {
      std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
      return EXIT_FAILURE;
    }
    // Copy binary name
    char* binaryName = argv[1];
    // Get target and set BUFSIZE 1024 times bigger for hw runs
    std::string target = getenv("XCL_EMULATION_MODE");
    const uint32_t BUFSIZE = (target.compare("hw") == 0) ? (1024 * 1024 * 32) : (1024 * 32);

    std::cout << "-- Parallelizing the Data Path --" << std::endl << std::endl;

    // Initialize the runtime (including a command queue) and load the
    // FPGA image
    std::cout << "Loading " << binaryName << " to program the board" << std::endl << std::endl;
    et.add("OpenCL Initialization");

    // This application will use the first Xilinx device found in the system
    swm::XilinxOcl xocl;
    xocl.initialize(binaryName);

    cl::CommandQueue q = xocl.get_command_queue();
    cl::Kernel krnl    = xocl.get_kernel("wide_vadd");
    et.finish();

    /// New code for example 01
    std::cout << "Running kernel test XRT-allocated buffers and wide data path:" << std::endl
              << std::endl;

    // Map our user-allocated buffers as OpenCL buffers using a shared
    // host pointer
    et.add("Allocate contiguous OpenCL buffers");
    cl_mem_ext_ptr_t bank_ext;
    bank_ext.flags = 2 | XCL_MEM_TOPOLOGY;
    bank_ext.obj   = NULL;
    bank_ext.param = 0;
    cl::Buffer a_buf(xocl.get_context(),
                     static_cast<cl_mem_flags>(CL_MEM_READ_ONLY),
                     BUFSIZE * sizeof(double),
                     NULL,
                     NULL);
    cl::Buffer b_buf(xocl.get_context(),
                     static_cast<cl_mem_flags>(CL_MEM_READ_ONLY),
                     BUFSIZE * sizeof(double),
                     NULL,
                     NULL);
    cl::Buffer c_buf(xocl.get_context(),
                     static_cast<cl_mem_flags>(CL_MEM_READ_WRITE),
                     BUFSIZE * sizeof(double),
                     NULL,
                     NULL);
    cl::Buffer d_buf(xocl.get_context(),
                     static_cast<cl_mem_flags>(CL_MEM_READ_WRITE |
                                               CL_MEM_ALLOC_HOST_PTR |
                                               CL_MEM_EXT_PTR_XILINX),
                     BUFSIZE * sizeof(double),
                     &bank_ext,
                     NULL);
    et.finish();

    // Set vadd kernel arguments. We do this before mapping the buffers to allow XRT
    // to allocate the buffers in the appropriate memory banks for the selected
    // kernels. For buffer 'd' we explicitly set a bank above, but this buffer is
    // never migrated to the Alveo card so this mapping is theoretical.
    et.add("Set kernel arguments");
    krnl.setArg(0, a_buf);
    krnl.setArg(1, b_buf);
    krnl.setArg(2, c_buf);
    krnl.setArg(3, BUFSIZE);

    et.add("Map buffers to user space pointers");
    double *a = (double *)q.enqueueMapBuffer(a_buf,
                                                 CL_TRUE,
                                                 CL_MAP_WRITE,
                                                 0,
                                                 BUFSIZE * sizeof(double));
    double *b = (double *)q.enqueueMapBuffer(b_buf,
                                                 CL_TRUE,
                                                 CL_MAP_WRITE,
                                                 0,
                                                 BUFSIZE * sizeof(double));
    double *d = (double *)q.enqueueMapBuffer(d_buf,
                                                 CL_TRUE,
                                                 CL_MAP_WRITE | CL_MAP_READ,
                                                 0,
                                                 BUFSIZE * sizeof(double));
    et.finish();

    et.add("Populating buffer inputs");
    for (uint32_t i = 0; i < BUFSIZE; i++) {
        a[i] = i;
        b[i] = 2 * i;
    }
    et.finish();

    // For comparison, let's have the CPU calculate the result
    et.add("Software VADD run");
    vadd_sw(a, b, d, BUFSIZE);
    et.finish();

    // Send the buffers down to the Alveo card
    et.add("Memory object migration enqueue");
    cl::Event event_sp;
    q.enqueueMigrateMemObjects({a_buf, b_buf}, 0, NULL, &event_sp);
    clWaitForEvents(1, (const cl_event *)&event_sp);

    et.add("OCL Enqueue task");

    q.enqueueTask(krnl, NULL, &event_sp);
    et.add("Wait for kernel to complete");
    clWaitForEvents(1, (const cl_event *)&event_sp);

    // Migrate memory back from device
    et.add("Read back computation results");
    double *c = (double *)q.enqueueMapBuffer(c_buf,
                                                 CL_TRUE,
                                                 CL_MAP_READ,
                                                 0,
                                                 BUFSIZE * sizeof(double));
    et.finish();


    // Verify the results
    bool verified = true;
    for (uint32_t i = 0; i < BUFSIZE; i++) {
        if (c[i] != d[i]) {
            verified = false;
            std::cout << "ERROR: software and hardware vadd do not match: "
                      << c[i] << "!=" << d[i] << " at position " << i << std::endl;
            break;
        }
    }

    if (verified) {
        std::cout
            << std::endl
            << "OCL-mapped contiguous buffer example complete successfully!"
            << std::endl
            << std::endl;
    }
    else {
        std::cout
            << std::endl
            << "OCL-mapped contiguous buffer example complete! (with errors)"
            << std::endl
            << std::endl;
    }

    std::cout << "--------------- Key execution times ---------------" << std::endl;


    q.enqueueUnmapMemObject(a_buf, a);
    q.enqueueUnmapMemObject(b_buf, b);
    q.enqueueUnmapMemObject(c_buf, c);
    q.enqueueUnmapMemObject(d_buf, d);
    q.finish();


    et.print();
}
