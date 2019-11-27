/** @file
 * Unit testing for QuEST's 'unitaries' API. 
 * The tests are in alphabetical order of the API doc. 
 *
 * These tests work by constructing, from the unitary specification (e.g. 
 * control and target qubits), a full-Hilbert space complex matrix. This is 
 * then multiplied onto statevectors, or multiplied and it's conjugate-transpose
 * right-multiplied onto density matrices.
 *
 * QuEST's user validation handling is unit tested by redefining exitWithError 
 * (a weak C symbol) to throw a C++ exception, caught by the Catch2 library.
 * 
 * All unitary unit tests follow the master template:

TEST_CASE( "OP", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
 
    SECTION( "state-vector correctness" ) {
    
    }
    
    SECTION( "density-matrix correctness" ) {
    
    }
    
    SECTION( "input validation" ) {
    
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}
    
    CLEANUP_TEST( env, quregVec, quregMatr );
}

 *
 * @author Tyson Jones
 */
 
#include "catch.hpp"
#include "QuEST.h"
#include "QuEST_test_utils.hpp"

/** The default number of qubits in the registers created for unit testing 
 * (both statevectors and density matrices). Creation of non-NUM_QUBITS sized 
 * Quregs should be justified in a comment. 
 * Note that the smaller this number is, the fewer nodes can be employed in 
 * distribution testing, since each node must contain at least one amplitude.
 * Furthermore, the larger this number is, the greater the deviation of correct 
 * results from their expected value, due to numerical error; this is especially 
 * apparent for density matrices.
 */
#define NUM_QUBITS 5

/** Prepares the needed data structures for unit testing. This creates 
 * the QuEST environment, a statevector and density matrix of the size 'numQb',
 * and corresponding QVector and QMatrix instances for analytic comparison.
 * numQb should be NUM_QUBITS unless motivated otherwise.
 */
#define PREPARE_TEST(env, quregVec, quregMatr, refVec, refMatr, numQb) \
    QuESTEnv env = createQuESTEnv(); \
    Qureg quregVec = createQureg(numQb, env); \
    Qureg quregMatr = createDensityQureg(numQb, env); \
    initDebugState(quregVec); \
    initDebugState(quregMatr); \
    QVector refVec = toQVector(quregVec); \
    QMatrix refMatr = toQMatrix(quregMatr);

/** Destroys the data structures made by PREPARE_TEST */
#define CLEANUP_TEST(env, quregVec, quregMatr) \
    destroyQureg(quregVec, env); \
    destroyQureg(quregMatr, env); \
    destroyQuESTEnv(env);

/* allows concise use of Contains in catch's REQUIRE_THROWS_WITH */
using Catch::Matchers::Contains;



TEST_CASE( "compactUnitary", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    
    qcomp a = getRandomReal(-1,1) * exp(1i * getRandomReal(0,2*M_PI));
    qcomp b = sqrt(1-abs(a)*abs(a)) * exp(1i * getRandomReal(0,2*M_PI));
    Complex alpha = toComplex( a );
    Complex beta = toComplex( b );
    QMatrix op = toQMatrix(alpha, beta);
    
    SECTION( "correctness" ) {
        
        int target = GENERATE( range(0,NUM_QUBITS) );
            
        SECTION( "state-vector" ) {
        
            compactUnitary(quregVec, target, alpha, beta);
            applyReferenceOp(refVec, target, op);    
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            compactUnitary(quregMatr, target, alpha, beta);
            applyReferenceOp(refMatr, target, op);    
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "qubit indices" ) {
            
            int target = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( compactUnitary(quregVec, target, alpha, beta), Contains("Invalid target") );
        }
        SECTION( "unitarity" ) {
        
            // unitary when |alpha|^2 + |beta|^2 = 1
            alpha = {.real=1, .imag=2}; 
            beta = {.real=3, .imag=4};
            REQUIRE_THROWS_WITH( compactUnitary(quregVec, 0, alpha, beta), Contains("unitary") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "controlledCompactUnitary", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    
    qcomp a = getRandomReal(-1,1) * exp(1i * getRandomReal(0,2*M_PI));
    qcomp b = sqrt(1-abs(a)*abs(a)) * exp(1i * getRandomReal(0,2*M_PI));
    Complex alpha = toComplex( a );
    Complex beta = toComplex( b );
    QMatrix op = toQMatrix(alpha, beta);
    
    SECTION( "correctness" ) {
        
        int target = GENERATE( range(0,NUM_QUBITS) );
        int control = GENERATE_COPY( filter([=](int c){ return c!=target; }, range(0,NUM_QUBITS)) );
        
        SECTION( "state-vector" ) {
        
            controlledCompactUnitary(quregVec, control, target, alpha, beta);
            applyReferenceOp(refVec, control, target, op);    
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            controlledCompactUnitary(quregMatr, control, target, alpha, beta);
            applyReferenceOp(refMatr, control, target, op);    
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "control and target collision" ) {
            
            int qb = 0;
            REQUIRE_THROWS_WITH( controlledCompactUnitary(quregVec, qb, qb, alpha, beta), Contains("Control") && Contains("target") );
        }    
        SECTION( "qubit indices" ) {
            
            int qb = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( controlledCompactUnitary(quregVec, qb, 0, alpha, beta), Contains("Invalid control") );
            REQUIRE_THROWS_WITH( controlledCompactUnitary(quregVec, 0, qb, alpha, beta), Contains("Invalid target") );
        }
        SECTION( "unitarity" ) {

            // unitary when |a|^2 + |b^2 = 1
            alpha = {.real=1, .imag=2};
            beta = {.real=3, .imag=4};
            REQUIRE_THROWS_WITH( controlledCompactUnitary(quregVec, 0, 1, alpha, beta), Contains("unitary") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "controlledMultiQubitUnitary", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    
    SECTION( "correctness" ) {
        
        // figure out max-num targs (inclusive) allowed by hardware backend
        int maxNumTargs = calcLog2(quregVec.numAmpsPerChunk);
        if (maxNumTargs == NUM_QUBITS)
            maxNumTargs -= 1; // make space for control qubit
            
        // generate all possible qubit arrangements
        int ctrl = GENERATE( range(0,NUM_QUBITS) );
        int numTargs = GENERATE_COPY( range(1,maxNumTargs+1) );
        int* targs = GENERATE_COPY( sublists(range(0,NUM_QUBITS), numTargs, ctrl) );
        
        // for each qubit arrangement, use a new random unitary
        QMatrix op = getRandomUnitary(numTargs);
        ComplexMatrixN matr = createComplexMatrixN(numTargs);
        toComplexMatrixN(op, matr);
    
        SECTION( "state-vector" ) {
            
            controlledMultiQubitUnitary(quregVec, ctrl, targs, numTargs, matr);
            applyReferenceOp(refVec, ctrl, targs, numTargs, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            controlledMultiQubitUnitary(quregMatr, ctrl, targs, numTargs, matr);
            applyReferenceOp(refMatr, ctrl, targs, numTargs, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
        destroyComplexMatrixN(matr);
    }
    SECTION( "input validation" ) {
        
        SECTION( "number of targets" ) {
            
            // there cannot be more targets than qubits in register
            // (numTargs=NUM_QUBITS is caught elsewhere, because that implies ctrl is invalid)
            int numTargs = GENERATE( -1, 0, NUM_QUBITS+1 );
            int targs[NUM_QUBITS+1]; // prevents seg-fault if validation doesn't trigger
            ComplexMatrixN matr = createComplexMatrixN(NUM_QUBITS+1); // prevent seg-fault
            REQUIRE_THROWS_WITH( controlledMultiQubitUnitary(quregVec, 0, targs, numTargs, matr), Contains("Invalid number of target"));
            destroyComplexMatrixN(matr);
        }
        SECTION( "repetition in targets" ) {
            
            int ctrl = 0;
            int numTargs = 3;
            int targs[] = {1,2,2};
            ComplexMatrixN matr = createComplexMatrixN(numTargs); // prevents seg-fault if validation doesn't trigger
            
            REQUIRE_THROWS_WITH( controlledMultiQubitUnitary(quregVec, ctrl, targs, numTargs, matr), Contains("target") && Contains("unique"));
            destroyComplexMatrixN(matr);
        }
        SECTION( "control and target collision" ) {
            
            int numTargs = 3;
            int targs[] = {0,1,2};
            int ctrl = targs[GENERATE_COPY( range(0,numTargs) )];
            ComplexMatrixN matr = createComplexMatrixN(numTargs); // prevents seg-fault if validation doesn't trigger
            
            REQUIRE_THROWS_WITH( controlledMultiQubitUnitary(quregVec, ctrl, targs, numTargs, matr), Contains("Control") && Contains("target"));
            destroyComplexMatrixN(matr);
        }
        SECTION( "qubit indices" ) {
            
            int ctrl = 0;
            int numTargs = 3;
            int targs[] = {1,2,3};
            ComplexMatrixN matr = createComplexMatrixN(numTargs); // prevents seg-fault if validation doesn't trigger
            
            int inv = GENERATE( -1, NUM_QUBITS );
            ctrl = inv;
            REQUIRE_THROWS_WITH( controlledMultiQubitUnitary(quregVec, ctrl, targs, numTargs, matr), Contains("Invalid control") );
            
            ctrl = 0; // restore valid ctrl
            targs[GENERATE_COPY( range(0,numTargs) )] = inv; // make invalid target
            REQUIRE_THROWS_WITH( controlledMultiQubitUnitary(quregVec, ctrl, targs, numTargs, matr), Contains("Invalid target") );
            
            destroyComplexMatrixN(matr);
        }
        SECTION( "unitarity" ) {
            
            int ctrl = 0;
            int numTargs = 3;
            int targs[] = {1,2,3};
            ComplexMatrixN matr = createComplexMatrixN(numTargs); // initially zero, hence not-unitary
            
            REQUIRE_THROWS_WITH( controlledMultiQubitUnitary(quregVec, ctrl, targs, numTargs, matr), Contains("unitary") );
            destroyComplexMatrixN(matr);
        }
        SECTION( "unitary dimensions" ) {
            
            int ctrl = 0;
            int targs[2] = {1,2};
            ComplexMatrixN matr = createComplexMatrixN(3);
            
            REQUIRE_THROWS_WITH( controlledMultiQubitUnitary(quregVec, ctrl, targs, 2, matr), Contains("matrix size"));
            destroyComplexMatrixN(matr);
        }
        SECTION( "unitary fits in node" ) {
                
            // pretend we have a very limited distributed memory (judged by matr size)
            quregVec.numAmpsPerChunk = 1;
            int qb[] = {1,2};
            ComplexMatrixN matr = createComplexMatrixN(2); // prevents seg-fault if validation doesn't trigger
            REQUIRE_THROWS_WITH( controlledMultiQubitUnitary(quregVec, 0, qb, 2, matr), Contains("targets too many qubits"));
            destroyComplexMatrixN(matr);
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE(  "controlledNot", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    QMatrix op{{0,1},{1,0}};
    
    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        int control = GENERATE_COPY( filter([=](int c){ return c!=target; }, range(0,NUM_QUBITS)) );
    
        SECTION( "state-vector" ) {

            controlledNot(quregVec, control, target);
            applyReferenceOp(refVec, control, target, op);    
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {
        
            controlledNot(quregMatr, control, target);
            applyReferenceOp(refMatr, control, target, op);    
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "control and target collision" ) {
            
            int qb = GENERATE( range(0,NUM_QUBITS) );
            REQUIRE_THROWS_WITH( controlledNot(quregVec, qb, qb), Contains("Control") && Contains("target") );
        }    
        SECTION( "qubit indices" ) {
            
            int qb = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( controlledNot(quregVec, qb, 0), Contains("Invalid control") );
            REQUIRE_THROWS_WITH( controlledNot(quregVec, 0, qb), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE(  "controlledPauliY", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    QMatrix op{{0,-1i},{1i,0}};
    
    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        int control = GENERATE_COPY( filter([=](int c){ return c!=target; }, range(0,NUM_QUBITS)) );
    
        SECTION( "state-vector" ) {

            controlledPauliY(quregVec, control, target);
            applyReferenceOp(refVec, control, target, op);    
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {
        
            controlledPauliY(quregMatr, control, target);
            applyReferenceOp(refMatr, control, target, op);    
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "control and target collision" ) {
            
            int qb = GENERATE( range(0,NUM_QUBITS) );
            REQUIRE_THROWS_WITH( controlledPauliY(quregVec, qb, qb), Contains("Control") && Contains("target") );
        }    
        SECTION( "qubit indices" ) {
            
            int qb = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( controlledPauliY(quregVec, qb, 0), Contains("Invalid control") );
            REQUIRE_THROWS_WITH( controlledPauliY(quregVec, 0, qb), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "controlledPhaseFlip", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    QMatrix op{{1,0},{0,-1}};
    
    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        int control = GENERATE_COPY( filter([=](int c){ return c!=target; }, range(0,NUM_QUBITS)) );
    
        SECTION( "state-vector" ) {

            controlledPhaseFlip(quregVec, control, target);
            applyReferenceOp(refVec, control, target, op);    
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {
        
            controlledPhaseFlip(quregMatr, control, target);
            applyReferenceOp(refMatr, control, target, op);    
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "control and target collision" ) {
            
            int qb = GENERATE( range(0,NUM_QUBITS) );
            REQUIRE_THROWS_WITH( controlledPhaseFlip(quregVec, qb, qb), Contains("Control") && Contains("target") );
        }    
        SECTION( "qubit indices" ) {
            
            int qb = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( controlledPhaseFlip(quregVec, qb, 0), Contains("Invalid control") );
            REQUIRE_THROWS_WITH( controlledPhaseFlip(quregVec, 0, qb), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "controlledPhaseShift", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    qreal param = getRandomReal(-2*M_PI, 2*M_PI);
    QMatrix op{{1,0},{0,exp(1i * param)}};
    
    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        int control = GENERATE_COPY( filter([=](int c){ return c!=target; }, range(0,NUM_QUBITS)) );
    
        SECTION( "state-vector" ) {

            controlledPhaseShift(quregVec, control, target, param);
            applyReferenceOp(refVec, control, target, op);    
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {
        
            controlledPhaseShift(quregMatr, control, target, param);
            applyReferenceOp(refMatr, control, target, op);    
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "control and target collision" ) {
            
            int qb = GENERATE( range(0,NUM_QUBITS) );
            REQUIRE_THROWS_WITH( controlledPhaseShift(quregVec, qb, qb, param), Contains("Control") && Contains("target") );
        }    
        SECTION( "qubit indices" ) {
            
            int qb = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( controlledPhaseShift(quregVec, qb, 0, param), Contains("Invalid control") );
            REQUIRE_THROWS_WITH( controlledPhaseShift(quregVec, 0, qb, param), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "controlledRotateAroundAxis", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    
    // each test will use a random parameter and axis vector    
    qreal param = getRandomReal(-4*M_PI, 4*M_PI);
    Vector vec = {.x=getRandomReal(-1,1), .y=getRandomReal(-1,1), .z=getRandomReal(-1,1)};
    
    // Rn(a) = cos(a/2)I - i sin(a/2) n . paulivector
    // (pg 24 of vcpc.univie.ac.at/~ian/hotlist/qc/talks/bloch-sphere-rotations.pdf)
    qreal c = cos(param/2);
    qreal s = sin(param/2);
    qreal m = sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
    QMatrix op{{c - 1i*vec.z*s/m, -(vec.y + 1i*vec.x)*s/m}, 
               {(vec.y - 1i*vec.x)*s/m, c + 1i*vec.z*s/m}};
    
    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        int control = GENERATE_COPY( filter([=](int c){ return c!=target; }, range(0,NUM_QUBITS)) );
    
        SECTION( "state-vector" ) {

            controlledRotateAroundAxis(quregVec, control, target, param, vec);
            applyReferenceOp(refVec, control, target, op);    
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {
        
            controlledRotateAroundAxis(quregMatr, control, target, param, vec);
            applyReferenceOp(refMatr, control, target, op);    
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "control and target collision" ) {
            
            int qb = GENERATE( range(0,NUM_QUBITS) );
            REQUIRE_THROWS_WITH( controlledRotateAroundAxis(quregVec, qb, qb, param, vec), Contains("Control") && Contains("target") );
        }    
        SECTION( "qubit indices" ) {
            
            int qb = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( controlledRotateAroundAxis(quregVec, qb, 0, param, vec), Contains("Invalid control") );
            REQUIRE_THROWS_WITH( controlledRotateAroundAxis(quregVec, 0, qb, param, vec), Contains("Invalid target") );
        }
        SECTION( "zero rotation axis" ) {
            
            vec = {.x=0, .y=0, .z=0};
            REQUIRE_THROWS_WITH( controlledRotateAroundAxis(quregVec, 0, 1, param, vec), Contains("Invalid axis") && Contains("zero") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "controlledRotateX", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    qreal param = getRandomReal(-4*M_PI, 4*M_PI);
    QMatrix op{{cos(param/2), -1i*sin(param/2)}, {-1i*sin(param/2), cos(param/2)}};
    
    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        int control = GENERATE_COPY( filter([=](int c){ return c!=target; }, range(0,NUM_QUBITS)) );
    
        SECTION( "state-vector" ) {

            controlledRotateX(quregVec, control, target, param);
            applyReferenceOp(refVec, control, target, op);    
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {
        
            controlledRotateX(quregMatr, control, target, param);
            applyReferenceOp(refMatr, control, target, op);    
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "control and target collision" ) {
            
            int qb = GENERATE( range(0,NUM_QUBITS) );
            REQUIRE_THROWS_WITH( controlledRotateX(quregVec, qb, qb, param), Contains("Control") && Contains("target") );
        }    
        SECTION( "qubit indices" ) {
            
            int qb = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( controlledRotateX(quregVec, qb, 0, param), Contains("Invalid control") );
            REQUIRE_THROWS_WITH( controlledRotateX(quregVec, 0, qb, param), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "controlledRotateY", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    qreal param = getRandomReal(-4*M_PI, 4*M_PI);
    QMatrix op{{cos(param/2), -sin(param/2)},{sin(param/2), cos(param/2)}};
    
    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        int control = GENERATE_COPY( filter([=](int c){ return c!=target; }, range(0,NUM_QUBITS)) );
    
        SECTION( "state-vector" ) {

            controlledRotateY(quregVec, control, target, param);
            applyReferenceOp(refVec, control, target, op);    
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {
        
            controlledRotateY(quregMatr, control, target, param);
            applyReferenceOp(refMatr, control, target, op);    
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "control and target collision" ) {
            
            int qb = GENERATE( range(0,NUM_QUBITS) );
            REQUIRE_THROWS_WITH( controlledRotateY(quregVec, qb, qb, param), Contains("Control") && Contains("target") );
        }    
        SECTION( "qubit indices" ) {
            
            int qb = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( controlledRotateY(quregVec, qb, 0, param), Contains("Invalid control") );
            REQUIRE_THROWS_WITH( controlledRotateY(quregVec, 0, qb, param), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "controlledRotateZ", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    qreal param = getRandomReal(-4*M_PI, 4*M_PI);
    QMatrix op{{exp(-1i*param/2.),0},{0,exp(1i*param/2.)}};
    
    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        int control = GENERATE_COPY( filter([=](int c){ return c!=target; }, range(0,NUM_QUBITS)) );
    
        SECTION( "state-vector" ) {

            controlledRotateZ(quregVec, control, target, param);
            applyReferenceOp(refVec, control, target, op);    
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {
        
            controlledRotateZ(quregMatr, control, target, param);
            applyReferenceOp(refMatr, control, target, op);    
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "control and target collision" ) {
            
            int qb = GENERATE( range(0,NUM_QUBITS) );
            REQUIRE_THROWS_WITH( controlledRotateZ(quregVec, qb, qb, param), Contains("Control") && Contains("target") );
        }    
        SECTION( "qubit indices" ) {
            
            int qb = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( controlledRotateZ(quregVec, qb, 0, param), Contains("Invalid control") );
            REQUIRE_THROWS_WITH( controlledRotateZ(quregVec, 0, qb, param), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "controlledTwoQubitUnitary", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    
    // in distributed mode, each node must be able to fit all amps modified by unitary 
    REQUIRE( quregVec.numAmpsPerChunk >= 4 );
    
    // every test will use a unique random matrix
    QMatrix op = getRandomUnitary(2);
    ComplexMatrix4 matr = toComplexMatrix4(op); 
    
    SECTION( "correctness" ) {
    
        int targ1 = GENERATE( range(0,NUM_QUBITS) );
        int targ2 = GENERATE_COPY( filter([=](int t){ return t!=targ1; }, range(0,NUM_QUBITS)) );
        int control = GENERATE_COPY( filter([=](int c){ return c!=targ1 && c!=targ2; }, range(0,NUM_QUBITS)) );
    
        SECTION( "state-vector" ) {

            controlledTwoQubitUnitary(quregVec, control, targ1, targ2, matr);
            applyReferenceOp(refVec, control, targ1, targ2, op);    
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {
        
            controlledTwoQubitUnitary(quregMatr, control, targ1, targ2, matr);
            applyReferenceOp(refMatr, control, targ1, targ2, op);    
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "repetition of targets" ) {
            int targ = 0;
            int ctrl = 1;
            REQUIRE_THROWS_WITH( controlledTwoQubitUnitary(quregVec, ctrl, targ, targ, matr), Contains("target") && Contains("unique") );
        }
        SECTION( "control and target collision" ) {
            
            int targ1 = 1;
            int targ2 = 2;
            int ctrl = GENERATE( 1,2 ); // catch2 bug; can't do GENERATE_COPY( targ1, targ2 )
            REQUIRE_THROWS_WITH( controlledTwoQubitUnitary(quregVec, ctrl, targ1, targ2, matr), Contains("Control") && Contains("target") );
        }
        SECTION( "qubit indices" ) {
            
            // valid config
            int ctrl = 0;
            int targ1 = 1;
            int targ2 = 2;
            
            int qb = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( controlledTwoQubitUnitary(quregVec, qb, targ1, targ2, matr), Contains("Invalid control") );
            REQUIRE_THROWS_WITH( controlledTwoQubitUnitary(quregVec, ctrl, qb, targ2, matr), Contains("Invalid target") );
            REQUIRE_THROWS_WITH( controlledTwoQubitUnitary(quregVec, ctrl, targ1, qb, matr), Contains("Invalid target") );
        }
        SECTION( "unitarity" ) {

            matr.real[0][0] = 0; // break matr unitarity
            REQUIRE_THROWS_WITH( controlledTwoQubitUnitary(quregVec, 0, 1, 2, matr), Contains("unitary") );
        }
        SECTION( "unitary fits in node" ) {
                
            // pretend we have a very limited distributed memory
            quregVec.numAmpsPerChunk = 1;
            REQUIRE_THROWS_WITH( controlledTwoQubitUnitary(quregVec, 0, 1, 2, matr), Contains("targets too many qubits"));
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "controlledUnitary", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    QMatrix op = getRandomUnitary(1);
    ComplexMatrix2 matr = toComplexMatrix2(op);
    
    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        int control = GENERATE_COPY( filter([=](int c){ return c!=target; }, range(0,NUM_QUBITS)) );
    
        SECTION( "state-vector" ) {

            controlledUnitary(quregVec, control, target, matr);
            applyReferenceOp(refVec, control, target, op);    
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {
        
            controlledUnitary(quregMatr, control, target, matr);
            applyReferenceOp(refMatr, control, target, op);    
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "control and target collision" ) {
            
            int qb = GENERATE( range(0,NUM_QUBITS) );
            REQUIRE_THROWS_WITH( controlledUnitary(quregVec, qb, qb, matr), Contains("Control") && Contains("target") );
        }    
        SECTION( "qubit indices" ) {
            
            int qb = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( controlledUnitary(quregVec, qb, 0, matr), Contains("Invalid control") );
            REQUIRE_THROWS_WITH( controlledUnitary(quregVec, 0, qb, matr), Contains("Invalid target") );
        }
        SECTION( "unitarity" ) {
            
            matr.real[0][0] = 0; // break unitarity
            REQUIRE_THROWS_WITH( controlledUnitary(quregVec, 0, 1, matr), Contains("unitary") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "hadamard", "[unitaries]" ) {

    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    qreal a = 1/sqrt(2);
    QMatrix op{{a,a},{a,-a}};

    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        
        SECTION( "state-vector ") {
        
            hadamard(quregVec, target);
            applyReferenceOp(refVec, target, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            hadamard(quregMatr, target);
            applyReferenceOp(refMatr, target, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {

        SECTION( "qubit indices" ) {

            int target = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( hadamard(quregVec, target), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "multiControlledMultiQubitUnitary", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    
    SECTION( "correctness" ) {
        
        // figure out max-num targs (inclusive) allowed by hardware backend
        int maxNumTargs = calcLog2(quregVec.numAmpsPerChunk);
        if (maxNumTargs == NUM_QUBITS)
            maxNumTargs--; // leave room for min-number of control qubits
        
        // try all possible numbers of targets and controls
        int numTargs = GENERATE_COPY( range(1,maxNumTargs+1) );
        int maxNumCtrls = NUM_QUBITS - numTargs;
        int numCtrls = GENERATE_COPY( range(1,maxNumCtrls+1) );
        
        // generate all possible valid qubit arrangements
        int* targs = GENERATE_COPY( sublists(range(0,NUM_QUBITS), numTargs) );
        int* ctrls = GENERATE_COPY( sublists(range(0,NUM_QUBITS), numCtrls, targs, numTargs) );
        
        // for each qubit arrangement, use a new random unitary
        QMatrix op = getRandomUnitary(numTargs);
        ComplexMatrixN matr = createComplexMatrixN(numTargs);
        toComplexMatrixN(op, matr);
    
        SECTION( "state-vector" ) {
            
            multiControlledMultiQubitUnitary(quregVec, ctrls, numCtrls, targs, numTargs, matr);
            applyReferenceOp(refVec, ctrls, numCtrls, targs, numTargs, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            multiControlledMultiQubitUnitary(quregMatr, ctrls, numCtrls, targs, numTargs, matr);
            applyReferenceOp(refMatr, ctrls, numCtrls, targs, numTargs, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
        destroyComplexMatrixN(matr);
    }
    SECTION( "input validation" ) {
        
        SECTION( "number of targets" ) {
            
            // there cannot be more targets than qubits in register
            // (numTargs=NUM_QUBITS is caught elsewhere, because that implies ctrls are invalid)
            int numTargs = GENERATE( -1, 0, NUM_QUBITS+1 );
            int targs[NUM_QUBITS+1]; // prevents seg-fault if validation doesn't trigger
            int ctrls[] = {0};
            ComplexMatrixN matr = createComplexMatrixN(NUM_QUBITS+1); // prevent seg-fault
            toComplexMatrixN(getRandomUnitary(NUM_QUBITS+1), matr); // ensure unitary
            
            REQUIRE_THROWS_WITH( multiControlledMultiQubitUnitary(quregVec, ctrls, 1, targs, numTargs, matr), Contains("Invalid number of target"));
            destroyComplexMatrixN(matr);
        }
        SECTION( "repetition in targets" ) {
            
            int ctrls[] = {0};
            int numTargs = 3;
            int targs[] = {1,2,2};
            ComplexMatrixN matr = createComplexMatrixN(numTargs); // prevents seg-fault if validation doesn't trigger
            toComplexMatrixN(getRandomUnitary(numTargs), matr); // ensure unitary
            
            REQUIRE_THROWS_WITH( multiControlledMultiQubitUnitary(quregVec, ctrls, 1, targs, numTargs, matr), Contains("target") && Contains("unique"));
            destroyComplexMatrixN(matr);
        }
        SECTION( "number of controls" ) {
            
            int numCtrls = GENERATE( -1, 0, NUM_QUBITS, NUM_QUBITS+1 );
            int ctrls[NUM_QUBITS+1]; // avoids seg-fault if validation not triggered
            int targs[1] = {0};
            ComplexMatrixN matr = createComplexMatrixN(1);
            toComplexMatrixN(getRandomUnitary(1), matr); // ensure unitary
            
            REQUIRE_THROWS_WITH( multiControlledMultiQubitUnitary(quregVec, ctrls, numCtrls, targs, 1, matr), Contains("Invalid number of control"));
            destroyComplexMatrixN(matr);
        }
        SECTION( "repetition in controls" ) {
            
            int ctrls[] = {0,1,1};
            int targs[] = {3};
            ComplexMatrixN matr = createComplexMatrixN(1);
            toComplexMatrixN(getRandomUnitary(1), matr); // ensure unitary
            
            REQUIRE_THROWS_WITH( multiControlledMultiQubitUnitary(quregVec, ctrls, 3, targs, 1, matr), Contains("control") && Contains("unique"));
            destroyComplexMatrixN(matr);
        }
        SECTION( "control and target collision" ) {
            
            int ctrls[] = {0,1,2};
            int targs[] = {3,1,4};
            ComplexMatrixN matr = createComplexMatrixN(3);
            toComplexMatrixN(getRandomUnitary(3), matr); // ensure unitary
            
            REQUIRE_THROWS_WITH( multiControlledMultiQubitUnitary(quregVec, ctrls, 3, targs, 3, matr), Contains("Control") && Contains("target") && Contains("disjoint"));
            destroyComplexMatrixN(matr);
        }
        SECTION( "qubit indices" ) {
            
            // valid inds
            int numQb = 2;
            int qb1[2] = {0,1};
            int qb2[3] = {2,3};
            ComplexMatrixN matr = createComplexMatrixN(numQb);
            toComplexMatrixN(getRandomUnitary(numQb), matr); // ensure unitary
            
            // make qb1 invalid
            int inv = GENERATE( -1, NUM_QUBITS );
            qb1[GENERATE_COPY(range(0,numQb))] = inv;
            
            REQUIRE_THROWS_WITH( multiControlledMultiQubitUnitary(quregVec, qb1, numQb, qb2, numQb, matr), Contains("Invalid control") );
            REQUIRE_THROWS_WITH( multiControlledMultiQubitUnitary(quregVec, qb2, numQb, qb1, numQb, matr), Contains("Invalid target") );
            destroyComplexMatrixN(matr);
        }
        SECTION( "unitarity" ) {
            
            int ctrls[3] = {0};
            int targs[3] = {1,2,3};
            ComplexMatrixN matr = createComplexMatrixN(3); // initially zero, hence not-unitary
            
            REQUIRE_THROWS_WITH( multiControlledMultiQubitUnitary(quregVec, ctrls, 1, targs, 3, matr), Contains("unitary") );
            destroyComplexMatrixN(matr);
        }
        SECTION( "unitary dimensions" ) {
            
            int ctrls[1] = {0};
            int targs[2] = {1,2};
            ComplexMatrixN matr = createComplexMatrixN(3); // intentionally wrong size
            toComplexMatrixN(getRandomUnitary(3), matr); // ensure unitary
            
            REQUIRE_THROWS_WITH( multiControlledMultiQubitUnitary(quregVec, ctrls, 1, targs, 2, matr), Contains("matrix size"));
            destroyComplexMatrixN(matr);
        }
        SECTION( "unitary fits in node" ) {
                
            // pretend we have a very limited distributed memory (judged by matr size)
            quregVec.numAmpsPerChunk = 1;
            int ctrls[1] = {0};
            int targs[2] = {1,2};
            ComplexMatrixN matr = createComplexMatrixN(2);
            toComplexMatrixN(getRandomUnitary(2), matr); // ensure unitary
            
            REQUIRE_THROWS_WITH( multiControlledMultiQubitUnitary(quregVec, ctrls, 1, targs, 2, matr), Contains("targets too many qubits"));
            destroyComplexMatrixN(matr);
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "multiControlledPhaseFlip", "[unitaries]" ) {

    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    
    // acts on the final control qubit
    QMatrix op{{1,0},{0,-1}};
 
    SECTION( "correctness" ) {
    
        // generate ALL valid qubit arrangements
        int numCtrls = GENERATE( range(1,NUM_QUBITS) ); // numCtrls=NUM_QUBITS stopped by overzealous validation
        int* ctrls = GENERATE_COPY( sublists(range(0,NUM_QUBITS), numCtrls) );
    
        SECTION( "state-vector" ) {
            
            multiControlledPhaseFlip(quregVec, ctrls, numCtrls);
            applyReferenceOp(refVec, ctrls, numCtrls-1, ctrls[numCtrls-1], op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            multiControlledPhaseFlip(quregMatr, ctrls, numCtrls);
            applyReferenceOp(refMatr, ctrls, numCtrls-1, ctrls[numCtrls-1], op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "number of controls" ) {
            
            int numCtrls = GENERATE( -1, 0, NUM_QUBITS+1 );
            int ctrls[NUM_QUBITS+1]; // avoids seg-fault if validation not triggered
            REQUIRE_THROWS_WITH( multiControlledPhaseFlip(quregVec, ctrls, numCtrls), Contains("Invalid number of control"));
        }
        SECTION( "repetition of controls" ) {
            
            int numCtrls = 3;
            int ctrls[] = {0,1,1};
            REQUIRE_THROWS_WITH( multiControlledPhaseFlip(quregVec, ctrls, numCtrls), Contains("control") && Contains("unique"));
        }
        SECTION( "qubit indices" ) {
            
            int numCtrls = 3;
            int ctrls[] = { 1, 2, GENERATE( -1, NUM_QUBITS ) };
            REQUIRE_THROWS_WITH( multiControlledPhaseFlip(quregVec, ctrls, numCtrls), Contains("Invalid control") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "multiControlledPhaseShift", "[unitaries]" ) {

    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    qreal param = getRandomReal(-2*M_PI, 2*M_PI);
    QMatrix op{{1,0},{0,exp(1i*param)}};
 
    SECTION( "correctness" ) {
    
        // generate ALL valid qubit arrangements
        int numCtrls = GENERATE( range(1,NUM_QUBITS) ); // numCtrls=NUM_QUBITS stopped by overzealous validation
        int* ctrls = GENERATE_COPY( sublists(range(0,NUM_QUBITS), numCtrls) );
    
        SECTION( "state-vector" ) {
            
            multiControlledPhaseShift(quregVec, ctrls, numCtrls, param);
            applyReferenceOp(refVec, ctrls, numCtrls-1, ctrls[numCtrls-1], op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            multiControlledPhaseShift(quregMatr, ctrls, numCtrls, param);
            applyReferenceOp(refMatr, ctrls, numCtrls-1, ctrls[numCtrls-1], op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "number of controls" ) {
            
            int numCtrls = GENERATE( -1, 0, NUM_QUBITS+1 );
            int ctrls[NUM_QUBITS+1]; // avoids seg-fault if validation not triggered
            REQUIRE_THROWS_WITH( multiControlledPhaseShift(quregVec, ctrls, numCtrls, param), Contains("Invalid number of control"));
        }
        SECTION( "repetition of controls" ) {
            
            int numCtrls = 3;
            int ctrls[] = {0,1,1};
            REQUIRE_THROWS_WITH( multiControlledPhaseShift(quregVec, ctrls, numCtrls, param), Contains("control") && Contains("unique"));
        }
        SECTION( "qubit indices" ) {
            
            int numCtrls = 3;
            int ctrls[] = { 1, 2, GENERATE( -1, NUM_QUBITS ) };
            REQUIRE_THROWS_WITH( multiControlledPhaseShift(quregVec, ctrls, numCtrls, param), Contains("Invalid control") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "multiControlledTwoQubitUnitary", "[unitaries]" ) {

    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );

    // in distributed mode, each node must be able to fit all amps modified by unitary 
    REQUIRE( quregVec.numAmpsPerChunk >= 4 );
    
    // every test will use a unique random matrix
    QMatrix op = getRandomUnitary(2);
    ComplexMatrix4 matr = toComplexMatrix4(op); 
 
    SECTION( "correctness" ) {
    
        // generate ALL valid qubit arrangements
        int targ1 = GENERATE( range(0,NUM_QUBITS) );
        int targ2 = GENERATE_COPY( filter([=](int t){ return t!=targ1; }, range(0,NUM_QUBITS)) );
        int targs[] = {targ1, targ2};
        int numCtrls = GENERATE( range(1,NUM_QUBITS-1) ); // leave room for 2 targets (upper bound is exclusive)
        int* ctrls = GENERATE_COPY( sublists(range(0,NUM_QUBITS), numCtrls, targs, 2) );
    
        SECTION( "state-vector" ) {
            
            multiControlledTwoQubitUnitary(quregVec, ctrls, numCtrls, targ1, targ2, matr);
            applyReferenceOp(refVec, ctrls, numCtrls, targ1, targ2, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            multiControlledTwoQubitUnitary(quregMatr, ctrls, numCtrls, targ1, targ2, matr);
            applyReferenceOp(refMatr, ctrls, numCtrls, targ1, targ2, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "number of controls" ) {
            
            // numCtrls=(NUM_QUBITS-1) is ok since requires ctrl qubit inds are invalid
            int numCtrls = GENERATE( -1, 0, NUM_QUBITS, NUM_QUBITS+1 ); 
            int ctrls[NUM_QUBITS+1]; // avoids seg-fault if validation not triggered
            REQUIRE_THROWS_WITH( multiControlledTwoQubitUnitary(quregVec, ctrls, numCtrls, 0, 1, matr), Contains("Invalid number of control"));
        }
        SECTION( "repetition of controls" ) {
            
            int numCtrls = 3;
            int ctrls[] = {0,1,1};
            int targ1 = 2;
            int targ2 = 3;
            REQUIRE_THROWS_WITH( multiControlledTwoQubitUnitary(quregVec, ctrls, numCtrls, targ1, targ2, matr), Contains("control") && Contains("unique"));;
        }
        SECTION( "repetition of targets" ) {
            
            int numCtrls = 3;
            int ctrls[] = {0,1,2};
            int targ1 = 3;
            int targ2 = targ1;
            REQUIRE_THROWS_WITH( multiControlledTwoQubitUnitary(quregVec, ctrls, numCtrls, targ1, targ2, matr), Contains("target") && Contains("unique"));
        }
        SECTION( "control and target collision" ) {
            
            int numCtrls = 3;
            int ctrls[] = {0,1,2};
            int targ1 = 3;
            int targ2 = ctrls[GENERATE_COPY( range(0,numCtrls) )];
            REQUIRE_THROWS_WITH( multiControlledTwoQubitUnitary(quregVec, ctrls, numCtrls, targ1, targ2, matr), Contains("Control") && Contains("target") );
        }
        SECTION( "qubit indices" ) {
            
            // valid indices
            int targ1 = 0;
            int targ2 = 1;
            int numCtrls = 3;
            int ctrls[] = { 2, 3, 4 };
            
            int inv = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( multiControlledTwoQubitUnitary(quregVec, ctrls, numCtrls, inv, targ2, matr), Contains("Invalid target") );
            REQUIRE_THROWS_WITH( multiControlledTwoQubitUnitary(quregVec, ctrls, numCtrls, targ1, inv, matr), Contains("Invalid target") );

            ctrls[numCtrls-1] = inv; // make ctrls invalid 
            REQUIRE_THROWS_WITH( multiControlledTwoQubitUnitary(quregVec, ctrls, numCtrls, targ1, targ2, matr), Contains("Invalid control") );
        }
        SECTION( "unitarity " ) {
            
            int ctrls[1] = {0};
            matr.real[0][0] = 0; // break unitarity
            REQUIRE_THROWS_WITH( multiControlledTwoQubitUnitary(quregVec, ctrls, 1, 1, 2, matr), Contains("unitary") );
        }
        SECTION( "unitary fits in node" ) {
                
            // pretend we have a very limited distributed memory
            quregVec.numAmpsPerChunk = 1;
            int ctrls[1] = {0};
            REQUIRE_THROWS_WITH( multiControlledTwoQubitUnitary(quregVec, ctrls, 1, 1, 2, matr), Contains("targets too many qubits"));
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "multiControlledUnitary", "[unitaries]" ) {

    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );

    // every test will use a unique random matrix
    QMatrix op = getRandomUnitary(1);
    ComplexMatrix2 matr = toComplexMatrix2(op); 
 
    SECTION( "correctness" ) {
        
        int target = GENERATE( range(0,NUM_QUBITS) );
        int numCtrls = GENERATE( range(1,NUM_QUBITS) ); // leave space for one target (exclusive upper bound)
        int* ctrls = GENERATE_COPY( sublists(range(0,NUM_QUBITS), numCtrls, target) );
        
        SECTION( "state-vector" ) {
        
            multiControlledUnitary(quregVec, ctrls, numCtrls, target, matr);
            applyReferenceOp(refVec, ctrls, numCtrls, target, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            multiControlledUnitary(quregMatr, ctrls, numCtrls, target, matr);
            applyReferenceOp(refMatr, ctrls, numCtrls, target, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "number of controls" ) {
            
            int numCtrls = GENERATE( -1, 0, NUM_QUBITS, NUM_QUBITS+1 );
            int ctrls[NUM_QUBITS+1]; // avoids seg-fault if validation not triggered
            REQUIRE_THROWS_WITH( multiControlledUnitary(quregVec, ctrls, numCtrls, 0, matr), Contains("Invalid number of control"));
        }
        SECTION( "repetition of controls" ) {
            
            int ctrls[] = {0,1,1};
            REQUIRE_THROWS_WITH( multiControlledUnitary(quregVec, ctrls, 3, 2, matr), Contains("control") && Contains("unique"));
        }
        SECTION( "control and target collision" ) {
            
            int ctrls[] = {0,1,2};
            int targ = ctrls[GENERATE( range(0,3) )];
            REQUIRE_THROWS_WITH( multiControlledUnitary(quregVec, ctrls, 3, targ, matr), Contains("Control") && Contains("target") );
        }
        SECTION( "qubit indices" ) {
            
            int ctrls[] = { 1, 2, GENERATE( -1, NUM_QUBITS ) };
            REQUIRE_THROWS_WITH( multiControlledUnitary(quregVec, ctrls, 3, 0, matr), Contains("Invalid control") );
            
            ctrls[2] = 3; // make ctrls valid 
            int targ = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( multiControlledUnitary(quregVec, ctrls, 3, targ, matr), Contains("Invalid target") );
        }
        SECTION( "unitarity" ) {

            matr.real[0][0] = 0; // break matr unitarity
            int ctrls[] = {0};
            REQUIRE_THROWS_WITH( multiControlledUnitary(quregVec, ctrls, 1, 1, matr), Contains("unitary") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "multiQubitUnitary", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    
    SECTION( "correctness" ) {
        
        // figure out max-num (inclusive) targs allowed by hardware backend
        int maxNumTargs = calcLog2(quregVec.numAmpsPerChunk);
        if (maxNumTargs == NUM_QUBITS)
            maxNumTargs -= 1; // make space for control qubit
            
        // generate all possible qubit arrangements
        int numTargs = GENERATE_COPY( range(1,maxNumTargs+1) );
        int* targs = GENERATE_COPY( sublists(range(0,NUM_QUBITS), numTargs) );
        
        // for each qubit arrangement, use a new random unitary
        QMatrix op = getRandomUnitary(numTargs);
        ComplexMatrixN matr = createComplexMatrixN(numTargs);
        toComplexMatrixN(op, matr);
    
        SECTION( "state-vector" ) {
            
            multiQubitUnitary(quregVec, targs, numTargs, matr);
            applyReferenceOp(refVec, targs, numTargs, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            multiQubitUnitary(quregMatr, targs, numTargs, matr);
            applyReferenceOp(refMatr, targs, numTargs, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
        destroyComplexMatrixN(matr);
    }
    SECTION( "input validation" ) {
        
        SECTION( "number of targets" ) {
            
            // there cannot be more targets than qubits in register
            int numTargs = GENERATE( -1, 0, NUM_QUBITS+1 );
            int targs[NUM_QUBITS+1]; // prevents seg-fault if validation doesn't trigger
            ComplexMatrixN matr = createComplexMatrixN(NUM_QUBITS+1); // prevent seg-fault
            
            REQUIRE_THROWS_WITH( multiQubitUnitary(quregVec, targs, numTargs, matr), Contains("Invalid number of target"));
            destroyComplexMatrixN(matr);
        }
        SECTION( "repetition in targets" ) {
            
            int numTargs = 3;
            int targs[] = {1,2,2};
            ComplexMatrixN matr = createComplexMatrixN(numTargs); // prevents seg-fault if validation doesn't trigger
            
            REQUIRE_THROWS_WITH( multiQubitUnitary(quregVec, targs, numTargs, matr), Contains("target") && Contains("unique"));
            destroyComplexMatrixN(matr);
        }
        SECTION( "qubit indices" ) {
            
            int numTargs = 3;
            int targs[] = {1,2,3};
            ComplexMatrixN matr = createComplexMatrixN(numTargs); // prevents seg-fault if validation doesn't trigger
            
            int inv = GENERATE( -1, NUM_QUBITS );
            targs[GENERATE_COPY( range(0,numTargs) )] = inv; // make invalid target
            REQUIRE_THROWS_WITH( multiQubitUnitary(quregVec, targs, numTargs, matr), Contains("Invalid target") );
            
            destroyComplexMatrixN(matr);
        }
        SECTION( "unitarity" ) {
            
            int numTargs = 3;
            int targs[] = {1,2,3};
            ComplexMatrixN matr = createComplexMatrixN(numTargs); // initially zero, hence not-unitary
            
            REQUIRE_THROWS_WITH( multiQubitUnitary(quregVec, targs, numTargs, matr), Contains("unitary") );
            destroyComplexMatrixN(matr);
        }
        SECTION( "unitary dimensions" ) {
            
            int targs[2] = {1,2};
            ComplexMatrixN matr = createComplexMatrixN(3); // intentionally wrong size
            
            REQUIRE_THROWS_WITH( multiQubitUnitary(quregVec, targs, 2, matr), Contains("matrix size"));
            destroyComplexMatrixN(matr);
        }
        SECTION( "unitary fits in node" ) {
                
            // pretend we have a very limited distributed memory (judged by matr size)
            quregVec.numAmpsPerChunk = 1;
            int qb[] = {1,2};
            ComplexMatrixN matr = createComplexMatrixN(2); // prevents seg-fault if validation doesn't trigger
            REQUIRE_THROWS_WITH( multiQubitUnitary(quregVec, qb, 2, matr), Contains("targets too many qubits"));
            destroyComplexMatrixN(matr);
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "multiRotatePauli", "[unitaries]" ) {
        
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    qreal param = getRandomReal(-4*M_PI, 4*M_PI);
        
    SECTION( "correctness" ) {
        
        int numTargs = GENERATE( range(1,NUM_QUBITS+1) );
        int* targs = GENERATE_COPY( sublists(range(0,NUM_QUBITS), numTargs) );
        
        /* it's too expensive to try ALL Pauli sequences, via 
         *      pauliOpType* paulis = GENERATE_COPY( pauliseqs(numTargs) );.
         * Furthermore, take(10, pauliseqs(numTargs)) will try the same pauli codes.
         * Hence, we instead opt to repeatedlyrandomly generate pauliseqs
         */
        GENERATE( range(0,10) ); // gen 10 random pauli-codes for every targs
        pauliOpType paulis[numTargs];
        for (int i=0; i<numTargs; i++)
            paulis[i] = (pauliOpType) getRandomInt(0,4);

        // exclude identities from reference matrix exp (they apply unwanted global phase)
        int refTargs[numTargs];
        int numRefTargs = 0;

        QMatrix xMatr{{0,1},{1,0}};
        QMatrix yMatr{{0,-1i},{1i,0}};
        QMatrix zMatr{{1,0},{0,-1}};
        
        // build correct reference matrix by pauli-matrix exponentiation...
        QMatrix pauliProd{{1}};
        for (int i=0; i<numTargs; i++) {
            QMatrix fac;
            if (paulis[i] == PAULI_I) continue; // exclude I-targets from ref list
            if (paulis[i] == PAULI_X) fac = xMatr;
            if (paulis[i] == PAULI_Y) fac = yMatr;
            if (paulis[i] == PAULI_Z) fac = zMatr;
            pauliProd = getKroneckerProduct(fac, pauliProd);
            
            // include this target in ref list
            refTargs[numRefTargs++] = targs[i];
        }

        // produces exp(-i param/2 pauliProd), unless pauliProd = I
        QMatrix op;
        if (numRefTargs > 0)
            op = getExponentialPauliMatrix(param, pauliProd);
        
        SECTION( "state-vector" ) {
            
            multiRotatePauli(quregVec, targs, paulis, numTargs, param);
            if (numRefTargs > 0)
                applyReferenceOp(refVec, refTargs, numRefTargs, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {
            
            multiRotatePauli(quregMatr, targs, paulis, numTargs, param);
            if (numRefTargs > 0)
                applyReferenceOp(refMatr, refTargs, numRefTargs, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "validation" ) {
        
        SECTION( "number of targets" ) {
            
            int numTargs = GENERATE( -1, 0, NUM_QUBITS+1 );
            int targs[NUM_QUBITS+1]; // prevent seg-fault if validation isn't triggered
            pauliOpType paulis[NUM_QUBITS+1] = {PAULI_I};
            REQUIRE_THROWS_WITH( multiRotatePauli(quregVec, targs, paulis, numTargs, param), Contains("Invalid number of target"));
            
        }
        SECTION( "repetition of targets" ) {
            
            int numTargs = 3;
            int targs[3] = {0, 1, 1};
            pauliOpType paulis[3] = {PAULI_I};
            REQUIRE_THROWS_WITH( multiRotatePauli(quregVec, targs, paulis, numTargs, param), Contains("target") && Contains("unique"));
        }
        SECTION( "qubit indices" ) {
            
            int numTargs = 3;
            int targs[3] = {0, 1, 2};
            targs[GENERATE_COPY(range(0,numTargs))] = GENERATE( -1, NUM_QUBITS );
            pauliOpType paulis[3] = {PAULI_I};
            REQUIRE_THROWS_WITH( multiRotatePauli(quregVec, targs, paulis, numTargs, param), Contains("Invalid target"));
        }
        SECTION( "pauli codes" ) {
            int numTargs = 3;
            int targs[3] = {0, 1, 2};
            pauliOpType paulis[3] = {PAULI_I, PAULI_I, PAULI_I};
            paulis[GENERATE_COPY(range(0,numTargs))] = (pauliOpType) GENERATE( -1, 4 );
            REQUIRE_THROWS_WITH( multiRotatePauli(quregVec, targs, paulis, numTargs, param), Contains("Invalid Pauli code"));
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}

TEST_CASE( "multiRotateZ", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    qreal param = getRandomReal(-4*M_PI, 4*M_PI);
        
    SECTION( "correctness" ) {
        
        int numTargs = GENERATE( range(1,NUM_QUBITS+1) );
        int* targs = GENERATE_COPY( sublists(range(0,NUM_QUBITS), numTargs) );
        
        // build correct reference matrix by diagonal-matrix exponentiation...
        QMatrix zMatr{{1,0},{0,-1}};
        QMatrix zProd = zMatr;
        for (int t=0; t<numTargs-1; t++)
            zProd = getKroneckerProduct(zMatr, zProd); // Z . Z ... Z
    
        // (-i param/2) Z . I . Z ...
        QMatrix expArg = getScalarMatrixProduct(
            -1i * param / 2.,
            getFullOperatorMatrix(NULL, 0, targs, numTargs, zProd, NUM_QUBITS));
            
        // exp( -i param/2 Z . I . Z ...)
        QMatrix op = getExponentialDiagonalMatrix(expArg);
        
        // all qubits to specify full operator matrix on reference structures
        int allQubits[NUM_QUBITS];
        for (int i=0; i<NUM_QUBITS; i++)
            allQubits[i] = i;
        
        SECTION( "state-vector" ) {
            
            multiRotateZ(quregVec, targs, numTargs, param);
            applyReferenceOp(refVec, allQubits, NUM_QUBITS, op);
            REQUIRE( areEqual(quregVec, refVec) );
            
        }
        SECTION( "density-matrix" ) {
            
            multiRotateZ(quregMatr, targs, numTargs, param);
            applyReferenceOp(refMatr, allQubits, NUM_QUBITS, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "validation" ) {
        
        SECTION( "number of targets" ) {
            
            int numTargs = GENERATE( -1, 0, NUM_QUBITS+1 );
            int targs[NUM_QUBITS+1]; // prevent seg-fault if validation isn't triggered
            REQUIRE_THROWS_WITH( multiRotateZ(quregVec, targs, numTargs, param), Contains("Invalid number of target"));
            
        }
        SECTION( "repetition of targets" ) {
            
            int numTargs = 3;
            int targs[3] = {0, 1, 1};
            REQUIRE_THROWS_WITH( multiRotateZ(quregVec, targs, numTargs, param), Contains("target") && Contains("unique"));
        }
        SECTION( "qubit indices" ) {
            
            int numTargs = 3;
            int targs[3] = {0, 1, 2};
            targs[GENERATE_COPY(range(0,numTargs))] = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( multiRotateZ(quregVec, targs, numTargs, param), Contains("Invalid target"));
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "multiStateControlledUnitary", "[unitaries]" ) {

    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );

    // every test will use a unique random matrix
    QMatrix op = getRandomUnitary(1);
    ComplexMatrix2 matr = toComplexMatrix2(op);
    
    // the zero-conditioned control qubits can be effected by notting before/after ctrls
    QMatrix notOp{{0,1},{1,0}};
 
    SECTION( "correctness" ) {
        
        int target = GENERATE( range(0,NUM_QUBITS) );
        int numCtrls = GENERATE( range(1,NUM_QUBITS) ); // leave space for one target (exclusive upper bound)
        int* ctrls = GENERATE_COPY( sublists(range(0,NUM_QUBITS), numCtrls, target) );
        int* ctrlState = GENERATE_COPY( bitsets(numCtrls) );
        
        SECTION( "state-vector" ) {
            
            multiStateControlledUnitary(quregVec, ctrls, ctrlState, numCtrls, target, matr);
            
            // simulate controlled-state by notting before & after controls
            for (int i=0; i<numCtrls; i++)
                if (ctrlState[i] == 0)
                    applyReferenceOp(refVec, ctrls[i], notOp);
            applyReferenceOp(refVec, ctrls, numCtrls, target, op);
            for (int i=0; i<numCtrls; i++)
                if (ctrlState[i] == 0)
                    applyReferenceOp(refVec, ctrls[i], notOp);
    
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            multiStateControlledUnitary(quregMatr, ctrls, ctrlState, numCtrls, target, matr);
            
            // simulate controlled-state by notting before & after controls
            for (int i=0; i<numCtrls; i++)
                if (ctrlState[i] == 0)
                    applyReferenceOp(refMatr, ctrls[i], notOp);
            applyReferenceOp(refMatr, ctrls, numCtrls, target, op);
            for (int i=0; i<numCtrls; i++)
                if (ctrlState[i] == 0)
                    applyReferenceOp(refMatr, ctrls[i], notOp);
            
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "number of controls" ) {
            
            int numCtrls = GENERATE( -1, 0, NUM_QUBITS, NUM_QUBITS+1 );
            int ctrls[NUM_QUBITS+1]; 
            int ctrlState[NUM_QUBITS+1] = {0}; 
            REQUIRE_THROWS_WITH( multiStateControlledUnitary(quregVec, ctrls, ctrlState, numCtrls, 0, matr), Contains("Invalid number of control"));
        }
        SECTION( "repetition of controls" ) {
            
            int ctrls[] = {0,1,1};
            int ctrlState[] = {0, 1, 0};
            REQUIRE_THROWS_WITH( multiStateControlledUnitary(quregVec, ctrls, ctrlState, 3, 2, matr), Contains("control") && Contains("unique"));
        }
        SECTION( "control and target collision" ) {
            
            int ctrls[] = {0,1,2};
            int ctrlState[] = {0, 1, 0};
            int targ = ctrls[GENERATE( range(0,3) )];
            REQUIRE_THROWS_WITH( multiStateControlledUnitary(quregVec, ctrls, ctrlState, 3, targ, matr), Contains("Control") && Contains("target") );
        }
        SECTION( "qubit indices" ) {
            
            int ctrls[] = { 1, 2, GENERATE( -1, NUM_QUBITS ) };
            int ctrlState[] = {0, 1, 0};
            REQUIRE_THROWS_WITH( multiStateControlledUnitary(quregVec, ctrls, ctrlState, 3, 0, matr), Contains("Invalid control") );
            
            ctrls[2] = 3; // make ctrls valid 
            int targ = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( multiStateControlledUnitary(quregVec, ctrls, ctrlState, 3, targ, matr), Contains("Invalid target") );
        }
        SECTION( "unitarity" ) {

            matr.real[0][0] = 0; // break matr unitarity
            int ctrls[] = {0};
            int ctrlState[1] = {0};
            REQUIRE_THROWS_WITH( multiStateControlledUnitary(quregVec, ctrls, ctrlState, 1, 1, matr), Contains("unitary") );
        }
        SECTION( "control state bits" ) {
            
            // valid qubits
            int ctrls[] = {0, 1, 2};
            int ctrlState[] = {0, 0, 0};
            int targ = 3;
            
            // make invalid 
            ctrlState[2] = GENERATE(-1, 2);
            REQUIRE_THROWS_WITH( multiStateControlledUnitary(quregVec, ctrls, ctrlState, 3, targ, matr), Contains("state") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "pauliX", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    QMatrix op{{0,1},{1,0}};
    
    SECTION( "correctness" ) {
        
        int target = GENERATE( range(0,NUM_QUBITS) );
        
        SECTION( "state-vector" ) {

            pauliX(quregVec, target);
            applyReferenceOp(refVec, target, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix correctness" ) {
        
            pauliX(quregMatr, target);
            applyReferenceOp(refMatr, target, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
                
        SECTION( "qubit indices" ) {
            
            int target = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( pauliX(quregVec, target), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "pauliY", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    QMatrix op{{0,-1i},{1i,0}};
    
    SECTION( "correctness" ) {
        
        int target = GENERATE( range(0,NUM_QUBITS) );
        
        SECTION( "state-vector" ) {

            pauliY(quregVec, target);
            applyReferenceOp(refVec, target, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix correctness" ) {
        
            pauliY(quregMatr, target);
            applyReferenceOp(refMatr, target, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
                
        SECTION( "qubit indices" ) {
            
            int target = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( pauliY(quregVec, target), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "pauliZ", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    QMatrix op{{1,0},{0,-1}};
    
    SECTION( "correctness" ) {
        
        int target = GENERATE( range(0,NUM_QUBITS) );
        
        SECTION( "state-vector" ) {

            pauliZ(quregVec, target);
            applyReferenceOp(refVec, target, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix correctness" ) {
        
            pauliZ(quregMatr, target);
            applyReferenceOp(refMatr, target, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
                
        SECTION( "qubit indices" ) {
            
            int target = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( pauliZ(quregVec, target), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "phaseShift", "[unitaries]" ) {

    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    qreal param = getRandomReal(-2*M_PI, 2*M_PI);
    QMatrix op{{1,0},{0,exp(1i*param)}};

    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        
        SECTION( "state-vector ") {
        
            phaseShift(quregVec, target, param);
            applyReferenceOp(refVec, target, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            phaseShift(quregMatr, target, param);
            applyReferenceOp(refMatr, target, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {

        SECTION( "qubit indices" ) {

            int target = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( phaseShift(quregVec, target, param), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "rotateAroundAxis", "[unitaries]" ) {

    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    
    // each test will use a random parameter and axis vector    
    qreal param = getRandomReal(-4*M_PI, 4*M_PI);
    Vector vec = {.x=getRandomReal(-1,1), .y=getRandomReal(-1,1), .z=getRandomReal(-1,1)};
    
    // Rn(a) = cos(a/2)I - i sin(a/2) n . paulivector
    // (pg 24 of vcpc.univie.ac.at/~ian/hotlist/qc/talks/bloch-sphere-rotations.pdf)
    qreal c = cos(param/2);
    qreal s = sin(param/2);
    qreal m = sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
    QMatrix op{{c - 1i*vec.z*s/m, -(vec.y + 1i*vec.x)*s/m}, 
               {(vec.y - 1i*vec.x)*s/m, c + 1i*vec.z*s/m}};

    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        
        SECTION( "state-vector ") {
        
            rotateAroundAxis(quregVec, target, param, vec);
            applyReferenceOp(refVec, target, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            rotateAroundAxis(quregMatr, target, param, vec);
            applyReferenceOp(refMatr, target, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {

        SECTION( "qubit indices" ) {

            int target = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( rotateAroundAxis(quregVec, target, param, vec), Contains("Invalid target") );
        }
        SECTION( "zero rotation axis" ) {
            
            int target = 0;
            vec = {.x=0, .y=0, .z=0};
            REQUIRE_THROWS_WITH( rotateAroundAxis(quregVec, target, param, vec), Contains("Invalid axis") && Contains("zero") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "rotateX", "[unitaries]" ) {

    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    qreal param = getRandomReal(-4*M_PI, 4*M_PI);
    QMatrix op{{cos(param/2), -1i*sin(param/2)}, {-1i*sin(param/2), cos(param/2)}};

    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        
        SECTION( "state-vector ") {
        
            rotateX(quregVec, target, param);
            applyReferenceOp(refVec, target, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            rotateX(quregMatr, target, param);
            applyReferenceOp(refMatr, target, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {

        SECTION( "qubit indices" ) {

            int target = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( rotateX(quregVec, target, param), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "rotateY", "[unitaries]" ) {

    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    qreal param = getRandomReal(-4*M_PI, 4*M_PI);
    QMatrix op{{cos(param/2), -sin(param/2)},{sin(param/2), cos(param/2)}};

    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        
        SECTION( "state-vector ") {
        
            rotateY(quregVec, target, param);
            applyReferenceOp(refVec, target, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            rotateY(quregMatr, target, param);
            applyReferenceOp(refMatr, target, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {

        SECTION( "qubit indices" ) {

            int target = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( rotateY(quregVec, target, param), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "rotateZ", "[unitaries]" ) {

    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    qreal param = getRandomReal(-4*M_PI, 4*M_PI);
    QMatrix op{{exp(-1i*param/2.),0},{0,exp(1i*param/2.)}};

    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        
        SECTION( "state-vector ") {
        
            rotateZ(quregVec, target, param);
            applyReferenceOp(refVec, target, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            rotateZ(quregMatr, target, param);
            applyReferenceOp(refMatr, target, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {

        SECTION( "qubit indices" ) {

            int target = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( rotateZ(quregVec, target, param), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "sGate", "[unitaries]" ) {

    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    QMatrix op{{1,0},{0,1i}};

    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        
        SECTION( "state-vector ") {
        
            sGate(quregVec, target);
            applyReferenceOp(refVec, target, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            sGate(quregMatr, target);
            applyReferenceOp(refMatr, target, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {

        SECTION( "qubit indices" ) {

            int target = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( sGate(quregVec, target), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "sqrtSwapGate", "[unitaries]" ) {
        
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    qcomp a = (1. + 1i)/2.;
    qcomp b = (1. - 1i)/2.;
    QMatrix op{{1,0,0,0},{0,a,b,0},{0,b,a,0},{0,0,0,1}};

    SECTION( "correctness" ) {
        
        int targ1 = GENERATE( range(0,NUM_QUBITS) );
        int targ2 = GENERATE_COPY( filter([=](int t){ return t!=targ1; }, range(0,NUM_QUBITS)) );
        int targs[] = {targ1, targ2};
        
        SECTION( "state-vector" ) {
        
            sqrtSwapGate(quregVec, targ1, targ2);
            applyReferenceOp(refVec, targs, 2, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            sqrtSwapGate(quregMatr, targ1, targ2);
            applyReferenceOp(refMatr, targs, 2, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "qubit indices" ) {
            
            int targ1 = GENERATE( -1, NUM_QUBITS );
            int targ2 = 0;
            REQUIRE_THROWS_WITH( sqrtSwapGate(quregVec, targ1, targ2), Contains("Invalid target") );
            REQUIRE_THROWS_WITH( sqrtSwapGate(quregVec, targ2, targ1), Contains("Invalid target") );
        }
        SECTION( "repetition of targets" ) {
            
            int qb = 0;
            REQUIRE_THROWS_WITH( sqrtSwapGate(quregVec, qb, qb), Contains("target") && Contains("unique") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "swapGate", "[unitaries]" ) {
        
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    QMatrix op{{1,0,0,0},{0,0,1,0},{0,1,0,0},{0,0,0,1}};

    SECTION( "correctness" ) {
        
        int targ1 = GENERATE( range(0,NUM_QUBITS) );
        int targ2 = GENERATE_COPY( filter([=](int t){ return t!=targ1; }, range(0,NUM_QUBITS)) );
        int targs[] = {targ1, targ2};
        
        SECTION( "state-vector" ) {
        
            swapGate(quregVec, targ1, targ2);
            applyReferenceOp(refVec, targs, 2, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            swapGate(quregMatr, targ1, targ2);
            applyReferenceOp(refMatr, targs, 2, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "qubit indices" ) {
            
            int targ1 = GENERATE( -1, NUM_QUBITS );
            int targ2 = 0;
            REQUIRE_THROWS_WITH( swapGate(quregVec, targ1, targ2), Contains("Invalid target") );
            REQUIRE_THROWS_WITH( swapGate(quregVec, targ2, targ1), Contains("Invalid target") );
        }
        SECTION( "repetition of targets" ) {
            
            int qb = 0;
            REQUIRE_THROWS_WITH( swapGate(quregVec, qb, qb), Contains("target") && Contains("unique") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "tGate", "[unitaries]" ) {

    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    QMatrix op{{1,0},{0,exp(1i*M_PI/4.)}};

    SECTION( "correctness" ) {
    
        int target = GENERATE( range(0,NUM_QUBITS) );
        
        SECTION( "state-vector ") {
        
            tGate(quregVec, target);
            applyReferenceOp(refVec, target, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            tGate(quregMatr, target);
            applyReferenceOp(refMatr, target, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {

        SECTION( "qubit indices" ) {

            int target = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( tGate(quregVec, target), Contains("Invalid target") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "twoQubitUnitary", "[unitaries]" ) {
    
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    
    // in distributed mode, each node must be able to fit all amps modified by unitary 
    REQUIRE( quregVec.numAmpsPerChunk >= 4 );
    
    // every test will use a unique random matrix
    QMatrix op = getRandomUnitary(2);
    ComplexMatrix4 matr = toComplexMatrix4(op); 

    SECTION( "correctness" ) {
        
        int targ1 = GENERATE( range(0,NUM_QUBITS) );
        int targ2 = GENERATE_COPY( filter([=](int t){ return t!=targ1; }, range(0,NUM_QUBITS)) );
        int targs[] = {targ1, targ2};
        
        SECTION( "state-vector" ) {
        
            twoQubitUnitary(quregVec, targ1, targ2, matr);
            applyReferenceOp(refVec, targs, 2, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {

            twoQubitUnitary(quregMatr, targ1, targ2, matr);
            applyReferenceOp(refMatr, targs, 2, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "qubit indices" ) {
            
            int targ1 = GENERATE( -1, NUM_QUBITS );
            int targ2 = 0;
            REQUIRE_THROWS_WITH( twoQubitUnitary(quregVec, targ1, targ2, matr), Contains("Invalid target") );
            REQUIRE_THROWS_WITH( twoQubitUnitary(quregVec, targ2, targ1, matr), Contains("Invalid target") );
        }
        SECTION( "repetition of targets" ) {
            
            int qb = 0;
            REQUIRE_THROWS_WITH( twoQubitUnitary(quregVec, qb, qb, matr), Contains("target") && Contains("unique") );
        }
        SECTION( "unitarity" ) {

            matr.real[0][0] = 0; // break matr unitarity
            REQUIRE_THROWS_WITH( twoQubitUnitary(quregVec, 0, 1, matr), Contains("unitary") );
        }
        SECTION( "unitary fits in node" ) {
                
            // pretend we have a very limited distributed memory
            quregVec.numAmpsPerChunk = 1;
            REQUIRE_THROWS_WITH( twoQubitUnitary(quregVec, 0, 1, matr), Contains("targets too many qubits"));
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}



TEST_CASE( "unitary", "[unitaries]" ) {
        
    PREPARE_TEST( env, quregVec, quregMatr, refVec, refMatr, NUM_QUBITS );
    
    // every test will use a unique random matrix
    QMatrix op = getRandomUnitary(1);
    ComplexMatrix2 matr = toComplexMatrix2(op); 

    SECTION( "correctness" ) {
        
        int target = GENERATE( range(0,NUM_QUBITS) );
        
        SECTION( "state-vector" ) {
        
            unitary(quregVec, target, matr);
            applyReferenceOp(refVec, target, op);
            REQUIRE( areEqual(quregVec, refVec) );
        }
        SECTION( "density-matrix" ) {
        
            unitary(quregMatr, target, matr);
            applyReferenceOp(refMatr, target, op);
            REQUIRE( areEqual(quregMatr, refMatr) );
        }
    }
    SECTION( "input validation" ) {
        
        SECTION( "qubit indices" ) {
            
            int target = GENERATE( -1, NUM_QUBITS );
            REQUIRE_THROWS_WITH( unitary(quregVec, target, matr), Contains("Invalid target") );
        }
        SECTION( "unitarity" ) {
            
            matr.real[0][0] = 0; // break matr unitarity
            REQUIRE_THROWS_WITH( unitary(quregVec, 0, matr), Contains("unitary") );
        }
    }
    CLEANUP_TEST( env, quregVec, quregMatr );
}
