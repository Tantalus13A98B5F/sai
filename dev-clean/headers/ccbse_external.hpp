#include <ccbse.hpp>

inline immer::flex_vector<std::pair<SS, PtrVal>> sym_print(SS state, immer::flex_vector<PtrVal> args) {
  PtrVal x = args.at(0);
  if (std::dynamic_pointer_cast<FloatV>(x)) {
    std::cout << "FloatV" << std::dynamic_pointer_cast<FloatV>(x)->f << ")\n";
  } else if (std::dynamic_pointer_cast<IntV>(x)) {
    std::cout << "IntV(" << std::dynamic_pointer_cast<IntV>(x)->i << ")\n";
  } else if (std::dynamic_pointer_cast<LocV>(x)){
    ABORT("Unimplemented LOCV");
  } else if ( x == nullptr ){
    ABORT("Unimplemented nullptr");
  }
  return immer::flex_vector<std::pair<SS, PtrVal>>{{state, make_IntV(0)}};
}

inline immer::flex_vector<std::pair<SS, PtrVal>> noop(SS state, immer::flex_vector<PtrVal> args) {
  return immer::flex_vector<std::pair<SS, PtrVal>>{{state, make_IntV(0)}};
}

inline immer::flex_vector<std::pair<SS, PtrVal>> malloc(SS state, immer::flex_vector<PtrVal> args) {
  IntData bytes = proj_IntV(args.at(0));
  auto emptyMem = immer::flex_vector<PtrVal>(bytes, nullptr);
  PtrVal memLoc = make_LocV(state.heap_size(), LocV::kHeap, bytes);
  return immer::flex_vector<std::pair<SS, PtrVal>>{{state.heap_append(emptyMem), memLoc}};
}

inline immer::flex_vector<std::pair<SS, PtrVal>> realloc(SS state, immer::flex_vector<PtrVal> args) {
  Addr src = proj_LocV(args.at(0));
  IntData bytes = proj_IntV(args.at(1));

  auto emptyMem = immer::flex_vector<PtrVal>(bytes, nullptr);
  std::cout << "realloc size: " << emptyMem.size() << std::endl;
  PtrVal memLoc = make_LocV(state.heap_size(), LocV::kHeap, bytes);
  IntData prevBytes = proj_LocV_size(args.at(0));
  std::cout << "prev size: " << prevBytes << std::endl;
  SS res = state.heap_append(emptyMem);
  for (int i = 0; i < prevBytes; i++) {
    res = res.update(make_LocV_inc(memLoc, i), res.heap_lookup(src + i));
  }
  return immer::flex_vector<std::pair<SS, PtrVal>>{{res, memLoc}};
}

inline void handle_pc(immer::set<SExpr> pc) {

}

inline immer::flex_vector<std::pair<SS, PtrVal>> llsc_assert(SS state, immer::flex_vector<PtrVal> args) {
  immer::set<SExpr> pc = state.getPC();
  handle_pc(pc);
  return immer::flex_vector<std::pair<SS, PtrVal>>{{state, make_IntV(0)}};
}

inline immer::flex_vector<std::pair<SS, PtrVal>> make_symbolic(SS state, immer::flex_vector<PtrVal> args) {
  PtrVal make_loc = args.at(0);
  IntData len = proj_IntV(args.at(1));
  SS res = state;
  //std::cout << "sym array size: " << proj_LocV_size(make_loc) << "\n";
  for (int i = 0; i < len; i++) {
    res = res.update(make_LocV_inc(make_loc, i), make_SymV("x" + std::to_string(var_name++)));
  }
  return immer::flex_vector<std::pair<SS, PtrVal>>{{res, make_IntV(0)}};
}

inline immer::flex_vector<std::pair<SS, PtrVal>> __assert_fail(SS state, immer::flex_vector<PtrVal> args) {
  // TODO get real argument string
  // std::cout << "Fail: Calling to __assert_fail" << std::endl;
  return immer::flex_vector<std::pair<SS, PtrVal>>{{state, make_IntV(0)}};
}

inline immer::flex_vector<std::pair<SS, PtrVal>> target(SS state, immer::flex_vector<PtrVal> args) {
  // print_pc(state.getPC());
  if (state.is_from_main()) {
    if (check_pc(state.getPC())) {
      std::cout << "Target reached from main, exit." << std::endl;
      cov.print_query_num();
      check_pc_to_file(state);
      exit(0);
    }
  }
  return immer::flex_vector<std::pair<SS, PtrVal>>{{state.set_target(), make_IntV(0)}};
}
