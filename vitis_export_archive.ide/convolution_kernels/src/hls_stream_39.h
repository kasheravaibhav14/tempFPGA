// 67d7842dbbe25473c3c32b93c0da8047785f30d78e8a024de1b57352245f9689
/* -*- c++ -*-*/
/*
#-  (c) Copyright 2011-2021 Xilinx, Inc. All rights reserved.
#-
#-  This file contains confidential and proprietary information
#-  of Xilinx, Inc. and is protected under U.S. and
#-  international copyright and other intellectual property
#-  laws.
#-
#-  DISCLAIMER
#-  This disclaimer is not a license and does not grant any
#-  rights to the materials distributed herewith. Except as
#-  otherwise provided in a valid license issued to you by
#-  Xilinx, and to the maximum extent permitted by applicable
#-  law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
#-  WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
#-  AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
#-  BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
#-  INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
#-  (2) Xilinx shall not be liable (whether in contract or tort,
#-  including negligence, or under any other theory of
#-  liability) for any loss or damage of any kind or nature
#-  related to, arising under or in connection with these
#-  materials, including for any direct, or any indirect,
#-  special, incidental, or consequential loss or damage
#-  (including loss of data, profits, goodwill, or any type of
#-  loss or damage suffered as a result of any action brought
#-  by a third party) even if such damage or loss was
#-  reasonably foreseeable or Xilinx had been advised of the
#-  possibility of the same.
#-
#-  CRITICAL APPLICATIONS
#-  Xilinx products are not designed or intended to be fail-
#-  safe, or for use in any application requiring fail-safe
#-  performance, such as life-support or safety devices or
#-  systems, Class III medical devices, nuclear facilities,
#-  applications related to the deployment of airbags, or any
#-  other applications that could lead to death, personal
#-  injury, or severe property or environmental damage
#-  (individually and collectively, "Critical
#-  Applications"). Customer assumes the sole risk and
#-  liability of any use of Xilinx products in Critical
#-  Applications, subject only to applicable laws and
#-  regulations governing limitations on product liability.
#-
#-  THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
#-  PART OF THIS FILE AT ALL TIMES. 
#- ************************************************************************

 *
 *
 */

#ifndef X_HLS_STREAM_39_SYN_H
#define X_HLS_STREAM_39_SYN_H

/*
 * This file contains a C++ model of hls::stream.
 * It defines AutoESL synthesis model.
 */
#ifndef __cplusplus
#error C++ is required to include this header file

#else

#include "etc/autopilot_enum.h"
#include "etc/autopilot_ssdm_op.h"

#ifndef _STREAM_UNUSED_PARAM
 #define _STREAM_UNUSED_PARAM(x) (void)(x)
#endif
namespace hls {

#ifndef INLINE
#define INLINE inline __attribute__((always_inline))
#endif
#ifndef NOINLINE
#define NOINLINE inline __attribute__((noinline))
#endif

#ifndef __has_attribute
#define __has_attribute(x) 0  // Compatibility with non-clang compilers.
#endif

#if __has_attribute(no_ctor)
#define NO_CTOR __attribute__((no_ctor))
#else
#define NO_CTOR
#endif

//////////////////////////////////////////////
// Synthesis models for stream
//////////////////////////////////////////////
template<typename __STREAM_T__, int DEPTH=0>
class stream;

template<typename __STREAM_T__>
class stream<__STREAM_T__, 0>
{
  public:
    /// Constructors
    INLINE stream() {
      __fpga_set_stream_depth(&this->V, 0);
    }

    INLINE stream(const char* name) {
      _STREAM_UNUSED_PARAM(name);
      __fpga_set_stream_depth(&this->V, 0);
    }

  /// Make copy constructor and assignment operator private
  private:  
    INLINE stream(const stream< __STREAM_T__ >& chn):V(chn.V) {
    }

    INLINE stream& operator= (const stream< __STREAM_T__ >& chn) {
        V = chn.V;
        return *this;
    }

  public:
    /// Overload >> and << operators to implement read() and write()
    INLINE void operator >> (__STREAM_T__& rdata) {
        read(rdata);
    }

    INLINE void operator << (const __STREAM_T__& wdata) {
        write(wdata);
    }

  /// Public API
  public:
    /// Empty & Full
    INLINE bool empty() const {
        return !__fpga_fifo_not_empty(&V);
    }

    INLINE bool full() const {
        return !__fpga_fifo_not_full(&V);
    }

    /// Blocking read
    INLINE void read(__STREAM_T__& dout) {
        __fpga_fifo_pop(&V, &dout);
    }

    /// Blocking read
    NOINLINE bool read_dep(__STREAM_T__& dout, volatile bool flag) {
        __fpga_fifo_pop(&V, &dout);
        return flag;
    }

    INLINE __STREAM_T__ read() {
        __STREAM_T__ tmp;
        read(tmp);
        return tmp;
    }

    /// Nonblocking read
    INLINE bool read_nb(__STREAM_T__& dout) {
        __STREAM_T__ tmp;

        if (__fpga_fifo_nb_pop(&V, &tmp)) {
            dout = tmp;
            return true;
        } else {
            return false;
        }
    }

    /// Blocking write
    INLINE void write(const __STREAM_T__& din) {
        __fpga_fifo_push(&V, &din);
    }

    /// Blocking write
    NOINLINE bool write_dep(const __STREAM_T__& din, volatile bool flag) {
        __fpga_fifo_push(&V, &din);
        return flag;
    }

    /// Nonblocking write
    INLINE bool write_nb(const __STREAM_T__& din) {
        return __fpga_fifo_nb_push(&V, &din);
    }

    /// Set name for c-sim debugging, ignored in synthesis.
    void set_name(const char* name) { _STREAM_UNUSED_PARAM(name); }

  public:
    __STREAM_T__ V NO_CTOR;
};

template<typename __STREAM_T__, int DEPTH>
class stream : public stream<__STREAM_T__, 0> {
  public:
    INLINE stream() {
      __fpga_set_stream_depth(&this->V, DEPTH);
    }

    INLINE stream(const char* name) {
      _STREAM_UNUSED_PARAM(name);
      __fpga_set_stream_depth(&this->V, DEPTH);
    }
};
} // namespace hls

#endif // __cplusplus
#endif  // X_HLS_STREAM_39_SYN_H


