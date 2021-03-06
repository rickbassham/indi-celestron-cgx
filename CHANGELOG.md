# [2.0.0](https://github.com/rickbassham/indi-celestron-cgx/compare/v1.4.0...v2.0.0) (2021-02-03)


### Features

* rollback to known good functionality ([9e6e7fa](https://github.com/rickbassham/indi-celestron-cgx/commit/9e6e7faa0ce6908bfd4842cf0b46561520c650cf))

# [1.4.0](https://github.com/rickbassham/indi-celestron-cgx/compare/v1.3.0...v1.4.0) (2021-01-16)


### Bug Fixes

* add debugging code ([334e98d](https://github.com/rickbassham/indi-celestron-cgx/commit/334e98d219ca913aa8f36f95e2c4c0a8fad1aa8d))

# [1.3.0](https://github.com/rickbassham/indi-celestron-cgx/compare/v1.2.7...v1.3.0) (2021-01-11)


### Bug Fixes

* indi versioning only uses major/minor ([bed3dec](https://github.com/rickbassham/indi-celestron-cgx/commit/bed3dec530a96c3a67442df7c383fbd2e4fe79d5))

## [1.2.7](https://github.com/rickbassham/indi-celestron-cgx/compare/v1.2.6...v1.2.7) (2021-01-10)


### Bug Fixes

* correct path for debs ([83928cb](https://github.com/rickbassham/indi-celestron-cgx/commit/83928cb9509ff328a94c7e85c7e979bf40c11fbf))

## [1.2.6](https://github.com/rickbassham/indi-celestron-cgx/compare/v1.2.5...v1.2.6) (2021-01-10)


### Bug Fixes

* debugging build ([4549547](https://github.com/rickbassham/indi-celestron-cgx/commit/454954708f527d76d42d1376e6979a625e44c56b))

## [1.2.5](https://github.com/rickbassham/indi-celestron-cgx/compare/v1.2.4...v1.2.5) (2021-01-10)


### Bug Fixes

* update semantic-release step ([abc8373](https://github.com/rickbassham/indi-celestron-cgx/commit/abc8373eeef1acd77f127ed1ed995e02dcd903e6))

## [1.2.4](https://github.com/rickbassham/indi-celestron-cgx/compare/v1.2.3...v1.2.4) (2021-01-10)


### Bug Fixes

* build system ([bcbefb5](https://github.com/rickbassham/indi-celestron-cgx/commit/bcbefb52d7756ff7e638d29a4d140f4da4a83219))

## [1.2.3](https://github.com/rickbassham/indi-celestron-cgx/compare/v1.2.2...v1.2.3) (2021-01-10)


### Bug Fixes

* update naming of device ([0898eae](https://github.com/rickbassham/indi-celestron-cgx/commit/0898eae6a39c5800faac1c896752b563fd8a5aef))

## [1.2.2](https://github.com/rickbassham/indi-celestron-cgx/compare/v1.2.1...v1.2.2) (2021-01-03)


### Bug Fixes

* fix generation of debian changelog ([b2d1efd](https://github.com/rickbassham/indi-celestron-cgx/commit/b2d1efdb8fd136b62e749a179d5c36352cac2c36))

## [1.2.1](https://github.com/rickbassham/indi-celestron-cgx/compare/v1.2.0...v1.2.1) (2021-01-03)


### Bug Fixes

* git config before committing ([f21c940](https://github.com/rickbassham/indi-celestron-cgx/commit/f21c940eb1447dc8684e735422e40427ad2771fe))

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
