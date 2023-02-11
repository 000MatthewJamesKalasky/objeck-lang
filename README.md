# Objeck
[![C/C++ CI](https://github.com/objeck/objeck-lang/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/objeck/objeck-lang/actions/workflows/c-cpp.yml)
[![Coverity Scan Build Status](https://img.shields.io/coverity/scan/10314.svg)](https://scan.coverity.com/projects/objeck)
[![Discord](https://badgen.net/badge/icon/discord?icon=discord&label)](https://discord.gg/qEaCGWR7nb)

Objeck is an object-oriented programming language with functional features. Objeck emphasizes, expression, simplicity, portability, and scalability. The programming environment consists of a compiler, virtual machine, and command line debugger with IDE plugins.

```ruby
class Hello {
  function : Main(args : String[]) ~ Nil {
    "Hello World" → PrintLine();
    "Καλημέρα κόσμε" → PrintLine();
    "こんにちは 世界" → PrintLine();
  }
}
```

See more on [Rosetta Code](http://rosettacode.org/wiki/Category:Objeck) and checkout the following [programming tasks](programs/tests/rc).

### Features
* Libraries 
  * [Collections](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/gen_collect.obs) (vectors, queues, trees, hashes, etc.)
  * [HTTPS](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/net_secure.obs) and HTTP](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/net.obs client and web server as a service
  * [RegEx](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/regex.obs)
  * [JSON](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/json.obs), [XML](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/xml.obs) and [CSV](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/csv.obs) libraries
  * Encryption
  * Database access
  * [Query framework](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/query.obs)
  * [2D Gaming framework](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/sdl_game.obs)
* Object-oriented with functional features
  * Classes, interfaces and first-class functions
  * Type inference
  * Lambda expressions
  * Closures
  * Generics with type boxing
  * Anonymous classes 
  * Reflection 
  * Object serialization
* Native support for threads, sockets, files, date/time, etc.
* Garbage collection
* JIT support for all targets
  * [arm64](https://github.com/objeck/objeck-lang/tree/master/core/vm/arch/jit/arm64): Linux (Raspberry Pi 4), macOS (Apple silicon)
  * [x86-64](https://github.com/objeck/objeck-lang/tree/master/core/vm/arch/jit/amd64): Windows, Linux and macOS (depreciated for Intel macOS)

### Screenshots
| <sub>[VS Code](https://github.com/objeck/objeck-lsp)</sub> | <sub>[Debugger](https://github.com/objeck/objeck-lang/tree/master/core/debugger)</sub> | <sub>[Dungeon Crawler](https://github.com/objeck/objeck-dungeon-crawler)</sub> | <sub>[Platformer](https://github.com/objeck/objeck-lang/blob/master/programs/deploy/2d_game_13.obs)</sub> | <sub>[Windows Utility](https://github.com/objeck/objeck-lang/tree/master/core/release/WindowsLauncher)</sub> |
| :---: | :----: | :---: | :---: | :---: |
![alt text](docs/images/web/comp.png "Visual Studio Code") | ![alt text](docs/images/web/debug.jpg "Command line debugger") | ![alt text](docs/images/web/crawler.png "Web Crawler") | ![alt text](docs/images/web/2d_game.jpg "Platformer") | ![alt text](docs/images/web/launch.png "Windows Launcher") |

## Documentation
Please refer to the programmer's guide [documentation](https://www.objeck.org/doc/api/index.html) and [online tutorial](https://www.objeck.org/getting_started.html).

## Architecture
* [Compiler](https://github.com/objeck/objeck-lang/blob/master/core/compiler)
* [Virtual Machine](https://github.com/objeck/objeck-lang/blob/master/core/vm)
* [Debugger](https://github.com/objeck/objeck-lang/blob/master/core/debugger)
* [Memory manager](https://github.com/objeck/objeck-lang/blob/master/core/vm/arch)
* [Just-In-Time (JIT) compiler](https://github.com/objeck/objeck-lang/blob/master/core/vm/arch/jit)

![alt text](docs/images/toolchain.svg "Objeck System Architecture")

## Code
Objeck can be built for a variety of targets.

### Linux (x64, ARM64)
*  For Linux, install required libraries: <code>sudo apt-get install build-essential git libssl-dev unixodbc-dev libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libreadline-dev unzip</code>
*  cd to <code>objeck-lang/core/release</code> and run <code>./deploy_posix.sh</code> with the parameter <code>64</code> or <code>rpi</code> for x64 or ARM64 Linux
*  Build output with binaries and documentation will be located in <code>objeck-lang/core/release/deploy</code>
*  Reference <code>objeck-lang/core/release/deploy/readme.html</code> to set additional paths and find examples

### macOS (Apple Silicon)
*  Install Xcode with command line tools
*  Open a command shell and go to <code>objeck-lang/core/release</code> and run <code>./deploy_macos_arm64.sh</code>
*  Build output with binaries and documentation will be located in <code>objeck-lang/core/release/deploy</code>
*  Reference <code>objeck-lang/core/release/deploy/readme.html</code> to set additional paths and find examples

### Windows (Visual Studio)
*  Unzip the OpenSSL Windows libraries in <code>objeck-lang/core/lib/openssl/win/x64</code>
*  Using Visual Studio 2022 or later open <code>objeck-lang/core/release/objeck.sln</code>
*  Ensure the core build completes successfully (in Release mode)
*  To build everything, open a Visual Studio x64 command prompt and go to <code>objeck-lang/core/release</code> directory and run <code>deploy_amd64.cmd</code>
*  Build output with binaries and documentation will be located in <code>objeck-lang/core/release/deploy64</code>
*  Reference <code>objeck-lang/core/release/deploy64/readme.html</code> to learn how to set the environment variables and find code examples

### Windows (MSYS2)
* Under the UCRT64 (Unicode shell support)
  * Package dependencies
  * <code>pacman --noconfirm -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-openssl make mingw-w64-ucrt-x86_64-SDL2 unzip mingw-w64-ucrt-x86_64-SDL2_ttf mingw-w64-ucrt-x86_64-SDL2_mixer mingw-w64-ucrt-x86_64-SDL2_image mingw-w64-ucrt-x86_64-unixodbc</code>
  * cd to <code>objeck-lang/core/release</code> and run <code>./deploy_msys2-ucrt.sh</code>
  *  Build output with binaries and documentation will be located in <code>objeck-lang/core/release/deploy-msys2-ucrt</code>
  *  Set the path for to the UCRT64 'bin' directory
  *  Reference <code>objeck-lang/core/release/deploy-msys2-ucrt/readme.html</code> to set additional paths and find examples
* Under the Clang64
  * Package dependencies
  * <code>pacman --noconfirm -S mingw-w64-clang-x86_64-gcc mingw-w64-clang-x86_64-openssl make mingw-w64-clang-x86_64-SDL2 unzip mingw-w64-clang-x86_64-SDL2_ttf mingw-w64-clang-x86_64-SDL2_mixer mingw-w64-clang-x86_64-SDL2_image mingw-w64-clang-x86_64-unixodbc</code>
  * cd to <code>objeck-lang/core/release</code> and run <code>./deploy_msys2-clang.sh</code>
  *  Build output with binaries and documentation will be located in <code>objeck-lang/core/release/deploy-msys2-clang</code>
  *  Set the path for to the Clang64 'bin' directory
  *  Reference <code>objeck-lang/core/release/deploy-msys2-clang/readme.html</code> to set additional paths and find examples
