/*
 * Copyright (c) 2011, Intel Corporation.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef _ADMINQROLLCHKDIFF_r10b_H_
#define _ADMINQROLLCHKDIFF_r10b_H_

#include "test.h"
#include "../Queues/asq.h"
#include "../Queues/acq.h"

namespace GrpQueues {


/** \verbatim
 * -----------------------------------------------------------------------------
 * ----------------Mandatory rules for children to follow-----------------------
 * -----------------------------------------------------------------------------
 * 1) See notes in the header file of the Test base class
 * \endverbatim
 */
class AdminQRollChkDiff_r10b : public Test
{
public:
    AdminQRollChkDiff_r10b(int fd, string grpName, string testName,
        ErrorRegs errRegs);
    virtual ~AdminQRollChkDiff_r10b();

    /**
     * IMPORTANT: Read Test::Clone() header comment.
     */
    virtual AdminQRollChkDiff_r10b *Clone() const
        { return new AdminQRollChkDiff_r10b(*this); }
    AdminQRollChkDiff_r10b &operator=(const AdminQRollChkDiff_r10b &other);
    AdminQRollChkDiff_r10b(const AdminQRollChkDiff_r10b &other);


protected:
    virtual bool RunCoreTest();


private:
    ///////////////////////////////////////////////////////////////////////////
    // Adding a member variable? Then edit the copy constructor and operator=().
    ///////////////////////////////////////////////////////////////////////////
    void VerifyCESQValues(SharedACQPtr acq, uint16_t expectedVal);
    void VerifyQPointers(SharedACQPtr acq, SharedASQPtr asq);
};

}   // namespace

#endif
