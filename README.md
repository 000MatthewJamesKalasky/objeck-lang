<p align="center">
  <a href="https://www.objeck.org"><img src="docs/images/gear_wheel_256.png"  width="192" height="192" alt="An Objeck"/></a>
</p>

<p align="center">
  <a href="https://github.com/objeck/objeck-lang/actions/workflows/c-cpp.yml"><img src="https://github.com/objeck/objeck-lang/actions/workflows/c-cpp.yml/badge.svg" alt="C/C++ CI"></a>
  <a href="https://discord.gg/qEaCGWR7nb"><img src="https://badgen.net/badge/icon/discord?icon=discord&label" alt="Discord"></a>
  <a href="https://scan.coverity.com/projects/objeck"><img src="https://img.shields.io/coverity/scan/10314.svg" alt="Coverity Scan Build Status"></a>
</p>

<h1 align="center">Intuitive, Fast & Efficient</h1>

```ruby
class Hello {
   function : Main(args : String[]) ~ Nil {
      hiya := Collection.Vector->New()<String>
      hiya->AddBack("Hello World")
      hiya->AddBack("Καλημέρα κόσμε")
      hiya->AddBack("こんにちは 世界")
      hiya->Each(\^(h) => h->PrintLine())
   }
}
```

To get started, take a look at the [guide](https://objeck.org/getting_started.html), [code examples](https://github.com/objeck/objeck-lang/tree/master/programs/tests/rc), and [IDE plug-ins](https://github.com/objeck/objeck-lsp). 

If you are intreated to contribute, look at the [system architecture](https://github.com/objeck/objeck-lang/tree/master/core). 

## Key Features

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
#### Reflection
```ruby
value := "Hello World!";
klass := value->GetClass();
klass->GetName()->PrintLine();
klass->GetMethodNumber()->PrintLine();
```

#### Dependency injection
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
map->Insert(925, "East Bay");
map->Size()->PrintLine();
```

#### Type boxing
```ruby
list := Collection.List->New()<IntRef>;
list->AddBack(17);
list->AddFront(4);
(list->Back() + list->Front())->PrintLine();
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
for(i := 0; i < 10; i += 1;) {
  funcs->AddBack(FuncRef->New(\() ~ IntRef : () 
    => System.Math.Routine->Factorial(i) * funcs->Size())<IntRef>);
};

each(i : funcs) {
  value := funcs->Get(i)<FuncRef>;
  func := value->Get();
  func()->Get()->PrintLine();
};
```

#### First-class functions
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


### [Unicode support](https://en.wikipedia.org/wiki/Unicode)
#### OS-level support
```ruby
"Καλημέρα κόσμε"->PrintLine();
```

#### File systems
```ruby
content := Sytem.IO.Filesystem.FileReader->ReadFile(filename);
content->Size()->PrintLine();
Sytem.IO.Filesystem.File->Size(filename)->PrintLine();
```

  * Sockets
  * Named pipes
  * Threads
  * Date/times
  * Native extension libraries (.so, .dll)
* [Generational garbage collection](https://en.wikipedia.org/wiki/Tracing_garbage_collection)
* JIT compilation
  * [arm64](https://github.com/objeck/objeck-lang/tree/master/core/vm/arch/jit/arm64): Linux (Raspberry Pi 4), macOS (Apple silicon)
  * [x86-64](https://github.com/objeck/objeck-lang/tree/master/core/vm/arch/jit/amd64): Windows (10/11), Linux and macOS
* [LSP support](https://github.com/objeck/objeck-lsp)
* Documentation
  * [Tutorial](https://www.objeck.org/getting_started.html)
  * [APIs](https://www.objeck.org/doc/api/index.html)

## Screenshots
| <sub>[VS Code](https://github.com/objeck/objeck-lsp)</sub> | <sub>[Debugger](https://github.com/objeck/objeck-lang/tree/master/core/debugger)</sub> | <sub>[Dungeon Crawler](https://github.com/objeck/objeck-dungeon-crawler)</sub> | <sub>[Platformer](https://github.com/objeck/objeck-lang/blob/master/programs/deploy/2d_game_13.obs)</sub> | <sub>[Windows Utility](https://github.com/objeck/objeck-lang/tree/master/core/release/WindowsLauncher)</sub> |
| :---: | :----: | :---: | :---: | :---: |
![alt text](docs/images/web/comp.png "Visual Studio Code") | ![alt text](docs/images/web/debug.jpg "Command line debugger") | ![alt text](docs/images/web/crawler.png "Web Crawler") | ![alt text](docs/images/web/2d_game.jpg "Platformer") | ![alt text](docs/images/web/launch.png "Windows Launcher") |

## Libraries
  * [HTTPS](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/net_secure.obs) and [HTTP](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/net.obs) server and client APIs
  * [JSON](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/json.obs), [XML](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/xml.obs) and [CSV](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/csv.obs) parsers
  * [Regular expression](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/regex.obs) library
  * Encryption and hashing
  * In memory [query framework](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/query.obs) with SQL-like syntax
  * Database access
  * [2D Gaming framework](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/sdl_game.obs) via SDL2
  * [Collections](https://github.com/objeck/objeck-lang/blob/master/core/compiler/lib_src/gen_collect.obs) (caches, vectors, queues, trees, hashes, etc.)
  * GTK windowing support [(work-in-progress)](core/lib/experimental/gtk)
  
