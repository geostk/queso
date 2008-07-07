/* uq/libs/mcmc/inc/uqParamSpace.h
 *
 * Copyright (C) 2008 The PECOS Team, http://www.ices.utexas.edu/centers/pecos

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __UQ_PARAM_SPACE_H__
#define __UQ_PARAM_SPACE_H__

#include <uqFinDimLinearSpace.h>
#include <uqParameter.h>

template <class V, class M>
class uqParamSpaceClass : public uqFinDimLinearSpaceClass<V,M>
{
public:
           uqParamSpaceClass();
           uqParamSpaceClass(const uqEnvironmentClass& env);
  virtual ~uqParamSpaceClass();

          unsigned int            dim                       () const;
          int                     setParameter              (unsigned int       paramId,
                                                             const std::string& name,
                                                             double             initialValue,
                                                             double             minValue = -INFINITY,
                                                             double             maxValue = INFINITY,
                                                             double             priorMu = 0.,
                                                             double             priorSigma = INFINITY);
          const uqParameterClass& parameter                 (unsigned int paramId) const;
          const V&                initialValues             () const;
          const V&                minValues                 () const;
          const V&                maxValues                 () const;
          const V&                priorMuValues             () const;
          const V&                priorSigmaValues          () const;

          bool                    outOfBounds               (const V& v) const;

  virtual void                    print                     (std::ostream& os) const;
          void                    printParameterNames       (std::ostream& os, bool printHorizontally) const; // See template specialization

protected:
          void                    defineMyOptions           (po::options_description& optionsDesc) const;
          void                    getMyOptionValues         (po::options_description& optionsDesc);
          void                    readParamsFromFile        (std::string& paramFileName);
          int                     readTermFromFile          (std::ifstream& ifs,
                                                             std::string&   termString,
                                                             double*        termValue);
          int                     readCharsFromFile         (std::ifstream& ifs,
                                                             std::string&   termString,
                                                             double*        termValue,
                                                             bool&          endOfLineAchieved);
          void                    resetValues               ();
          void                    createInitialValues       () const; // See template specialization
          void                    createMinValues           () const; // See template specialization
          void                    createMaxValues           () const; // See template specialization
          void                    createPriorMuValues       () const; // See template specialization
          void                    createPriorSigmaValues    () const; // See template specialization

  po::options_description*       m_optionsDesc;

  std::vector<uqParameterClass*> m_parameters; // FIXME: will need to be a parallel vector in case of a very large number of parameters
  uqParameterClass               m_dummyParameter;
  mutable V*                     m_initialValues;
  mutable V*                     m_minValues;
  mutable V*                     m_maxValues;
  mutable V*                     m_priorMuValues;
  mutable V*                     m_priorSigmaValues;

  using uqFinDimLinearSpaceClass<V,M>::m_env;
  using uqFinDimLinearSpaceClass<V,M>::m_dim;
};

template <class V, class M>
uqParamSpaceClass<V,M>::uqParamSpaceClass()
  :
  uqFinDimLinearSpaceClass<V,M>()
{
  UQ_FATAL_TEST_MACRO(true,
                      m_env.rank(),
                      "uqParamSpaceClass<V,M>::constructor(), default",
                      "should not be used by user");
}

template <class V, class M>
uqParamSpaceClass<V,M>::uqParamSpaceClass(
  const uqEnvironmentClass& env)
  :
  uqFinDimLinearSpaceClass<V,M>(env),
  m_optionsDesc                (NULL),
  m_parameters                 (0,NULL),
  m_dummyParameter             ("NonExistentYet",0.),
  m_initialValues              (NULL),
  m_minValues                  (NULL),
  m_maxValues                  (NULL),
  m_priorMuValues              (NULL),
  m_priorSigmaValues           (NULL)
{
  //std::cout << "Entering uqParamSpaceClass<V,M>::constructor()"
  //          << std::endl;

  m_optionsDesc = new po::options_description("Parameter space options");
  defineMyOptions                (*m_optionsDesc);
  m_env.scanInputFileForMyOptions(*m_optionsDesc);
  getMyOptionValues              (*m_optionsDesc);

  if (m_env.rank() == 0) std::cout << "After getting option values, state of uqParamSpaceClass object is:"
                                   << "\ndimension = " << m_parameters.size()
                                   << "\n"
                                   << std::endl;

  //std::cout << "Leaving uqParamSpaceClass<V,M>::constructor()"
  //          << std::endl;
}

template <class V, class M>
uqParamSpaceClass<V,M>::~uqParamSpaceClass()
{
  //std::cout << "Entering uqParamSpaceClass<V,M>::destructor()"
  //          << std::endl;

  if (m_priorSigmaValues) delete m_priorSigmaValues;
  if (m_priorMuValues   ) delete m_priorMuValues;
  if (m_maxValues       ) delete m_maxValues;
  if (m_minValues       ) delete m_minValues;
  if (m_initialValues   ) delete m_initialValues;
  for (unsigned int i = 0; i < m_parameters.size(); ++i) {
    if (m_parameters[i]) delete m_parameters[i];
  }

  if (m_optionsDesc) delete m_optionsDesc;

  //std::cout << "Leaving uqParamSpaceClass<V,M>::destructor()"
  //          << std::endl;
}

template <class V, class M>
void
uqParamSpaceClass<V,M>::defineMyOptions(
  po::options_description& optionsDesc) const
{
  m_optionsDesc->add_options()
    ("uqParamSpace_help",                                                   "produce help message for UQ PS"                 )
    ("uqParamSpace_dim",       po::value<unsigned int>()->default_value(0), "Space dimension"                                )
    ("uqParamSpace_inputFile", po::value<std::string>()->default_value(""), "File with definition and data of all parameters")
  ;

  return;
}

template <class V, class M>
void
uqParamSpaceClass<V,M>::getMyOptionValues(po::options_description& optionsDesc)
{
  if (m_env.allOptionsMap().count("uqParamSpace_help")) {
    std::cout << optionsDesc
              << std::endl;
  }

  if (m_env.allOptionsMap().count("uqParamSpace_dim")) {
    const po::variables_map& tmpMap = m_env.allOptionsMap();
    m_dim = tmpMap["uqParamSpace_dim"].as<unsigned int>();
  }

  // Read parameter input file only if 0 dimension was passed to constructor
  if (m_parameters.size() == 0) {
    std::string paramFileName("");
    if (m_env.allOptionsMap().count("uqParamSpace_inputFile")) {
      const po::variables_map& tmpMap = m_env.allOptionsMap();
      paramFileName = tmpMap["uqParamSpace_inputFile"].as<std::string>();
      readParamsFromFile(paramFileName);
    }
  }

  return;
}

template <class V, class M>
void
uqParamSpaceClass<V,M>::readParamsFromFile(std::string& paramFileName)
{
  unsigned int maxCharsPerLine = 512;

  std::ifstream ifs(paramFileName.c_str());

  // Determine number of lines
  unsigned int numLines = std::count(std::istreambuf_iterator<char>(ifs),
                                     std::istreambuf_iterator<char>(),
                                     '\n');

  // Determine number of parameters
  int iRC;
  ifs.seekg(0,std::ios_base::beg);
  unsigned int lineId = 0;
  unsigned int numParameters = 0;
  std::string tempString;
  while ((lineId < numLines) && (ifs.eof() == false)) {
    iRC = readTermFromFile(ifs,tempString,NULL);
    UQ_FATAL_TEST_MACRO(iRC,
                        m_env.rank(),
                        "uqParamSpaceClass<V,M>::constructor()",
                        "failed reading during the determination of the number of parameters");
    if (tempString[0] != '#') numParameters++;
    lineId++;
  }
  UQ_FATAL_TEST_MACRO(lineId != numLines,
                      m_env.rank(),
                      "uqParamSpaceClass<V,M>::constructor()",
                      "the first number of lines read is nonconsistent");
  UQ_FATAL_TEST_MACRO(m_dim != numParameters,
                      m_env.rank(),
                      "uqParamSpaceClass<V,M>::constructor()",
                      "number of parameters in parameter input file does not match dimension passed in the main input file");

  std::cout << "Parameter input file '" << paramFileName
            << "' has "                 << numLines
            << " lines and specifies "  << numParameters
            << " parameters."
            << std::endl;
  m_parameters.resize(numParameters,NULL);

  // Read file until End Of File character is reached
  ifs.seekg(0,std::ios_base::beg);
  lineId = 0;
  unsigned int paramId = 0;
  std::string  paramName         ("");
  std::string  initialValueString("");
  std::string  minValueString    ("");
  std::string  maxValueString    ("");
  std::string  priorMuString     ("");
  std::string  priorSigmaString  ("");
  double       initialValue;
  double       minValue;
  double       maxValue;
  double       priorMu;
  double       priorSigma;
  while ((lineId < numLines) && (ifs.eof() == false)) {
    //std::cout << "Beginning read of line (in parameter input file) of id = " << lineId << std::endl;
    bool endOfLineAchieved = false;

    UQ_FATAL_TEST_MACRO(paramId >= m_parameters.size(),
                        m_env.rank(),
                        "uqParamSpaceClass<V,M>::constructor()",
                        "paramId got too large during reading of parameter input file");

    iRC = readCharsFromFile(ifs, paramName, NULL, endOfLineAchieved);
    UQ_FATAL_TEST_MACRO(iRC,
                        m_env.rank(),
                        "uqParamSpaceClass<V,M>::constructor()",
                        "failed reading a parameter name during the parameters reading loop");

    lineId++;
    if (paramName[0] == '#') {
      if (!endOfLineAchieved) ifs.ignore(maxCharsPerLine,'\n');
      continue;
    }
    UQ_FATAL_TEST_MACRO(endOfLineAchieved,
                        m_env.rank(),
                        "uqParamSpaceClass<V,M>::constructor()",
                        "failed to provide information beyond parameter name during the parameters reading loop");

    iRC = readCharsFromFile(ifs, initialValueString, &initialValue, endOfLineAchieved);
    UQ_FATAL_TEST_MACRO(iRC,
                        m_env.rank(),
                        "uqParamSpaceClass<V,M>::constructor()",
                        "failed reading an initial value during the parameters reading loop");

    minValue   = -INFINITY;
    maxValue   = INFINITY;
    priorMu    = nan("");
    priorSigma = INFINITY;

    if (!endOfLineAchieved) {
      iRC = readCharsFromFile(ifs, minValueString, &minValue, endOfLineAchieved);
      UQ_FATAL_TEST_MACRO(iRC,
                          m_env.rank(),
                          "uqParamSpaceClass<V,M>::constructor()",
                          "failed reading a minimal value during the parameters reading loop");
    }

    if (!endOfLineAchieved) {
      iRC = readCharsFromFile(ifs, maxValueString, &maxValue, endOfLineAchieved);
      UQ_FATAL_TEST_MACRO(iRC,
                          m_env.rank(),
                          "uqParamSpaceClass<V,M>::constructor()",
                          "failed reading a maximum value during the parameters reading loop");
    }

    if (!endOfLineAchieved) {
      iRC = readCharsFromFile(ifs, priorMuString, &priorMu, endOfLineAchieved);
      UQ_FATAL_TEST_MACRO(iRC,
                          m_env.rank(),
                          "uqParamSpaceClass<V,M>::constructor()",
                          "failed reading a prior mu value during the parameters reading loop");
    }

    if (!endOfLineAchieved) {
      iRC = readCharsFromFile(ifs, priorSigmaString, &priorSigma, endOfLineAchieved);
      UQ_FATAL_TEST_MACRO(iRC,
                          m_env.rank(),
                          "uqParamSpaceClass<V,M>::constructor()",
                          "failed reading a prior sigma value during the parameters reading loop");
    }

    if (!endOfLineAchieved) ifs.ignore(maxCharsPerLine,'\n');
    //std::cout << "Just read, for paramId = " << paramId
    //          << ": paramName = "    << paramName
    //          << ", initialValue = " << initialValue
    //          << ", minValue = "     << minValue
    //          << ", maxValue = "     << maxValue
    //          << ", priorMu = "      << priorMu
    //          << ", priorSigma = "   << priorSigma
    //          << std::endl;
    setParameter(paramId,
                 paramName,
                 initialValue,
                 minValue,
                 maxValue,
                 priorMu,
                 priorSigma);
    paramId++;
  }

  UQ_FATAL_TEST_MACRO(lineId != numLines,
                      m_env.rank(),
                      "uqParamSpaceClass<V,M>::constructor()",
                      "the second number of lines read is nonconsistent");
  UQ_FATAL_TEST_MACRO(paramId != m_parameters.size(),
                      m_env.rank(),
                      "uqParamSpaceClass<V,M>::constructor()",
                      "the number of parameters just read is nonconsistent");

  return;
}

template <class V, class M>
int
uqParamSpaceClass<V,M>::readTermFromFile(
  std::ifstream& ifs,
  std::string&   termString,
  double*        termValue)
{
  int iRC = UQ_OK_RC;

  ifs >> termString;
  if ((ifs.rdstate() & std::ifstream::failbit)) {
    iRC = UQ_FAILED_READING_FILE_RC;
  }
  else if (termValue) {
    if (termString == std::string("inf")) {
      *termValue = INFINITY;
    }
    else if (termString == std::string("-inf")) {
      *termValue = -INFINITY;
    }
    else if (termString == std::string("nan")) {
      *termValue = nan("");
    }
    else {
      *termValue = strtod(termString.c_str(),NULL);
    }
  }
  //if (!iRC) std::cout << "Read termString = " << termString << std::endl;

  return iRC;
}

template <class V, class M>
int
uqParamSpaceClass<V,M>::readCharsFromFile(
  std::ifstream& ifs,
  std::string&   termString,
  double*        termValue,
  bool&          endOfLineAchieved)
{
  int iRC = UQ_OK_RC;
  endOfLineAchieved = false;

  char c = ' ';
  while (c == ' ') {
    ifs.get(c);
    if ((ifs.rdstate() & std::ifstream::failbit)) {
      iRC = UQ_FAILED_READING_FILE_RC;
      break;
    }
  };

  char term[512];
  unsigned int pos = 0;

  if (!iRC) {
    while ((pos < 512) && (c != '\n') && (c != '\0') && (c != ' ')) {
      term[pos++] = c;
      if ((ifs.rdstate() & std::ifstream::failbit)) {
        iRC = UQ_FAILED_READING_FILE_RC;
        break;
      }
      ifs.get(c);
    };
  }

  if (!iRC) {
    if (c == '\n') endOfLineAchieved = true;
    term[pos] = '\0';
    termString = term;
    //std::cout << "Read chars = " << termString << std::endl;
    if (termValue) {
      if (termString == std::string("inf")) {
        *termValue = INFINITY;
      }
      else if (termString == std::string("-inf")) {
        *termValue = -INFINITY;
      }
      else if (termString == std::string("nan")) {
        *termValue = nan("");
      }
      else {
        *termValue = strtod(termString.c_str(),NULL);
      }
    }
  }

  return iRC;
}

template <class V, class M>
unsigned int
uqParamSpaceClass<V,M>::dim() const
{
  UQ_FATAL_TEST_MACRO(m_parameters.size() != m_dim,
                      m_env.rank(),
                      "uqParamSpaceClass<V,M>::dim()",
                      "internal inconsistency");

  return m_parameters.size();
}

template <class V, class M>
int
uqParamSpaceClass<V,M>::setParameter(
  unsigned int       paramId,
  const std::string& name,
  double             initialValue,
  double             minValue,
  double             maxValue,
  double             priorMu,
  double             priorSigma)
{
  UQ_TEST_MACRO((paramId > m_parameters.size()),
                m_env.rank(),
                "uqParamSpaceClass<V,M>::setParameter()",
                "paramId is too big",
                UQ_INVALID_INPUT_PARAMETER_RC);

  if (m_parameters[paramId] == NULL) {
    m_parameters[paramId] = new uqParameterClass(name,
                                                 initialValue,
                                                 minValue,
                                                 maxValue,
                                                 priorMu,
                                                 priorSigma);
  }
  else {
    m_parameters[paramId]->setName        (name);
    m_parameters[paramId]->setInitialValue(initialValue);
    m_parameters[paramId]->setMinValue    (minValue);
    m_parameters[paramId]->setMaxValue    (maxValue);
    m_parameters[paramId]->setPriorMu     (priorMu);
    m_parameters[paramId]->setPriorSigma  (priorSigma);
  }

  // These values cannot be trusted anymore
  // They need to updated
  // They will be updated the next time they are requested
  resetValues();

  return 0;
}

template <class V, class M>
const uqParameterClass&
uqParamSpaceClass<V,M>::parameter(unsigned int paramId) const
{
  if (paramId > m_parameters.size()) return m_dummyParameter;
  if (m_parameters[paramId] == NULL) return m_dummyParameter;
  return *(m_parameters[paramId]);
}

template <class V, class M>
void
uqParamSpaceClass<V,M>::resetValues()
{
  if (m_priorSigmaValues) delete m_priorSigmaValues;
  if (m_priorMuValues   ) delete m_priorMuValues;
  if (m_maxValues       ) delete m_maxValues;
  if (m_minValues       ) delete m_minValues;
  if (m_initialValues   ) delete m_initialValues;
  m_priorSigmaValues = NULL;
  m_priorMuValues    = NULL;
  m_maxValues        = NULL;
  m_minValues        = NULL;
  m_initialValues    = NULL;
}

template <class V, class M>
const V&
uqParamSpaceClass<V,M>::initialValues() const
{
  if (m_initialValues == NULL) this->createInitialValues();
  return *m_initialValues;
}

template <class V, class M>
const V&
uqParamSpaceClass<V,M>::minValues() const
{
  if (m_minValues == NULL) this->createMinValues();
  return *m_minValues;
}

template <class V, class M>
const V&
uqParamSpaceClass<V,M>::maxValues() const
{
  if (m_maxValues == NULL) this->createMaxValues();
  return *m_maxValues;
}

template <class V, class M>
const V&
uqParamSpaceClass<V,M>::priorMuValues() const
{
  if (m_priorMuValues == NULL) this->createPriorMuValues();
  return *m_priorMuValues;
}

template <class V, class M>
const V&
uqParamSpaceClass<V,M>::priorSigmaValues() const
{
  if (m_priorSigmaValues == NULL) this->createPriorSigmaValues();
  return *m_priorSigmaValues;
}

template <class V, class M>
bool
uqParamSpaceClass<V,M>::outOfBounds(const V& v) const
{
  if (m_minValues == NULL) this->createMinValues();
  bool result = v.atLeastOneComponentSmallerThan(*m_minValues);

  if (result == false) {
    if (m_maxValues == NULL) this->createMaxValues();
    result = v.atLeastOneComponentBiggerThan(*m_maxValues);
  }

  return result;
}

template <class V, class M>
void
uqParamSpaceClass<V,M>::print(std::ostream& os) const
{
  os << "m_dim = " << m_dim
     << std::endl;
  for (unsigned int i = 0; i < this->dim(); ++i) {
    os << i << " ";
    if (m_parameters[i]) {
      os << *(m_parameters[i]);
    }
    else {
      os << "NULL";
    }
    os << std::endl;
  }

  return;
}

template<class V, class M>
std::ostream& operator<<(std::ostream& os, const uqParamSpaceClass<V,M>& space)
{
  space.print(os);

  return os;
}
#endif // __UQ_PARAM_SPACE_H__

