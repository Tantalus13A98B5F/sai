#ifndef LLSC_EXTERNAL_IMP_HEADERS
#define LLSC_EXTERNAL_IMP_HEADERS

#include "external_shared.hpp"

/******************************************************************************/

template<typename T>
inline T __llsc_assert(SS& state, List<PtrVal>& args, __Cont<T> k, __Halt<T> h) {
  auto v = args.at(0);
  auto i = v->to_IntV();
  if (i) {
    if (i->i == 0) return h(state, { make_IntV(-1) }); // concrete false - generate the test and ``halt''
    return k(state, make_IntV(1, 32));
  }
  // otherwise add a symbolic condition that constraints it to be true
  // undefined/error if v is a value of other types
  auto cond = to_SMTNeg(v);
  auto pc = state.get_PC();
  pc.add(cond);
  if (check_pc(pc)) return h(state, { make_IntV(-1) }); // check if v == 1 is not valid
  pc.pop_back();
  pc.add(v);
  state.set_PC(pc);
  return k(state, make_IntV(1, 32));
}

/******************************************************************************/

template<typename T>
inline T __make_symbolic(SS& state, List<PtrVal>& args, __Cont<T> k) {
  PtrVal loc = args.at(0);
  ASSERT(std::dynamic_pointer_cast<LocV>(loc) != nullptr, "Non-location value");
  IntData len = proj_IntV(args.at(1));
  //std::cout << "sym array size: " << proj_LocV_size(loc) << "\n";
  for (int i = 0; i < len; i++) {
    state.update(loc + i, make_SymV(fresh(), 8));
  }
  return k(state, make_IntV(0));
}

inline List<SSVal> make_symbolic(SS& state, List<PtrVal> args) {
  return __make_symbolic<List<SSVal>>(state, args, [](auto s, auto v) { return List<SSVal>{{s, v}}; });
}

inline std::monostate make_symbolic(SS& state, List<PtrVal> args, Cont k) {
  return __make_symbolic<std::monostate>(state, args, [&k](auto s, auto v) { return k(s, v); });
}

template<typename T>
inline T __make_symbolic_whole(SS& state, List<PtrVal>& args, __Cont<T> k) {
  PtrVal loc = args.at(0);
  ASSERT(std::dynamic_pointer_cast<LocV>(loc) != nullptr, "Non-location value");
  IntData sz = proj_IntV(args.at(1));
  state.update(loc, make_SymV(fresh(), sz*8));
  return k(state, make_IntV(0));
}

inline List<SSVal> make_symbolic_whole(SS& state, List<PtrVal> args) {
  return __make_symbolic_whole<List<SSVal>>(state, args, [](auto s, auto v) { return List<SSVal>{{s, v}}; });
}

inline std::monostate make_symbolic_whole(SS& state, List<PtrVal> args, Cont k) {
  return __make_symbolic_whole<std::monostate>(state, args, [&k](auto s, auto v) { return k(s, v); });
}


/******************************************************************************/

template<typename T>
inline T __malloc(SS& state, List<PtrVal>& args, __Cont<T> k) {
  IntData bytes = proj_IntV(args.at(0));
  auto emptyMem = List<PtrVal>(bytes, nullptr);
  PtrVal memLoc = make_LocV(state.heap_size(), LocV::kHeap, bytes);
  if (exlib_failure_branch)
    return k(state.heap_append(emptyMem), memLoc) + k(state, make_LocV_null());
  return k(state.heap_append(emptyMem), memLoc);
}

inline List<SSVal> malloc(SS& state, List<PtrVal> args) {
  return __malloc<List<SSVal>>(state, args, [](auto s, auto v) { return List<SSVal>{{s, v}}; });
}

inline std::monostate malloc(SS& state, List<PtrVal> args, Cont k) {
  return __malloc<std::monostate>(state, args, [&k](auto s, auto v) { return k(s, v); });
}

/******************************************************************************/

template<typename T>
inline T __realloc(SS& state, List<PtrVal>& args, __Cont<T> k) {
  Addr src = proj_LocV(args.at(0));
  IntData bytes = proj_IntV(args.at(1));
  auto emptyMem = List<PtrVal>(bytes, nullptr);
  std::cout << "realloc size: " << emptyMem.size() << std::endl;
  PtrVal memLoc = make_LocV(state.heap_size(), LocV::kHeap, bytes);
  IntData prevBytes = proj_LocV_size(args.at(0));
  std::cout << "prev size: " << prevBytes << std::endl;
  state.heap_append(emptyMem);
  for (int i = 0; i < prevBytes; i++) {
    state.update(memLoc + i, state.heap_lookup(src + i));
  }
  return k(state, memLoc);
}

inline List<SSVal> realloc(SS& state, List<PtrVal> args) {
  return __realloc<List<SSVal>>(state, args, [](auto s, auto v) { return List<SSVal>{{s, v}}; });
}

inline std::monostate realloc(SS& state, List<PtrVal> args, Cont k) {
  return __realloc<std::monostate>(state, args, [&k](auto s, auto v) { return k(s, v); });
}

/******************************************************************************/

template<typename T>
inline T __llvm_memcpy(SS& state, List<PtrVal>& args, __Cont<T> k) {
  PtrVal dest = args.at(0);
  PtrVal src = args.at(1);
  IntData bytes_int = proj_IntV(args.at(2));
  ASSERT(std::dynamic_pointer_cast<LocV>(dest) != nullptr, "Non-location value");
  ASSERT(std::dynamic_pointer_cast<LocV>(src) != nullptr, "Non-location value");
  for (int i = 0; i < bytes_int; i++) {
    state.update(dest + i, state.at(src + i));
  }
  return k(state, IntV0);
}

inline List<SSVal> llvm_memcpy(SS state, List<PtrVal> args) {
  return __llvm_memcpy<List<SSVal>>(state, args, [](auto s, auto v) { return List<SSVal>{{s, v}}; });
}

inline std::monostate llvm_memcpy(SS state, List<PtrVal> args, Cont k) {
  return __llvm_memcpy<std::monostate>(state, args, [&k](auto s, auto v) { return k(s, v); });
}

/******************************************************************************/

template<typename T>
inline T __llvm_memmove(SS& state, List<PtrVal>& args, __Cont<T> k) {
  PtrVal dest = args.at(0);
  PtrVal src = args.at(1);
  ASSERT(std::dynamic_pointer_cast<LocV>(dest) != nullptr, "Non-location value");
  ASSERT(std::dynamic_pointer_cast<LocV>(src) != nullptr, "Non-location value");
  IntData bytes_int = proj_IntV(args.at(2));
  // Optimize: flex_vector_transient
  auto temp_mem = List<PtrVal>{};
  for (int i = 0; i < bytes_int; i++) {
    temp_mem = temp_mem.push_back(state.at(src + i));
  }
  for (int i = 0; i < bytes_int; i++) {
    state.update(dest + i, temp_mem.at(i));
  }
  return k(state, IntV0);
}

inline List<SSVal> llvm_memmove(SS state, List<PtrVal> args) {
  return __llvm_memmove<List<SSVal>>(state, args, [](auto s, auto v) { return List<SSVal>{{s, v}}; });
}

inline std::monostate llvm_memmove(SS state, List<PtrVal> args, Cont k) {
  return __llvm_memmove<std::monostate>(state, args, [&k](auto s, auto v) { return k(s, v); });
}

/******************************************************************************/

template<typename T>
inline T __llvm_memset(SS& state, List<PtrVal>& args, __Cont<T> k) {
  PtrVal dest = args.at(0);
  IntData bytes_int = proj_IntV(args.at(2));
  ASSERT(std::dynamic_pointer_cast<LocV>(dest) != nullptr, "Non-location value");
  auto v = make_IntV(0, 8);
  for (int i = 0; i < bytes_int; i++) {
    state.update(dest + i, v);
  }
  return k(state, IntV0);
}

inline List<SSVal> llvm_memset(SS& state, List<PtrVal> args) {
  return __llvm_memset<List<SSVal>>(state, args, [](auto s, auto v) { return List<SSVal>{{s, v}}; });
}

inline std::monostate llvm_memset(SS& state, List<PtrVal> args, Cont k) {
  return __llvm_memset<std::monostate>(state, args, [&k](auto s, auto v) { return k(s, v); });
}

#endif
