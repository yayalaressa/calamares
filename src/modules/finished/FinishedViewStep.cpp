/* === This file is part of Calamares - <https://github.com/calamares> ===
 *
 *   Copyright 2014-2015, Teo Mrnjavac <teo@kde.org>
 *   Copyright 2017, 2019, Adriaan de Groot <groot@kde.org>
 *   Copyright 2019, Collabora Ltd <arnaud.ferraris@collabora.com>
 *
 *   Calamares is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Calamares is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Calamares. If not, see <http://www.gnu.org/licenses/>.
 */

#include "FinishedViewStep.h"
#include "FinishedPage.h"
#include "JobQueue.h"

#include "utils/Logger.h"
#include "utils/Variant.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QVariantMap>

#include "Branding.h"
#include "Settings.h"

FinishedViewStep::FinishedViewStep( QObject* parent )
    : Calamares::ViewStep( parent )
    , m_widget( new FinishedPage() )
    , installFailed( false )
    , m_notifyOnFinished( false )
{
    auto jq = Calamares::JobQueue::instance();
    connect( jq, &Calamares::JobQueue::failed,
             m_widget, &FinishedPage::onInstallationFailed );
    connect( jq, &Calamares::JobQueue::failed,
             this, &FinishedViewStep::onInstallationFailed );

    emit nextStatusChanged( true );
}


FinishedViewStep::~FinishedViewStep()
{
    if ( m_widget && m_widget->parent() == nullptr )
        m_widget->deleteLater();
}


QString
FinishedViewStep::prettyName() const
{
    return tr( "Finish" );
}


QWidget*
FinishedViewStep::widget()
{
    return m_widget;
}


bool
FinishedViewStep::isNextEnabled() const
{
    return false;
}


bool
FinishedViewStep::isBackEnabled() const
{
    return false;
}


bool
FinishedViewStep::isAtBeginning() const
{
    return true;
}


bool
FinishedViewStep::isAtEnd() const
{
    return true;
}

void
FinishedViewStep::sendNotification()
{
    // If the installation failed, don't send notification popup;
    // there's a (modal) dialog popped up with the failure notice.
    if ( installFailed )
        return;

    QDBusInterface notify( "org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications" );
    if ( notify.isValid() )
    {
        QDBusReply<uint> r = notify.call( "Notify",
                                          QString( "Calamares" ),
                                          QVariant( 0U ),
                                          QString( "calamares" ),
                                          Calamares::Settings::instance()->isSetupMode()
                                              ? tr( "Setup Complete" )
                                              : tr( "Installation Complete" ),
                                          Calamares::Settings::instance()->isSetupMode()
                                              ? tr( "The setup of %1 is complete." ).arg( *Calamares::Branding::VersionedName )
                                              : tr( "The installation of %1 is complete." ).arg( *Calamares::Branding::VersionedName ),
                                          QStringList(),
                                          QVariantMap(),
                                          QVariant( 0 )
                                        );
        if ( !r.isValid() )
            cDebug() << "Could not call notify for end of installation." << r.error();
    }
    else
        cDebug() << "Could not get dbus interface for notifications." << notify.lastError();
}


void
FinishedViewStep::onActivate()
{
    m_widget->setUpRestart();

    if ( m_notifyOnFinished )
        sendNotification();
}


QList< Calamares::job_ptr >
FinishedViewStep::jobs() const
{
    return QList< Calamares::job_ptr >();
}

void
FinishedViewStep::onInstallationFailed( const QString& message, const QString& details )
{
    Q_UNUSED( message )
    Q_UNUSED( details )
    installFailed = true;
}

void
FinishedViewStep::setConfigurationMap( const QVariantMap& configurationMap )
{
    bool restartNowEnabled = CalamaresUtils::getBool( configurationMap, "restartNowEnabled", false );
    m_widget->setRestartNowEnabled( restartNowEnabled );

    bool restartNowChecked = restartNowEnabled && CalamaresUtils::getBool( configurationMap, "restartNowChecked", false );
    m_widget->setRestartNowChecked( restartNowChecked );

    if ( restartNowEnabled )
    {
        QString restartNowCommand = CalamaresUtils::getString( configurationMap, "restartNowCommand" );
        if ( restartNowCommand.isEmpty() )
            restartNowCommand = QStringLiteral( "shutdown -r now" );
        m_widget->setRestartNowCommand( restartNowCommand );
    }

    m_notifyOnFinished = CalamaresUtils::getBool( configurationMap, "notifyOnFinished", false );
}

CALAMARES_PLUGIN_FACTORY_DEFINITION( FinishedViewStepFactory, registerPlugin<FinishedViewStep>(); )
