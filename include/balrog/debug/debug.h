#ifndef BALROGOS_DEBUG_H
#define BALROGOS_DEBUG_H

#define KDB_CALL            254

enum klog_logging_level
{
    KDB_LVL_VERBOSE     = 0x01,
    KDB_LVL_INFO        = 0x02,
    KDB_LVL_WARNING     = 0x03,
    KDB_LVL_ERROR       = 0x04,
    KDB_LVL_CRITICAL    = 0x05,
    KDB_LVL_FATAL       = 0x06,
    KDB_NONE            = 0x10,
};

#endif //BALROGOS_DEBUG_H
