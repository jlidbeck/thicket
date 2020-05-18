Thicket
=======

An application for thicket growing.

Requirements
------------

Install [Vcpkg](https://github.com/microsoft/vcpkg).

Using Vcpkg, install the following packages:

- opencv
- nlohmann-json

Build
-----

```powershell
cmake -G "Visual Studio 16 2019" -B build -S .
cmake --build build
```

The application will only build with the _Visual Studio_ generators.
