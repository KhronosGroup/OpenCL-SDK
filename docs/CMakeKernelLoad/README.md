# Locating kernel code when building with CMake

When using CMake to build your program, CMake can help the application locate the kernel sources to be loaded at runtime using various methods. These methods are detailed and elaborated on in the following folders:

- [Writing a config header](ConfigureFile)
- [Current working directory relative](CwdRelative)
- [Embedding kernel source into executable](Embed)
- [Executable relative](ExeRelative)
- [Embedding location via preprocessor directive](PreprocessorDefine)