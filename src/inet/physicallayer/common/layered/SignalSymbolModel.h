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

#ifndef __INET_SIGNALSYMBOLMODEL_H
#define __INET_SIGNALSYMBOLMODEL_H

#include "inet/physicallayer/contract/layered/ISignalSymbolModel.h"
#include "inet/physicallayer/contract/IModulation.h"

namespace inet {

namespace physicallayer {

class INET_API SignalSymbolModel : public virtual ISignalSymbolModel
{
  protected:
    const int headerSymbolLength;
    const int payloadSymbolLength;
    const double headerSymbolRate;
    const double payloadSymbolRate;
    const std::vector<const ISymbol*> *symbols;

  public:
    SignalSymbolModel(int headerSymbolLength, double headerSymbolRate, int payloadSymbolLength, double payloadSymbolRate, const std::vector<const ISymbol*> *symbols);
    virtual ~SignalSymbolModel();

    virtual void printToStream(std::ostream &stream) const;
    virtual int getPayloadSymbolLength() const { return payloadSymbolLength; }
    virtual double getPayloadSymbolRate() const { return payloadSymbolRate; }
    virtual int getHeaderSymbolLength() const { return headerSymbolLength; }
    virtual double getHeaderSymbolRate() const { return headerSymbolRate; }
    virtual const std::vector<const ISymbol*> *getSymbols() const { return symbols; }
};

class INET_API TransmissionSymbolModel : public SignalSymbolModel, public virtual ITransmissionSymbolModel
{
  protected:
    const IModulation *headerModulation;
    const IModulation *payloadModulation;

  public:
    TransmissionSymbolModel(int headerSymbolLength, double headerSymbolRate, int payloadSymbolLength, double payloadSymbolRate, const std::vector<const ISymbol*> *symbols, const IModulation *headerModulation, const IModulation *payloadModulation);

    virtual const IModulation *getHeaderModulation() const { return headerModulation; }
    virtual const IModulation *getPayloadModulation() const { return payloadModulation; }
};

class INET_API ReceptionSymbolModel : public SignalSymbolModel, public virtual IReceptionSymbolModel
{
  public:
    ReceptionSymbolModel(int headerSymbolLength, double headerSymbolRate, int payloadSymbolLength, double payloadSymbolRate, const std::vector<const ISymbol*> *symbols);
};

} // namespace physicallayer

} // namespace inet

#endif /* ifndef __INET_SIGNALSYMBOLMODEL_H */
