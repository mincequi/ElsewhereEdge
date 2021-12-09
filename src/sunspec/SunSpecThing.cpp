#include "SunSpecThing.h"

#include <QVariant>

#include "../../src/Logger.h"

SunSpecThing::SunSpecThing(const QString& host, uint16_t port) {
    connect(&m_modbusClient, &QModbusTcpClient::stateChanged, this, &SunSpecThing::onStateChanged);
    connect(&m_modbusClient, &QModbusTcpClient::errorOccurred, this, &SunSpecThing::onErrorOccurred);

    m_modbusClient.setConnectionParameter(QModbusDevice::NetworkPortParameter, port);
    m_modbusClient.setConnectionParameter(QModbusDevice::NetworkAddressParameter, host);

    m_modbusClient.setTimeout(2000);
    m_modbusClient.setNumberOfRetries(1);

    connectDevice();
}

QString SunSpecThing::host() const {
    return m_modbusClient.connectionParameter(QModbusDevice::NetworkAddressParameter).toString();
}

uint8_t SunSpecThing::modbusUnitId() const {
    return m_unitId;
}

std::string SunSpecThing::sunSpecId() {
    if (m_sunSpecId.empty() && commonModel) {
        m_sunSpecId = commonModel.value().manufacturer() + "_" + commonModel.value().product() + "_" + commonModel.value().serial();
        std::transform(m_sunSpecId.begin(), m_sunSpecId.end(), m_sunSpecId.begin(), [](uchar c) { return std::tolower(c); });
        std::replace(m_sunSpecId.begin(), m_sunSpecId.end(), ' ', '_');
        std::replace(m_sunSpecId.begin(), m_sunSpecId.end(), '#', '_');
        std::replace(m_sunSpecId.begin(), m_sunSpecId.end(), '+', '_');
    }

    return m_sunSpecId;
}

QList<uint16_t> SunSpecThing::models() const {
    return m_modelAddresses.keys();
}

QMap<uint16_t, QVector<uint16_t>> SunSpecThing::blocks() const {
    return m_blocks;
}

bool SunSpecThing::connectDevice() {
    return m_modbusClient.connectDevice();
}

void SunSpecThing::disconnectDevice() {
    return m_modbusClient.disconnectDevice();
}

bool SunSpecThing::isValid() const {
    return m_modbusClient.state() == QModbusDevice::State::ConnectedState;
}

void SunSpecThing::readModel(uint16_t modelId, uint32_t timestamp) {
    LOG_S(1) << this->sunSpecId() << "> reading model: " << modelId;
    if (m_modelAddresses.count(modelId)) {
        readBlock(m_modelAddresses[modelId].first, m_modelAddresses[modelId].second, timestamp);
    }
}

void SunSpecThing::reset() {
    meterModel.reset();
    inverterModel.reset();
    inverterExtensionModel.reset();
}

uint8_t SunSpecThing::nextUnitId() {
    // unit ids: 1 to 247
    if (m_unitId > 246) {
        return 0;
    }

    static const std::vector<uint8_t> ids = {
          1,   2,   3,   4,   5,    // Any Modbus device
        126, 127, 128, 129, 130,    // SMA Solar Inverter
        100,                        // Fronius Solar Inverter
        240, 241, 242, 243, 244,    // Fronius Smart Meter

        6, 7, 8, 9, 10, 11, 12, 13, 14, 15, // Any Modbus device
        131, 132, 132, 133, 134, 135,       // SMA Solar Inverter

        // Any Modbus devices
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
        26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
        36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
        46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
        56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
        66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
        76, 77, 78, 79, 80, 81, 82, 83, 84, 85,
        86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
        96, 97, 98, 99,

        // SMA Solar inverters
        136, 137, 138, 139, 140, 141, 142, 143, 144, 145,
        146, 147, 148, 149, 150,

        // Fronius String Controls
        101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
        111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
        121, 122, 123, 124, 125,

        // Any Modbus devices
        151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
        161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
        171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
        181, 182, 183, 184, 185, 186, 187, 188, 189, 190,
        191, 192, 193, 194, 195, 196, 197, 198, 199, 200,
        201, 202, 203, 204, 205, 206, 207, 208, 209, 210,
        211, 212, 213, 214, 215, 216, 217, 218, 219, 220,
        221, 222, 223, 224, 225, 226, 227, 228, 229, 230,
        231, 232, 233, 234, 235, 236, 237, 238, 239,

        // Fronius Sensor Card
        245, 246, 247               // Last items to check
    };

    m_unitId = ids.at(m_unitIdx++);
    return m_unitId;
}

void SunSpecThing::pollNextUnitId() {
    const uint8_t id = nextUnitId();
    if (id == 0) {
        emit stateChanged(State::Failed);
        return;
    }

    readHeader(id);
}

void SunSpecThing::readHeader(uint8_t id) {
    const QModbusDataUnit dataUnit(QModbusDataUnit::HoldingRegisters, 40000, 4);
    auto* reply = m_modbusClient.sendReadRequest(dataUnit, id);
    if  (!reply) {
        emit stateChanged(State::Failed);
    } else if (reply->isFinished()) {   // broadcast replies return immediately
        delete reply;
        emit stateChanged(State::Failed);
    } else {
        connect(reply, &QModbusReply::finished, this, &SunSpecThing::onReadHeader);
    }
}

void SunSpecThing::onReadHeader() {
    auto reply = qobject_cast<QModbusReply*>(sender());
    if (!reply) {
        emit stateChanged(State::Failed);
        return;
    }

    if (reply->error() == QModbusDevice::ProtocolError) {
        pollNextUnitId();
    } else if (reply->error() == QModbusDevice::NoError) {
        //qInfo() << "Host:" << m_modbusClient.connectionParameter(QModbusDevice::NetworkAddressParameter).toString();
        const QModbusDataUnit unit = reply->result();
        if (unit.value(0) == 0x5375 &&
                unit.value(1) == 0x6E53 &&
                unit.value(2) == 1 &&
                (unit.value(3) == 65 || unit.value(3) == 66)) {
            readBlock(40002, 2 + unit.value(3), 0);
            readModelTable(40004 + unit.value(3));
        } else {
            emit stateChanged(State::Failed);
        }
    } else {
        emit stateChanged(State::Failed);
    }

    reply->deleteLater();
}

void SunSpecThing::readModelTable(uint16_t address) {
    const QModbusDataUnit dataUnit(QModbusDataUnit::HoldingRegisters, address, 2);
    auto* reply = m_modbusClient.sendReadRequest(dataUnit, m_unitId);
    if  (!reply) {
        emit stateChanged(State::Failed);
    } else if (reply->isFinished()) {   // broadcast replies return immediately
        delete reply;
        emit stateChanged(State::Failed);
    } else {
        connect(reply, &QModbusReply::finished, this, &SunSpecThing::onReadModelTable);
    }
}

void SunSpecThing::onReadModelTable() {
    auto reply = qobject_cast<QModbusReply*>(sender());
    if (reply->error() != QModbusDevice::NoError) {
        emit stateChanged(State::Failed);
    } else {
        const QModbusDataUnit unit = reply->result();
        addModelAddress(unit.value(0), unit.startAddress(), unit.value(1) + 2);
        if (unit.value(0) == 0xFFFF) {
            emit stateChanged(State::Connected);
        } else {
            readModelTable(unit.startAddress() + 2 + unit.value(1));
        }
    }

    reply->deleteLater();
}

void SunSpecThing::addModelAddress(uint16_t modelId, uint16_t startAddress, uint16_t length) {
    // Special handling for blocks, that do not fit into a 256 bytes payload (modbus tcp limit).
    //if (modelId == 160 && length > 125) {
    //    length = 110;
    //}

    // Special handling for SMA solar inverters: we only request first two MPPs
    if (modelId == 160) {
        length = 50;
    }
    m_modelAddresses[modelId] = { startAddress, length };
}

void SunSpecThing::readBlock(uint16_t address, uint16_t length, uint32_t timestamp) {
    const QModbusDataUnit dataUnit(QModbusDataUnit::HoldingRegisters, address, length);
    auto* reply = m_modbusClient.sendReadRequest(dataUnit, m_unitId);
    if  (!reply) {
        emit stateChanged(State::Failed);
    } else if (reply->isFinished()) {   // broadcast replies return immediately
        delete reply;
        emit stateChanged(State::Failed);
    } else {
        LOG_S(1) << sunSpecId() << "> reading block at: " << address;
        connect(reply, &QModbusReply::finished, this, std::bind(&SunSpecThing::onReadBlock, this, timestamp));
        //connect(reply, &QModbusReply::finished, this, [&]() { onReadBlock(timestamp); });
    }
}

void SunSpecThing::onReadBlock(uint32_t timestamp) {
    auto reply = qobject_cast<QModbusReply*>(sender());
    if (reply->error() != QModbusDevice::NoError) {
        onReadBlockError(reply);
    } else {
        const QModbusDataUnit unit = reply->result();
        m_blocks[unit.value(0)] = unit.values().mid(2);
        const auto buffer = unit.values().toStdVector();
        parseModel(buffer, timestamp);
        m_timeoutCount = 0;
    }

    reply->deleteLater();
}

void SunSpecThing::onReadBlockError(QModbusReply* reply) {
    switch (reply->error()) {
    case QModbusDevice::TimeoutError:
        LOG_S(WARNING) << sunSpecId() << "> reading block timed out";
        ++m_timeoutCount;
        // Set device as failed if sunSpecId is empty or it timed out 5 times in a row.
        if (m_timeoutCount > 4 || sunSpecId().empty()) {
            emit stateChanged(State::Failed);
        }
        break;
    default:
        LOG_S(WARNING) << sunSpecId() << "> error: " << reply->errorString();
        emit stateChanged(State::Failed);
        break;
    }
}

void SunSpecThing::onStateChanged(QModbusDevice::State state) {
    if (state == QModbusDevice::State::ConnectedState) {
        pollNextUnitId();
    }
}

void SunSpecThing::onErrorOccurred(QModbusDevice::Error error) {
    if (error != QModbusDevice::Error::NoError) {
        emit stateChanged(State::Failed);
    }
}

void SunSpecThing::parseModel(const std::vector<uint16_t>& buffer, uint32_t timestamp) {
    if (buffer.at(0) == 1) {
        SunSpecCommonModel::updateFromBuffer(commonModel, buffer);
        //m_commonModel.emplace(*SunSpecCommonModel::fromBuffer(buffer));
    } else if (buffer.at(0) == 203) {
        SunSpecWyeConnectMeterModel::updateFromBuffer(meterModel, buffer, timestamp);
        if (meterModel) {
            emit modelRead(*meterModel);
        }
    } else if (buffer.at(0) == 160) {
        SunSpecMpptInverterExtensionModel::updateFromBuffer(inverterExtensionModel, buffer, timestamp);
        if (inverterExtensionModel) {
            emit modelRead(*inverterExtensionModel);
        }
    } else if (buffer.at(0) == 103) {
        SunSpecInverterModel::updateFromBuffer(inverterModel, buffer, timestamp);
        if (inverterModel) {
            emit modelRead(*inverterModel);
        }
    }
}