//
// Copyright (C) 2013 OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_ISIGNALBITMODEL_H
#define __INET_ISIGNALBITMODEL_H

#include "ICyclicRedundancyCheck.h"
#include "IFECEncoder.h"
#include "IScrambler.h"
#include "IInterleaver.h"

namespace inet {

namespace physicallayer {

/**
 * This purely virtual interface provides an abstraction for different radio
 * signal models in the bit domain.
 */
class INET_API ISignalBitModel : public IPrintableObject
{
  public:
    // TODO: separate header and payload
    virtual int getBitLength() const = 0;

    // TODO: separate header and payload
    virtual double getBitRate() const = 0;

    virtual const BitVector *getBits() const = 0;
    virtual const IFECInfo *getForwardErrorCorrectionInfo() const = 0;
    virtual const IScramblerInfo *getScramblerInfo() const = 0;
    virtual const IInterleaverInfo *getInterleaverInfo() const = 0;
};

class INET_API ITransmissionBitModel : public virtual ISignalBitModel
{
};

class INET_API IReceptionBitModel : public virtual ISignalBitModel
{
  public:
    /**
     * Returns the bit error rate (probability).
     */
    virtual double getBER() const = 0;

    /**
     * Returns the actual number of erroneous bits.
     */
    virtual int getBitErrorCount() const = 0;
};

} // namespace physicallayer

} // namespace inet

#endif // ifndef __INET_ISIGNALBITMODEL_H