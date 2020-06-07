::
:: Copyright 2014 Dario Manesku. All rights reserved.
:: License: http://www.opensource.org/licenses/BSD-2-Clause
::

@echo off
SET cmft="./../_build/win64_vs2012/bin/cmftRelease.exe"

:: Prints help.
REM %cmft% --help

:: Use this to list available OpenCL devices that can be used with cmft for processing.
::%cmft% --printCLDevices

::Typical parameters for irradiance filter.
REM %cmft% %* --input "okretnica.tga"           ^
REM           --filter irradiance               ^
REM           --srcFaceSize 0                   ^
REM           --dstFaceSize 0                   ^
REM           --outputNum 1                     ^
REM           --output0 "okretnica_irr"         ^
REM           --output0params dds,bgra8,cubemap

:: Typical parameters for generating spherical harmonics coefficients.
REM %cmft% %* --input "okretnica.tga"       ^
REM           --filter shcoeffs             ^
REM           --outputNum 1                 ^
REM           --output0 "okretnica"

:: Typical parameters for radiance filter.
%cmft% %* --input "okretnica.tga"           ^
          ::Filter options                  ^
          --filter radiance                 ^
          --srcFaceSize 256                 ^
          --excludeBase false               ^
          --mipCount 9                      ^
          --glossScale 10                   ^
          --glossBias 1                     ^
          --lightingModel phongbrdf         ^
          --edgeFixup none                  ^
          --dstFaceSize 256                 ^
          ::Processing devices              ^
          --numCpuProcessingThreads 4       ^
          --useOpenCL true                  ^
          --clVendor anyGpuVendor           ^
          --deviceType gpu                  ^
          --deviceIndex 0                   ^
          ::Aditional operations            ^
          --inputGammaNumerator 2.2         ^
          --inputGammaDenominator 1.0       ^
          --outputGammaNumerator 1.0        ^
          --outputGammaDenominator 2.2      ^
          --generateMipChain false          ^
          ::Output                          ^
          --outputNum 2                     ^
          --output0 "okretnica_pmrem"       ^
          --output0params dds,bgra8,cubemap ^
          --output1 "okretnica_pmrem"       ^
          --output1params ktx,rgba8,cubemap

:: Cmft can also be run without any processing filter. This can be used for performing image manipulations or exporting different image format.
REM %cmft% %* --input "okretnica.tga"           ^
REM           --filter none                     ^
REM           ::Aditional operations            ^
REM           --inputGamma 1.0                  ^
REM           --inputGammaDenominator 1.0       ^
REM           --outputGamma 1.0                 ^
REM           --outputGammaDenominator 1.0      ^
REM           --generateMipChain true           ^
REM           ::Cubemap transformations         ^
REM           --posXrotate90                    ^
REM           --posXrotate180                   ^
REM           --posXrotate270                   ^
REM           --posXflipH                       ^
REM           --posXflipV                       ^
REM           --negXrotate90                    ^
REM           --negXrotate180                   ^
REM           --negXrotate270                   ^
REM           --negXflipH                       ^
REM           --negXflipV                       ^
REM           --posYrotate90                    ^
REM           --posYrotate180                   ^
REM           --posYrotate270                   ^
REM           --posYflipH                       ^
REM           --posYflipV                       ^
REM           --negYrotate90                    ^
REM           --negYrotate180                   ^
REM           --negYrotate270                   ^
REM           --negYflipH                       ^
REM           --negYflipV                       ^
REM           --posZrotate90                    ^
REM           --posZrotate180                   ^
REM           --posZrotate270                   ^
REM           --posZflipH                       ^
REM           --posZflipV                       ^
REM           --negZrotate90                    ^
REM           --negZrotate180                   ^
REM           --negZrotate270                   ^
REM           --negZflipH                       ^
REM           --negZflipV                       ^
REM           ::Output                          ^
REM           --outputNum 1                     ^
REM           --output0 "okretnica_dds"         ^
REM           --output0params dds,bgra8,cubemap ^

:: Cmft with all parameters listed. This is mainly to have a look at what is all possible.
REM %cmft% %* --inputFacePosX "okretnica_posx.tga" ^
REM           --inputFaceNegX "okretnica_negx.tga" ^
REM           --inputFacePosY "okretnica_posy.tga" ^
REM           --inputFaceNegY "okretnica_negy.tga" ^
REM           --inputFacePosZ "okretnica_posz.tga" ^
REM           --inputFaceNegZ "okretnica_negz.tga" ^
REM           ::Filter options              ^
REM           --filter radiance             ^
REM           --srcFaceSize 256             ^
REM           --excludeBase false           ^
REM           --mipCount 9                  ^
REM           --glossScale 10               ^
REM           --glossBias 1                 ^
REM           --lightingModel phongbrdf     ^
REM           --edgeFixup none              ^
REM           --dstFaceSize 256             ^
REM           ::Processing devices          ^
REM           --numCpuProcessingThreads 4   ^
REM           --useOpenCL true              ^
REM           --clVendor anyGpuVendor       ^
REM           --deviceType gpu              ^
REM           --deviceIndex 0               ^
REM           ::Aditional operations        ^
REM           --inputGamma 1.0              ^
REM           --inputGammaDenominator 1.0   ^
REM           --outputGamma 1.0             ^
REM           --outputGammaDenominator 1.0  ^
REM           --generateMipChain false      ^
REM           ::Cubemap transformations     ^
REM           --posXrotate90                ^
REM           --posXrotate180               ^
REM           --posXrotate270               ^
REM           --posXflipH                   ^
REM           --posXflipV                   ^
REM           --negXrotate90                ^
REM           --negXrotate180               ^
REM           --negXrotate270               ^
REM           --negXflipH                   ^
REM           --negXflipV                   ^
REM           --posYrotate90                ^
REM           --posYrotate180               ^
REM           --posYrotate270               ^
REM           --posYflipH                   ^
REM           --posYflipV                   ^
REM           --negYrotate90                ^
REM           --negYrotate180               ^
REM           --negYrotate270               ^
REM           --negYflipH                   ^
REM           --negYflipV                   ^
REM           --posZrotate90                ^
REM           --posZrotate180               ^
REM           --posZrotate270               ^
REM           --posZflipH                   ^
REM           --posZflipV                   ^
REM           --negZrotate90                ^
REM           --negZrotate180               ^
REM           --negZrotate270               ^
REM           --negZflipH                   ^
REM           --negZflipV                   ^
REM           ::Output                      ^
REM           --outputNum 5                 ^
REM           --output0 "cmft_cubemap"    --output0params dds,bgra8,cubemap  ^
REM           --output1 "cmft_hstrip"     --output1params dds,bgra8,hstrip   ^
REM           --output2 "cmft_cubecross"  --output2params ktx,rgba32f,hcross ^
REM           --output3 "cmft_facelist"   --output3params tga,bgra8,facelist ^
REM           --output4 "cmft_latlong"    --output4params hdr,rgbe,latlong   ^
REM           ::Misc                        ^
REM           --silent
