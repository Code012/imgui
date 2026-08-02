/*  date = December 06th 2025 02:25 PM */ 

#ifndef BASE_PROJECT_H
#define BASE_PROJECT_H

// main entry point
// NOTE(sb): Entry point for single-threaded applications, have not implemented multi-threaded logic yet
// TODO(sb): Could pass a cmdline struct to entry: void BaseMainThreadEntry(void (*entry)(struct CmdLine *cmdln), U64 argument_count, char **arguments);
internal void BaseMainEntry(void (*entry)(U64 argument_count, char** arguments), U64 argument_count, char** arguments);

// TODO(sb): BaseMainThreadEntry

#endif // BASE_PROJECT_H