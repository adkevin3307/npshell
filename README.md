# npshell
Simple shell with number pipe feature

## Environment
* `g++` 10.2.0 / `clang` 3.4.2

## Usage
* `make` compile
* `./npshell` execute npshell

## Env
* `setenv {ENV} {CONTEXT}` set env
    * `setenv PATH /usr/bin`, default `PATH` is `bin:.`
* `printenv {ENV}` print env
    * `printenv PATH`