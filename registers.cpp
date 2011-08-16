#include <sys/ioctl.h>
#include "registers.h"
#include "tnvme.h"
#include "dnvme/dnvme_ioctls.h"


// Register metrics (meta data) to aid interfacing with the kernel driver
#define ZZ(a, b, c, d, e, f, g, h)          { b, c, d, e, f, g, h },
PciSpcType Registers::mPciSpcMetrics[] =
{
    PCISPC_TABLE
};
#undef ZZ

#define ZZ(a, b, c, d, e, f, g)             { b, c, d, e, f, g },
CtlSpcType Registers::mCtlSpcMetrics[] =
{
    CTLSPC_TABLE
};
#undef ZZ



Registers::Registers(int fd, SpecRev specRev)
{
    LOG_NRM("Constructing register access");

    mFd = fd;
    if (mFd < 0) {
        LOG_DBG("Object created with a bad FD=%d", fd);
        return;
    }

    mSpecRev = specRev;
    DiscoverPciCapabilities();
}


Registers::~Registers()
{
}


bool
Registers::Read(PciSpc reg, unsigned long long &value)
{
    if (mPciSpcMetrics[reg].specRev != mSpecRev) {
        LOG_ERR("Attempting reg (%d) access to incompatible spec release", reg);
        return false;
    }
    return Read(NVMEIO_PCI_HDR, mPciSpcMetrics[reg].size,
        mPciSpcMetrics[reg].offset, value, mPciSpcMetrics[reg].desc);
}


bool
Registers::Read(CtlSpc reg, unsigned long long &value)
{
    if (mPciSpcMetrics[reg].specRev != mSpecRev) {
        LOG_ERR("Attempting reg (%d) access to incompatible spec release", reg);
        return false;
    }
    return Read(NVMEIO_BAR01, mCtlSpcMetrics[reg].size,
        mCtlSpcMetrics[reg].offset, value, mCtlSpcMetrics[reg].desc);
}


bool
Registers::Read(nvme_io_space regSpc, unsigned int rsize, unsigned int roffset,
    unsigned long long &value, const char *rdesc)
{
    int rc;
    struct rw_generic io = { regSpc, roffset, rsize, (unsigned char *)&value };

    // Verify we discovered the true offset of requested register
    if (roffset == INT_MAX) {
        LOG_ERR("Offset of %s could not be discovered", rdesc);
        return false;
    } else if (rsize > MAX_SUPPORTED_REG_SIZE) {
        LOG_ERR("Size of %s is larger than supplied buffer", rdesc);
        return false;
    } else if ((rc = ioctl(mFd, NVME_IOCTL_READ_GENERIC, &io)) < 0) {
        LOG_ERR("Error reading %s: %d returned", rdesc, rc);
        LOG_DBG("io.{type,offset,nBytes,buffer} = {%d, 0x%04X, 0x%04X, %p}",
            io.type, io.offset, io.nBytes, io.buffer);
        return false;
    }

    value = REGMASK(value, rsize);
    LOG_NRM("Reading %s", FormatRegister(rsize, rdesc, value).c_str());
    return true;
}


bool
Registers::Read(nvme_io_space regSpc, unsigned int rsize, unsigned int roffset,
    unsigned char *value)
{
    int rc;
    struct rw_generic io = { regSpc, roffset, rsize, value };

    if ((rc = ioctl(mFd, NVME_IOCTL_READ_GENERIC, &io)) < 0) {
        LOG_ERR("Error reading reg offset 0x%08X: %d returned", roffset, rc);
        LOG_DBG("io.{type,offset,nBytes,buffer} = {%d, 0x%04X, 0x%04X, %p}",
            io.type, io.offset, io.nBytes, io.buffer);
        return false;
    }

    LOG_NRM("Reading %s",
        FormatRegister(regSpc, rsize, roffset, value).c_str());
    return true;
}


bool
Registers::Write(PciSpc reg, unsigned long long value)
{
    if (mPciSpcMetrics[reg].specRev != mSpecRev) {
        LOG_ERR("Attempting reg (%d) access to incompatible spec release", reg);
        return false;
    }
    return Write(NVMEIO_PCI_HDR, mPciSpcMetrics[reg].size,
        mPciSpcMetrics[reg].offset, value, mPciSpcMetrics[reg].desc);
}


bool
Registers::Write(CtlSpc reg, unsigned long long value)
{
    if (mPciSpcMetrics[reg].specRev != mSpecRev) {
        LOG_ERR("Attempting reg (%d) access to incompatible spec release", reg);
        return false;
    }
    return Write(NVMEIO_BAR01, mCtlSpcMetrics[reg].size,
        mCtlSpcMetrics[reg].offset, value, mCtlSpcMetrics[reg].desc);
}


bool
Registers::Write(nvme_io_space regSpc, unsigned int rsize, unsigned int roffset,
    unsigned long long &value, const char *rdesc)
{
    int rc;
    struct rw_generic io = { regSpc, roffset, rsize, (unsigned char *)&value };

    // Verify we discovered the true offset of requested register
    if (roffset == INT_MAX) {
        LOG_ERR("Offset of %s could not be discovered", rdesc);
        return false;
    } else if (rsize > MAX_SUPPORTED_REG_SIZE) {
        LOG_ERR("Size of %s is larger than supplied buffer", rdesc);
        return false;
    } else if ((rc = ioctl(mFd, NVME_IOCTL_WRITE_GENERIC, &io)) < 0) {
        LOG_ERR("Error writing %s: %d returned", rdesc, rc);
        LOG_DBG("io.{type,offset,nBytes,buffer} = {%d, 0x%04X, 0x%04X, %p}",
            io.type, io.offset, io.nBytes, io.buffer);
        return false;
    }

    value = REGMASK(value, rsize);
    LOG_NRM("Writing %s", FormatRegister(rsize, rdesc, value).c_str());
    return true;
}


bool
Registers::Write(nvme_io_space regSpc, unsigned int rsize, unsigned int roffset,
    unsigned char *value)
{
    int rc;
    struct rw_generic io = { regSpc, roffset, rsize, value };

    if ((rc = ioctl(mFd, NVME_IOCTL_WRITE_GENERIC, &io)) < 0) {
        LOG_ERR("Error writing reg offset 0x%08X: %d returned", roffset, rc);
        LOG_DBG("io.{type,offset,nBytes,buffer} = {%d, 0x%04X, 0x%04X, %p}",
            io.type, io.offset, io.nBytes, io.buffer);
        return false;
    }

    LOG_NRM("Writing %s",
        FormatRegister(regSpc, rsize, roffset, value).c_str());
    return true;
}


string
Registers::FormatRegister(unsigned int regSize, const char *regDesc,
    unsigned long long regValue)
{
    string result;
    char buffer[80];
    bool truncated = false;
    const char *regFmt[MAX_SUPPORTED_REG_SIZE + 1] = {
        "%s = 0x",      // not intending in over using this, just a place holder
        "%s = 0x%02llX",
        "%s = 0x%04llX",
        "%s = 0x%06llX",
        "%s = 0x%08llX",
        "%s = 0x%010llX",
        "%s = 0x%012llX",
        "%s = 0x%014llX",
        "%s = 0x%016llX"
    };

    if (regSize > MAX_SUPPORTED_REG_SIZE) {
        regSize = MAX_SUPPORTED_REG_SIZE;
        truncated = true;
    }

    snprintf(buffer, sizeof(buffer), regFmt[regSize], regDesc,
        REGMASK(regValue, regSize));
    result = buffer;
    if (truncated)
        result += "(TRUNCATED VALUE)";
    return result;
}


string
Registers::FormatRegister(nvme_io_space regSpc, unsigned int rsize,
    unsigned int roffset, unsigned char *value)
{
    unsigned char *tmp = value;
    char buffer[80];
    string result;
    int i;
    unsigned int j;

    switch (regSpc) {

    case NVMEIO_PCI_HDR:
        snprintf(buffer, sizeof(buffer), "PCI space register...");
        break;
    case NVMEIO_BAR01:
        snprintf(buffer, sizeof(buffer), "ctrl'r space register...");
        break;
    default:
        snprintf(buffer, sizeof(buffer), "unknown space register");
        break;
    }
    result = buffer;

    for (i = 0, j = 0; j < rsize; i++, j++) {
        if (i == 0) {
            snprintf(buffer, sizeof(buffer), "\n    0x%08X: 0x", roffset+j);
            result += buffer;
        }
        snprintf(buffer, sizeof(buffer), "%02X ", *tmp++);
        result += buffer;
        if (i >= 15)
            i = -1;
    }

    return result;
}

#include <stdlib.h>
void
Registers::DiscoverPciCapabilities()
{
    int rc;
    int capIdx;
    unsigned int capId;
    unsigned int capOffset;
    unsigned long long work;
    unsigned long long nextCap;
    struct rw_generic io = { NVMEIO_PCI_HDR, 0, 4, (unsigned char *)&nextCap };


    // NOTE: We cannot report errors/violations of the spec as we parse PCI
    //       space because any non-conformance we may find could be changed
    //       in later releases of the NVME spec. Being that this is a
    //       non-versioned utility class we have no ability to note changes in
    //       the spec. The test architecture does handle specification mods but
    //       that is handled in the versioning of the test cases themselves.
    //       This is not a test case, thus we can't flag spec violations, this
    //       is a helper class for the test cases only.
    LOG_NRM("Discovering PCI capabilities");
    mPciCap.clear();
    if (Read(PCISPC_STS, work) == false)
        return;
    if ((work & STS_CL) == 0) {
        LOG_NRM("%s states there are no capabilities",
            mPciSpcMetrics[PCISPC_STS].desc);
        return;
    }

    // Find the index/offset to the 1st capability
    if (Read(PCISPC_CAP, nextCap) == false)
        return;
    nextCap <<= 8;   // small trick to make the while() work for 1st CAP reg

    // Traverse the link list of capabilities, PCI spec states when next ptr
    // becomes 0, then that is the capabilities among many.
    while (REGMASK((nextCap >> 8), 1)) {
        io.offset = (unsigned int)REGMASK((nextCap >> 8), 1);
        if ((rc = ioctl(mFd, NVME_IOCTL_READ_GENERIC, &io)) < 0) {
            LOG_ERR("Error reading offset 0x%08X from PCI space: %d returned",
                io.offset, rc);
            return;
        }
        LOG_NRM("Reading PCI space offset 0x%04X=0x%04X", io.offset,
            (unsigned int)REGMASK(nextCap, 2));

        // For each capability we find, log the order in which it was found
        capId = (unsigned char)REGMASK(nextCap, 1);
        capOffset = io.offset;
        switch (capId) {

        case 0x01:  // PCICAP_PMCAP
            LOG_NRM("Decoding PMCAP capabilities");
            mPciCap.push_back(PCICAP_PMCAP);
            capIdx = PCISPC_PID;
            mPciSpcMetrics[capIdx].offset = capOffset;
            break;

        case 0x05:  // PCICAP_MSICAP
            LOG_NRM("Decoding MSICAP capabilities");
            mPciCap.push_back(PCICAP_MSICAP);
            capIdx = PCISPC_MID;
            mPciSpcMetrics[capIdx].offset = capOffset;
            break;

        case 0x10:  // PCICAP_PXCAP
            LOG_NRM("Decoding PXCAP capabilities");
            mPciCap.push_back(PCICAP_PXCAP);
            capIdx = PCISPC_PXID;
            mPciSpcMetrics[capIdx].offset = capOffset;
            break;

        case 0x11:  // PCICAP_MSIXCAP
            LOG_NRM("Decoding MSIXCAP capabilities");
            mPciCap.push_back(PCICAP_MSIXCAP);
            capIdx = PCISPC_MXID;
            mPciSpcMetrics[capIdx].offset = capOffset;
            break;

        default:
            LOG_ERR("Decoded an unknown capability ID: 0x%02X", capId);
            return;
        }

        // For each capability we find update our knowledge of each reg's
        // offset within that capability starting with the offset we know.
        for (int i = (capIdx+1); i < PCISPC_FENCE; i++) {
            if (mPciSpcMetrics[i].cap == mPciCap.back()) {
                mPciSpcMetrics[i].offset =
                    mPciSpcMetrics[i-1].offset + mPciSpcMetrics[i-1].size;
            }
        }
    }

    // Handle PCI extended capabilities which must start at offset 0x100.
    // Only one of these is possible, i.e. the AERCAP capabilities.
    io.offset = 0x100;
    io.nBytes = 4;
    if ((rc = ioctl(mFd, NVME_IOCTL_READ_GENERIC, &io)) < 0) {
        LOG_ERR("Error reading offset 0x%08X from PCI space: %d returned",
            io.offset, rc);
        return;
    }
    LOG_NRM("Reading extended PCI space offset 0x%04X=0x%08X", io.offset,
        (unsigned int)REGMASK(nextCap, 4));
    capId = (unsigned int)REGMASK(nextCap, 2);
    capOffset = (unsigned int)REGMASK((nextCap >> 20), 2);
    if (capId == 0x0001) {
        LOG_NRM("Decoding AERCAP capabilities");
        mPciCap.push_back(PCICAP_AERCAP);
        mPciSpcMetrics[PCISPC_AERID].offset = capOffset;
        for (int i = PCISPC_AERUCES; i <= PCISPC_AERTLP; i++) {
            mPciSpcMetrics[i].offset =
                mPciSpcMetrics[i-1].offset + mPciSpcMetrics[i-1].size;
        }
    } else {
        LOG_ERR("Decoded an unknown extended capability ID: 0x%04X", capId);
        return;
    }
}


