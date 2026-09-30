/*
  Q Light Controller Plus
  configureartnet.h

  Copyright (c) Massimo Callegari

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

#ifndef CONFIGUREARTNET_H
#define CONFIGUREARTNET_H

#include <QHostAddress>

#include "ui_configureartnet.h"

class ArtNetPlugin;

class ConfigureArtNet final : public QDialog, public Ui_ConfigureArtNet
{
    Q_OBJECT

    /*********************************************************************
     * Initialization
     *********************************************************************/
public:
    ConfigureArtNet(ArtNetPlugin* plugin, QWidget* parent = 0);
    virtual ~ConfigureArtNet();

    /** @reimp */
    void accept() override;

public slots:
    int exec() override;

private slots:
    /** Search for an Easy ArtNet device and set it as output destination */
    void slotAutoConfigure();

    /** Second attempt of the auto-configuration, after an ArtPoll round trip */
    void slotAutoConfigureRetry();

    /** Force an ArtPoll transmission and rebuild the Nodes Tree */
    void slotRefreshNodes();

    /** Rebuild the Nodes Tree after the ArtPoll replies have been received */
    void slotRebuildNodesTree();

private:
    void fillNodesTree();
    void fillMappingTree();
    void showIPAlert(QString ip);

    /** Return true and fill 'address' with the IP of the first discovered
     *  Easy ArtNet device */
    bool findEasyArtNetNode(QHostAddress &address) const;

    /** Set the given IP address on every output item of the mapping tree */
    void applyDeviceIP(const QHostAddress &address);

    /** Send an ArtPoll packet on every available controller */
    void triggerNodePoll() const;

private:
    ArtNetPlugin* m_plugin;

};

#endif
