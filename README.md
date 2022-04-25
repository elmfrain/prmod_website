# Parkour Recorder Website
An interactive website that is home to my Minecraft mod, Parkour Recorder.

![Main Screenshot](https://raw.githubusercontent.com/wiki/elmfrain/prmod_website/main_screenshot.png)

It is made to mimic the GUI of the actual mod.

## Building

You can build this project with standard native complilers or with Emscripten (compiling for the web).

### Linux

#### Dependencies
* Cmake v3.2 or newer

```
git clone https://github.com/elmfrain/prmod_website.git --recursive
mkdir build
cd build
cmake ..
make -j
```
And run with command:
```
./pr_web
```
---

### Emscripten (JavaScript, Website, Browser)

#### Dependencies
* Cmake v3.2 or newer
* Emscripten SDK v2.0.31 or newer

```
git clone https://github.com/elmfrain/prmod_website.git --recursive
mkdir build
cd build
emcmake cmake ..
make -j
```

And run with command:
```
emrun index.html
```

## How It's Made
This project's workflow (stack) is unconventional because it is written in C instead of JavaScript, CSS, and HTML.
Yet, it is able to be run in a browser thanks to Emscripten.

#### Emscripten
Emscripten is a complier designed to convert C or C++ code into JavaScript, and it is very powerful.
You can compile most C or C++ programs into JavaScript and run them in a browser. This means that this website can be run
natively in Linux, MacOS, or Windows and in the browser too.

#### Why C?
I like performance and customizability. This project is made with a game-engine-development mindset where performance is everything, and
the ability to be able to write custom fast code is thrilling (but only when it works, haha). Additionally, this project helped me teach
myself C, and learning it was a fun expierience.

In my opinion, C makes you a better programmer by making you aware of what your machine does while running code on a lower level
(especially with memory management), and this awareness gives you a lead on how to improve and optimize your code.

While this project is written in C, I'm open to using C++ as it may be easier to code with it and by allowing you to focus more on the
project itself.
