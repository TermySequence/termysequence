// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#define MI(text, tip, icon, ...) menu->addAction(text, tip, icon, __VA_ARGS__)
#define MS() menu->addSeparator()
#define SI(text, tip, icon, ...) subMenu->addAction(text, tip, icon, __VA_ARGS__)
#define SS() subMenu->addSeparator()

#define MS_F(flag) menu->enable(menu->addSeparator(), flag)
#define MF(flag) menu->enable(menu->addAction(), flag)

#define MENU(id, text) \
    menu = new DynamicMenu(id, this, bar); \
    menu->setTitle(text); \
    bar->addMenu(menu)

#define MENU_E(id, feature, text) \
    menu = new DynamicMenu(id, this, bar); \
    menu->enable(feature); \
    menu->setTitle(text); \
    bar->addMenu(menu)

#define MENU_T(id, feature, text) \
    menu = new DynamicMenu((TermInstance*)0, id, this, bar); \
    menu->enable(feature); \
    menu->setTitle(text); \
    bar->addMenu(menu)

#define MENU_S(id, feature, text) \
    menu = new DynamicMenu((ServerInstance*)0, id, this, bar); \
    menu->enable(feature); \
    menu->setTitle(text); \
    bar->addMenu(menu)

#define SUBMENU(id, ...) \
    subMenu = new DynamicMenu(id, this, menu); \
    menu->addMenu(subMenu, __VA_ARGS__)

#define SUBMENU_P(id, ...) \
    subMenu = new DynamicMenu(id, parent, menu); \
    menu->addMenu(subMenu, __VA_ARGS__)

#define SUBMENU_E(id, feature, ...) \
    subMenu = new DynamicMenu(id, this, menu); \
    subMenu->enable(feature); \
    menu->addMenu(subMenu, __VA_ARGS__)

#define SUBMENU_EF(id, feature, flag, ...) \
    subMenu = new DynamicMenu(id, this, menu); \
    subMenu->enable(feature); \
    menu->enable(menu->addMenu(subMenu, __VA_ARGS__), flag)

#define SUBMENU_T(term, id, feature, ...) \
    subMenu = new DynamicMenu((TermInstance*)term, id, this, menu); \
    subMenu->enable(feature); \
    menu->addMenu(subMenu, __VA_ARGS__)

#define SUBMENU_T0(term, id, ...) \
    subMenu = new DynamicMenu((TermInstance*)term, id, this, menu); \
    menu->addMenu(subMenu, __VA_ARGS__)

#define SUBMENU_S(server, id, feature, ...) \
    subMenu = new DynamicMenu((ServerInstance*)server, id, this, menu); \
    subMenu->enable(feature); \
    menu->addMenu(subMenu, __VA_ARGS__)

#define SUBMENU_S0(server, id, ...) \
    subMenu = new DynamicMenu((ServerInstance*)server, id, this, menu); \
    menu->addMenu(subMenu, __VA_ARGS__)

#define POPUP_DOH(id, parent) \
    auto *menu = new DynamicMenu(id, this, parent); \
    connect(menu, SIGNAL(aboutToHide()), menu, SLOT(deleteLater()))

#define POPUP_TDOH(term, id, parent) \
    auto *menu = new DynamicMenu(term, id, this, parent); \
    connect(menu, SIGNAL(aboutToHide()), menu, SLOT(deleteLater()))

#define POPUP_OBJ(id, obj) \
    auto *menu = new DynamicMenu(id, this, this); \
    connect(obj, SIGNAL(destroyed()), menu, SLOT(deleteLater())); \
    connect(obj, SIGNAL(destroyed(QObject*)), SLOT(handleObjectDestroyed(QObject*))); \
    m_popups.insert(obj, menu)

#define POPUP_TOBJ(base, id, obj) \
    DynamicMenu *subMenu; \
    auto *menu = new DynamicMenu(base, id, this, this); \
    connect(obj, SIGNAL(destroyed()), menu, SLOT(deleteLater())); \
    connect(obj, SIGNAL(destroyed(QObject*)), SLOT(handleObjectDestroyed(QObject*))); \
    m_popups.insert(obj, menu)
