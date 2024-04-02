/*

    SPDX-FileCopyrightText: 2011-2015 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// std
#include <iomanip>
#include <iostream>

// own
#include "module.h"
#include "modulesproxymodel.h"
#include "settingsapp.h"
#include "version.h"

// Qt
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>

// Frameworks
#include <KAboutData>
#include <KLocalizedContext>
#include <KLocalizedString>
#include <KPluginMetaData>

using namespace Qt::Literals::StringLiterals;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("mobile.plasmasettings");

    // About data
    KAboutData aboutData(u"org.kde.mobile.plasmasettings"_s,
                         i18n("Settings"),
                         QStringLiteral(PLASMA_SETTINGS_VERSION_STRING),
                         i18n("Touch-friendly settings application."),
                         KAboutLicense::GPL,
                         i18n("Copyright 2011-2015, Sebastian Kügler"));
    aboutData.addAuthor(i18n("Sebastian Kügler"), i18n("Maintainer"), u"sebas@kde.org"_s);
    aboutData.addAuthor(i18n("Marco Martin"), i18n("Maintainer"), u"mart@kde.org"_s);
    aboutData.setDesktopFileName(u"org.kde.mobile.plasmasettings"_s);
    KAboutData::setApplicationData(aboutData);

    QApplication::setWindowIcon(QIcon::fromTheme(u"preferences-system"_s));

    QCommandLineParser parser;

    const QCommandLineOption listOption({QStringLiteral("l"), QStringLiteral("list")}, i18n("List available settings modules"));
    const QCommandLineOption formfactorOption(
        {QStringLiteral("x"), QStringLiteral("formfactor")},
        i18n("Limit to modules suitable for <formfactor>, e.g. handset, tablet, mediacenter, desktop, test, all (default handset)"),
        i18n("formfactor"));
    const QCommandLineOption moduleOption({QStringLiteral("m"), QStringLiteral("module")}, i18n("Settings module to open"), i18n("modulename"));
    const QCommandLineOption singleModuleOption({QStringLiteral("s"), QStringLiteral("singleModule")}, i18n("Only show a single module, requires --module"));
    const QCommandLineOption fullscreenOption({QStringLiteral("f"), QStringLiteral("fullscreen")}, i18n("Start window fullscreen"));
    const QCommandLineOption layoutOption(QStringLiteral("layout"), i18n("Package to use for the UI (default org.kde.mobile.settings)"), i18n("packagename"));

    parser.addOption(listOption);
    parser.addOption(formfactorOption);
    parser.addOption(moduleOption);
    parser.addOption(singleModuleOption);
    parser.addOption(fullscreenOption);
    parser.addOption(layoutOption);
    aboutData.setupCommandLine(&parser);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    if (parser.isSet(listOption)) {
        int nameWidth = 24;
        QSet<QString> seen;
        std::cout << std::setfill('.');

        auto formfactor = parser.value(formfactorOption);

        const auto plugins = KPluginMetaData::findPlugins(u"kcms"_s)
            << KPluginMetaData::findPlugins(u"plasma/kcms"_s) << KPluginMetaData::findPlugins(u"plasma/kcms/systemsettings"_s);
        for (const auto &plugin : plugins) {
            if (seen.contains(plugin.pluginId())) {
                continue;
            }
            // Filter out modules that are not explicitly suitable for the "handset" formfactor
            // const QStringList &formFactors = plugin.formFactors();
            if (!formfactor.isEmpty() && !plugin.formFactors().contains(formfactor) && formfactor != QStringLiteral("all")) {
                continue;
            }
            const int len = plugin.pluginId().length();
            if (len > nameWidth) {
                nameWidth = len;
            }

            seen << plugin.pluginId();
            std::cout << plugin.pluginId().toLocal8Bit().data() << ' ' << std::setw(nameWidth - plugin.pluginId().length() + 2) << '.' << ' '
                      << plugin.description().toLocal8Bit().data() << std::endl;

            // qDebug() << "Formafactors: " << formFactors;
        }

        const auto kcmPlugin = KPluginMetaData::findPlugins(u"kcms"_s)
            << KPluginMetaData::findPlugins(u"plasma/kcms"_s) << KPluginMetaData::findPlugins(u"plasma/kcms/systemsettings"_s);
        for (const auto &plugin : kcmPlugin) {
            if (seen.contains(plugin.pluginId())) {
                continue;
            }
            if (!formfactor.isEmpty() && !plugin.formFactors().contains(formfactor) && formfactor != QStringLiteral("all")) {
                continue;
            }
            const int len = plugin.pluginId().length();
            if (len > nameWidth) {
                nameWidth = len;
            }
            std::cout << plugin.pluginId().toLocal8Bit().data() << ' ' << std::setw(nameWidth - plugin.pluginId().length() + 2) << '.' << ' '
                      << plugin.description().toLocal8Bit().data() << std::endl;
        }

        return 0;
    }

    const QString module = parser.value(moduleOption);
    const bool singleModule = parser.isSet(singleModuleOption);

    if (singleModule && module.isEmpty()) {
        parser.showHelp();
        return 0;
    }

    auto *settingsApp = new SettingsApp(parser);
    settingsApp->setStartModule(module);
    settingsApp->setSingleModule(singleModule);

    qmlRegisterType<ModulesProxyModel>("org.kde.plasma.settings", 0, 1, "ModulesProxyModel");
    qmlRegisterType<Module>("org.kde.plasma.settings", 0, 1, "Module");
    qmlRegisterSingletonInstance<SettingsApp>("org.kde.plasma.settings", 0, 1, "SettingsApp", settingsApp);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    return app.exec();
}
