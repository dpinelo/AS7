//
// C++ Interface: alepherpglobal
//
// Description:
// Esta librería incluye el código necesario para marcar aquellas clases o funciones
// que se expone públicamente en las DLL de Windows. Es por tanto necesario para Windows.
//
// Author: David Pinelo <alepherp@alephsistemas.es>
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef __ALEPHERP_GLOBAL_H
#define __ALEPHERP_GLOBAL_H

#include <QFlags>

#ifdef ALEPHERP_BUILD_DLL
#ifdef Q_OS_WIN
#define ALEPHERP_DLL_EXPORT __declspec(dllexport)
#else
#define ALEPHERP_DLL_EXPORT Q_DECL_EXPORT
#endif
#else
#ifdef Q_OS_WIN
#define ALEPHERP_DLL_EXPORT __declspec(dllimport)
#else
#define ALEPHERP_DLL_EXPORT Q_DECL_IMPORT
#endif
#endif

#endif
