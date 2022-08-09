// Copyright (C) 2022 Mathieu Dutour Sikiric <mathieu.dutour@gmail.com>
#ifndef SRC_DUALDESC_POLY_RECURSIVEDUALDESC_MPI_H_
#define SRC_DUALDESC_POLY_RECURSIVEDUALDESC_MPI_H_

#include "POLY_RecursiveDualDesc.h"
#include "MPI_functionality.h"
#include "Balinski_basic.h"
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <thread>
#include <chrono>

struct message_facet {
  size_t e_hash;
  vectface vf; // List of vectface by the DatabaseBank
};

struct message_query {
  size_t e_hash;
  uint8_t query;
  // 0: for TerminationInfo
  // 1: For destroying the databank and sending back the vectface
};


namespace boost::serialization {

template <class Archive>
inline void serialize(Archive &ar, message_facet &mesg,
                      [[maybe_unused]] const unsigned int version) {
  ar &make_nvp("hash", mesg.e_hash);
  ar &make_nvp("vf", mesg.vf);
}

template <class Archive>
inline void serialize(Archive &ar, message_query &mesg,
                      [[maybe_unused]] const unsigned int version) {
  ar &make_nvp("hash", mesg.e_hash);
  ar &make_nvp("query", mesg.query);
}

} // namespace boost::serialization






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
template <typename Tbank, typename TbasicBank, typename T, typename Tgroup, typename Tidx_value>
vectface MPI_Kernel_DUALDESC_AdjacencyDecomposition(
    boost::mpi::communicator &comm,
    Tbank &TheBank, TbasicBank &bb,
    PolyHeuristicSerial<typename Tgroup::Tint> &AllArr,
    std::string const &ePrefix,
    std::map<std::string, typename Tgroup::Tint> const &TheMap, std::ostream& os) {
  using DataFacet = typename TbasicBank::DataFacet;
  using Tint = typename TbasicBank::Tint;
  SingletonTime start;
  int i_rank = comm.rank();
  int n_proc = comm.size();
  std::string lPrefix = ePrefix + std::to_string(n_proc) + "_" + std::to_string(i_rank);
  DatabaseOrbits<TbasicBank> RPL(bb, lPrefix, AllArr.Saving, AllArr.AdvancedTerminationCriterion, os);
  Tint CritSiz = RPL.CritSiz;
  UndoneOrbitInfo<Tint> uoi_local;
  os << "DirectFacetOrbitComputation, step 1\n";
  int n_vert = bb.nbRow;
  int n_vert_div8 = (n_vert + 7) / 8;
  bool HasReachedRuntimeException = false;
  std::vector<uint8_t> V_hash(n_vert_div8,0);
  auto get_hash=[&](Face const& x) -> size_t {
    for (int i_vert=0; i_vert<n_vert; i_vert++) {
      setbit(V_hash, i_vert, x[i_vert]);
    }
    uint32_t seed = 0x1b873560;
    return robin_hood_hash_bytes(V_hash.data(), n_vert_div8, seed);
  };
  if (AllArr.max_runtime < 0) {
    std::cerr << "The MPI version requires a strictly positive runtime\n";
    throw TerminalException{1};
  }
  //
  // The types of exchanges
  //
  // New facets to be added, the most common request
  const int tag_new_facets = 36;
  // undone information for Balinski termination
  const int tag_termination = 38;
  //
  // Reading the input
  //
  size_t MaxBuffered = 10000 * n_proc;
  int MaxFly = 4 * n_proc;
  //
  // The parallel MPI classes
  //
  os << "DirectFacetOrbitComputation, step 2\n";
  empty_message_management emm_termin(comm, 0, tag_termination);
  os << "DirectFacetOrbitComputation, step 3\n";
  buffered_T_exchanges<Face,vectface> bte_facet(comm, MaxFly, tag_new_facets);
  os << "DirectFacetOrbitComputation, step 4\n";

  std::vector<int> StatusNeighbors(n_proc, 0);
  auto fInsertUnsent = [&](Face const &face) -> void {
    int res = int(get_hash(face) % size_t(n_proc));
    os << "|face|=" << face.count() << " res=" << res << "\n";
    if (res == i_rank) {
      RPL.FuncInsert(face);
    } else {
      bte_facet.insert_entry(res, face);
    }
  };
  //
  // Initial invocation of the synchronization code
  //
  os << "Compute initital\n";
  size_t n_orb_tot = 0, n_orb_loc = RPL.FuncNumberOrbit();
  all_reduce(comm, n_orb_loc, n_orb_tot, boost::mpi::maximum<size_t>());
  os << "n_orb_loc=" << n_orb_loc << " n_orb_tot=" << n_orb_tot << "\n";
  if (n_orb_tot == 0) {
    std::string ansSamp = HeuristicEvaluation(TheMap, AllArr.InitialFacetSet);
    os << "ansSamp=" << ansSamp << "\n";
    for (auto &face : RPL.ComputeInitialSet(ansSamp, os)) {
      fInsertUnsent(face);
    }
  }
  os << "DirectFacetOrbitComputation, step 6\n";
  //
  // The infinite loop
  //
  auto process_mpi_status=[&](boost::mpi::status const& stat) -> void {
    int e_tag = stat.tag();
    int e_src = stat.source();
    if (e_tag == tag_new_facets) {
      os << "RECV of tag_new_facets from " << e_src << "\n";
      StatusNeighbors[e_src] = 0;
      vectface l_recv_face = bte_facet.recv_message(e_src);
      os << "|l_recv_face|=" << l_recv_face.size() << "\n";
      for (auto & face : l_recv_face)
        RPL.FuncInsert(face);
    }
    if (e_tag == tag_termination) {
      os << "RECV of tag_termination from " << e_src << "\n";
      StatusNeighbors[e_src] = 1;
      emm_termin.recv_message(e_src);
    }
    os << "Exiting process_mpi_status\n";
  };
  auto process_database=[&]() -> void {
    os << "process_database, begin\n";
    DataFacet df = RPL.FuncGetMinimalUndoneOrbit();
    os << "process_database, we have df\n";
    size_t SelectedOrbit = df.SelectedOrbit;
    std::string NewPrefix =
      ePrefix + "PROC" + std::to_string(i_rank) + "_ADM" + std::to_string(SelectedOrbit) + "_";
    try {
      os << "Before call to DUALDESC_AdjacencyDecomposition\n";
      vectface TheOutput =
        DUALDESC_AdjacencyDecomposition<Tbank, T, Tgroup, Tidx_value>(TheBank, df.FF.EXT_face, df.Stab, AllArr, NewPrefix, os);
      os << "We have TheOutput, |TheOutput|=" << TheOutput.size() << "\n";
      for (auto &eOrbB : TheOutput) {
        Face eFlip = df.flip(eOrbB);
        fInsertUnsent(eFlip);
      }
      RPL.FuncPutOrbitAsDone(SelectedOrbit);
    } catch (RuntimeException const &e) {
      HasReachedRuntimeException = true;
      os << "The computation of DUALDESC_AdjacencyDecomposition has ended by runtime exhaustion\n";
    }
    os << "process_database, EXIT\n";
  };
  auto get_maxruntimereached=[&]() -> bool {
    if (HasReachedRuntimeException)
      return true;
    return si(start) > AllArr.max_runtime;
  };
  auto send_termination_notice=[&]() -> void {
    // This function should be passed only one time.
    // It says to the receiving nodes: "You wil never ever received anything more from me"
    os << "Sending messages for terminating the run\n";
    for (int i_proc = 0; i_proc < n_proc; i_proc++) {
      if (i_proc == i_rank) {
        StatusNeighbors[i_rank] = 1;
      } else {
        os << "Before send_message at i_proc=" << i_proc << "\n";
        emm_termin.send_message(i_proc);
        os << "After send_message\n";
      }
    }
  };
  auto get_nb_finished=[&]() -> int {
    int nb_finished = 0;
    for (int i_proc = 0; i_proc < n_proc; i_proc++) {
      os << "get_nb_finished : i_proc=" << i_proc << " status=" << StatusNeighbors[i_proc] << "\n";
      nb_finished += StatusNeighbors[i_proc];
    }
    os << "nb_finished=" << nb_finished << "\n";
    return nb_finished;
  };
  auto get_nb_finished_oth=[&]() -> int {
    int nb_finished_oth = 0;
    for (int i_proc = 0; i_proc < n_proc; i_proc++) {
      if (i_proc != i_rank) {
        os << "get_nb_finished_oth : i_proc=" << i_proc << " status=" << StatusNeighbors[i_proc] << "\n";
        nb_finished_oth += StatusNeighbors[i_proc];
      }
    }
    os << "nb_finished_oth=" << nb_finished_oth << "\n";
    return nb_finished_oth;
  };
  bool HasSendTermination = false;
  while (true) {
    os << "DirectFacetOrbitComputation, inf loop, start\n";
    bool MaxRuntimeReached = get_maxruntimereached();
    if (MaxRuntimeReached && !HasSendTermination) {
      if (bte_facet.get_unsent_size() == 0) {
        HasSendTermination = true;
        send_termination_notice();
      }
    }
    bool SomethingToDo = !MaxRuntimeReached && !RPL.IsFinished();
    os << "DirectFacetOrbitComputation, MaxRuntimeReached=" << MaxRuntimeReached << " SomethingToDo=" << SomethingToDo << "\n";
    boost::optional<boost::mpi::status> prob = comm.iprobe();
    if (prob) {
      os << "prob is not empty\n";
      process_mpi_status(*prob);
    } else {
      if (SomethingToDo) {
        os << "Case something to do\n";
        // we have to clear our buffers sometimes while running
        // otherwise we will only do it in the very end
        // which could lead to extremely large buffers
        // and possibly treating orbits with unnecessary large incidence
        if (bte_facet.get_unsent_size() >= MaxBuffered) {
          os << "Calling clear_one_entry after reaching MaxBuffered\n";
          bte_facet.clear_one_entry(os);
        }
        process_database();
      } else {
        if (!bte_facet.is_buffer_empty()) {
          os << "Calling clear_one_entry\n";
          bte_facet.clear_one_entry(os);
        } else {
          int nb_finished_oth = get_nb_finished_oth();
          os << "Nothing to do, entering the busy loop status=" << get_maxruntimereached() << " get_nb_finished_oth()=" << nb_finished_oth << "\n";
          /*
          if (!get_maxruntimereached() || nb_finished_oth < n_proc - 1) {
            boost::mpi::status stat = comm.probe();
            process_mpi_status(stat);
          }
          */
          while (!get_maxruntimereached()) {
            boost::optional<boost::mpi::status> prob = comm.iprobe();
            if (prob) {
              process_mpi_status(*prob);
              break;
            }
            int n_milliseconds = 1000;
            std::this_thread::sleep_for(std::chrono::milliseconds(n_milliseconds));
          }
        }
      }
    }
    //
    // Termination criterion
    //
    if (get_nb_finished() == n_proc)
      break;
    os << "End of the while loop. From start=" << si(start) << "\n";
  }
  os << "We just exited the infinite loop\n";
  bool test_termination = EvaluationConnectednessCriterion_MPI(comm, bb, os);
  os << "We have test_termination=" << test_termination << "\n";
  if (test_termination) {
    os << "Correct termination, returning the database\n";
    return RPL.FuncListOrbitIncidence();
  } else {
    os << "RuntimeException, terminating the computation\n";
    throw RuntimeException{1};
  }
}


template<typename T>
void Reset_Directories(boost::mpi::communicator & comm, PolyHeuristicSerial<T> & AllArr) {
  int n_proc = comm.size();
  int i_rank = comm.rank();
  std::string postfix = "_nproc" + std::to_string(n_proc) + "_rank" + std::to_string(i_rank);
  auto update_string=[&](std::string & str_ref) -> void {
    size_t len = str_ref.size();
    std::string part1 = str_ref.substr(0,len-1);
    std::string part2 = str_ref.substr(len-1,1);
    if (part2 != "/") {
      std::cerr << "str_ref=" << str_ref << "\n";
      std::cerr << "Last character should be a /\n";
      throw TerminalException{1};
    }
    str_ref = part1 + postfix + part2;
    CreateDirectory(str_ref);
  };
  update_string(AllArr.BANK_Prefix);
  update_string(AllArr.DD_Prefix);
}




template <typename T, typename Tgroup, typename Tidx_value>
void MPI_MainFunctionDualDesc(boost::mpi::communicator & comm, FullNamelist const &eFull) {
  using Tint = typename Tgroup::Tint;
  using Telt = typename Tgroup::Telt;
  using Tidx = typename Telt::Tidx;
  using Tkey = MyMatrix<T>;
  using Tval = PairStore<Tgroup>;
  int i_rank = comm.rank();
  int n_proc = comm.size();
  //
  std::string FileLog = "log_" + std::to_string(n_proc) + "_" + std::to_string(i_rank);
  std::cerr << "We have moved. See the log in FileLog=" << FileLog << "\n";
  std::ofstream os(FileLog);
  os << std::unitbuf;
  //  std::ostream& os = std::cerr;
  os << "Initial writing of the log\n";
  os.flush();
  //
  MyMatrix<T> EXT = Get_EXT_DualDesc<T,Tidx>(eFull, os);
  Tgroup GRP = Get_GRP_DualDesc<Tgroup>(eFull, os);
  PolyHeuristicSerial<Tint> AllArr = Read_AllStandardHeuristicSerial<Tint>(eFull, os);
  Reset_Directories(comm, AllArr);
  MyMatrix<T> EXTred = ColumnReduction(EXT);
  //
  using Tbank = DataBankClient<Tkey, Tval>;
  Tbank TheBank(AllArr.port);

  using TbasicBank = DatabaseCanonic<T, Tint, Tgroup>;
  TbasicBank bb(EXTred, GRP);
  std::map<std::string, Tint> TheMap = ComputeInitialMap<Tint>(EXTred, GRP);
  vectface vf = MPI_Kernel_DUALDESC_AdjacencyDecomposition<Tbank, TbasicBank, T, Tgroup, Tidx_value>(comm, TheBank, bb, AllArr, AllArr.DD_Prefix, TheMap, os);
  os << "We have vf\n";
  int i_proc_ret = 0;
  vectface vf_tot = my_mpi_gather(comm, vf, i_proc_ret);
  os << "We have vf_tot\n";
  if (comm.rank() == i_proc_ret)
    OutputFacets(vf_tot, AllArr.OUTfile, AllArr.OutFormat);
  os << "We have done our output\n";
}









// clang-format off
#endif  // SRC_DUALDESC_POLY_RECURSIVEDUALDESC_MPI_H_
// clang-format on

