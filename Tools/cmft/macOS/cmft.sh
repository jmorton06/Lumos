#!/bin/bash

#
# Copyright 2014-2015 Dario Manesku. All rights reserved.
# License: http://www.opensource.org/licenses/BSD-2-Clause
#

CMFT=./cmft

# Prints help.
#eval $CMFT --help

# Use this to list available OpenCL devices that can be used with cmft for processing.
#eval $CMFT --printCLDevices

# Typical parameters for irradiance filter.
#eval $CMFT $@ --input "okretnica.tga"           \
#              --filter irradiance               \
#              --srcFaceSize 0                   \
#              --dstFaceSize 0                   \
#              --outputNum 1                     \
#              --output0 "okretnica_irr"         \
#              --output0params dds,bgra8,cubemap

# Typical parameters for generating spherical harmonics coefficients.
#eval $CMFT $@ --input "okretnica.tga"           \
#              --filter shcoeffs                 \
#              --outputNum 1                     \
#              --output0 "okretnica"

# Typical parameters for radiance filter.

#              ::Aditional operations            \
#--inputGamma 2.2                  \
#--inputGammaDenominator 2.2       \
#--outputGamma 1.0                \
#--outputGammaDenominator 1.0      \
#              --srcFaceSize 512                 \

eval $CMFT $@ --input "noga_4k.hdr" \
              ::Filter options                  \
              --filter Radiance                 \
              --excludeBase true               \
              --edgeFixup none                  \
              --mipCount 7                     \
              --dstFaceSize 1024                 \
              --glossScale 10                   \
              --glossBias 2                     \
              --lightingModel blinnbrdf         \
              --generateMipChain true           \
            ::Processing devices          \
            --numCpuProcessingThreads 8   \
            --useOpenCL false              \
            --clVendor anyGpuVendor       \
            --deviceType gpu              \
            --deviceIndex 0               \
              ::Aditional operations            \
              --inputGammaNumerator 1.0         \
              --inputGammaDenominator 2.2       \
              --outputGammaNumerator 1.0        \
              --outputGammaDenominator 2.2      \
              ::Output                          \
              --outputNum 1                     \
              --output0 "noga_Env"\
              --output0params tga,bgr8,VCross   \
              
eval $CMFT $@ --input "noga_4k.hdr" \
            ::Filter options                  \
            --filter irradiance               \
            --edgeFixup none                  \
            --excludeBase true               \
            --dstFaceSize 32                 \
            --generateMipChain true           \
            --glossScale 10                   \
            --glossBias 2                     \
            ::Processing devices          \
            --numCpuProcessingThreads 8   \
            --useOpenCL false              \
            --clVendor anyGpuVendor       \
            --deviceType gpu              \
            --deviceIndex 0               \
            ::Aditional operations            \
            --inputGammaNumerator 1.0         \
            --inputGammaDenominator 2.2       \
            --outputGammaNumerator 1.0        \
            --outputGammaDenominator 2.2      \
            ::Output                          \
            --outputNum 1                     \
            --output0 "noga_Irr"\
            --output0params tga,bgr8,VCross   \


#eval $CMFT $@ --input "Arches_E_PineTree_3k.hdr" \
#              ::Filter options                  \
#              --filter none                     \
#              --mipCount 10                      \
#              --edgeFixup none                  \
#              --dstFaceSize 512                 \
#              --excludeBase false               \
#              --glossScale 10                   \
#              --glossBias 1                     \
#              --lightingModel blinnbrdf         \
#              --edgeFixup none                  \
#              ::Aditional operations            \
#              --inputGammaNumerator 1.0         \
#              --inputGammaDenominator 2.2       \
#              --outputGammaNumerator 1.0        \
#              --outputGammaDenominator 2.0      \
#              --generateMipChain true          \
#              ::Output                          \
#              --outputNum 1                     \
#              --output0 "Arches_E_PineTree_Env"\
#              --output0params tga,bgra8,VCross  \
#
#eval $CMFT $@ --input "Arches_E_PineTree_3k.hdr" \
#            ::Filter options                  \
#            --filter irradiance                     \
#            --mipCount 10                      \
#            --edgeFixup none                  \
#            --dstFaceSize 512                 \
#            --excludeBase false               \
#            --glossScale 10                   \
#            --glossBias 1                     \
#            --lightingModel blinnbrdf         \
#            --edgeFixup none                  \
#            ::Aditional operations            \
#            --inputGammaNumerator 1.0         \
#            --inputGammaDenominator 2.2       \
#            --outputGammaNumerator 1.0        \
#            --outputGammaDenominator 2.0      \
#            --generateMipChain true          \
#            ::Output                          \
#            --outputNum 1                     \
#            --output0 "Arches_E_PineTree_Irr"\
#            --output0params tga,bgra8,VCross  \
            
# Cmft can also be run without any processing filter. This can be used for performing image manipulations or exporting different image format.
#eval $CMFT $@ --input "okretnica.tga"           \
#              --filter none                     \
#              ::Aditional operations            \
#              --inputGamma 1.0                  \
#              --inputGammaDenominator 1.0       \
#              --outputGamma 1.0                 \
#              --outputGammaDenominator 1.0      \
#              --generateMipChain true           \
#              ::Cubemap transformations         \
#              --posXrotate90                    \
#              --posXrotate180                   \
#              --posXrotate270                   \
#              --posXflipH                       \
#              --posXflipV                       \
#              --negXrotate90                    \
#              --negXrotate180                   \
#              --negXrotate270                   \
#              --negXflipH                       \
#              --negXflipV                       \
#              --posYrotate90                    \
#              --posYrotate180                   \
#              --posYrotate270                   \
#              --posYflipH                       \
#              --posYflipV                       \
#              --negYrotate90                    \
#              --negYrotate180                   \
#              --negYrotate270                   \
#              --negYflipH                       \
#              --negYflipV                       \
#              --posZrotate90                    \
#              --posZrotate180                   \
#              --posZrotate270                   \
#              --posZflipH                       \
#              --posZflipV                       \
#              --negZrotate90                    \
#              --negZrotate180                   \
#              --negZrotate270                   \
#              --negZflipH                       \
#              --negZflipV                       \
#              ::Output                          \
#              --outputNum 1                     \
#              --output0 "okretnica_dds"         \
#              --output0params dds,bgra8,cubemap \

# Cmft with all parameters listed. This is mainly to have a look at what is all possible.
#eval $CMFT $@ --input "okretnica.tga"       \
#              ::Filter options              \
#              --filter radiance             \
#              --srcFaceSize 256             \
#              --excludeBase false           \
#              --mipCount 7                  \
#              --glossScale 10               \
#              --glossBias 3                 \
#              --lightingModel blinnbrdf     \
#              --edgeFixup none              \
#              --dstFaceSize 256             \
#              ::Processing devices          \
#              --numCpuProcessingThreads 4   \
#              --useOpenCL true              \
#              --clVendor anyGpuVendor       \
#              --deviceType gpu              \
#              --deviceIndex 0               \
#              ::Aditional operations        \
#              --inputGamma 1.0              \
#              --inputGammaDenominator 1.0   \
#              --outputGamma 1.0             \
#              --outputGammaDenominator 1.0  \
#              --generateMipChain false      \
#              ::Cubemap transformations     \
#              --posXrotate90                \
#              --posXrotate180               \
#              --posXrotate270               \
#              --posXflipH                   \
#              --posXflipV                   \
#              --negXrotate90                \
#              --negXrotate180               \
#              --negXrotate270               \
#              --negXflipH                   \
#              --negXflipV                   \
#              --posYrotate90                \
#              --posYrotate180               \
#              --posYrotate270               \
#              --posYflipH                   \
#              --posYflipV                   \
#              --negYrotate90                \
#              --negYrotate180               \
#              --negYrotate270               \
#              --negYflipH                   \
#              --negYflipV                   \
#              --posZrotate90                \
#              --posZrotate180               \
#              --posZrotate270               \
#              --posZflipH                   \
#              --posZflipV                   \
#              --negZrotate90                \
#              --negZrotate180               \
#              --negZrotate270               \
#              --negZflipH                   \
#              --negZflipV                   \
#              ::Output                      \
#              --outputNum 5                 \
#              --output0 "cmft_cubemap"    --output0params dds,bgra8,cubemap  \
#              --output1 "cmft_hstrip"     --output1params dds,bgra8,hstrip   \
#              --output2 "cmft_cubecross"  --output2params ktx,rgba32f,hcross \
#              --output3 "cmft_facelist"   --output3params tga,bgra8,facelist \
#              --output4 "cmft_latlong"    --output4params hdr,rgbe,latlong
