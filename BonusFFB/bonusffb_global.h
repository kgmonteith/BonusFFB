#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(BONUSFFB_LIB)
#  define BONUSFFB_EXPORT Q_DECL_EXPORT
# else
#  define BONUSFFB_EXPORT Q_DECL_IMPORT
# endif
#else
# define BONUSFFB_EXPORT
#endif
