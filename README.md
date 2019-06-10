# Objeck
Objeck is an object-oriented computer language with functional features. The language has ties with Java, Scheme and UML. In this language all data types, except for higher-order functions, are treated as objects.

![alt text](images/design2.png "Compiler & VM")

Objeck is a general-purpose programming language with an emphasis placed on simplicity. The programming environment consists of a compiler, virtual machine and command line debugger.

```ruby
class Hello {
  function : Main(args : String[]) ~ Nil {
    "Hello World" → PrintLine();
    "Καλημέρα κόσμε" → PrintLine();
    "こんにちは 世界" → PrintLine();
  }
}
```

See more on [Rosetta Code](http://rosettacode.org/wiki/Category:Objeck) and checkout the following [programming tasks](programs/rc).

Notable features:
* Object-oriented and functional
  * Classes, interfaces and higher-order functions
  * Generics with type boxing
  * Anonymous classes 
  * Reflection 
  * Object serialization 
  * Type inference
* Full native platform support
  * Windows, macOS and Linux (64-bit)
  * Windows and Linux (32-bit)  
* Native support for threads, sockets, files, date/time, etc.
* Libraries 
  * Collections (vectors, queues, trees, hashes, etc.)
  * HTTP and HTTPS clients
  * RegEx
  * JSON and XML parsers
  * Encryption
  * Database access
  * Data structure querying
  * 2D Gaming
* Garbage collection
* JIT support (IA-32 and AMD64)

Work in progress:
* Lambda expressions (v5.1)

## Documentation
Please refer to the programmer's guide [documentation](http://www.objeck.org/documentation/) and [online tutorial](http://www.objeck.org/tutorial/). Also checkout [Rosetta Code](http://rosettacode.org/wiki/Category:Objeck) [examples](programs/rc).

## Building
Building on Linux:
*  Install required libraries: "sudo apt-get install build-essential git libssl-dev unixodbc-dev libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev"
*  cd to "objeck-lang/core/release" and run "./deploy.sh" with the parameter 32 or 64 representing the bit version of Linux
*  Build output with binaries and documentation will be located in "objeck-lang/core/release/deploy"

## Binaries
Get the latest [binaries](https://sourceforge.net/projects/objeck-lang/).
