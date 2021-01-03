# [1.2.0](https://github.com/rickbassham/indi-celestron-cgx/compare/v1.1.0...v1.2.0) (2021-01-03)


### Features

* auto add debs to repo ([0912598](https://github.com/rickbassham/indi-celestron-cgx/commit/0912598a4c3a1a26c1d5652832dadb6f4c8cfc0f))

# [1.1.0](https://github.com/rickbassham/indi-celestron-cgx/compare/v1.0.3...v1.1.0) (2021-01-02)


### Bug Fixes

* fix parking to use stepper counts ([a6fa6ea](https://github.com/rickbassham/indi-celestron-cgx/commit/a6fa6ea1f9e565fdeb00b41464789bd4eea82fd1))
* more code cleanup ([3c2b251](https://github.com/rickbassham/indi-celestron-cgx/commit/3c2b251460c8f6779b109bd563e9d4f6d894dcf1))
* refactor mount commands into a separate class ([084d91c](https://github.com/rickbassham/indi-celestron-cgx/commit/084d91cc4c07b890c452672604f81b75e4b5db17))
* update logging and handle parked mount better ([fe1e3e8](https://github.com/rickbassham/indi-celestron-cgx/commit/fe1e3e84695fcfb7b0b0c1e754cf15ac60e9b864))
* use IDLog for logging instead of directly to stderr ([36077d8](https://github.com/rickbassham/indi-celestron-cgx/commit/36077d8b15c63522f4b4eed978a687ab2ecb89e3))


### Features

* add alignment subsystem to driver; once 2 sync points have been added ([cf5e07d](https://github.com/rickbassham/indi-celestron-cgx/commit/cf5e07dff71790fbeb371063b379b4b93747ace1))
* add switch to force location update in alignment subsystem ([5fdba79](https://github.com/rickbassham/indi-celestron-cgx/commit/5fdba7973b9b98d7eaaa3dfae1cf83d7e61a20bf))
* approach the final slew slowly ([218c633](https://github.com/rickbassham/indi-celestron-cgx/commit/218c6335612399d1f748a3474961ca8945255e99))
* finish adding alignment subsystem to driver ([7c41ee4](https://github.com/rickbassham/indi-celestron-cgx/commit/7c41ee48347da6a1ec49a5f532c45a5678c37d0f))
* improve close goto ([071c6e9](https://github.com/rickbassham/indi-celestron-cgx/commit/071c6e9e00e053f83c30720da405a7f6e41ea74b))

## [1.0.3](https://github.com/rickbassham/indi-celestron-cgx/compare/v1.0.2...v1.0.3) (2021-01-02)


### Bug Fixes

* fix deb name in raspbian build ([623e02b](https://github.com/rickbassham/indi-celestron-cgx/commit/623e02b272782756c071fe8bf86a1989bc5662f7))
* fix deb name in ubuntu arm64 build ([39ed092](https://github.com/rickbassham/indi-celestron-cgx/commit/39ed0924b7b0bc2610355e26b499b24326957b1e))

## [1.0.2](https://github.com/rickbassham/indi-celestron-cgx/compare/v1.0.1...v1.0.2) (2021-01-02)


### Bug Fixes

* add devscripts to release ([ccbceff](https://github.com/rickbassham/indi-celestron-cgx/commit/ccbcefff6d75fd8431c37c801a897b4a0318737a))
* sudo ([49b9d6e](https://github.com/rickbassham/indi-celestron-cgx/commit/49b9d6ed23d8435f37eb38838e80b19dee50d7df))

## [1.0.1](https://github.com/rickbassham/indi-celestron-cgx/compare/v1.0.0...v1.0.1) (2021-01-02)


### Bug Fixes

* include get_version in raspbian build ([350cba3](https://github.com/rickbassham/indi-celestron-cgx/commit/350cba3d779ad0034d2e84a60941f89a4103c221))

# 1.0.0 (2021-01-02)


### Bug Fixes

* and remove the last fix ([f4a7f28](https://github.com/rickbassham/indi-celestron-cgx/commit/f4a7f2863b43015350da038b040f6094882e90f7))
* another try at fixing nameserver resolution ([ed4ea6e](https://github.com/rickbassham/indi-celestron-cgx/commit/ed4ea6e54f8786a6f34f8f5da10aac7b26b98aaf))
* fix nameserver inside docker ([8e43bcf](https://github.com/rickbassham/indi-celestron-cgx/commit/8e43bcfc34f3c5ec1a49819885a3d164e8918d1b))
* fix raspbian builds ([36b55ec](https://github.com/rickbassham/indi-celestron-cgx/commit/36b55ece01cc70ec18c88565548038ff1c77023c))
* test autorelease and build debs ([edddaec](https://github.com/rickbassham/indi-celestron-cgx/commit/edddaec9a39a2bc2080294de88ecd0e70414d461))
