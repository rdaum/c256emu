#ifndef LOADER_H_
#define LOADER_H_

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short  uint16_t;
typedef signed long int32_t;
typedef unsigned long uint32_t;

typedef volatile uint8_t far* ProgramPtr;

#define O65_PROGRAM_SIZE 2
#define O65_INVALID_SIGNATURE 1
#define O65_LOAD_OK 0

ProgramPtr RelocO65(ProgramPtr program, uint8_t *error);

#endif  // LOADER_H_