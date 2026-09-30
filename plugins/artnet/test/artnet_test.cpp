/*
  Q Light Controller Plus
  artnet_test.cpp

  Copyright (c) Jano Svitok

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <QTest>

#define private public
#include "artnet_test.h"
#include "artnetpacketizer.h"
#undef private

/****************************************************************************
 * ArtNet tests
 ****************************************************************************/

void ArtNet_Test::setupArtNetDmx()
{
    ArtNetPacketizer ap;

    QByteArray data;
    const QByteArray empty;
    const QByteArray fifty(50, 10);
    const QByteArray fiftyone(51, 10);
    const QByteArray full(512, 20);

    // empty data
    ap.setupArtNetDmx(data, 0, empty);

    QCOMPARE(data.size(), 20);
    QCOMPARE(data.data(), "Art-Net");

    // full data
    ap.setupArtNetDmx(data, 0, full);

    QCOMPARE(data.size(), 18 + 512);
    QCOMPARE(data.data(), "Art-Net");

    // partial data
    ap.setupArtNetDmx(data, 0, fifty);

    QCOMPARE(data.size(), 18 + 50);
    QCOMPARE(data.data(), "Art-Net");

    ap.setupArtNetDmx(data, 0, fiftyone);

    QCOMPARE(data.size(), 18 + 52);
    QCOMPARE(data.data(), "Art-Net");
}

void ArtNet_Test::fillArtPollReplyInfo()
{
    ArtNetPacketizer ap;

    // Build a minimal, spec-compliant ArtPollReply (239 bytes)
    QByteArray data(239, 0);
    data.replace(0, 8, QByteArray("Art-Net\0", 8));
    data[8] = 0x00;
    data[9] = 0x21; // OpCode 0x2100 (little endian)
    // IP address 10.0.0.1
    data[10] = 10; data[11] = 0; data[12] = 0; data[13] = 1;
    // Port 0x1936 (little endian)
    data[14] = 0x36; data[15] = 0x19;
    // NetSwitch / SubSwitch
    data[18] = 0x00; data[19] = 0x00;
    // OEM 0xFFFE (Hi, Lo)
    data[20] = char(0xFF); data[21] = char(0xFE);
    // Short name
    data.replace(26, 10, QByteArray(EASYARTNET_SHORTNAME));
    // Long name
    data.replace(44, 18, QByteArray("Easy ArtNet Interface"));
    // NumPorts = 1
    data[172] = 0x00; data[173] = 0x01;
    // PortTypes[0] = output capable
    data[174] = char(0x80);
    // SwOut[0] = ArtNet universe 3
    data[190] = 0x03;

    ArtNetNodeInfo info;
    QVERIFY(ap.fillArtPollReplyInfo(data, info) == true);
    QCOMPARE(info.shortName, QString(EASYARTNET_SHORTNAME));
    QCOMPARE(info.longName, QString("Easy ArtNet Interface"));
    QCOMPARE(info.oem, quint16(EASYARTNET_OEM));
    QCOMPARE(info.isOutput, true);
    QCOMPARE(info.isInput, false);
    QCOMPARE(info.universe, ushort(3));
    QVERIFY(info.isEasyArtNet() == true);

    // ShortName only is enough to identify the device
    ArtNetNodeInfo byName = info;
    byName.oem = 0x0001;
    QVERIFY(byName.isEasyArtNet() == true);

    // Any other ArtNet node must not be identified as Easy ArtNet
    QByteArray otherData = data;
    otherData[20] = 0x00; otherData[21] = 0x01;
    otherData.replace(26, 10, QByteArray("SomeOtherNode"));
    ArtNetNodeInfo other;
    QVERIFY(ap.fillArtPollReplyInfo(otherData, other) == true);
    QVERIFY(other.isEasyArtNet() == false);

    // Packets too short to be an ArtPollReply must be rejected
    ArtNetNodeInfo shortInfo;
    QVERIFY(ap.fillArtPollReplyInfo(QByteArray(50, 0), shortInfo) == false);
}

QTEST_MAIN(ArtNet_Test)
