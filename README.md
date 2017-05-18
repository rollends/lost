# Lua On Serious Threads (LOST)

This is a fun project I decided to make with the goal of creating user-level threads out of Lua scripts.
The idea is to allow for many Lua tasks to be performed in parallel on a number of kernel-level worker threads. Of course,
it would be no fun if the project didn't interleave execution of these Lua tasks and so the project includes a lightly-gutted
version of the Lua 5.3.4 runtime that supports interrupting a Lua task at pre-defined intervals.
