<h1 align="center">Intuitive, Fast & Efficient</h1>

<p align="center">
  <a href="https://www.objeck.org"><img src="docs/images/gear_wheel_256.png""  width="300" alt="An Objeck"/></a>
</p>

<hr/>

<p align="center">
  <a href="https://github.com/objeck/objeck-lang/actions/workflows/c-cpp.yml"><img src="https://github.com/objeck/objeck-lang/actions/workflows/c-cpp.yml/badge.svg" alt="GitHub CI"></a>
  <a href="https://scan.coverity.com/projects/objeck"><img src="https://img.shields.io/coverity/scan/10314.svg" alt="Coverity SCA"></a>
  <a href="https://github.com/objeck/objeck-lang/actions/workflows/codeql.yml"><img src="https://github.com/objeck/objeck-lang/actions/workflows/codeql.yml/badge.svg" alt="GitHub CodeQL"></a>
  <a href="https://discord.gg/qEaCGWR7nb"><img src="https://badgen.net/badge/icon/discord?icon=discord&label" alt="Discord"></a>
</p>

Objeck is a fast, intuitive, and lightweight programming language that supports both object-oriented and functional programming paradigms, and is designed to be compatible with multiple platforms, including Windows, Linux, and macOS.

```ruby
use API.Google.Gemini, System.IO.Filesystem;

class IdentifyImage {
  function : Main(args : String[]) ~ Nil {
    content := Content->New("user")->AddPart(TextPart->New("What number is this image showing?"))
      ->AddPart(BinaryPart->New(FileReader->ReadBinaryFile("thirdteen.png"), "image/png"))
      ->AddPart(TextPart->New("Format output as JSON"));

    candidates := Model->GenerateContent("models/gemini-pro-vision", content, EndPoint->GetApiKey());
    if(candidates->Size() > 0) {
      candidates->First()->GetAllText()->Trim()->PrintLine();
    };
  }
}
```

| <sub>[VS Code](https://github.com/objeck/objeck-lsp)</sub> | <sub>[Debugger](https://github.com/objeck/objeck-lang/tree/master/core/debugger)</sub> | <sub>[Dungeon Crawler](https://github.com/objeck/objeck-dungeon-crawler)</sub> | <sub>[Platformer](https://github.com/objeck/objeck-lang/blob/master/programs/deploy/2d_game_13.obs)</sub> | <sub>[Windows Utility](https://github.com/objeck/objeck-lang/tree/master/core/release/WindowsLauncher)</sub> |
| :---: | :----: | :---: | :---: | :---: |
![alt text](docs/images/web/comp.png "Visual Studio Code") | ![alt text](docs/images/web/debug.jpg "Command line debugger") | ![alt text](docs/images/web/crawler.png "Web Crawler") | ![alt text](docs/images/web/2d_game.jpg "Platformer") | ![alt text](docs/images/web/launch.png "Windows Launcher") |

Want to get started? Take a look at the [language guide](https://objeck.org/getting_started.html) and [code examples](https://github.com/objeck/objeck-lang/tree/master/programs/examples). If you want to contribute, start with the [system architecture](https://github.com/objeck/objeck-lang/tree/master/core).

## What is it?

* Modern object-oriented and functional
* Cross-platform: Linux (x64 and ARM64), macOS (ARM64), Windows (x64)
* Optimized and JIT-compiled runtimes
* API support for
  * Secure web servers and clients
  * Encryption
  * JSON, CSV, and XML parsing libraries
  * Regular expressions
  * 2D gaming
  * Linear matrix mathematics
  * Collections
  * Files, directories, sockets, STDIO, logging, serialization, and pipes
* REPL and IDE [LSP](https://github.com/objeck/objeck-lsp) support (for VSCode, Sublime, Kate, etc.) 
* Online guides and API [documentation](https://www.objeck.org).

## What's New?

* v2024.5.0
  * Extend LSP functionality
  * Explore changing all Collection 'ToString()' methods to return JSON

* v2024.4.0 **[current release]**
  * Added support for Google DeepMind (Gemini) APIs
    * Model
    * Corpus (v1beta)
    * Chat
  * Open AI support for external function calls
  * OAuth2 support (session and file based support)
  * Refactored KMeans ML implementation to support arrays
  * Improved support for Date <=> String operations
  * Improved Base64 encoding and decoding
  * Added support for private functions
  * Tuples classes moved to Collection.Tuple
  * Added 'First' and 'Last' methods to Vector classes
  * Fixed ARM64 JIT compiler 'eor' instruction issue impacting macOS and ARM64


* v2024.3.0
  * Added support for OpenAI APIs
  * Fixed GC bug #462 and #482
  
* v2024.2.1
  * Support for the [unary bitwise not operator](https://gist.github.com/objeck/d7d5c94de9a1361a20f0eccd65226ded) (#480)
  * Added support for [response files](https://gist.github.com/objeck/b1a11ea34f4697b06e31f8cadb59f601) (#545)
  * Webserver library enhancements
    * Improved performance
    * More [configuration](https://github.com/objeck/objeck-web-server/blob/master/config/weather_config.json) options
  * Add an optional [in](https://gist.github.com/objeck/27219d777b636208d88ab197d1cf1270) keyword for use in `each` loops; an alternative to `:=` operator

* v2024.2.0
  * New [incremental JSON parser](https://www.objeck.org/doc/api/classes.html#data.json.stream)
    * Improved parsing performance for large JSON documents
  * Immutable [Tuple types](https://www.objeck.org/doc/api/classes.html#tuples)
    * Pair<A,B>
    * Triplet<A,B,C>
    * Quartet<A,B,C,D>
  * Range support
    * Added ``CharRange``, ``IntRange`` and, ``FloatRange`` classes

## How to Use It?

### [Object-oriented](https://en.wikipedia.org/wiki/Object-oriented_programming)
  
#### Inheritance
```ruby
class Triangle from Shape {
  New() {
    Parent();
  }
}
```

#### Interfaces
```ruby
class Triangle from Shape implements Color {
  New() {
    Parent();
  }

  method : public : GetRgb() ~ Int {
    return 0xadd8e6;
  }
}

interface Color {
  method : virtual : public : GetRgb() ~ Int;
}
```

#### Type Inference
```ruby
value := "Hello World!";
value->Size()->PrintLine();
```

#### Anonymous Classes
```ruby
interface Greetings {
  method : virtual : public : SayHi() ~ Nil;
}

class Hello {
  function : Main(args : String[]) ~ Nil {
    hey := Base->New() implements Greetings {
      New() {}

      method : public : SayHi() ~ Nil {
        "Hey..."->PrintLine();
      }
    };
}
```

#### Reflection
```ruby
klass := "Hello World!"->GetClass();
klass->GetName()->PrintLine();
klass->GetMethodNumber()->PrintLine();
```

#### Dependency Injection
```ruby
value := Class->Instance("System.String")->As(String);
value += "510";
value->PrintLine();
```
#### Generics
```ruby
map := Collection.Map->New()<IntRef, String>;
map->Insert(415, "San Francisco");
map->Insert(510, "Oakland");
map->Insert(408, "Sunnyvale");
map->ToString()->PrintLine();
```

#### Type Boxing
```ruby
list := Collection.List->New()<IntRef>;
list->AddBack(17);
list->AddFront(4);
(list->Back() + list->Front())->PrintLine();
```

#### Static import
```ruby
use function Int;

class Test {
  function : Main(args : String[]) ~ Nil {
    Abs(-256)->Sqrt()->PrintLine();
  }
}
```

#### Serialization
```ruby
serializer := System.IO.Serializer->New();
serializer->Write(map);
serializer->Write("Fin.");
bytes := serializer->Serialize();
bytes->Size()->PrintLine();
```

### [Functional](https://en.wikipedia.org/wiki/Functional_programming)
#### Closures and Lambda Expressions
```ruby
funcs := Vector->New()<FuncRef<IntRef>>;
each(i : 10) {
  funcs->AddBack(FuncRef->New(\() ~ IntRef : () 
    => System.Math.Routine->Factorial(i) * funcs->Size())<IntRef>);
};

each(i : funcs) {
  value := funcs->Get(i)<FuncRef>;
  func := value->Get();
  func()->Get()->PrintLine();
};
```

#### First-Class Functions
```ruby
@f : static : (Int) ~ Int;
@g : static : (Int) ~ Int;

function : Main(args : String[]) ~ Nil {
  compose := Composer(F(Int) ~ Int, G(Int) ~ Int);
  compose(13)->PrintLine();
}

function : F(a : Int) ~ Int {
  return a + 14;
}

function : G(a : Int) ~ Int {
  return a + 15;
}

function : native : Compose(x : Int) ~ Int {
  return @f(@g(x));
}

function : Composer(f : (Int) ~ Int, g : (Int) ~ Int) ~ (Int) ~ Int {
  @f := f;
  @g := g;
  return Compose(Int) ~ Int;
}
```

### Host Support
#### Unicode
```ruby
"Καλημέρα κόσμε"->PrintLine();
```

#### File System
```ruby
content := Sytem.IO.Filesystem.FileReader->ReadFile(filename);
content->Size()->PrintLine();
Sytem.IO.Filesystem.File->Size(filename)->PrintLine();
```

#### Sockets
```ruby
socket->WriteString("GET / HTTP/1.1\nHost:google.com\nUser Agent: Mozilla/5.0 (compatible)\nConnection: Close\n\n");
line := socket->ReadString();
while(line <> Nil & line->Size() > 0) {
  line->PrintLine();  
  line := socket->ReadString();
};
socket->Close();
```

#### Named Pipes
```ruby
pipe := System.IO.Pipe->New("foobar", Pipe->Mode->CREATE);
if(pipe->Connect()) {
  pipe->ReadLine()->PrintLine();
  pipe->WriteString("Hi Ya!");
  pipe->Close();
};
```

#### Threads
```ruby
class CaculateThread from Thread {
  ...
  @inc_mutex : static : ThreadMutex;

  New() {
    @inc_mutex := ThreadMutex->New("inc_mutex");
  }
  
  method : public : Run(param : System.Base) ~ Nil {
    Compute();
  }

  method : native : Compute() ~ Nil {
    y : Int;

    while(true) {
      critical(@inc_mutex) {
        y := @current_line;
        @current_line+=1;
      };
      ...
    };
  }
}
```
#### Date/Times
```ruby
yesterday := System.Time.Date->New();
yesterday->AddDays(-1);
yesterday->ToString()->PrintLine();
```

### Notable Libraries
  * [Machine learning](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/ml.obs)
  * [HTTPS](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/net_secure.obs) and [HTTP](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/net.obs) server and client APIs
  * [JSON](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/json.obs), [XML](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/xml.obs) and [CSV](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/csv.obs) parsers
  * [Regular expression](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/regex.obs) library
  * Encryption and hashing
  * In memory [query framework](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/query.obs) with SQL-like syntax
  * Database access
  * [2D Gaming framework](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/sdl_game.obs) via SDL2
  * [Collections](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/gen_collect.obs) (caches, vectors, queues, trees, hashes, etc.)
  * GTK windowing support [(on-hold)](core/lib/experimental/gtk)
