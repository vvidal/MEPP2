#include "MSDM2Plugin.h"

#include <QtPlugin>

#if QT_VERSION < 0x050000 // (for QT4)
Q_EXPORT_PLUGIN2(MSDM2Plugin, FEVV::MSDM2Plugin)
#endif
