#ifndef GROSS_CODEGEN_TARGETS_H
#define GROSS_CODEGEN_TARGETS_H
#include <cstdlib>

namespace gross {
struct DLXTargetTraits {
  struct RegisterFile;

  static constexpr size_t ReturnStorage = 1;
  static constexpr size_t FramePointer = 28;
  static constexpr size_t StackPointer = 29;
  static constexpr size_t GlobalPointer = 30;
  static constexpr size_t LinkRegister = 31;
};
struct DLXTargetTraits::RegisterFile {
  static constexpr size_t size() { return 32; }

  static constexpr size_t FirstCallerSaved = 10;
  static constexpr size_t LastCallerSaved = 25;

  static constexpr size_t FirstCalleeSaved = 6;
  static constexpr size_t LastCalleeSaved = 9;

  static constexpr size_t FirstParameter = 2;
  static constexpr size_t LastParameter = 5;

  static constexpr size_t FirstScratch = 26;
  static constexpr size_t LastScratch = 27;
};

struct CompactDLXTargetTraits : public DLXTargetTraits {
  struct RegisterFile : public DLXTargetTraits::RegisterFile {
    static constexpr size_t FirstCallerSaved = 7;
    static constexpr size_t LastCallerSaved = 7;

    static constexpr size_t FirstCalleeSaved = 6;
    static constexpr size_t LastCalleeSaved = 6;
  };
};
} // namespace gross
#endif
