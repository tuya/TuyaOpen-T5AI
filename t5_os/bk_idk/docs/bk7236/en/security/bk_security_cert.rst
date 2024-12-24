Security Certification
=======================

:link_to_translation:`zh_CN:[中文]`

.. important::

  BK7236 is currently undergoing PSA-L2/SESIP-L2 safety certification...

PSA-L2 Certification Overview
-----------------------------------------

PSA-L2 is the most authoritative security certification standard in the IoT field. Devices certified by PSA-L2 have the following security functions:

  - ``Security ID`` - Each device can be uniquely identified.
  - ``Security Lifecycle`` - The device has a complete lifecycle management.
  - ``authentication`` - the device can prove to a third party that it is authentic and cannot be cloned.
  - ``Isolation`` - The hardware layer supports the isolation of the secure and non-secure worlds.
  - ``Secure Boot`` - Ensures that legitimate programs are loaded and run.
  - ``Safe Upgrade`` - Ensures that the upgrade process is safe.
  - ``Anti-Rollback`` - Prevents legitimate, but older versions of software from starting.
  - ``Safe Interface`` - Ensure that the interface provided by the secure world is safe, and will not cause security problems because the non-secure world calls the interface.
  - ``Security Services`` - Provide hardware security configuration/deployment methods to ensure that other security functions are implemented.

You can refer to `PSA <https://www.arm.com/zh-TW/architecture/security-features/platform-security>`_ for information about PSA-L2 and SESIP-L2.
