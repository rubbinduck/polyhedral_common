#ifndef INCLUDE_POLY_NETCDF_H
#define INCLUDE_POLY_NETCDF_H

#include <netcdf>

//
// Reading and writing polytope
//



template<typename T>
constexpr long GetMaximumPossibleCoefficient()
{
  long val1 = std::numeric_limits<T>::max();
  long val2 = std::numeric_limits<T>::min();
  return std::min(-val2, val1);
}



template<typename T, typename Twrite>
void POLY_NC_WritePolytope_SpecType(netCDF::NcFile & dataFile, MyMatrix<T> const& EXT, std::string const& nameType)
{
  int n_col = EXT.cols();
  int n_row = EXT.rows();
  int eDim = n_col * n_row;
  std::vector<std::string> LDim{"n_col", "n_row"};
  netCDF::NcVar varVAL = dataFile.addVar("EXT", nameType, LDim);
  std::vector<Twrite> A(eDim);
  int idx = 0;
  for (int i_row=0; i_row<n_row; i_row++)
    for (int i_col=0; i_col<n_col; i_col++) {
      Twrite eVal_write = UniversalTypeConversion<Twrite,T>(EXT(i_row,i_col));
      A[idx] = eVal_write;
      idx++;
    }
  varVAL.putVar(A.data());
}


template<typename T>
void POLY_NC_WritePolytope_INT(netCDF::NcFile & dataFile, MyMatrix<T> const& EXT)
{
  static_assert(is_implementation_of_Z<T>::value, "Requires T to be an integer domain");
  int n_col = EXT.cols();
  int n_row = EXT.rows();
  netCDF::NcDim eDimCol = dataFile.addDim("n_col", n_col);
  netCDF::NcDim eDimRow = dataFile.addDim("n_row", n_row);
  //
  // Determining the maximum coefficient in order to be able to write down the values.
  //
  T MaxCoeff=0;
  for (int i_row=0; i_row<n_row; i_row++)
    for (int i_col=0; i_col<n_col; i_col++)
      MaxCoeff = T_max(T_abs(EXT(i_row, i_col)), MaxCoeff);

  T MaxCoeff_int8  = GetMaximumPossibleCoefficient<int8_t>();
  T MaxCoeff_int16 = GetMaximumPossibleCoefficient<int16_t>();
  T MaxCoeff_int32 = GetMaximumPossibleCoefficient<int32_t>();
  T MaxCoeff_int64 = GetMaximumPossibleCoefficient<int64_t>();
  if (MaxCoeff < MaxCoeff_int8) {
    POLY_NC_WritePolytope_SpecType<T,int8_t> (dataFile, EXT, "byte");
    return;
  }
  if (MaxCoeff < MaxCoeff_int16) {
    POLY_NC_WritePolytope_SpecType<T,int16_t>(dataFile, EXT, "short");
    return;
  }
  if (MaxCoeff < MaxCoeff_int32) {
    POLY_NC_WritePolytope_SpecType<T,int32_t>(dataFile, EXT, "int");
    return;
  }
  if (MaxCoeff < MaxCoeff_int64) {
    POLY_NC_WritePolytope_SpecType<T,int64_t>(dataFile, EXT, "int64");
    return;
  }
  std::cerr << "We failed to find a type for writing the data type\n";
  throw TerminalException{1};
}


template<typename T>
void POLY_NC_WritePolytope(netCDF::NcFile & dataFile, MyMatrix<T> const& EXT)
{
  if (!IsIntegralMatrix(EXT)) {
    std::cerr << "We do not have code for writing down non-rational matrices right now\n";
    throw TerminalException{1};
  }
  using Tring = typename underlying_ring<T>::ring_type;
  MyMatrix<Tring> EXT_i=ConvertMatrixUniversal<Tring,T>(EXT);
  POLY_NC_WritePolytope_INT(dataFile, EXT_i);
}




// Reading of polytope


template<typename T>
MyMatrix<T> POLY_NC_ReadPolytope(netCDF::NcFile & dataFile)
{
  netCDF::NcVar varVAL = dataFile.getVar("EXT");
  netCDF::NcType eType = varVAL.getType();
  if (eType.isNull()) {
    std::cerr << "The variable EXT is missing\n";
    throw TerminalException{1};
  }
  int nbDim=varVAL.getDimCount();
  if (nbDim != 2) {
    std::cerr << "We have nbDim=" << nbDim << " but it should be 2\n";
    throw TerminalException{1};
  }
  netCDF::NcDim eDim0=varVAL.getDim(0);
  int n_col=eDim0.getSize();
  netCDF::NcDim eDim1=varVAL.getDim(1);
  int n_row=eDim1.getSize();
  int eProd = n_col * n_row;
  MyMatrix<T> EXT(n_row, n_col);
  //
  bool IsMatch = false;
  if (eType == netCDF::NcType::nc_BYTE) {
    std::vector<signed char> V(eProd);
    data.getVar(V.data());
    int idx=0;
    for (int i_row=0; i_row<n_row; i_row++)
      for (int i_col=0; i_col<n_col; i_col++) {
        EXT(i_row,i_col)=V[idx];
        idx++;
      }
    IsMatch=true;
  }
  if (eType == netCDF::NcType::nc_SHORT) {
    std::vector<signed short int> V(eProd);
    data.getVar(V.data());
    int idx=0;
    for (int i_row=0; i_row<n_row; i_row++)
      for (int i_col=0; i_col<n_col; i_col++) {
        EXT(i_row,i_col)=V[idx];
        idx++;
      }
    IsMatch=true;
  }
  if (eType == netCDF::NcType::nc_INT) {
    std::vector<int> V(eProd);
    data.getVar(V.data());
    int idx=0;
    for (int i_row=0; i_row<n_row; i_row++)
      for (int i_col=0; i_col<n_col; i_col++) {
        EXT(i_row,i_col)=V[idx];
        idx++;
      }
    IsMatch=true;
  }
  if (eType == netCDF::NcType::nc_INT64) {
    std::vector<int64_t> V(eProd);
    data.getVar(V.data());
    int idx=0;
    for (int i_row=0; i_row<n_row; i_row++)
      for (int i_col=0; i_col<n_col; i_col++) {
        EXT(i_row,i_col)=V[idx];
        idx++;
      }
    IsMatch=true;
  }
  if (!IsMatch) {
    std::cerr << "We failed to find a matching type\n";
    throw TerminalException{1};
  }
  return EXT;
}


//
// The group with respect to netcdf.
//


template<typename Telt, typename Tint>
struct GRPinfo {
  int n_act;
  std::vector<Telt> LGen;
  Tint grpSize;
};

template<typename Tint>
std::vector<unsigned char> GetVectorUnsignedChar(Tint const& eVal)
{
  std::vector<unsigned char> V;
  Tint workVal = eVal;
  Tint cst256 = 256;
  while(true) {
    Tint res = ResInt(workVal, cst256);
    unsigned char res_i = UniversalTypeConversion<uint8_t,Tint>(res);
    V.push_back(res_i);
    workVal = QuoInt(workVal, cst256);
    if (workVal != 0)
      break;
  }
  return V;
}

template<typename Tint>
Tint GetTint_from_VectorUnsignedChar(std::vector<unsigned char> const& V)
{
  Tint cst256 = 256;
  Tint retVal = 0;
  for (size_t i=0; i<V.size(); i++) {
    size_t j = V.size() - 1 - i;
    Tint eVal = V[j];
    retVal = eVal + cst256 * retVal;
  }
  return retVal;
}







template<typename Telt, typename Tint>
void POLY_NC_WriteGroup(netCDF::NcFile & dataFile, GRPinfo const& eGRP, bool const& orbit_setup, bool const& orbit_status)
{
  int n_act = eGRP.n_act;
  int n_gen = LGen.size();
  netCDF::NcDim eDimAct = dataFile.addDim("n_act", n_act);
  netCDF::NcDim eDimGen = dataFile.addDim("n_gen", n_gen);
  std::vector<std::string> LDim1{"n_act", "n_gen"};
  netCDF::NcVar varGEN = dataFile.addVar("ListGenerator", "int", LDim1);
  std::vector<int> A(n_act * n_gen);
  int idx = 0;
  for (auto & eGen : LGen) {
    for (int i_act=0; i_act<n_act; i_act++) {
      A[idx] = OnPoints(i_act, eGen);
      idx++;
    }
  }
  varGEN.putVar(A.data());
  //
  std::vector<unsigned char> V_grpsize = GetVectorUnsignedChar(eGRP.grpSize);
  int n_grpsize = V_grpsize.size();
  netCDF::NcDim eDimGRP = dataFile.addDim("n_grpsize", n_grpsize);
  std::vector<std::string> LDim2{"n_grpsize"};
  netCDF::NcVar varGRPSIZE = dataFile.addVar("grpsize", "ubyte", LDim2);
  varGRPSIZE.putVar(V_grpsize.data());
  //
  if (orbit_setup) {
    int n_act_div8 = (n_act + int(orbit_status) + 1) / 8; // We put an additional
    netCDF::NcDim eDimAct = dataFile.addDim("n_act_div8", n_act_div8);
    netCDF::NcDim eDimOrbit = dataFile.addDim("n_orbit");
    std::vector<std::string> LDim3{"n_act_div8", "n_orbit"};
    std::string name = "orbit_incidence";
    if (orbit_status)
      name = "orbit_status_incidence";
    netCDF::NcVar varORB_INCD = dataFile.addVar(name, "ubyte", LDim3);
    if (orbit_status) {
      std::vector<std::string> LDim4{"n_grpsize", "n_orbit"};
      netCDF::NcVar varORB_SIZE = dataFile.addVar("orbit_size", "ubyte", LDim4);
    }
  }
}



template<typename Telt>
GRPinfo<Telt,Tint> POLY_NC_ReadGroup(netCDF::NcFile & dataFile)
{
  netCDF::NcVar varGEN = dataFile.getVar("ListGenerator");
  if (varGEN.isNull()) {
    std::cerr << "The variable ListGenerator is missing\n";
    throw TerminalException{1};
  }
  netCDF::NcType eType = varGEN.getType();
  int nbDim=varGEN.getDimCount();
  if (nbDim != 2) {
    std::cerr << "We have nbDim=" << nbDim << " but it should be 2\n";
    throw TerminalException{1};
  }
  netCDF::NcDim eDim0 = varGEN.getDim(0);
  int n_act=eDim0.getSize();
  netCDF::NcDim eDim1 = varGEN.getDim(1);
  int n_gen=eDim1.getSize();
  int eProd = n_act * n_gen;
  //
  std::vector<Telt> LGen(n_gen);
  std::vector<int> V1(eProd);
  varGEN.getVar(V1.data());
  int idx=0;
  for (int i_gen=0; i_gen<n_gen; i_gen++) {
    std::vector<int> eList(n_act);
    for (int i_act=0; i_act<n_act; i_act++) {
      eList[i_act] = V1[idx];
      idx++;
    }
    Telt eGen(eList);
    LGen[i_gen] = eGen;
  }
  //
  netCDF::NcVar varGRPSIZE = dataFile.getVar("grpsize");
  netCDF::NcDim fDim0 = varGRPSIZE.getDim(0);
  int n_grpsize = fDim0.getSize();
  std::vector<unsigned char> V2(n_grpsize);
  varGRPSIZE.getVar(V2.data());
  Tint grpSize = GetTint_from_VectorUnsignedChar<Tint>(V2);
  //
  return {n_act, LGen, grpSize};
}




//
// Individual bitset vectors
//

template<typename Tint>
struct SingleEntryStatus {
  bool status;
  Face face;
  Tint OrbSize;
};




template<typename Tint>
void POLY_NC_WriteVface_Vsize(netCDF::NcFile & dataFile, size_t const& iOrbit, std::vector<unsigned char> const& Vface, std::vector<unsigned char> const& Vsize, bool const& orbit_status)
{
  std::string name = "orbit_incidence";
  if (orbit_status)
    name = "orbit_status_incidence";
  netCDF::NcVar varORB_INCD = dataFile.getVar(name);
  std::vector<size_t> start_incd={iOrbit, 0};
  std::vector<size_t> count_incd={1,Vface.size()};
  varORB_INCD.putVar(Vface.data(), start_incd, count_incd);
  //
  if (orbit_status) {
    netCDF::NcVar varORB_SIZE = dataFile.getVar("orbit_size");
    std::vector<size_t> start_size={iOrbit, 0};
    std::vector<size_t> count_size={1,Vsize.size()};
    varORB_SIZE.putVar(Vsize.data(), start_size, count_size);
  }
}


void POLY_NC_WriteFace(netCDF::NcFile & dataFile, size_t const& iOrbit, Face const& face)
{
  std::vector<unsigned char> Vface;
  uint8_t expo = 1;
  uint8_t val = 0;
  uint8_t siz = 0;
  auto insertBit=[&](bool const& bit) -> void {
    val += int(bit) * expo;
    expo *= 2;
    siz++;
    if (siz == 8) {
      Vface.push_back(val);
      expo = 1;
      siz = 0;
      val = 0;
    }
  };
  for (int i=0; i<face.size(); i++) {
    bool bit = face[i];
    insertBit(bit);
  }
  if (siz > 0)
    Vface.insert(val);
  //
  std::vector<unsigned char> Vsize;
  POLY_NC_WriteVface_Vsize(dataFile, iOrbit, Vface, Vsize, false);
}


template<typename Tint>
void POLY_NC_WriteSingleEntryStatus(netCDF::NcFile & dataFile, size_t const& iOrbit, SingleEntryStatus<Tint> const& eEnt, size_t const& n_grpsize)
{
  std::vector<unsigned char> Vface;
  uint8_t expo = 1;
  uint8_t val = 0;
  uint8_t siz = 0;
  auto insertBit=[&](bool const& bit) -> void {
    val += int(bit) * expo;
    expo *= 2;
    siz++;
    if (siz == 8) {
      Vface.push_back(val);
      expo = 1;
      siz = 0;
      val = 0;
    }
  };
  insertBit(eEnt.status);
  for (int i=0; i<eEnt.face.size(); i++) {
    bool bit = eEnt.face[i];
    insertBit(bit);
  }
  if (siz > 0)
    Vface.insert(val);
  //
  std::vector<unsigned char> Vsize = GetVectorUnsignedChar(eEnt.OrbSize);
  for (size_t pos=Vsize.size();  pos<n_grpsize; pos++)
    Vsize.push_back(0);
  POLY_NC_WriteVface_Vsize(dataFile, iOrbit, Vface, Vsize, true);
}


void POLY_NC_SetBit(netCDF::NcFile & dataFile, size_t const& iOrbit, bool const& status)
{
  uint8_t val;
  netCDF::NcVar varORB_INCD = dataFile.getVar("orbit_status_incidence");
  std::vector<size_t> start_incd={iOrbit, 0};
  std::vector<size_t> count_incd={1,1};
  varORB_INCD.getVar(&val, start_incd, count_incd);
  uint8_t res = val / 2;
  //
  uint8_t val_ret = int(status) + 2 * res;
  varORB_INCD.putVar(&val_ret, start_incd, count_incd);
}




template<typename Tint>
struct PairVface_OrbSize {
  std::vector<unsigned char> Vface;
  Tint OrbSize;
};


template<typename Tint>
PairVface_Orbsize<Tint> POLY_NC_ReadVface_OrbSize(netCDF::NcFile & dataFile, size_t const& iOrbit, size_t const& n_act_div8, bool const& orbit_status)
{
  netCDF::NcDim dimGRP_INCD = dataFile.getDim("n_act_div8");
  size_t n_act_div8 = dimGRP_INCD.getSize();
  netCDF::NcDim dimGRP_SIZE = dataFile.getDim("n_grpsize");
  size_t n_grpsize = dimGRP_SIZE.getSize();
  //
  std::string name = "orbit_incidence";
  if (orbit_status)
    name = "orbit_status_incidence";
  netCDF::NcVar varORB_INCD = dataFile.getVar(name);
  std::vector<size_t> start_incd={iOrbit, 0};
  std::vector<size_t> count_incd={1,n_act_div8};
  std::vector<unsigned char> Vface(n_act_div8);
  varORB_INCD.putVar(Vface.data(), start_incd, count_incd);
  //
  if (orbit_status) {
    netCDF::NcVar varORB_SIZE = dataFile.getVar("orbit_size");
    std::vector<size_t> start_size={iOrbit, 0};
    std::vector<size_t> count_size={1,n_grpsize};
    std::vector<unsigned char> Vsize(n_grpsize);
    varORB_SIZE.putVar(Vsize.data(), start_size, count_size);
    //
    return {Vface, GetTint_from_VectorUnsignedChar(Vsize)};
  } else {
    return {Vface,{}};
  }
}



Face POLY_NC_ReadFace(netCDF::NcFile & dataFile, size_t const& iOrbit)
{
  netCDF::NcDim dimGRP_INCD = dataFile.getDim("n_act");
  size_t n_act = dimGRP_INCD.getSize();
  bool orbit_status = false;
  int n_act_div8 = (n_act + int(orbit_status) + 1) / 8; // We put an additional
  //
  PairVface_Orbsize<Tint> ePair = POLY_NC_ReadVface_OrbSize(dataFile, iOrbit, n_act_div8, orbit_status);
  Face face(n_act);
  int idx=0;
  int n_actremain = n_act;
  for (int iFace=0; iFace<ePair.Vface.size(); iFace++) {
    size_t sizw = 8;
    if (n_actremain < 8)
      sizw = n_actremain;
    uint8_t val = Vface[iFace];
    for (size_t i=0; i<sizw; i++) {
      face[idx] = val % 2;
      idx++;
      val = val / 2
    }
    n_actremain -= 8;
  }
  return face;
}




template<typename Tint>
SingleEntryStatus<Tint> POLY_NC_ReadSingleEntryStatus(netCDF::NcFile & dataFile, size_t const& iOrbit)
{
  netCDF::NcDim dimGRP_INCD = dataFile.getDim("n_act");
  size_t n_act = dimGRP_INCD.getSize();
  bool orbit_status = true;
  int n_act_div8 = (n_act + int(orbit_status) + 1) / 8; // We put an additional 
  //
  PairVface_Orbsize<Tint> ePair = POLY_NC_ReadVface_OrbSize(dataFile, iOrbit, n_act_div8, orbit_status);
  bool status = ePair.Vface[0] % 2;
  Face face(n_act);
  int idx=0;
  int n_actremain = n_act + 1;
  for (int iFace=0; iFace<ePair.Vface.size(); iFace++) {
    size_t sizw = 8;
    if (n_actremain < 8)
      sizw = n_actremain;
    uint8_t val = Vface[iFace];
    for (size_t i=0; i<sizw; i++) {
      if (idx > 0)
        face[idx-1] = val % 2;
      idx++;
      val = val / 2
    }
    n_actremain -= 8;
  }
  return {face, ePair.OrbSize};
}













#endif
