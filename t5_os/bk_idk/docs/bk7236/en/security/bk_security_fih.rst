.. _fault_injection_attack:

Protection Against FIA
======================================================

:link_to_translation:`zh_CN:[中文]`

Overview
--------------------------

Fault Injection Attack (FIA) is an active side channel attack method. Common means of fault injection include voltage or clock glitches, electromagnetic radiation,
Physical probing, fault injection into devices using laser beams, etc. Through fault injection, an attacker can interfere with bit flips in memory,
Thereby destroying program execution, such as skipping related instructions, changing judgment results, and so on.

BK7236 provides both soft and hard protection for core security components.

Software Protection Against FIA
---------------------------------------------------------

BK7236 software prevents fault injection attacks mainly by paying attention to branch statements in BL1/BL2/TFM-S to increase random delay,
Use complex bool variables, use random memcpy/memcmp and other mechanisms to achieve.

Hardware Protection Against FIA
----------------------------------------------------

The BK7236 hardware anti-injection attack is mainly implemented by using hardware comparison at the key judgment statement of BL1. For example,
Write the first 16bits of the HASH value of the secure boot public key into the hardware comparison register DST, and the BL1 boot process will actually
The first 16 bits of the calculated public key HASH are written into the hardware comparison register SRC, and before BL1 jumps to BL2, the hardware will
Compare SRC/DST and abort startup if inconsistent. In this way, if an attacker HASHs the "public key" through fault injection
If the verification result is false" and changed to "true", the hardware can still catch this error before jumping to BL2, thus
Abort Secure Boot.
