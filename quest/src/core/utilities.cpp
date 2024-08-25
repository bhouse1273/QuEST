/** @file
 * Miscellaneous utility functions needed internally.
 */

#include "quest/include/types.h"
#include "quest/include/qureg.h"
#include "quest/include/matrices.h"

#include "quest/src/core/errors.hpp"
#include "quest/src/core/bitwise.hpp"
#include "quest/src/core/utilities.hpp"
#include "quest/src/comm/comm_config.hpp"
#include "quest/src/comm/comm_routines.hpp"

#include <algorithm>
#include <complex>
#include <vector>
#include <array>

using std::vector;
using std::array;



/*
 * QUBIT PROCESSING
 */

int util_getPrefixInd(int qubit, Qureg qureg) {
    if (qubit < qureg.logNumAmpsPerNode)
        error_utilsGetPrefixIndGivenSuffixQubit();

    return qubit - qureg.logNumAmpsPerNode;
}

int util_getBraQubit(int ketQubit, Qureg qureg) {
    if (!qureg.isDensityMatrix)
        error_utilsGetBraIndGivenNonDensMatr();

    return ketQubit + qureg.numQubits;
}

int util_getPrefixBraInd(int ketQubit, Qureg qureg) {
    if (!qureg.isDensityMatrix)
        error_utilsGetPrefixBraIndGivenNonDensMatr();
    if (ketQubit < qureg.logNumColsPerNode)
        error_utilsGetPrefixBraIndGivenSuffixQubit();
    
    // equivalent to util_getPrefixInd of util_getBraQubit
    return ketQubit - qureg.logNumColsPerNode;
}

bool util_isQubitInSuffix(int qubit, Qureg qureg) {

    return qubit < qureg.logNumAmpsPerNode;
}

bool util_isBraQubitInSuffix(int ketQubit, Qureg qureg) {
    if (!qureg.isDensityMatrix)
        error_utilsIsBraQubitInSuffixGivenNonDensMatr();

    return ketQubit < qureg.logNumColsPerNode;
}

int util_getRankBitOfQubit(int ketQubit, Qureg qureg) {

    int rankInd = util_getPrefixInd(ketQubit, qureg);
    int rankBit = getBit(qureg.rank, rankInd);
    return rankBit;
}

int util_getRankBitOfBraQubit(int ketQubit, Qureg qureg) {
    
    int rankInd = util_getPrefixBraInd(ketQubit, qureg);
    int rankBit = getBit(qureg.rank, rankInd);
    return rankBit;
}

int util_getRankWithQubitFlipped(int ketQubit, Qureg qureg) {

    int rankInd = util_getPrefixInd(ketQubit, qureg);
    int rankFlip = flipBit(qureg.rank, rankInd);
    return rankFlip;
}

int util_getRankWithBraQubitFlipped(int ketQubit, Qureg qureg) {

    int rankInd = util_getPrefixBraInd(ketQubit, qureg);
    int rankFlip = flipBit(qureg.rank, rankInd);
    return rankFlip;
}

vector<int> util_getBraQubits(vector<int> ketQubits, Qureg qureg) {

    vector<int> braInds(0);
    braInds.reserve(ketQubits.size());

    for (int qubit : ketQubits)
        braInds.push_back(util_getBraQubit(qubit, qureg));

    return braInds;
}

vector<int> util_getSorted(vector<int> qubits) {
    vector<int> copy = qubits;
    std::sort(copy.begin(), copy.end());
    return copy;
}

vector<int> util_getSorted(vector<int> ctrls, vector<int> targs) {
    vector<int> qubits = ctrls;
    qubits.insert(qubits.end(), targs.begin(), targs.end());
    return util_getSorted(qubits);
}

qindex util_getBitMask(vector<int> qubits) {

    // inserts qubits in state 1
    return getBitMask(qubits.data(), qubits.size());
}

qindex util_getBitMask(vector<int> qubits, vector<int> states) {

    return getBitMask(qubits.data(), states.data(), states.size());
}

qindex util_getBitMask(vector<int> ctrls, vector<int> ctrlStates, vector<int> targs, vector<int> targStates) {

    auto qubits = ctrls;
    qubits.insert(qubits.end(), targs.begin(), targs.end());

    auto states = ctrlStates;
    states.insert(states.end(), targStates.begin(), targStates.end());

    return util_getBitMask(qubits, states);
}

vector<int> util_getVector(int* qubits, int numQubits) {

    return vector<int> (qubits, qubits + numQubits);
}



/*
 * MATRIX CONJUGATION
 */

// type T can be qcomp** or qcomp*[]
template <typename T>
void setDenseElemsConj(T elems, qindex dim) {
    for (qindex i=0; i<dim; i++)
        for (qindex j=0; j<dim; j++)
           elems[i][j] = conj(elems[i][j]);
}

// diagonals don't need templating because arrays decay to pointers, yay!
void setDiagElemsConj(qcomp* elems, qindex dim) {
    for (qindex i=0; i<dim; i++)
        elems[i] = conj(elems[i]);
}

CompMatr1 util_getConj(CompMatr1 matrix) {
    CompMatr1 conj = matrix;
    setDenseElemsConj(conj.elems, matrix.numRows);
    return conj;
}
CompMatr2 util_getConj(CompMatr2 matrix) {
    CompMatr2 conj = matrix;
    setDenseElemsConj(conj.elems, matrix.numRows);
    return conj;
}

DiagMatr1 util_getConj(DiagMatr1 matrix) {
    DiagMatr1 conj = matrix;
    setDiagElemsConj(conj.elems, matrix.numElems);
    return conj;
}
DiagMatr2 util_getConj(DiagMatr2 matrix) {
    DiagMatr2 conj = matrix;
    setDiagElemsConj(conj.elems, matrix.numElems);
    return conj;
}

void util_setConj(CompMatr matrix) {
    setDenseElemsConj(matrix.cpuElems, matrix.numRows);
}
void util_setConj(DiagMatr matrix) {
    setDiagElemsConj(matrix.cpuElems, matrix.numElems);
}



/*
 * MATRIX UNITARITY
 */

// type T can be qcomp** or qcomp*[]
template <typename T>
bool isUnitary(T elems, qindex dim) {

    qreal epsSq = VALIDATION_EPSILON * VALIDATION_EPSILON;

    // check m * dagger(m) == identity
    for (qindex r=0; r<dim; r++) {
        for (qindex c=0; c<dim; c++) {

            // compute m[r,...] * dagger(m)[...,c]
            qcomp elem = 0;
            for (qindex i=0; i<dim; i++)
                elem += elems[r][i] * conj(elems[c][i]);

            // check if further than epsilon from identity[r,c]
            qcomp dif = elem - qcomp(r == c, 0);
            qreal dist = real(dif)*real(dif) + imag(dif)*imag(dif);
            if (dist > epsSq)
                return false;
        }
    }

    return true;
}

// diagonal version doesn't need templating because array decays to pointer, yay!
bool isUnitary(qcomp* diags, qindex dim) {

    // check every element has unit magnitude
    for (qindex i=0; i<dim; i++) {
        qreal mag = std::abs(diags[i]);
        qreal dif = std::abs(1 - mag);

        if (dif > VALIDATION_EPSILON)
            return false;
    }

    return true;
}

bool util_isUnitary(CompMatr1 matrix) {
    return isUnitary(matrix.elems, matrix.numRows);
}
bool util_isUnitary(CompMatr2 matrix) {
    return isUnitary(matrix.elems, matrix.numRows);
}
bool util_isUnitary(CompMatr matrix) {
    return isUnitary(matrix.cpuElems, matrix.numRows);
}

bool util_isUnitary(DiagMatr1 matrix) {
    return isUnitary(matrix.elems, matrix.numElems);
}
bool util_isUnitary(DiagMatr2 matrix) {
    return isUnitary(matrix.elems, matrix.numElems);
}
bool util_isUnitary(DiagMatr matrix) {
    return isUnitary(matrix.cpuElems, matrix.numElems);
}

bool util_isUnitary(FullStateDiagMatr matrix) {

    // we must check all node's sub-diagonals satisfy unitarity
    bool res = isUnitary(matrix.cpuElems, matrix.numElems);
    if (comm_isInit())
        res = comm_isTrueOnAllNodes(res);

    return res;
}



/*
 * DISTRIBUTED ELEMENTS INDEXING
 */

bool util_areAnyElemsWithinThisNode(int numElemsPerNode, qindex elemStartInd, qindex numInds) {

    qindex elemEndIndExcl = elemStartInd + numInds;
    qindex nodeStartInd = comm_getRank() * numElemsPerNode;
    qindex nodeEndIndExcl = nodeStartInd + numElemsPerNode;

    // 'no' if all targeted elems occur after this node
    if (elemStartInd >= nodeEndIndExcl)
        return false;

    // 'no' if all targeted elems occur before this node
    if (elemEndIndExcl <= nodeStartInd)
        return false;

    // otherwise yes; this node MUST contain one or more targeted elems
    return true;
}

util_IndexRange util_getLocalIndRangeOfElemsWithinThisNode(int numElemsPerNode, qindex elemStartInd, qindex numInds) {

    if (!util_areAnyElemsWithinThisNode(numElemsPerNode, elemStartInd, numInds))
        error_nodeUnexpectedlyContainedNoElems();

    // global indices of the user's targeted elements
    qindex elemEndInd = elemStartInd + numInds;

    // global indices of all elements contained in the node
    qindex nodeStartInd = numElemsPerNode * comm_getRank();
    qindex nodeEndInd   = nodeStartInd + numElemsPerNode;

    // global indices of user's targeted elements which are contained within node
    qindex globalRangeStartInd = std::max(elemStartInd, nodeStartInd);
    qindex globalRangeEndInd   = std::min(elemEndInd,   nodeEndInd);
    qindex numLocalElems       = globalRangeEndInd - globalRangeStartInd;

    // local indices of user's targeted elements to overwrite
    qindex localRangeStartInd = globalRangeStartInd % numElemsPerNode;
    
    // local indices of user's passed elements that correspond to above
    qindex localOffsetInd = globalRangeStartInd - elemStartInd;
    
    return (util_IndexRange) {
        .localDistribStartInd = localRangeStartInd,
        .localDuplicStartInd = localOffsetInd,
        .numElems = numLocalElems
    };
}



/*
 * OPERATOR PARAMETERS
 */

qreal util_getOneQubitDephasingFactor(qreal prob) {

    return 1 - (2 * prob);
}

qreal util_getTwoQubitDephasingTerm(qreal prob) {

    return - 4 * prob / 3;
}

array<qreal,3> util_getOneQubitDepolarisingFactors(qreal prob) {

    // effected where braQubit == ketQubit
    qreal facAA = 1 - (2 * prob / 3);
    qreal facBB = 2 * prob / 3;

    // effected where braQubit != ketQubit
    qreal facAB  = 1 - (4 * prob / 3);

    return {facAA, facBB, facAB};
}

array<qreal,3> util_getTwoQubitDepolarisingFactors(qreal prob) {

    qreal fac1 = 1 - (4 * prob / 5);
    qreal fac2 = 4 * prob / 15;
    qreal fac3 = - (16 * prob / 15);

    return {fac1, fac2, fac3};
}

array<qreal,2> util_getFirstTwoFactorsOfTwoQubitDepolarising(qreal prob) {

    auto facs = util_getTwoQubitDepolarisingFactors(prob);
    return {facs[0], facs[1]};
}

array<qreal,4> util_getOneQubitPauliChannelFactors(qreal pI, qreal pX, qreal pY, qreal pZ) {

    // effected where braQubit == ketQubit
    qreal facAA = pI + pZ;
    qreal facBB = pX + pY;

    // effected where braQubit != ketQubit
    qreal facAB = pI - pZ;
    qreal facBA = pX - pY;

    return {facAA, facBB, facAB, facBA};
}

array<qcomp,2> util_getOneQubitDampingFactors(qreal prob) {

    // sqrt() can return complex for unnnormalised prob
    qcomp c1 = sqrt(1 - qcomp(prob,0));
    qcomp c2 = 1 - prob;

    return {c1, c2};
}