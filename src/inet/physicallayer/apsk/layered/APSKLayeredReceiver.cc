//
// Copyright (C) 2014 OpenSim Ltd.
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

#include "inet/physicallayer/contract/layered/ISymbol.h"
#include "inet/physicallayer/common/BandListening.h"
#include "inet/physicallayer/common/ListeningDecision.h"
#include "inet/physicallayer/common/layered/LayeredReceptionDecision.h"
#include "inet/physicallayer/common/layered/LayeredReception.h"
#include "inet/physicallayer/common/layered/SignalSymbolModel.h"
#include "inet/physicallayer/common/layered/SignalSampleModel.h"
#include "inet/physicallayer/common/layered/SignalBitModel.h"
#include "inet/physicallayer/analogmodel/ScalarAnalogModel.h"
#include "inet/physicallayer/analogmodel/layered/ScalarSignalAnalogModel.h"
#include "inet/physicallayer/apsk/layered/APSKLayeredReceiver.h"
#include "inet/physicallayer/apsk/layered/APSKDecoder.h"
#include "inet/physicallayer/apsk/layered/APSKDemodulator.h"
#include "inet/physicallayer/apsk/layered/APSKSerializer.h"
#include "inet/physicallayer/apsk/layered/APSKPhyFrame_m.h"
#include "inet/physicallayer/base/NarrowbandNoiseBase.h"

namespace inet {

namespace physicallayer {

Define_Module(APSKLayeredReceiver);

APSKLayeredReceiver::APSKLayeredReceiver() :
    levelOfDetail((LevelOfDetail)-1),
    errorModel(nullptr),
    decoder(nullptr),
    demodulator(nullptr),
    pulseFilter(nullptr),
    analogDigitalConverter(nullptr),
    modulation(nullptr),
    energyDetection(W(NaN)),
    sensitivity(W(NaN)),
    carrierFrequency(Hz(NaN)),
    bandwidth(Hz(NaN)),
    snirThreshold(NaN)
{
}

void APSKLayeredReceiver::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL)
    {
        errorModel = dynamic_cast<ILayeredErrorModel *>(getSubmodule("errorModel"));
        decoder = dynamic_cast<IDecoder *>(getSubmodule("decoder"));
        demodulator = dynamic_cast<IDemodulator *>(getSubmodule("demodulator"));
        pulseFilter = dynamic_cast<IPulseFilter *>(getSubmodule("pulseFilter"));
        analogDigitalConverter = dynamic_cast<IAnalogDigitalConverter *>(getSubmodule("analogDigitalConverter"));
        modulation = APSKModulationBase::findModulation(par("modulation"));
        energyDetection = mW(math::dBm2mW(par("energyDetection")));
        // TODO: temporary parameters
        sensitivity = mW(math::dBm2mW(par("sensitivity")));
        carrierFrequency = Hz(par("carrierFrequency"));
        bandwidth = Hz(par("bandwidth"));
        snirThreshold = math::dB2fraction(par("snirThreshold"));
        const char *levelOfDetailStr = par("levelOfDetail").stringValue();
        if (strcmp("packet", levelOfDetailStr) == 0)
            levelOfDetail = PACKET_DOMAIN;
        else if (strcmp("bit", levelOfDetailStr) == 0)
            levelOfDetail = BIT_DOMAIN;
        else if (strcmp("symbol", levelOfDetailStr) == 0)
            levelOfDetail = SYMBOL_DOMAIN;
        else if (strcmp("sample", levelOfDetailStr) == 0)
            levelOfDetail = SAMPLE_DOMAIN;
        else
            throw cRuntimeError("Unknown level of detail='%s'", levelOfDetailStr);
        if (levelOfDetail >= BIT_DOMAIN && !decoder)
            throw cRuntimeError("Decoder not configured");
        if (levelOfDetail >= SYMBOL_DOMAIN && !demodulator)
            throw cRuntimeError("Demodulator not configured");
        if (levelOfDetail >= SAMPLE_DOMAIN && !pulseFilter)
            throw cRuntimeError("Pulse filter not configured");
    }
}

const IReceptionAnalogModel *APSKLayeredReceiver::createAnalogModel(const LayeredTransmission *transmission, const ISNIR *snir) const
{
    // TODO: interference + receptionAnalogModel;
    return nullptr;
}

const IReceptionSampleModel *APSKLayeredReceiver::createSampleModel(const LayeredTransmission *transmission, const ISNIR *snir, const IReceptionAnalogModel *analogModel) const
{
    if (levelOfDetail == SAMPLE_DOMAIN)
        return errorModel->computeSampleModel(transmission, snir);
    else if (analogDigitalConverter)
        return analogDigitalConverter->convertAnalogToDigital(analogModel);
    else
        return nullptr;
}

const IReceptionSymbolModel *APSKLayeredReceiver::createSymbolModel(const LayeredTransmission *transmission, const ISNIR *snir, const IReceptionSampleModel *sampleModel) const
{
    if (levelOfDetail == SYMBOL_DOMAIN)
        return errorModel->computeSymbolModel(transmission, snir);
    else if (levelOfDetail >= SAMPLE_DOMAIN)
        return pulseFilter->filter(sampleModel);
    else
        return nullptr;
}

const IReceptionBitModel *APSKLayeredReceiver::createBitModel(const LayeredTransmission *transmission, const ISNIR *snir, const IReceptionSymbolModel *symbolModel) const
{
    if (levelOfDetail == BIT_DOMAIN)
        return errorModel->computeBitModel(transmission, snir);
    else if (levelOfDetail >= SYMBOL_DOMAIN)
        return demodulator->demodulate(symbolModel);
    else
        return nullptr;
}

const IReceptionPacketModel *APSKLayeredReceiver::createPacketModel(const LayeredTransmission *transmission, const ISNIR *snir, const IReceptionBitModel *bitModel) const
{
    if (levelOfDetail == PACKET_DOMAIN)
        return errorModel->computePacketModel(transmission, snir);
    else if (levelOfDetail >= BIT_DOMAIN)
        return decoder->decode(bitModel);
    else
        return nullptr;
}

const APSKPhyFrame *APSKLayeredReceiver::createPhyFrame(const IReceptionPacketModel* packetModel) const
{
    const BitVector* bits = packetModel->getSerializedPacket();
    if (bits != nullptr)
        return APSKSerializer().deserialize(bits);
    else
        return check_and_cast<const APSKPhyFrame *>(packetModel->getPacket()->dup());
}

const IReceptionDecision *APSKLayeredReceiver::computeReceptionDecision(const IListening *listening, const IReception *reception, const IInterference *interference) const
{
    const IRadio *receiver = reception->getReceiver();
    const IRadioMedium *medium = receiver->getMedium();
    const LayeredTransmission *transmission = dynamic_cast<const LayeredTransmission*>(reception->getTransmission());
    const ISNIR *snir = medium->getSNIR(receiver, transmission);
    const IReceptionAnalogModel *analogModel = createAnalogModel(transmission, snir);
    const IReceptionSampleModel *sampleModel = createSampleModel(transmission, snir, analogModel);
    const IReceptionSymbolModel *symbolModel = createSymbolModel(transmission, snir, sampleModel);
    const IReceptionBitModel *bitModel = createBitModel(transmission, snir, symbolModel);
    const IReceptionPacketModel *packetModel = createPacketModel(transmission, snir, bitModel);
    const APSKPhyFrame *phyFrame = createPhyFrame(packetModel);
    ReceptionIndication *receptionIndication = new ReceptionIndication();
    receptionIndication->setMinSNIR(snir->getMin());
//    receptionIndication->setMinRSSI(analogModel->getMinRSSI());
//    receptionIndication->setSymbolErrorRate(bitModel->getBitErrorRate());
//    receptionIndication->setBitErrorRate(symbolModel->getSymbolErrorRate());
//    receptionIndication->setPacketErrorRate(packetModel->getPacketErrorRate());
    const ReceptionPacketModel *receptionPacketModel = new ReceptionPacketModel(phyFrame, new BitVector(*packetModel->getSerializedPacket()), bps(NaN), -1, packetModel->isPacketErrorless());
    delete packetModel;
    return new LayeredReceptionDecision(reception, receptionIndication, receptionPacketModel, bitModel, symbolModel, sampleModel, analogModel, true, true, receptionPacketModel->isPacketErrorless());
}

const IListening *APSKLayeredReceiver::createListening(const IRadio *radio, const simtime_t startTime, const simtime_t endTime, const Coord startPosition, const Coord endPosition) const
{
    return new BandListening(radio, startTime, endTime, startPosition, endPosition, carrierFrequency, bandwidth);
}

// TODO: copy
const IListeningDecision *APSKLayeredReceiver::computeListeningDecision(const IListening *listening, const IInterference *interference) const
{
    const IRadio *receiver = listening->getReceiver();
    const IRadioMedium *radioMedium = receiver->getMedium();
    const IAnalogModel *analogModel = radioMedium->getAnalogModel();
    const INoise *noise = analogModel->computeNoise(listening, interference);
    const NarrowbandNoiseBase *flatNoise = check_and_cast<const NarrowbandNoiseBase *>(noise);
    W maxPower = flatNoise->computeMaxPower(listening->getStartTime(), listening->getEndTime());
    bool isListeningPossible = maxPower >= energyDetection;
    delete noise;
    EV_DEBUG << "Computing listening possible: maximum power = " << maxPower << ", energy detection = " << energyDetection << " -> listening is " << (isListeningPossible ? "possible" : "impossible") << endl;
    return new ListeningDecision(listening, isListeningPossible);
}

// TODO: this is not purely functional, see interface comment
// TODO: copy
bool APSKLayeredReceiver::computeIsReceptionPossible(const IListening *listening, const IReception *reception) const
{
    const BandListening *bandListening = check_and_cast<const BandListening *>(listening);
    const LayeredReception *scalarReception = check_and_cast<const LayeredReception *>(reception);
    // TODO: scalar
    const ScalarReceptionSignalAnalogModel *analogModel = dynamic_cast<const ScalarReceptionSignalAnalogModel*>(scalarReception->getAnalogModel());
    if (bandListening->getCarrierFrequency() != analogModel->getCarrierFrequency() || bandListening->getBandwidth() != analogModel->getBandwidth()) {
        EV_DEBUG << "Computing reception possible: listening and reception bands are different -> reception is impossible" << endl;
        return false;
    }
    else {
        const INarrowbandSignal *narrowbandSignalAnalogModel = check_and_cast<const INarrowbandSignal *>(scalarReception->getAnalogModel());
        W minReceptionPower = narrowbandSignalAnalogModel->computeMinPower(reception->getStartTime(), reception->getEndTime());
        bool isReceptionPossible = minReceptionPower >= sensitivity;
        EV_DEBUG << "Computing reception possible: minimum reception power = " << minReceptionPower << ", sensitivity = " << sensitivity << " -> reception is " << (isReceptionPossible ? "possible" : "impossible") << endl;
        return isReceptionPossible;
    }
}

} // namespace physicallayer

} // namespace inet
