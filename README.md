## Objeck
Objeck is an object-oriented computer language with functional features. The language has ties with Java, Scheme and UML. In this language all data types, except for higher-order functions, are treated as objects.

The language contains all of the features of a general-purpose computing language with an emphasis placed on simplicity. The programming environment consists of a compiler, virtual machine and command line debugger.

### Downloading

OS	| Version |	CPU
----|---------|-----
Windows (XP-8.1) | [3.3.4](http://sourceforge.net/projects/objeck-lang/files/binaries/objeck_r3.3.4_1_win32.msi/download) | x86
OS X (Yosemite) | [3.3.4](http://sourceforge.net/projects/objeck-lang/files/binaries/objeck_r3.3.4_1_osx.tgz/download) | AMD64
Linux (64-bit) | [3.3.4](http://sourceforge.net/projects/objeck-lang/files/binaries/objeck_r3.3.4_1_linux64.tgz/download) | AMD64
Linux (32-bit) | [3.3.4](http://sourceforge.net/projects/objeck-lang/files/binaries/objeck_r3.3.4_1_linux32.tgz/download) | x86

### Short example
```objeck
class Hello {
  function : Main(args : String[]) ~ Nil {
    "Hello World"->PrintLine();
    "Καλημέρα κόσμε"->PrintLine();
    "こんにちは 世界"->PrintLine();
  }
}
```

Compiling: ```obc -src hello.obs -dest hello.obe```

Running: ```obr hello.obe```

### Documentation
Please refer to the project website for [documentation](http://www.objeck.org/documentation/) and [tutorials](http://www.objeck.org/tutorial/).

### Pulling the code
```git clone https://github.com/objeck/objeck-lang.git objeck-lang```

### Building
[Build instructions](http://www.objeck.org/developers/) for Windows, Linux and OS X. 


