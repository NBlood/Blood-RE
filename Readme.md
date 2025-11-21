# Blood-RE
Matching decompilation of various versions Blood (1997) by nukeykt.

## Covered Blood versions
* Shareware 1.10
* Shareware 1.11 (19 August 1997)
* Shareware 1.11 (23 September 1997)
* Retail Shareware 1.10
* Retail Shareware 1.11 (19 August 1997)
* Retail Shareware 1.11 (23 September 1997)
* Registered 1.10
* Registered 1.11 (19 August 1997)
* Registered 1.11 (23 September 1997)
* Registered 1.20
* Registered 1.20 3DFX
* Plasma Pak 1.10
* Plasma Pak 1.11 (19 August 1997)
* Plasma Pak 1.11 (23 September 1997)
* Plasma Pak 1.20
* Plasma Pak 1.20 3DFX
* Plasma Pak 1.21 (One Unit Whole Blood)

## Building

Watcom 10.6 and TASM 3.1 are required to build.

### Build instructions
1) Build helix32 (i.e. `cd helix32` and then `wmake`)
2) Build qtools (i.e `cd qtools`, then `dobuild` and then select revision)
2) Build blood (i.e. `cd blood`, then `dobuild` and then select revision)

For 3DFX versions also build 3DFX files: `cd 3dfx`, then `wmake`

Special thanks to NY00123, Hendricks266, sirlemonhead, tmyqlfpir and Maxi Clouds.

