# Origami

__Origami__ is a tiny and rather limited 2D game engine built on top of the Vulkan API.
It is not intended to be used professionally but as learning aid or just another C Vulkan
codebase to look at (there are not many btw).


# Dependencies

* Vulkan
* GLFW


# Structure

The engine is written in C and exposes a much higher level API which depends on Vulkan under the hood.
The entire engine code is compiled into a static library, **liborigami.a** which can then be linked into your programs. 


# Building

```bash
cd origami
make
```

# Using Origami

Sample programs on how to use origami are in the examples folder. Run any of the examples with the command

```bash
cd examples
make TARGET=blank_window.c    # Replace The Filename
```

# Resources

1. [Vulkan Tutorial By Alexander Overvoode](https://www.vulkan-tutorial.com)

1. [Code Samples By Sascha Willems](https://github.com/SaschaWillems/Vulkan)


# Author

Da Vinci
