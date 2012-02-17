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

#ifndef _CQ_H_
#define _CQ_H_

#include "queue.h"
#include "ce.h"

class CQ;    // forward definition
typedef boost::shared_ptr<CQ>               SharedCQPtr;
#define CAST_TO_CQ(shared_trackable_ptr)    \
        boost::shared_polymorphic_downcast<CQ>(shared_trackable_ptr);


/**
* This class extends the base class. It is also not meant to be instantiated.
* This class contains all things common to CQ's at a high level. After
* instantiation by a child the Init() methods must be called to attain
* something useful.
*
* @note This class may throw exceptions.
*/
class CQ : public Queue
{
public:
    /**
     * @param fd Pass the opened file descriptor for the device under test
     * @param objBeingCreated Pass the type of object this child class is
     */
    CQ(int fd, Trackable::ObjType objBeingCreated);
    virtual ~CQ();

    /// Used to compare for NULL pointers being returned by allocations
    static SharedCQPtr NullCQPtr;

    virtual bool GetIsCQ() { return true; }

    struct nvme_gen_cq GetQMetrics();

    /**
     * Even though a particular CQ may have IRQ's enabled, this does not mean
     * IRQ's will be used when reaping CE's. Reference
     * gCtrlrConfig.IrqsEnabled() to decipher if the OS/kernel/dnvme will poll
     * or use IRQ's for this CQ.
     * @return true when an IRQ for this CQ is enabled, otherwise false
     */
    bool GetIrqEnabled() { return mIrqEnabled; }
    uint16_t GetIrqVector() { return mIrqVec; }

    /**
     * Peek at a Completion Element (CE) at CQ position indicated by indexPtr.
     * Only dnvme can reap CE's from a CQ by calling Reap(), however user space
     * does have eyes into that CQ's memory, and thus allows peeking at any CE
     * at any time without reaping anything.
     * @param indexPtr Pass [0 to (GetNumEntries()-1)] as the index into the CQ.
     * @return The CE requested.
     */
    union CE PeekCE(uint16_t indexPtr);

    /**
     * Log the entire contents of CE at CQ position indicated by indexPtr to
     * the logging endpoint. Similar constraints as PeekCE() but logs the CE
     * instead of peeking at it.
     * @param indexPtr Pass the index into the Q for which element to log
     */
    void LogCE(uint16_t indexPtr);

    /**
     * Dump the entire contents of CE at CQ position indicated by indexPtr to
     * the named file. Similar constraints as PeekCE() but dumps the CE
     * instead of peeking at it.
     * @param indexPtr Pass the index into the Q for which element to log
     * @param filename Pass the filename as generated by macro
     *      FileSystem::PrepLogFile().
     * @param fileHdr Pass a custom file header description to dump
     */
    void DumpCE(uint16_t indexPtr, LogFilename filename, string fileHdr);

    /**
     * Send the entire contents of this Q to the named file.
     * @param filename Pass the filename as generated by macro
     *      FileSystem::PrepLogFile().
     * @param fileHdr Pass a custom file header description to dump
     */
    virtual void Dump(LogFilename filename, string fileHdr);

    /**
     * Inquire as to the number of CE's which are present in this CQ. Returns
     * immediately, does not block.
     * @param isrCount Returns the number of ISR's which fired and were counted
     *        that are assoc with this CQ. If this CQ does not use IRQ's, then
     *        this value will remain 0.
     * @reportOn0 Pass true to report when 0 CE's are awaiting in the CQ
     * @return The number of unreap'd CE's awaiting
     */
    uint16_t ReapInquiry(uint32_t &isrCount, bool reportOn0 = false);

    /**
     * Inquire as to the number of CE's which are present in this CQ. If the
     * number of CE's are 0, then a wait period is entered until such time
     * a CE arrives or a timeout period expires.
     * @param ms Pass the max number of milliseconds to wait until CE's arrive.
     * @param numCE Returns the number of unreap'd CE's awaiting
     * @param isrCount Returns the number of ISR's which fired and were counted
     *        that are assoc with this CQ. If this CQ does not use IRQ's, then
     *        this value will remain 0.
     * @return true when CE's are awaiting to be reaped, otherwise a timeout
     */
    bool ReapInquiryWaitAny(uint16_t ms, uint16_t &numCE, uint32_t &isrCount);

    /**
     * Wait until at least the specified number of CE's become available or
     * until a time out period expires.
     * @param ms Pass the max number of ms to wait until numTil CE's arrive.
     * @param numTil Pass the number of CE's that need to become available
     * @param numCE Returns the number of unreap'd CE's awaiting
     * @param isrCount Returns the number of ISR's which fired and were counted
     *        that are assoc with this CQ. If this CQ does not use IRQ's, then
     *        this value will remain 0.
     * @return true when CE's are awaiting to be reaped, otherwise a timeout
     */
    bool ReapInquiryWaitSpecify(uint16_t ms, uint16_t numTil, uint16_t &numCE,
        uint32_t &isrCount);

    /**
     * Reap a specified number of Completion Elements (CE) from this CQ. The
     * memBuffer will be resized. Calling this method when (ReapInquiry() == 0)
     * is fine.
     * @param ceRemain Returns the number of CE's left in the CQ after reaping
     * @param memBuffer Pass a buffer to contain the CE's requested. The
     *      contents of the buffer will be lost and the buffer will be resized
     *      to fulfill ceDesire.
     * @param isrCount Returns the number of ISR's which fired and were counted
     *        that are assoc with this CQ. If this CQ does not use IRQ's, then
     *        this value will remain 0.
     * @param ceDesire Pass the number of CE's desired to be reaped, 0 indicates
     *      reap all which can be reaped.
     * @param zeroMem Pass true to zero out memBuffer before reaping, otherwise
     *      the buffer is not modified.
     * @return Returns the actual number of CE's reaped
     */
    uint16_t Reap(uint16_t &ceRemain, SharedMemBufferPtr memBuffer,
        uint32_t &isrCount, uint16_t ceDesire = 0, bool zeroMem = false);


protected:
    /**
     * Initialize this object and allocates contiguous Q content memory.
     * @param qId Pass the queue's ID
     * @param entrySize Pass the number of bytes encompassing each element
     * @param numEntries Pass the number of elements within the Q
     * @param irqEnabled Pass true if IRQ's are to be enabled for this Q
     * @param irqVec if (irqEnabled==true) then what the IRQ's vector
     */
    void Init(uint16_t qId, uint16_t entrySize, uint16_t numEntries,
        bool irqEnabled, uint16_t irqVec);

    /**
     * Initialize this object and allocates discontiguous Q content memory.
     * @param qId Pass the queue's ID
     * @param entrySize Pass the number of bytes encompassing each element
     * @param numEntries Pass the number of elements within the Q
     * @param memBuffer Hand off this Q's memory. It must satisfy
     *      MemBuffer.GetBufSize()>=(numEntries * entrySize). It must only ever
     *      be accessed as RO. Writing to this buffer will have unpredictable
     *      results.
     * @param irqEnabled Pass true if IRQ's are to be enabled for this Q
     * @param irqVec if (irqEnabled==true) then what the IRQ's vector
     */
    void Init(uint16_t qId, uint16_t entrySize, uint16_t numEntries,
        const SharedMemBufferPtr memBuffer, bool irqEnabled, uint16_t irqVec);


private:
    CQ();

    bool mIrqEnabled;
    uint16_t mIrqVec;

    /**
     * Create an IOCQ
     * @param q Pass the IOCQ's definition
     */
    void CreateIOCQ(struct nvme_prep_cq &q);

    /**
     * Calculate if a timeout (TO) period has expired
     * @param ms Pass the number of ms indicating the TO period
     * @param initial Pass the time when the period starting
     * @return true if the TO has expired, false otherwise
     */
    bool CalcTimeout(uint16_t ms, struct timeval initial);
};


#endif
