// Copyright (C) 2022 Mathieu Dutour Sikiric <mathieu.dutour@gmail.com>
#ifndef SRC_DUALDESC_POLY_RECURSIVEDUALDESC_MPI_H_
#define SRC_DUALDESC_POLY_RECURSIVEDUALDESC_MPI_H_

#include "POLY_RecursiveDualDesc.h"
#include "MPI_functionality.h"
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
/*
template <typename Tbank, typename T, typename Tgroup, typename Tidx_value>
vectface MPI_DUALDESC_AdjacencyDecomposition_General(
    Tbank &TheBank, MyMatrix<T> const &EXT, Tgroup const &GRP,
    boost::mpi::communicator &comm,
    PolyHeuristicSerial<typename Tgroup::Tint> const &AllArr,
    std::string const &ePrefix) {
  //  using Tgr = GraphListAdj;
  using Tint = typename Tgroup::Tint;
  int irank = comm.rank();
  int size = comm.size();
  // New Facets (processed asynchronously)
  const int tag_new_facets =
      36; // New facets to be added, the most common request
  // Message query being received
  const int tag_message_query = 37;
  // undone information for Balinski termination
  const int tag_nbundone_balinski = 38;
  // undone information for Balinski termination
  const int tag_terminate_send_vf = 38;
  // undone information for Balinski termination
  const int tag_setup_databank = 39;

  using TbasicBank = DatabaseCanonic<T, Tint, Tgroup>;
  //  using DataFacet = typename TbasicBank::DataFacet;
  struct BigRecordEntry {
    std::vector<size_t> subset_index_proc;
    DatabaseOrbits<TbasicBank> databank;
    bool did_something;
    std::vector<UndoneOrbitInfo<Tint>> list_undoneinfo;
    int initiating_proc;
  };

  struct SetupDatabank {
    MyMatrix<T> EXT;
    Tgroup GRP;
    size_t e_hash;
  };

  // We can have DatabseBank created on disjoint processes.
  // The order will not be the same between processors.
  std::vector<BigRecordEntry> ListRPL;
  TbasicBank bb(EXT, GRP);
  ListRPL.emplace_back({
      get_subset_index_rev(EXT.rows()),
      DatabaseOrbits<TbasicBank>(bb, ePrefix, AllArr.Saving, std::cerr), false,
      std::vector<UndoneOrbitInfo<Tint>>(size,
                                         get_default_undoneinfo(EXT.rows())),
      -1 // no initiating for the main one
  });
  std::unordered_map<size_t, uint8_t>
      map_databank; // mapping from the hash of database orbit to the index in
                    // ListRPL
  std::vector<size_t> map_databank_rev; // mapping from index of ListRPL to hash
  map_databank[0] = 0;
  map_databank_rev.push_back(0);
  uint8_t selected_pos = 0;
  size_t selected_hash = 0;
  auto get_pos = [&](auto &x) -> uint8_t { return map_databank[x.e_hash]; };
  auto set_selected_pos = [&]() -> void {
    int min_dim = std::numeric_limits<int>::max();
    for (size_t i = 0; i < ListRPL.size(); i++) {
      int dim = ListRPL[i].databank.EXT.cols();
      if (dim < min_dim) {
        selected_pos = i;
        min_dim = dim;
      }
    }
    selected_hash = map_databank_rev[selected_pos];
  };
  auto remove_databank = [&](const size_t &e_hash, const uint8_t &pos) -> void {
    // Correct ListRPL
    ListRPL.erase(pos);
    // Correct map_databank
    map_databank.erase(e_hash);
    for (auto &kv : map_databank) {
      if (kv.second > pos)
        map_databank[kv.first] = kv.second - 1;
    }
    // Correct map_databank
    for (size_t i = pos + 1; i < map_databank_rev.size(); i++)
      map_databank_rev[i - 1] = map_databank_rev[i];
    map_databank_rev.pop_back();
    // Recomputed working_pos
    set_selected_pos();
  };
  auto insert_databank = [&](int init_proc,
                             const SetupDatabank &setup_db) -> void {
    // Update ListRPL
    TbasicBank bb(setup_db.EXT,
                  setup_db.GRP); // Bugged as we are passing a reference to a
                                 // temporary object.
    ListRPL.emplace_back(
        {get_subset_index_rev(setup_db.EXT.rows()),
         DatabaseOrbits<TbasicBank>(bb, ePrefix, AllArr.Saving, std::cerr),
         false,
         std::vector<UndoneOrbitInfo<Tint>>(
             size, get_default_undoneinfo(setup_db.EXT.rows())),
         init_proc});
    uint8_t pos = ListRPL.size();
    // map_databank
    map_databank[setup_db.e_hash] = pos;
    // map_databank_rev
    map_databank_rev.push_back(setup_db.e_hash);
    // Recomputed working_pos
    set_selected_pos();
  };
  // The Buffers in output and receive
  std::vector<message_facet> ListEntries_OUT(size);
  std::vector<message_facet> ListEntries_IN;
  // The queries
  std::vector<std::pair<int, message_query>> ListMesgQuery;
  // The entry nbundone
  std::vector<std::pair<int, HashUndoneOrbitInfo<Tint>>> ListMesgUndone;
  // The partial dual desc (at the end of computation)
  std::vector<std::pair<int, message_facet>> ListMesgPartDualDesc;
  // The bank setup
  std::vector<std::pair<int, SetupDatabank>> ListSetupDatabank;
  // The MPI related stuff
  std::vector<boost::mpi::request> ListRequest;
  std::vector<int> RequestStatus;
  auto GetFreeIndex = [&]() -> size_t {
    size_t len = RequestStatus.size();
    for (size_t i = 0; i < len; i++) {
      if (RequestStatus[i] == 1) {
        boost::optional<boost::mpi::status> stat = ListRequest[i].test();
        if (stat) { // that request has ended. Let's read it.
          RequestStatus[i] = 0;
        }
      }
    }
    for (size_t i = 0; i < len; i++)
      if (RequestStatus[i] == 0)
        return i;
    ListRequest.push_back(boost::mpi::request());
    RequestStatus.push_back(1);
    return len;
  };
  // The infinite loop to do the enumeration
  while (true) {
    boost::optional<boost::mpi::status> prob = comm.iprobe();
    if (prob) {
      // We only do data reception processing is done afterwards
      if (prob->tag() == tag_new_facets) {
        message_facet e_mesg;
        comm.recv(prob->source(), prob->tag(), e_mesg);
        ListEntries_IN.push_back(e_mesg);
      }
      if (prob->tag() == tag_message_query) {
        message_query e_mesg;
        comm.recv(prob->source(), prob->tag(), e_mesg);
        ListMesgQuery.push_back({prob->source(), e_mesg});
      }
      if (prob->tag() == tag_nbundone_balinski) {
        HashUndoneOrbitInfo<Tint> e_mesg;
        comm.recv(prob->source(), prob->tag(), e_mesg);
        ListMesgUndone.push_back({prob->source(), e_mesg});
      }
      if (prob->tag() == tag_terminate_send_vf) {
        message_facet e_mesg;
        comm.recv(prob->source(), prob->tag(), e_mesg);
        ListMesgPartDualDesc.push_back({prob->source(), std::move(e_mesg)});
      }
      if (prob->tag() == tag_setup_databank) {
        SetupDatabank e_mesg;
        comm.recv(prob->source(), prob->tag(), e_mesg);
        ListSetupDatabank.push_back({prob->source(), std::move(e_mesg)});
      }
    } else {
      // First clearing the buffers of facets
      for (auto &eEntry_IN : ListEntries_IN) {
        uint8_t pos = get_pos(eEntry_IN);
        for (auto &eFace : eEntry_IN.vf)
          ListRPL[pos].databank.FuncInsert(eFace);
      }
      ListEntries_IN.clear();
      // Processing the message queries
      for (auto &eMesgQuery : ListMesgQuery) {
        uint8_t pos = get_pos(eMesgQuery.second);
        if (eMesgQuery.second.query == 0) {
          UndoneOrbitInfo<Tint> recundone =
              ListRPL[pos].databank.GetTerminationInfo();
          HashUndoneOrbitInfo<Tint> hashrecundone{eMesgQuery.second.e_hash,
                                                  recundone};
          size_t u = GetFreeIndex();
          ListRequest[u] = comm.isend(0, tag_nbundone_balinski, hashrecundone);
          RequestStatus[u] = 1;
        }
        if (eMesgQuery.second.query == 1) {
          message_facet e_mesg{eMesgQuery.second.e_hash,
                               ListRPL[pos].databank.FuncListOrbitIncidence()};
          size_t u = GetFreeIndex();
          ListRequest[u] = comm.isend(0, tag_terminate_send_vf, e_mesg);
          RequestStatus[u] = 1;
          remove_databank(eMesgQuery.second.e_hash, pos);
        }
      }
      ListMesgQuery.clear();
      // Update information regarding the undone status of orbits
      for (auto &e_ent : ListMesgUndone) {
        int jrank = e_ent.first;
        uint8_t pos = get_pos(e_ent.second);
        ListRPL[pos].list_undoneinfo[jrank] = e_ent.second.erec;
      }
      ListMesgUndone.clear();
      // Create new databanks as required
      for (auto &e_ent : ListSetupDatabank) {
        int jrank = e_ent.first;
        insert_databank(jrank, e_ent.second);
      }
      ListSetupDatabank.clear();
      // Now treating the selected_block
      if (ListRPL[selected_pos].did_something) {
        ListRPL[selected_pos].did_something = false;
        if (irank == 0) {
          ListRPL[selected_pos].list_undoneinfo[0] =
              ListRPL[selected_pos].databank.GetTerminationInfo();
          if (MonotonicCheckStatusUndone(
                  ListRPL[selected_pos].list_undoneinfo[0],
                  ListRPL[selected_pos].databank.CritSiz)) {
            UndoneOrbitInfo<Tint> undoneinfo =
                CombineUndoneOrbitInfo(ListRPL[selected_pos].list_undoneinfo);
            if (ComputeStatusUndone(undoneinfo)) {
              if (selected_pos ==
                  0) { // This is the main databank. Work is finished
                break;
              } else { // Not the main one, more work needed. Relocation to the
                       // initiating processor
              }
            } else {
              message_query mq{selected_hash, 0};
              for (int jrank = 1; jrank < size; jrank++) {
                size_t u = GetFreeIndex();
                ListRequest[u] = comm.isend(0, tag_message_query, mq);
                RequestStatus[u] = 1;
              }
            }
          }
        }
      } else {
        ListRPL[selected_pos].did_something = true;
      }
    }
  }
  return ListRPL[0].FuncListOrbitIncidence();
}
*/








/*
  This is the code for the MPI parallelization.
  Since the first attempt (see above) was maybe too complicated.
  --
  Designs:
  ---At the beginning, read the existing databases.
     ---Compute the total number of existing entries with a mpi.allreduce.
     ---If zero, then all the process do sampling and insert into databases.
     ---If not zero, read it and start.
  ---A needed signal of balinski termination has to be computed from time to time.
  ---A termination check (from runtime or Ctrl-C) has to be handled.
  ---Computation of existing with a call to the serial code. Should be modelled
  on the C-type code.


 */
template <typename Tbank, typename T, typename Tgroup, typename Tidx_value>
vectface MPI_Kernel_DUALDESC_AdjacencyDecomposition(
    Tbank &TheBank, TbasicBank &bb,
    PolyHeuristicSerial<typename Tgroup::Tint> const &AllArr,
    boost::mpi::communicator &comm,
    std::string const &ePrefix,
    std::map<std::string, typename Tgroup::Tint> const &TheMap) {
  using DataFacet = typename TbasicBank::DataFacet;
  std::ostream& os = get_standard_outstream(comm);
  DatabaseOrbits<TbasicBank> RPL(bb, ePrefix, AllArr.Saving, os);


  int MaxNumberFlyingMessage =
      BlDATA.ListIntValues.at("MaxNumberFlyingMessage");
  int MaxIncidenceTreating = BlDATA.ListIntValues.at("MaxIncidenceTreating");
  int MaxStoredUnsentMatrices =
      BlDATA.ListIntValues.at("MaxStoredUnsentMatrices");
  int MinIncidenceRealized = BlDATA.ListIntValues.at("MinIncidenceRealized");
  int MaxIncidenceRealized = BlDATA.ListIntValues.at("MaxIncidenceRealized");
  int MaxRunTimeSecond = BlDATA.ListIntValues.at("MaxRunTimeSecond");


  SingletonTime start;
  int irank = comm.rank();

  size_t n_orb_tot = 0, n_orb_loc = RPL.FuncNumberOrbit();
  all_reduce(world, n_orb_loc, n_orb_tot, mpi::minimum<size_t>());
  if (n_orb_tot > 0) {
    std::string ansSamp = HeuristicEvaluation(TheMap, AllArr.InitialFacetSet);
    for (auto &face : RPL.ComputeInitialSet(ansSamp))
      RPL.FuncInsert(face);
  }



  std::vector<boost::mpi::request> ListRequest(MaxNumberFlyingMessage);
  std::vector<int> RequestStatus(MaxNumberFlyingMessage, 0);
  auto GetFreeIndex = [&]() -> int {
    for (int u = 0; u < MaxNumberFlyingMessage; u++) {
      if (RequestStatus[u] == 0)
        return u;
      boost::optional<boost::mpi::status> stat = ListRequest[u].test();
      if (stat) { // that request has ended. Let's read it.
        if (stat->error() != 0) {
          std::cerr << "something went wrong in the MPI" << std::endl;
          throw TerminalException{1};
        }
        RequestStatus[u] = 0;
        return u;
      }
    }
    return -1;
  };

    int nbCaseIncidence = MaxIncidenceRealized + 1 - MinIncidenceRealized;
  std::vector<std::unordered_map<TypePerfectExch<Tint>, KeyData>>
      ListCasesNotDone(nbCaseIncidence);
  std::unordered_map<TypePerfectExch<Tint>, KeyData> ListCasesDone;
  int idxMatrixCurrent = 0;
  auto fInsert = [&](PairExch<Tint> const &ePair) -> void {
    TypePerfectExch<Tint> ePerfect = ePair.ePerfect;
    auto it1 = ListCasesDone.find(ePerfect);
    if (it1 != ListCasesDone.end()) {
      return;
    }
    int pos = ePerfect.incd - MinIncidenceRealized;
    auto it2 = ListCasesNotDone[pos].find(ePerfect);
    if (it2 != ListCasesNotDone[pos].end()) {
      return;
    }
    ListCasesNotDone[pos][ePerfect] = {idxMatrixCurrent};
    idxMatrixCurrent++;
  };
  

  auto GetLowestIncidenceUndone =
      [&]() -> boost::optional<std::pair<TypePerfectExch<Tint>, int>> {
    for (int iCaseIncidence = 0; iCaseIncidence < nbCaseIncidence;
         iCaseIncidence++) {
      int incd = iCaseIncidence + MinIncidenceRealized;
      if (incd <= MaxIncidenceTreating) {
        auto it1 = ListCasesNotDone[iCaseIncidence].begin();
        if (it1 != ListCasesNotDone[iCaseIncidence].end()) {
          std::pair<TypePerfectExch<Tint>, int> ePair = {it1->first,
                                                         it1->second.idxMatrix};
          return boost::optional<std::pair<TypePerfectExch<Tint>, int>>(ePair);
        }
      }
    }
    return {};
  };
  auto SetMatrixAsDone = [&](TypePerfectExch<Tint> const &TheMat) -> void {
    int pos = TheMat.incd - MinIncidenceRealized;
    KeyData eKey = ListCasesNotDone[pos].at(TheMat);
    ListCasesNotDone[pos].erase(TheMat);
    ListCasesDone[TheMat] = eKey;
  };
  
  auto fSendMatrix = [&](PairExch<Tint> const &ePair, int const &u) -> void {
    int res = IntegerDiscriminantInvariant(ePair.ePerfect.eMat, size);
    ListRequest[u] = world.isend(res, tag_new_form, ePair);
    RequestStatus[u] = 1;
  };
  std::vector<PairExch<Tint>> ListMatrixUnsent;
  auto ClearUnsentAsPossible = [&]() -> void {
    int pos = ListMatrixUnsent.size() - 1;
    while (true) {
      if (pos == -1)
        break;
      int idx = GetFreeIndex();
      if (idx == -1)
        break;
      fSendMatrix(ListMatrixUnsent[pos], idx);
      ListMatrixUnsent.pop_back();
      pos--;
    }
  };
  auto fInsertUnsent = [&](PairExch<Tint> const &ePair) -> void {
    int res = IntegerDiscriminantInvariant(ePair.ePerfect.eMat, size);
    if (res == irank) {
      fInsert(ePair);
    } else {
      ListMatrixUnsent.push_back(ePair);
      ClearUnsentAsPossible();
    }
  };
  
  
  
  int nbCaseNotDone = 0;
  for (int iMatStart = 0; iMatStart < nbMatrixStart; iMatStart++) {
    int eStatus;
    is >> eStatus;
    int incd;
    is >> incd;
    MyMatrix<Tint> TheMat = ReadMatrix<Tint>(is);
    TypePerfectExch<Tint> eRecMat{incd, TheMat};
    int res = IntegerDiscriminantInvariant(TheMat, size);
    if (res == irank) {
      KeyData eData{idxMatrixCurrent};
      if (eStatus == 0) {
        int pos = incd - MinIncidenceRealized;
        ListCasesNotDone[pos][eRecMat] = eData;
        nbCaseNotDone++;
      } else {
        ListCasesDone[eRecMat] = eData;
      }
      log << "Reading existing matrix=" << eRecMat
          << " idxMatrixCurrent=" << idxMatrixCurrent << "END" << std::endl;
      idxMatrixCurrent++;
    }
  }
  
  while (true) {
    boost::optional<boost::mpi::status> prob = world.iprobe();
    if (prob) {
      std::cerr << "We are probing something\n";
      if (prob->tag() == tag_new_form) {
        PairExch<Tint> ePair;
        world.recv(prob->source(), prob->tag(), ePair);
        fInsert(ePair);
      }
    } else {
      std::cerr << "irank=" << irank
                << " |ListMatrixUnsent|=" << ListMatrixUnsent.size()
                << " MaxStoredUnsentMatrices=" << MaxStoredUnsentMatrices
                << "\n";
      if (int(ListMatrixUnsent.size()) < MaxStoredUnsentMatrices) {
        boost::optional<std::pair<TypePerfectExch<Tint>, int>> eReq =
            GetLowestIncidenceUndone();
        if (eReq) {
          std::cerr << "irank=" << irank << " eReq is non zero\n";
          SetMatrixAsDone(eReq->first);
          std::cerr << "irank=" << irank << " ePerfect=" << eReq->first << "\n";
          MyMatrix<T> eMat_T =
              UniversalMatrixConversion<T, Tint>(eReq->first.eMat);
          int idxMatrixF = eReq->second;
          std::cerr << "irank=" << irank << " Starting Adjacent Form Method\n";
          std::vector<TypePerfectExch<Tint>> ListAdjacentObject =
              GetAdjacentObjects<T, Tint>(eReq->first);
          int nbAdjacent = ListAdjacentObject.size();
          log << "Number of Adjacent for idxMatrixF=" << idxMatrixF
              << " nbAdjacent=" << nbAdjacent << " END" << std::endl;
          std::cerr << "irank=" << irank
                    << " Number of Adjacent for idxMatrixF=" << idxMatrixF
                    << " nbAdjacent=" << nbAdjacent << " END\n";
          int iAdj = 0;
          for (auto &eObj1 : ListAdjacentObject) {
            TypeIndex eIndex{irank, idxMatrixF, iAdj};
            PairExch<Tint> ePair{eObj1, eIndex};
            fInsertUnsent(ePair);
            iAdj++;
          }
        }
      }
    }
    os << "Before ClearUnsentAsPossible\n";
    ClearUnsentAsPossible();
    os << "After ClearUnsentAsPossible\n";
    int elapsed_seconds = si(start);
    if (MaxRunTimeSecond > 0) {
      if (elapsed_seconds > MaxRunTimeSecond) {
        os << "Exiting because the runtime is higher than the one expected\n";
        break;
      }
    }
  }



  
  using Tint = typename Tgroup::Tint;
  int irank = comm.rank();
  int size = comm.size();
  // New Facets (processed asynchronously)
  const int tag_new_facets =
      36; // New facets to be added, the most common request
  // Message query being received
  const int tag_message_query = 37;
  // undone information for Balinski termination
  const int tag_balinski_info = 38;
  // undone information for Balinski termination
  const int tag_terminate_send_vf = 38;
  // undone information for Balinski termination
  const int tag_setup_databank = 39;

  using TbasicBank = DatabaseCanonic<T, Tint, Tgroup>;
  //  using DataFacet = typename TbasicBank::DataFacet;
  struct BigRecordEntry {
    std::vector<size_t> subset_index_proc;
    DatabaseOrbits<TbasicBank> databank;
    bool did_something;
    std::vector<UndoneOrbitInfo<Tint>> list_undoneinfo;
    int initiating_proc;
  };

  struct SetupDatabank {
    MyMatrix<T> EXT;
    Tgroup GRP;
    size_t e_hash;
  };





  
  // We can have DatabseBank created on disjoint processes.
  // The order will not be the same between processors.
  std::vector<BigRecordEntry> ListRPL;
  TbasicBank bb(EXT, GRP);
  ListRPL.emplace_back({
      get_subset_index_rev(EXT.rows()),
      DatabaseOrbits<TbasicBank>(bb, ePrefix, AllArr.Saving, std::cerr), false,
      std::vector<UndoneOrbitInfo<Tint>>(size,
                                         get_default_undoneinfo(EXT.rows())),
      -1 // no initiating for the main one
  });
  std::unordered_map<size_t, uint8_t>
      map_databank; // mapping from the hash of database orbit to the index in
                    // ListRPL
  std::vector<size_t> map_databank_rev; // mapping from index of ListRPL to hash
  map_databank[0] = 0;
  map_databank_rev.push_back(0);
  uint8_t selected_pos = 0;
  size_t selected_hash = 0;
  auto get_pos = [&](auto &x) -> uint8_t { return map_databank[x.e_hash]; };
  auto set_selected_pos = [&]() -> void {
    int min_dim = std::numeric_limits<int>::max();
    for (size_t i = 0; i < ListRPL.size(); i++) {
      int dim = ListRPL[i].databank.EXT.cols();
      if (dim < min_dim) {
        selected_pos = i;
        min_dim = dim;
      }
    }
    selected_hash = map_databank_rev[selected_pos];
  };
  auto remove_databank = [&](const size_t &e_hash, const uint8_t &pos) -> void {
    // Correct ListRPL
    ListRPL.erase(pos);
    // Correct map_databank
    map_databank.erase(e_hash);
    for (auto &kv : map_databank) {
      if (kv.second > pos)
        map_databank[kv.first] = kv.second - 1;
    }
    // Correct map_databank
    for (size_t i = pos + 1; i < map_databank_rev.size(); i++)
      map_databank_rev[i - 1] = map_databank_rev[i];
    map_databank_rev.pop_back();
    // Recomputed working_pos
    set_selected_pos();
  };
  auto insert_databank = [&](int init_proc,
                             const SetupDatabank &setup_db) -> void {
    // Update ListRPL
    TbasicBank bb(setup_db.EXT,
                  setup_db.GRP); // Bugged as we are passing a reference to a
                                 // temporary object.
    ListRPL.emplace_back(
        {get_subset_index_rev(setup_db.EXT.rows()),
         DatabaseOrbits<TbasicBank>(bb, ePrefix, AllArr.Saving, std::cerr),
         false,
         std::vector<UndoneOrbitInfo<Tint>>(
             size, get_default_undoneinfo(setup_db.EXT.rows())),
         init_proc});
    uint8_t pos = ListRPL.size();
    // map_databank
    map_databank[setup_db.e_hash] = pos;
    // map_databank_rev
    map_databank_rev.push_back(setup_db.e_hash);
    // Recomputed working_pos
    set_selected_pos();
  };
  // The Buffers in output and receive
  std::vector<message_facet> ListEntries_OUT(size);
  std::vector<message_facet> ListEntries_IN;
  // The queries
  std::vector<std::pair<int, message_query>> ListMesgQuery;
  // The entry nbundone
  std::vector<std::pair<int, HashUndoneOrbitInfo<Tint>>> ListMesgUndone;
  // The partial dual desc (at the end of computation)
  std::vector<std::pair<int, message_facet>> ListMesgPartDualDesc;
  // The bank setup
  std::vector<std::pair<int, SetupDatabank>> ListSetupDatabank;
  // The MPI related stuff
  std::vector<boost::mpi::request> ListRequest;
  std::vector<int> RequestStatus;
  auto GetFreeIndex = [&]() -> size_t {
    size_t len = RequestStatus.size();
    for (size_t i = 0; i < len; i++) {
      if (RequestStatus[i] == 1) {
        boost::optional<boost::mpi::status> stat = ListRequest[i].test();
        if (stat) { // that request has ended. Let's read it.
          RequestStatus[i] = 0;
        }
      }
    }
    for (size_t i = 0; i < len; i++)
      if (RequestStatus[i] == 0)
        return i;
    ListRequest.push_back(boost::mpi::request());
    RequestStatus.push_back(1);
    return len;
  };
  // The infinite loop to do the enumeration
  while (true) {
    boost::optional<boost::mpi::status> prob = comm.iprobe();
    if (prob) {
      // We only do data reception processing is done afterwards
      if (prob->tag() == tag_new_facets) {
        message_facet e_mesg;
        comm.recv(prob->source(), prob->tag(), e_mesg);
        ListEntries_IN.push_back(e_mesg);
      }
      if (prob->tag() == tag_message_query) {
        message_query e_mesg;
        comm.recv(prob->source(), prob->tag(), e_mesg);
        ListMesgQuery.push_back({prob->source(), e_mesg});
      }
      if (prob->tag() == tag_nbundone_balinski) {
        HashUndoneOrbitInfo<Tint> e_mesg;
        comm.recv(prob->source(), prob->tag(), e_mesg);
        ListMesgUndone.push_back({prob->source(), e_mesg});
      }
      if (prob->tag() == tag_terminate_send_vf) {
        message_facet e_mesg;
        comm.recv(prob->source(), prob->tag(), e_mesg);
        ListMesgPartDualDesc.push_back({prob->source(), std::move(e_mesg)});
      }
      if (prob->tag() == tag_setup_databank) {
        SetupDatabank e_mesg;
        comm.recv(prob->source(), prob->tag(), e_mesg);
        ListSetupDatabank.push_back({prob->source(), std::move(e_mesg)});
      }
    } else {
      // First clearing the buffers of facets
      for (auto &eEntry_IN : ListEntries_IN) {
        uint8_t pos = get_pos(eEntry_IN);
        for (auto &eFace : eEntry_IN.vf)
          ListRPL[pos].databank.FuncInsert(eFace);
      }
      ListEntries_IN.clear();
      // Processing the message queries
      for (auto &eMesgQuery : ListMesgQuery) {
        uint8_t pos = get_pos(eMesgQuery.second);
        if (eMesgQuery.second.query == 0) {
          UndoneOrbitInfo<Tint> recundone =
              ListRPL[pos].databank.GetTerminationInfo();
          HashUndoneOrbitInfo<Tint> hashrecundone{eMesgQuery.second.e_hash,
                                                  recundone};
          size_t u = GetFreeIndex();
          ListRequest[u] = comm.isend(0, tag_nbundone_balinski, hashrecundone);
          RequestStatus[u] = 1;
        }
        if (eMesgQuery.second.query == 1) {
          message_facet e_mesg{eMesgQuery.second.e_hash,
                               ListRPL[pos].databank.FuncListOrbitIncidence()};
          size_t u = GetFreeIndex();
          ListRequest[u] = comm.isend(0, tag_terminate_send_vf, e_mesg);
          RequestStatus[u] = 1;
          remove_databank(eMesgQuery.second.e_hash, pos);
        }
      }
      ListMesgQuery.clear();
      // Update information regarding the undone status of orbits
      for (auto &e_ent : ListMesgUndone) {
        int jrank = e_ent.first;
        uint8_t pos = get_pos(e_ent.second);
        ListRPL[pos].list_undoneinfo[jrank] = e_ent.second.erec;
      }
      ListMesgUndone.clear();
      // Create new databanks as required
      for (auto &e_ent : ListSetupDatabank) {
        int jrank = e_ent.first;
        insert_databank(jrank, e_ent.second);
      }
      ListSetupDatabank.clear();
      // Now treating the selected_block
      if (ListRPL[selected_pos].did_something) {
        ListRPL[selected_pos].did_something = false;
        if (irank == 0) {
          ListRPL[selected_pos].list_undoneinfo[0] =
              ListRPL[selected_pos].databank.GetTerminationInfo();
          if (MonotonicCheckStatusUndone(
                  ListRPL[selected_pos].list_undoneinfo[0],
                  ListRPL[selected_pos].databank.CritSiz)) {
            UndoneOrbitInfo<Tint> undoneinfo =
                CombineUndoneOrbitInfo(ListRPL[selected_pos].list_undoneinfo);
            if (ComputeStatusUndone(undoneinfo)) {
              if (selected_pos ==
                  0) { // This is the main databank. Work is finished
                break;
              } else { // Not the main one, more work needed. Relocation to the
                       // initiating processor
              }
            } else {
              message_query mq{selected_hash, 0};
              for (int jrank = 1; jrank < size; jrank++) {
                size_t u = GetFreeIndex();
                ListRequest[u] = comm.isend(0, tag_message_query, mq);
                RequestStatus[u] = 1;
              }
            }
          }
        }
      } else {
        ListRPL[selected_pos].did_something = true;
      }
    }
  }
  return ListRPL[0].FuncListOrbitIncidence();
}



// clang-format off
#endif  // SRC_DUALDESC_POLY_RECURSIVEDUALDESC_MPI_H_
// clang-format on
