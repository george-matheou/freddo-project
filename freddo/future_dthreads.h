/*
 * Copyright (C) 2017 George Matheou (cs07mg2@cs.ucy.ac.cy)
 *
 * This file is part of FREDDO.
 *
 * FREDDO is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FREDDO is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FREDDO.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * future_dthreads.h
 *
 *  Created on: Nov 8, 2015
 *      Author: geomat
 */

#ifndef FUTURE_DTHREADS_H_
#define FUTURE_DTHREADS_H_

#include "dthreads.h"

namespace ddm {

    /////////////////////////////////////////////////////////////////////////////////////////////////
    /// Future DThread Classes:                                            												///
    ///  - these classes manages DThreads that their RC attribute is missing											///
    ///  - the RC will be calculated at runtime using the Consumer List of the DThreads						///
    /////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * FutureSimpleDThread implements simple DThreads, i.e. DThreads with only one instance
     */
    class FutureSimpleDThread: public SimpleDThread {
      public:
        /**
         * Creates a FutureSimpleDThread object. The RC of the DThread will be evaluated at runtime.
         * After that the FutureSimpleDThread will be converted to SimpleDThread and it will be stored in TSU.
         * @param[in] dFunction the pointer of the DThread's function
         * @param[in] nesting	the Dthread's nesting
         */
        FutureSimpleDThread(SimpleDFunction sDFunction) {
            m_ifp.simpleDFunction = sDFunction;
            m_tid = m_tsu->addDThread(&m_ifp, Nesting::ZERO);  // Inserts a Pending DThread
        }
    };

    /**
     * FutureMultipleDThread implements DThreads that have multiple instances with Nesting=1
     */
    class FutureMultipleDThread: public MultipleDThread {
      public:
        /**
         * Creates a FutureMultipleDThread object. The RC of the DThread will be evaluated at runtime.
         * After that the FutureMultipleDThread will be converted to MultipleDThread and it will be stored in TSU.
         * @param[in] mDFunction the pointer of the DThread's function
         * @param[in] numOfInstances the number of instances of the DThread
         * @note A static SM will be used
         */
        FutureMultipleDThread(MultipleDFunction mDFunction, UInt numOfInstances) {
            m_ifp.multipleDFunction = mDFunction;
            m_tid = m_tsu->addDThread(&m_ifp, Nesting::ONE, numOfInstances, 1, 1);  // Inserts a Pending DThread
        }

        /**
         * Creates a FutureMultipleDThread object. The RC of the DThread will be evaluated at runtime.
         * After that the FutureMultipleDThread will be converted to MultipleDThread and it will be stored in TSU.
         * @param[in] mDFunction the pointer of the DThread's function
         * @note A dynamic SM will be used
         */
        FutureMultipleDThread(MultipleDFunction mDFunction) {
            m_ifp.multipleDFunction = mDFunction;
            m_tid = m_tsu->addDThread(&m_ifp, Nesting::ONE);  // Inserts a Pending DThread
        }
    };

    /**
     * FutureMultipleDThread2D implements DThreads that have multiple instances with Nesting=2
     */
    class FutureMultipleDThread2D: public MultipleDThread2D {
      public:
        /**
         * Creates a FutureMultipleDThread2D object. The RC of the DThread will be evaluated at runtime.
         * After that the FutureMultipleDThread2D will be converted to MultipleDThread2D and it will be stored in TSU.
         * @param[in] mDFunction2D the pointer of the DThread's function
         * @param[in] innerRange the range of the inner Context
         * @param[in] outerRange the range of the outer Context
         * @note A static SM will be used
         */
        FutureMultipleDThread2D(MultipleDFunction2D mDFunction2D, UInt innerRange, UInt outerRange) {
            m_ifp.multipleDFunction2D = mDFunction2D;
            m_tid = m_tsu->addDThread(&m_ifp, Nesting::TWO, innerRange, 1, outerRange);  // Inserts a Pending DThread
        }

        /**
         * Creates a FutureMultipleDThread2D object. The RC of the DThread will be evaluated at runtime.
         * After that the FutureMultipleDThread2D will be converted to MultipleDThread2D and it will be stored in TSU.
         * @param[in] mDFunction2D the pointer of the DThread's function
         * @note A dynamic SM will be used
         */
        FutureMultipleDThread2D(MultipleDFunction2D mDFunction2D) {
            m_ifp.multipleDFunction2D = mDFunction2D;
            m_tid = m_tsu->addDThread(&m_ifp, Nesting::TWO);  // Inserts a Pending DThread
        }
    };

    /**
     * FutureMultipleDThread3D implements DThreads that have multiple instances with Nesting=3
     */
    class FutureMultipleDThread3D: public MultipleDThread3D {
      public:
        /**
         * Creates a FutureMultipleDThread3D object. The RC of the DThread will be evaluated at runtime.
         * After that the FutureMultipleDThread3D will be converted to MultipleDThread3D and it will be stored in TSU.
         * @param[in] mDFunction3D the pointer of the DThread's function
         * @param[in] innerRange the range of the inner Context
         * @param[in] middleRange the range of the middle Context
         * @param[in] outerRange the range of the outer Context
         * @note A static SM will be used
         */
        FutureMultipleDThread3D(MultipleDFunction3D mDFunction3D, UInt innerRange, UInt middleRange, UInt outerRange) {
            m_ifp.multipleDFunction3D = mDFunction3D;
            m_tid = m_tsu->addDThread(&m_ifp, Nesting::THREE, innerRange, middleRange, outerRange);  // Inserts a Pending DThread
        }

        /**
         * Creates a FutureMultipleDThread3D object. The RC of the DThread will be evaluated at runtime.
         * After that the FutureMultipleDThread3D will be converted to MultipleDThread3D and it will be stored in TSU.
         * @param[in] mDFunction3D the pointer of the DThread's function
         * @note A dynamic SM will be used
         */
        FutureMultipleDThread3D(MultipleDFunction3D mDFunction3D) {
            m_ifp.multipleDFunction3D = mDFunction3D;
            m_tid = m_tsu->addDThread(&m_ifp, Nesting::THREE);  // Inserts a Pending DThread
        }
    };
}

#endif /* FUTURE_DTHREADS_H_ */
