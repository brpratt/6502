# 6502

This is my attempt at a cycle-accurate [6502](https://en.wikipedia.org/wiki/MOS_Technology_6502).

My ultimate goal is to either build an NES emulator or a Commodor 64 emulator.

## Usage

The 6502 and its variants were integrated into all sorts of different systems, such as the [Apple II](https://en.wikipedia.org/wiki/Apple_II), the [Atari 2600](https://en.wikipedia.org/wiki/Atari_2600), the [NES](https://en.wikipedia.org/wiki/Nintendo_Entertainment_System), and the [Commodore 64](https://en.wikipedia.org/wiki/Commodore_64). Often, these systems used memory or register banking to extend number of addressable peripherals beyond what could be reached with the 16 address pins.

Accordingly, you'll notice the `cpu_tick()` and `cpu_step()` functions receive a pointer to a `bus`. While the CPU code uses `bus_peek()` and `bus_poke()` to read and write data, it doesn't assume any specific bus implementation. You can provide a bus implementation that handles memory banking or capture register reads/wrtes to forward to other components.

You'll find example bus implementations inside:

- `example/main.c` 
- `test/6502_functional_test.c`
- `test/test.c`

These are simple examples, but it should give you an idea of how more complex buses could be constructed.

## Developing

### VS Code + Dev Container

I use a [VS Code dev container](https://code.visualstudio.com/docs/remote/containers) to work on this project.

If you have Docker, VS Code, and the [Remote Container Extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers), you shouldn't need to install anything else on your machine. Open this project in the dev container and have fun!

### I use something else

If you have...

- `make`
- `gcc`
- a typical *nix environment

...you should be good to go. You will need [`dasm`](https://dasm-assembler.github.io/) in order to assemble the example program.

## Testing

`make test` will build and run all the tests.

There are a few unit tests for the `bus` and `cpu` inside `test/bus_test.c` and `cpu/cpu_test.c`. However, the most meaningful test comes from `test/6502_functional_test.c`. This test verifies the functionality of all legal opcodes and address modes. It runs the functional test program that can be found in [this repo](https://github.com/Klaus2m5/6502_65C02_functional_tests). Thank you @Klaus2m5!

## Building

The code in this repo is meant to be integrated into other code. It doesn't produce a final artifact.

You can choose to build a shared library, use git submodules, or copy the code directly.

The [Example](#example) section describes a small, example computer provided in this repo.

## Example

Compy is a small, example computer that utilizes this code. It's implementation can be found in `example/main.c`.

On startup, it will load a 64K image and set the PC to the address found in the reset vector (`0xFFFC`). Compy will run the program until it detects that the CPU is in a branch-to-self loop, at which point it will print out the contents of address `0x0000`.

The example program at `example/program.asm` computes `1 + 2`. This example program uses the [`dasm`](https://dasm-assembler.github.io/) assembler.

`make example` will build Compy, assemble the example program, and run it.
