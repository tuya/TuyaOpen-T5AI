#ifndef ASR_FST_TYPES_H_
#define ASR_FST_TYPES_H_

#include <stdint.h>

typedef struct {
  int nextstate;
  int ilabel;
  int olabel;
  float weight;
} Arc;

typedef struct {
  const Arc* arcs;
  int num_arcs;
} State;

typedef struct {
  const State* states;
  int num_states;
  const int* finals;
  int num_finals;
  const char** words; 
} Fst;

#endif
